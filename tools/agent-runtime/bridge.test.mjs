import { test } from 'node:test';
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { createInterface } from 'node:readline';
import { fileURLToPath } from 'node:url';

const fixture = fileURLToPath(new URL('./fixture-server.mjs', import.meta.url));
function bridge({ waitForHost = false } = {}) {
  const process = spawn(globalThis.process.execPath, [fileURLToPath(new URL('./bridge.mjs', import.meta.url))], { stdio: ['pipe', 'pipe', 'pipe'], env: { ...globalThis.process.env, VSR_AGENT_WAIT_FOR_HOST: waitForHost ? '1' : '0' } });
  const pending = new Map(); let next = 0;
  createInterface({ input: process.stdout }).on('line', line => { const message = JSON.parse(line); if (pending.has(message.id)) { pending.get(message.id)(message.result); pending.delete(message.id); } });
  return {
    request(body) { const id = ++next; return { id, result: new Promise(resolve => { pending.set(id, resolve); process.stdin.write(`${JSON.stringify({ id, ...body })}\n`); }) }; },
    ready() { process.stdin.write('{"op":"host-ready"}\n'); },
    async close() { process.stdin.end(); await new Promise(resolve => process.once('exit', resolve)); }
  };
}

test('managed bridge waits for host process containment before starting a source', async () => {
  const client = bridge({ waitForHost: true });
  try {
    let completed = false;
    const request = client.request({ op: 'connect', server: 'fixture', config: { transport: 'stdio', command: process.execPath, arguments: [fixture] } }).result.then(value => { completed = true; return value; });
    await new Promise(resolve => setTimeout(resolve, 150)); assert.equal(completed, false);
    client.ready(); assert.equal((await request).ok, true);
  } finally { await client.close(); }
});

test('stdio initializes, discovers and executes concurrent MCP tools', async () => {
  const client = bridge();
  try {
    const connected = await client.request({ op: 'connect', server: 'fixture', config: { transport: 'stdio', command: process.execPath, arguments: [fixture] } }).result;
    assert.equal(connected.ok, true); assert.equal(connected.tools.length, 3);
    const results = await Promise.all([
      client.request({ op: 'call', server: 'fixture', name: 'add', arguments: { a: 19, b: 23 } }).result,
      client.request({ op: 'call', server: 'fixture', name: 'echo', arguments: { text: '软件工具桥' } }).result
    ]);
    assert.equal(results[0].structuredContent.sum, 42); assert.equal(results[1].content[0].text, '软件工具桥');
    const waiting = client.request({ op: 'call', server: 'fixture', name: 'wait', arguments: { milliseconds: 2000 } });
    await new Promise(resolve => setTimeout(resolve, 50));
    await client.request({ op: 'cancel', target: waiting.id }).result;
    assert.equal((await waiting.result).ok, false);
    assert.equal((await client.request({ op: 'disconnect', server: 'fixture' }).result).ok, true);
    assert.equal((await client.request({ op: 'call', server: 'fixture', name: 'add', arguments: { a: 1, b: 2 } }).result).ok, false);
  } finally { await client.close(); }
});

test('Streamable HTTP initializes, lists and calls a real SDK server', async () => {
  const server = spawn(process.execPath, [fixture, '--http'], { stdio: ['pipe', 'pipe', 'pipe'] });
  const url = await new Promise(resolve => createInterface({ input: server.stdout }).once('line', line => resolve(JSON.parse(line).url)));
  const client = bridge();
  try {
    const connected = await client.request({ op: 'connect', server: 'http', config: { transport: 'http', url } }).result;
    assert.equal(connected.ok, true); assert.equal(connected.tools.length, 3);
    const result = await client.request({ op: 'call', server: 'http', name: 'add', arguments: { a: 13, b: 29 } }).result;
    assert.equal(result.ok, true); assert.equal(result.structuredContent.sum, 42);
  } finally { await client.close(); server.stdin.end(); await new Promise(resolve => server.once('exit', resolve)); }
});
