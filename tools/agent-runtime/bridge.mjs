import { Client, StreamableHTTPClientTransport } from '@modelcontextprotocol/client';
import { StdioClientTransport, getDefaultEnvironment } from '@modelcontextprotocol/client/stdio';
import { createInterface } from 'node:readline';
import { existsSync } from 'node:fs';
import { basename, dirname, delimiter, join } from 'node:path';

const clients = new Map();
const pending = new Map();
let hostReady = process.env.VSR_AGENT_WAIT_FOR_HOST !== '1';
const startupRequests = [];
const output = value => process.stdout.write(`${JSON.stringify(value)}\n`);

function stdioCommand(config) {
  const command = String(config.command ?? '');
  let args = config.arguments ?? config.args ?? [];
  if (!command || !Array.isArray(args) || !args.every(arg => typeof arg === 'string')) throw new TypeError('Invalid command');
  if (/^node(?:\.exe)?$/i.test(command)) return { command: process.execPath, args };
  const leaf = basename(command).toLowerCase().replace(/\.cmd$/, '');
  if (leaf === 'npm' || leaf === 'npx') {
    const directories = [dirname(command), dirname(process.execPath), ...(process.env.PATH ?? '').split(delimiter)];
    for (const directory of directories) {
      for (const cli of [join(directory, 'npm', 'bin', `${leaf}-cli.js`), join(directory, 'node_modules', 'npm', 'bin', `${leaf}-cli.js`)])
        if (existsSync(cli)) return { command: process.execPath, args: [cli, ...args] };
    }
    throw new Error('npm CLI is unavailable');
  }
  return { command, args };
}

async function listTools(client, signal) {
  const tools = [];
  let cursor;
  for (let page = 0; page < 32; ++page) {
    const result = await client.listTools(cursor ? { cursor } : {}, { signal, timeout: 60000 });
    tools.push(...result.tools);
    if (tools.length > 1000) throw new RangeError('Too many tools');
    cursor = result.nextCursor;
    if (!cursor) return tools;
  }
  throw new RangeError('Too many pages');
}

async function handle(message) {
  const { id, op, server } = message;
  if (!Number.isSafeInteger(id) || id < 1) return;
  if (op === 'cancel') {
    pending.get(message.target)?.abort();
    output({ id, result: { ok: true } });
    return;
  }
  const abort = new AbortController();
  pending.set(id, abort);
  try {
    if (op === 'connect') {
      if (!server || typeof message.config !== 'object') throw new TypeError('Invalid source');
      const previous = clients.get(server);
      clients.delete(server);
      if (previous) await previous.client.close();
      const config = message.config;
      let transport;
      if (config.transport === 'http') {
        const url = new URL(config.url);
        if (!['http:', 'https:'].includes(url.protocol) || url.username || url.password) throw new TypeError('Invalid URL');
        transport = new StreamableHTTPClientTransport(url, { requestInit: { headers: config.headers ?? {}, redirect: 'error' } });
      } else {
        const environment = { ...getDefaultEnvironment(), ...(config.environment ?? config.env ?? {}) };
        environment.PATH = `${dirname(process.execPath)}${delimiter}${environment.PATH ?? environment.Path ?? process.env.PATH ?? ''}`;
        transport = new StdioClientTransport({ ...stdioCommand(config), env: environment, cwd: config.workingDirectory || undefined, stderr: 'pipe', maxBufferSize: 8 * 1024 * 1024 });
        transport.stderr?.on('data', () => {});
      }
      const client = new Client({ name: 'VS-Renderer-GUI', version: '1.1.1' }, { capabilities: {} });
      const entry = { client, transport };
      clients.set(server, entry);
      client.onclose = () => {
        if (clients.get(server) === entry) { clients.delete(server); output({ event: 'disconnected', server }); }
      };
      try {
        await client.connect(transport, { signal: abort.signal, timeout: 60000 });
        const tools = await listTools(client, abort.signal);
        output({ id, result: { ok: true, tools, serverInfo: client.getServerVersion() ?? {} } });
      } catch (error) {
        if (clients.get(server) === entry) clients.delete(server);
        await client.close().catch(() => {});
        throw error;
      }
    } else if (op === 'disconnect') {
      const entry = clients.get(server);
      clients.delete(server);
      if (entry) await entry.client.close();
      output({ id, result: { ok: true } });
    } else if (op === 'tools') {
      const entry = clients.get(server);
      if (!entry) throw new Error('Disconnected');
      output({ id, result: { ok: true, tools: await listTools(entry.client, abort.signal) } });
    } else if (op === 'call') {
      const entry = clients.get(server);
      if (!entry) throw new Error('Disconnected');
      const result = await entry.client.callTool({ name: message.name, arguments: message.arguments ?? {} }, { signal: abort.signal, timeout: 120000 });
      output({ id, result: { ok: !result.isError, ...result } });
    } else if (op === 'ping') {
      output({ id, result: { ok: true, node: process.version, sdk: '2.4.0' } });
    } else throw new TypeError('Unknown operation');
  } catch (error) {
    // Do not echo server stderr, credentials or request headers in transport errors.
    output({ id, result: { ok: false, error: abort.signal.aborted ? 'MCP request cancelled.' : `MCP ${op} failed (${error?.name ?? 'Error'}${Number.isInteger(error?.code) ? ` ${error.code}` : ''}).` } });
  } finally {
    pending.delete(id);
  }
}

const input = createInterface({ input: process.stdin, crlfDelay: Infinity });
input.on('line', line => {
  if (Buffer.byteLength(line) > 8 * 1024 * 1024) return;
  try {
    const message = JSON.parse(line);
    if (message.op === 'host-ready') { hostReady = true; for (const request of startupRequests.splice(0)) void handle(request); return; }
    if (!hostReady) { if (startupRequests.length < 1000) startupRequests.push(message); return; }
    void handle(message);
  } catch { /* A malformed line never becomes a command. */ }
});
input.on('close', async () => {
  for (const abort of pending.values()) abort.abort();
  await Promise.allSettled([...clients.values()].map(entry => entry.client.close()));
  process.exit(0);
});
