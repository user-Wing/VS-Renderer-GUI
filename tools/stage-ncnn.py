"""Stage the user's installed VideoEnhancer NCNN still-image bundle."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
a.output.mkdir(parents=True, exist_ok=True)
python = a.output / 'python'
python.mkdir(exist_ok=True)
with zipfile.ZipFile(root / '.deps/ai-subtitles110/python-3.12.10-embed-amd64.zip') as z:
    z.extractall(python)
package = a.source / 'python/python/Lib/site-packages/upscale_ncnn_py'
metadata = next(package.parent.glob('upscale_ncnn_py-*.dist-info'))
shutil.copytree(metadata / 'licenses', a.output / 'licenses/upscale-ncnn-py', dirs_exist_ok=True)
shutil.copy2(metadata / 'METADATA', a.output / 'licenses/upscale-ncnn-py/METADATA.txt')
for file in package.iterdir():
    if file.suffix in ('.pyd', '.dll'):
        shutil.copy2(file, a.output / file.name)
models = a.source / 'models/Param-Bin'
shutil.copytree(models, a.output / 'models', dirs_exist_ok=True,
                ignore=shutil.ignore_patterns('EfficientNet-SceneDetect'))
records = []
for param in sorted(models.rglob('*.param')):
    name = param.parent.name
    if name == 'EfficientNet-SceneDetect':
        continue
    match = re.search(r'(?:([1-4])x|x([1-4])|([1-4])-?x)', name, re.I)
    if not match:
        raise ValueError('Unknown model scale: ' + name)
    scale = int(next(group for group in match.groups() if group))
    records.append({'name': name, 'param': param.relative_to(models).as_posix(), 'scale': scale})
(a.output / 'models.json').write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
shutil.copy2(root / 'assets/ncnn-worker.py', a.output / 'ncnn-worker.py')
hashes = {str(file.relative_to(a.output)).replace('\\', '/'): hashlib.sha256(file.read_bytes()).hexdigest()
          for file in a.output.rglob('*') if file.is_file() and file.name != 'manifest.json'}
(a.output / 'manifest.json').write_text(json.dumps({'source': str(a.source), 'files': hashes}, indent=2) + '\n', encoding='utf-8')
print('Staged NCNN models:', len(records))
