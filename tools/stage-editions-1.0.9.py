"""Stage both editions, resolving native/FFmpeg DLL dependencies recursively."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--root', required=True)
p.add_argument('--runtime', required=True)
p.add_argument('--native', required=True)
p.add_argument('--native-dependencies', required=True)
p.add_argument('--qt', required=True)
p.add_argument('--mingw', required=True)
p.add_argument('--dumpbin', required=True)
p.add_argument('--crt', default='C:/BuildTools/2026/VC/Redist/MSVC/14.51.36231/x64/Microsoft.VC145.CRT')
p.add_argument('--edition', choices=['Full', 'Lite', 'Both'], default='Both')
a = p.parse_args()
root, runtime, native = Path(a.root), Path(a.runtime), Path(a.native)
build, dist = root / 'build/mingw-release', root / 'dist'
version = re.search(r'project\(VSRenderer VERSION ([0-9.]+)', (root / 'CMakeLists.txt').read_text(encoding='utf-8')).group(1)
qt, mingw = Path(a.qt), Path(a.mingw)
system = Path('C:/Windows/System32')

def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)

def closure(target, seeds, skip_ffmpeg=False):
    # FFmpeg's current pthread runtime has time64 exports absent in Qt's older
    # toolchain runtime; prefer the bundled runtime for shared dependencies.
    directories = [target, runtime, Path(a.native_dependencies), qt / 'bin', mingw, system]
    seen = set()
    queue = list(seeds)
    while queue:
        file = queue.pop()
        key = str(file).lower()
        if key in seen:
            continue
        seen.add(key)
        listing = subprocess.check_output([a.dumpbin, '/dependents', str(file)]).decode(errors='replace')
        for name in re.findall(r'^\s+([\w.+-]+\.dll)\s*$', listing, re.M | re.I):
            if skip_ffmpeg and re.match(r'^(avcodec|avdevice|avformat|avutil|avfilter|swscale|swresample)', name, re.I):
                continue
            if name.lower().startswith(('api-ms-', 'ext-ms-')):
                continue
            # D3DCompiler 47 is supplied by the supported Windows 10+ system.
            if name.lower() == 'd3dcompiler_47.dll' and (system / name).is_file():
                continue
            source = next((d / name for d in directories if (d / name).is_file()), None)
            if source is None:
                raise RuntimeError(f'Missing dependency: {file.name} -> {name}')
            if source.parent == system:
                if not name.lower().startswith(('msvcp140', 'vcruntime140')):
                    continue
            dest = target / name
            if source != dest:
                copy(source, dest)
            queue.append(dest)
    return seen

# RIFE/ncnn's model loader uses narrow Windows paths. Keep the shipping path ASCII.
for edition, folder in [('Full', f'{version}-Full'), ('Lite', f'{version}-PlayerLite')]:
    if a.edition not in (edition, 'Both'):
        continue
    target = dist / folder
    target.mkdir(parents=True, exist_ok=True)
    if any(target.iterdir()):
        raise RuntimeError(f'Staging directory must be empty: {target}')
    if edition == 'Full':
        for directory in ['runtime', 'languages', 'vpy', 'shaders', 'shader-licenses',
                          'filter-licenses', 'rife-licenses', 'LAVFilters64', 'madVR09217',
                          'tools', 'color', 'imageformats', 'iconengines', 'styles', 'tls',
                          'networkinformation']:
            source = runtime / directory
            if source.is_dir():
                shutil.copytree(source, target / directory, ignore=shutil.ignore_patterns('__pycache__', '*.pyc', '*.log', '*.ini', '*.lwi', '*.ffindex', 'cache', 'data', 'screenshots', 'shader-cache', 'user', 'PhotoCraftData'))
        for source in runtime.glob('*.dll'):
            if source.name != 'FFF.Native.dll':
                copy(source, target / source.name)
        for name in ['ffmpeg.exe', 'ffprobe.exe']:
            copy(runtime / name, target / name)
        programs = ['VSRenderer.exe', 'vs-player.exe', 'vs-cap.exe']
        for name in programs:
            copy(build / name, target / name)
    else:
        programs = ['vs-player.exe']
        copy(build / 'lite/vs-player.exe', target / 'vs-player.exe')
        # Text subtitle rendering calls FFmpeg; retain the full FFmpeg build
        # for now, as requested, while collecting only its dependency closure.
        for name in ['ffmpeg.exe', 'ffprobe.exe']:
            copy(runtime / name, target / name)
        for directory in ['languages', 'tls', 'networkinformation', 'imageformats', 'iconengines', 'styles']:
            source = runtime / directory
            if source.is_dir():
                # Windows uses Schannel for TLS and certificate parsing.
                shutil.copytree(source, target / directory,
                    ignore=shutil.ignore_patterns('qcertonlybackend.dll') if directory == 'tls' else None)
    copy(native, target / 'FFF.Native.dll')
    for name in ['msvcp140.dll', 'msvcp140_2.dll', 'vcruntime140.dll', 'vcruntime140_1.dll']:
        copy(Path(a.crt) / name, target / name)
    for source in root.glob('LICENSE*'):
        if source.is_file():
            copy(source, target / source.name)
    for source in Path(a.native_dependencies).glob('*.dll'):
        if edition == 'Full':
            copy(source, target / source.name)
    for name in ['zh_CN.json', 'en_US.json']:
        copy(root / 'assets/languages' / name, target / 'languages' / name)
    for name in ['README.md', 'project.md', 'changelog.md', 'THIRD_PARTY_NOTICES.md']:
        copy(root / name, target / name)
    for name in [f'validation-{version}.md', f'release-{version}.md']:
        copy(root / 'docs' / name, target / 'docs' / name)
    for name in ['Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Network.dll', 'Qt6Concurrent.dll']:
        copy(qt / 'bin' / name, target / name)
    copy(qt / 'plugins/platforms/qwindows.dll', target / 'platforms/qwindows.dll')
    plugins = [f for d in ['platforms', 'imageformats', 'iconengines', 'styles', 'tls', 'networkinformation']
               for f in (target / d).glob('*.dll')]
    closure(target, [target / name for name in programs] + [target / 'FFF.Native.dll',
            target / 'ffmpeg.exe', target / 'ffprobe.exe'] + plugins)
    for name in programs:
        subprocess.run([str(mingw / 'strip.exe'), '--strip-all', str(target / name)], check=True)
    if edition == 'Lite':
        for name in ['libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Network.dll', 'Qt6Concurrent.dll']:
            subprocess.run([str(mingw / 'strip.exe'), '--strip-unneeded', str(target / name)], check=True)
    files = sorted(f for f in target.rglob('*') if f.is_file())
    total = sum(f.stat().st_size for f in files)
    app_dependencies = closure(target, [target / name for name in programs] + [target / 'FFF.Native.dll'] + plugins, True)
    ff_dependencies = closure(target, [target / 'ffmpeg.exe', target / 'ffprobe.exe'])
    ff_only = ff_dependencies - app_dependencies
    without_ff = sum(f.stat().st_size for f in files if str(f).lower() not in ff_only)
    (target / 'release.json').write_text(json.dumps(dict(version=version, edition=edition,
        platform='windows-x64', totalBytes=total, excludingFFmpegBytes=without_ff,
        ffmpegOnlyFiles=sorted(Path(f).name for f in ff_only)), indent=2), encoding='utf-8')
    manifest = {f.relative_to(target).as_posix(): {'bytes': f.stat().st_size,
        'sha256': hashlib.file_digest(f.open('rb'), 'sha256').hexdigest()}
        for f in sorted(target.rglob('*')) if f.is_file()}
    (target / 'resource-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    if edition == 'Lite' and without_ff >= 50 * 1024 * 1024:
        raise RuntimeError(f'Lite exceeds 50 MiB excluding FFmpeg: {without_ff / 1048576:.2f} MiB')
    print(f'{folder}: {total/1048576:.2f} MiB total; {without_ff/1048576:.2f} MiB excluding FFmpeg', flush=True)
