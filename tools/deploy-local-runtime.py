"""Update the two existing local runtimes without packaging or changing settings."""
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import tempfile
from datetime import datetime

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'build/mingw-release'
NATIVE = BUILD / 'FFF.Native.dll'
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
    report = {'upstream': 'cc545d2afc3f7b45058f47434628db6a703c7b07', 'base': '146b5e03c784a01215bcc21b5dee60c97dabb089', 'startupPr': 48, 'editions': {}}
    editions = {
        ROOT / 'dist/VS-GUI-Full': {'VSRenderer.exe': BUILD / 'VSRenderer.exe', 'vs-player.exe': BUILD / 'vs-player.exe', 'vs-cap.exe': BUILD / 'vs-cap.exe'},
        ROOT / 'build/playerlite-runtime': {'vs-player.exe': BUILD / 'lite/vs-player.exe'},
    }
    for target, programs in editions.items():
        folder = target.name
        if not (target / 'release.json').is_file():
            raise RuntimeError(f'Existing runtime required: {target}')
        for source in [*programs.values(), NATIVE]:
            if not source.is_file():
                raise FileNotFoundError(source)
    for target, programs in editions.items():
        folder = target.name
        before = settings(target)
        sources = {**programs, 'FFF.Native.dll': NATIVE, 'THIRD_PARTY_NOTICES.md': ROOT / 'THIRD_PARTY_NOTICES.md'}
        for language in ('zh_CN', 'en_US'):
            sources['languages/' + language + '.json'] = ROOT / 'assets/languages' / (language + '.json')
        sources['licenses/jpeg-xl-LICENSE.txt'] = ROOT / 'assets/jpeg-xl-LICENSE.txt'
        for source in (ROOT / 'third_party/native-api20').iterdir():
            if source.is_file():
                sources['licenses/native-api20/' + source.name] = source
        sources['licenses/preset-community-LICENSE.txt'] = ROOT / 'third_party/preset-community-LICENSE.txt'
        for file in ['README.md', 'project.md', 'changelog.md']:
            sources[file] = ROOT / file
        for source in (ROOT / 'docs').glob('*.md'):
            sources[source.relative_to(ROOT).as_posix()] = source
        if folder == 'VS-GUI-Full':
            for source in (ROOT / 'assets/ai-subtitles').iterdir():
                if source.is_file():
                    sources['runtime/ai-subtitles/' + source.name] = source
            for source in (BUILD / 'color').iterdir():
                if source.is_file():
                    sources['color/' + source.name] = source
            sources['licenses/vsr-placebo-COPYING.txt'] = ROOT / 'third_party/vsr-placebo/COPYING'
            for source in (ROOT / 'third_party/vsr-placebo').iterdir():
                if source.is_file():
                    sources['licenses/vsr-placebo-source/' + source.name] = source
            sources['plugins/3FUI/VsrExportBridge.3fui.dll'] = ROOT / 'build/release-1.0.14/3fui-plugin/VsrExportBridge.3fui.dll'
            sources['plugins/3FUI/README.md'] = ROOT / 'plugins/3fui-export-bridge/README.md'
            sources['plugins/3fui-export-bridge/README.md'] = ROOT / 'plugins/3fui-export-bridge/README.md'
            sources['assets/ai-subtitles/README.md'] = ROOT / 'assets/ai-subtitles/README.md'
            for source in (BUILD / 'runtime/tools').glob('apply-*.ps1'):
                sources['runtime/tools/' + source.name] = source
            for source in [*(BUILD / 'runtime').glob('*.zip'), *(BUILD / 'runtime').glob('*.version')]:
                sources['runtime/' + source.name] = source
            for source in (BUILD / 'runtime/vlc').rglob('*'):
                if source.is_file():
                    sources['runtime/vlc/' + source.relative_to(BUILD / 'runtime/vlc').as_posix()] = source
            for source in (BUILD / 'runtime/awj-source').glob('*'):
                sources['runtime/awj-source/' + source.name] = source
            package = BUILD / 'runtime/python/Lib/site-packages/vapoursynth'
            for source in package.rglob('*'):
                if source.is_file() and (source.suffix in ('.dll', '.pyd', '.py', '.pyi', '.exe') or 'licenses' in source.parts or source.name == 'runtime-components.json'):
                    sources['runtime/python/Lib/site-packages/vapoursynth/' + source.relative_to(package).as_posix()] = source
        changed = {}
        for relative, source in sources.items():
            destination = target / relative
            if destination.is_file():
                saved = backup / folder / relative
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(destination, saved)
            replace(source, destination, relative in programs)
            changed[relative] = digest(destination)
        if folder == 'VS-GUI-Full':
            # Superseded loose tools/plugins must not shadow the new bundles.
            protected = {'.ini', '.vpy', '.cube', '.icc', '.icm'}
            for component in ('awj', 'mkvtoolnix', 'vlc'):
                directory = target / 'runtime' / component
                if component != 'vlc' and not (BUILD / 'runtime' / (component + '.zip')).is_file():
                    continue
                for file in directory.rglob('*'):
                    relative = file.relative_to(target).as_posix()
                    if file.is_file() and relative not in sources and file.suffix.lower() not in protected and file.name != 'settings.bin':
                        saved = backup / folder / relative
                        saved.parent.mkdir(parents=True, exist_ok=True)
                        shutil.move(str(file), saved)
        after = settings(target)
        if before != after:
            raise RuntimeError(f'Settings/source data changed unexpectedly: {folder}')
        release = json.loads((target / 'release.json').read_text(encoding='utf-8'))
        files = [p for p in target.rglob('*') if p.is_file() and p.name not in ('release.json', 'resource-manifest.json')]
        release['totalBytes'] = sum(p.stat().st_size for p in files)
        ffmpeg = set(release['ffmpegOnlyFiles'])
        release['excludingFFmpegBytes'] = sum(p.stat().st_size for p in files if p.name not in ffmpeg)
        release['version'] = re.search(r'project\(VSRenderer VERSION ([0-9.]+)', (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')).group(1)
        release['nativeApi'] = 20
        release['nativeUpstream'] = report['upstream']
        release['nativeBase'] = report['base']
        release['nativeStartupPr'] = report['startupPr']
        write_json(target / 'release.json', release)
        manifest = {p.relative_to(target).as_posix(): {'bytes': p.stat().st_size, 'sha256': digest(p)}
                    for p in sorted(target.rglob('*')) if p.is_file() and p.name != 'resource-manifest.json'}
        write_json(target / 'resource-manifest.json', manifest)
        report['editions'][folder] = {'files': changed, 'preservedSettings': before, 'settingsUnchanged': True}
    backup.mkdir(parents=True, exist_ok=True)
    write_json(backup / 'deployment.json', report)
    print(json.dumps({'backup': str(backup), 'editions': [str(p.relative_to(ROOT)) for p in editions], 'settingsUnchanged': True}))


if __name__ == '__main__':
    main()
