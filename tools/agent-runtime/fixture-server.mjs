import { McpServer, WebStandardStreamableHTTPServerTransport } from '@modelcontextprotocol/server';
import { StdioServerTransport } from '@modelcontextprotocol/server/stdio';
import { z } from 'zod';
import { createServer } from 'node:http';
import { spawn } from 'node:child_process';

let child;
if (process.argv.includes('--stubborn')) {
  child = spawn(process.execPath, ['-e', 'setInterval(() => {}, 1000)'], { stdio: 'ignore', windowsHide: true });
  process.stdin.on('end', () => setInterval(() => {}, 1000));
}

function fixture() {
  const server = new McpServer({ name: 'vsr-mcp-fixture', version: '1.0.0' });
  server.registerTool('add', { description: 'Add two numbers', inputSchema: z.object({ a: z.number(), b: z.number() }) }, async ({ a, b }) => ({ content: [{ type: 'text', text: String(a + b) }], structuredContent: { sum: a + b } }));
  server.registerTool('echo', { description: 'Echo text', inputSchema: z.object({ text: z.string() }) }, async ({ text }) => ({ content: [{ type: 'text', text }], structuredContent: { pid: process.pid, childPid: child?.pid ?? 0 } }));
  server.registerTool('wait', { description: 'Wait a short time', inputSchema: z.object({ milliseconds: z.number().int().min(0).max(5000) }) }, async ({ milliseconds }) => { await new Promise(resolve => setTimeout(resolve, milliseconds)); return { content: [{ type: 'text', text: 'done' }] }; });
  return server;
}

if (process.argv.includes('--http')) {
  const http = createServer(async (req, res) => {
    const server = fixture();
    const transport = new WebStandardStreamableHTTPServerTransport({ sessionIdGenerator: undefined, enableJsonResponse: true });
    await server.connect(transport);
    try {
      const chunks = []; for await (const chunk of req) chunks.push(chunk);
      const body = Buffer.concat(chunks);
      const request = new Request(`http://127.0.0.1:${http.address().port}${req.url}`, { method: req.method, headers: req.headers, ...(body.length ? { body } : {}) });
      const response = await transport.handleRequest(request);
      res.writeHead(response.status, Object.fromEntries(response.headers));
      res.end(Buffer.from(await response.arrayBuffer()));
    } catch { res.writeHead(500); res.end(); }
    finally { await server.close(); }
  });
  http.listen(0, '127.0.0.1', () => process.stdout.write(`${JSON.stringify({ url: `http://127.0.0.1:${http.address().port}/mcp` })}\n`));
  process.stdin.resume(); process.stdin.on('end', () => http.close(() => process.exit(0)));
} else {
  await fixture().connect(new StdioServerTransport());
}
