import { spawn } from 'node:child_process';
import { cp, mkdir } from 'node:fs/promises';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const directory = dirname(fileURLToPath(import.meta.url));
const npm = process.argv[2];
if (!npm || Number(process.versions.node.split('.')[0]) < 20) process.exit(2);
const child = spawn(process.execPath, [npm, 'ci', '--ignore-scripts', '--omit=dev', '--no-fund', '--no-audit'], { cwd: directory, stdio: 'ignore', windowsHide: true });
const code = await new Promise(resolve => { child.once('error', () => resolve(1)); child.once('exit', code => resolve(code ?? 1)); });
if (code) process.exit(code);
const source = dirname(dirname(npm));
const target = join(directory, 'npm');
if (resolve(source) !== resolve(target)) { await mkdir(target, { recursive: true }); await cp(source, target, { recursive: true, force: true }); }
