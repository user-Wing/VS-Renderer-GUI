"""Compile each bundled shader in its own bounded process; retain explicit failures."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--compiler', type=Path, required=True)
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--timeout', type=float, default=30)
a = parser.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
report = a.output / 'shader-report.json'
results = []
for shader in sorted(p for p in a.source.rglob('*') if p.is_file() and p.suffix.lower() in ('.glsl', '.hook', '.hlsl')):
    started = time.monotonic()
    flags = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0
    process = subprocess.Popen([str(a.compiler.resolve()), str(a.source.resolve()), str(a.output.resolve()), str(shader.resolve())], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=flags)
    try:
        code = process.wait(timeout=a.timeout)
        data = json.loads(report.read_text(encoding='utf-8')) if code == 0 else {}
        row = next((x for x in data.get('results', []) if Path(x['file']).resolve() == shader.resolve()), {'file': str(shader), 'compiled': False, 'error': f'Compiler exited with code {code}'})
    except subprocess.TimeoutExpired:
        if os.name == 'nt':
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        else:
            process.kill()
        process.wait()
        row = {'file': str(shader), 'compiled': False, 'error': f'Offline compilation exceeded {a.timeout:g}s'}
    row['seconds'] = round(time.monotonic()-started, 3)
    results.append(row)
    summary = {'compiled': sum(bool(r['compiled']) for r in results), 'total': len(results), 'results': results}
    (a.output / 'shader-batch-report.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    if len(results) % 50 == 0:
        print(f"{len(results)} processed, {summary['compiled']} compiled", flush=True)
print(f"{summary['compiled']} / {summary['total']}", flush=True)
