"""Archive the working source and selected validation evidence for manual transfer."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--output', required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
version = re.search(r'project\(VSRenderer VERSION ([0-9.]+)', (root / 'CMakeLists.txt').read_text(encoding='utf-8')).group(1)
output = Path(a.output).resolve()
if output.exists():
    raise RuntimeError(f'Archive already exists: {output}')
files = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others',
                                 '--exclude-standard'], cwd=root).decode().split('\0')
selected = {root / name for name in files if name and (root / name).is_file() and Path(name).suffix.lower() != '.zip'}
evidence = root / 'build/session-20261010'
for pattern in ['shipping-*.txt', 'shipping-*.csv', 'final-hdd-*.txt', 'final-hdd-*.csv',
                'final-hdd-*-summary.json', 'startup-final.*', 'hdd-streams.json',
                'upstream-prs.json', 'same-ssd-*.txt', 'reserve500-*.txt']:
    selected.update(evidence.glob(pattern))
for pattern in ['*.txt', '*.csv', '*.json', '*.py']:
    selected.update((root / 'build/session-20261010-op').glob(pattern))
manifest = dict(version=version,
                guiBase=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root).decode().strip(),
                nativeBase='20bcc001b24f6b6de4575bddf9b93ec452795b0d',
                nativePatch=f'patches/3fp-api18-vsrenderer-{version}.patch', files=[])
output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for file in sorted(selected):
        relative = file.relative_to(root).as_posix()
        data = file.read_bytes()
        manifest['files'].append(dict(path=relative, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        archive.writestr(f'VS-Renderer-GUI-{version}/' + relative, data)
    archive.writestr(f'VS-Renderer-GUI-{version}/source-manifest.json',
                     json.dumps(manifest, ensure_ascii=False, indent=2))
digest = hashlib.sha256(output.read_bytes()).hexdigest()
output.with_suffix(output.suffix + '.sha256').write_text(f'{digest}  {output.name}\n', encoding='utf-8')
print(f'{output}: {output.stat().st_size} bytes; {len(selected)} files; SHA256 {digest}')
