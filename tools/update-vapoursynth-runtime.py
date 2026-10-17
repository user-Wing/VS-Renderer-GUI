"""Install pinned R81 and GPU/temporal plugins without removing existing plugins."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile
import urllib.request
import zipfile

PACKAGES = [
    ('vapoursynth', '81', 'vapoursynth-81-cp312-abi3-win_amd64.whl', '8de011b9d372ca1593ccbe6ea164c5d093ef44905a69597fa3d8099d38ce381c'),
    ('vapoursynth-bm3d', '11.0', 'vapoursynth_bm3d-11.0-py3-none-win_amd64.whl', 'd6b2223e2bf55df785d19fe1b347c59b6a7c78bd84cb328041ff52f2f10403a7'),
    ('vapoursynth-bm3dvk2', '1.0', 'vapoursynth_bm3dvk2-1.0-py3-none-win_amd64.whl', 'd213fe02022d62b77fab412f90e48028bbbb2045066ae2438507498902a8c633'),
    ('vapoursynth-bestsource', '22', 'vapoursynth_bestsource-22-py3-none-win_amd64.whl', '4dc2275d145128a2a86fa32efaab9fbab8e22582bfdf173b460f2ff33858c230'),
]


def replace(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(dir=path.parent, suffix='.new')
    try:
        with os.fdopen(descriptor, 'wb') as file:
            file.write(data)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site-packages', type=Path, required=True)
    parser.add_argument('--portable-archive', type=Path)
    parser.add_argument('--cache', type=Path, default=Path('.deps/vs-r81-wheels'))
    args = parser.parse_args()
    args.cache.mkdir(parents=True, exist_ok=True)
    manifest = []
    for package, version, filename, sha256 in PACKAGES:
        wheel = args.cache / filename
        if package == 'vapoursynth' and args.portable_archive:
            with zipfile.ZipFile(args.portable_archive) as portable:
                data = portable.read('wheel/' + filename)
        elif wheel.exists():
            data = wheel.read_bytes()
        else:
            with urllib.request.urlopen(f'https://pypi.org/pypi/{package}/{version}/json') as response:
                release = json.load(response)
            url = next(file['url'] for file in release['urls'] if file['filename'] == filename)
            with urllib.request.urlopen(url) as response:
                data = response.read()
        if hashlib.sha256(data).hexdigest() != sha256:
            raise RuntimeError(f'SHA256 mismatch: {filename}')
        replace(wheel, data)
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            for name in archive.namelist():
                if name.endswith(('/', '.pdb')):
                    continue
                if name.startswith('vapoursynth/'):
                    replace(args.site_packages / name, archive.read(name))
                elif '/licenses/' in name:
                    replace(args.site_packages / 'vapoursynth' / 'licenses' / package / Path(name).name, archive.read(name))
        manifest.append({'package': package, 'version': version, 'filename': filename, 'sha256': sha256})
    replace(args.site_packages / 'vapoursynth' / 'runtime-components.json', (json.dumps(manifest, indent=2) + '\n').encode())
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
