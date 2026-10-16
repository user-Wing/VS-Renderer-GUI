"""Update the two existing local runtimes without packaging or changing settings."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from datetime import datetime

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'build/mingw-release'
NATIVE = ROOT / 'build/native-api20/FFF.Native/x64/Release/FFF.Native.dll'
STRIP = Path('C:/Private/Soft/QT/Tools/mingw1310_64/bin/strip.exe')


def digest(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def settings(root):
    return {str(p.relative_to(root)): digest(p) for p in root.rglob('*')
            if p.is_file() and p.suffix.lower() in ('.ini', '.vpy', '.cube', '.icc', '.icm')}


def replace(source, target, strip=False):
    target.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix='.' + target.name + '-', suffix='.new', dir=target.parent)
    os.close(descriptor)
    temporary = Path(name)
    try:
        shutil.copy2(source, temporary)
        if strip:
            subprocess.run([str(STRIP), '--strip-all', str(temporary)], check=True)
        os.replace(temporary, target)
    finally:
        temporary.unlink(missing_ok=True)


def write_json(target, data):
    descriptor, name = tempfile.mkstemp(prefix='.' + target.name + '-', suffix='.new', dir=target.parent)
    os.close(descriptor)
    temporary = Path(name)
    try:
        temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        os.replace(temporary, target)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    backup = ROOT / 'build' / ('deployment-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    report = {'upstream': '0fe651ddab8a2bb6ddb729f314f9f4c5117fb565', 'editions': {}}
    editions = {
        'VS-GUI-Full': {'VSRenderer.exe': BUILD / 'VSRenderer.exe', 'vs-player.exe': BUILD / 'vs-player.exe', 'vs-cap.exe': BUILD / 'vs-cap.exe'},
        '1.0.10-PlayerLite': {'vs-player.exe': BUILD / 'lite/vs-player.exe'},
    }
    for folder, programs in editions.items():
        target = ROOT / 'dist' / folder
        if not (target / 'release.json').is_file():
            raise RuntimeError(f'Existing runtime required: {target}')
        for source in [*programs.values(), NATIVE]:
            if not source.is_file():
                raise FileNotFoundError(source)
    for folder, programs in editions.items():
        target = ROOT / 'dist' / folder
        before = settings(target)
        sources = {**programs, 'FFF.Native.dll': NATIVE, 'THIRD_PARTY_NOTICES.md': ROOT / 'THIRD_PARTY_NOTICES.md'}
        for source in (ROOT / 'third_party/native-api20').iterdir():
            if source.is_file():
                sources['licenses/native-api20/' + source.name] = source
        sources['licenses/preset-community-LICENSE.txt'] = ROOT / 'third_party/preset-community-LICENSE.txt'
        if folder == 'VS-GUI-Full':
            for file in ['README.md', 'project.md', 'changelog.md', 'docs/update-20261016.md', 'docs/native-api20-build.md', 'docs/vscap-2026-10-16-multipin-awj-hdr.md']:
                sources[file] = ROOT / file
        changed = {}
        for relative, source in sources.items():
            destination = target / relative
            if destination.is_file():
                saved = backup / folder / relative
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(destination, saved)
            replace(source, destination, relative in programs)
            changed[relative] = digest(destination)
        after = settings(target)
        if before != after:
            raise RuntimeError(f'Settings/source data changed unexpectedly: {folder}')
        release = json.loads((target / 'release.json').read_text(encoding='utf-8'))
        files = [p for p in target.rglob('*') if p.is_file() and p.name not in ('release.json', 'resource-manifest.json')]
        release['totalBytes'] = sum(p.stat().st_size for p in files)
        ffmpeg = set(release['ffmpegOnlyFiles'])
        release['excludingFFmpegBytes'] = sum(p.stat().st_size for p in files if p.name not in ffmpeg)
        release['nativeApi'] = 20
        release['nativeUpstream'] = report['upstream']
        write_json(target / 'release.json', release)
        manifest = {p.relative_to(target).as_posix(): {'bytes': p.stat().st_size, 'sha256': digest(p)}
                    for p in sorted(target.rglob('*')) if p.is_file() and p.name != 'resource-manifest.json'}
        write_json(target / 'resource-manifest.json', manifest)
        report['editions'][folder] = {'files': changed, 'preservedSettings': before, 'settingsUnchanged': True}
    backup.mkdir(parents=True, exist_ok=True)
    write_json(backup / 'deployment.json', report)
    print(json.dumps({'backup': str(backup), 'editions': list(editions), 'settingsUnchanged': True}))


if __name__ == '__main__':
    main()
