"""Trim a clean Full payload; retain all executable features and matching sources."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile


def pe(data):
    """Read the PE fields needed to guard symbol stripping."""
    offset = struct.unpack_from('<I', data, 60)[0]
    if data[offset:offset + 4] != b'PE\0\0':
        raise ValueError('Not a PE image')
    machine, count, _, symbols, symbol_count, optional, _ = struct.unpack_from('<HHIIIHH', data, offset + 4)
    header = offset + 24
    directories = header + (112 if struct.unpack_from('<H', data, header)[0] == 0x20b else 96)
    sections = []
    for index in range(count):
        at = header + optional + index * 40
        name = data[at:at + 8].split(b'\0')[0]
        if name.startswith(b'/') and symbols:
            start = symbols + symbol_count * 18 + int(name[1:])
            name = data[start:data.index(b'\0', start)]
        size, rva, raw_size, raw = struct.unpack_from('<IIII', data, at + 8)
        sections.append((name, size, rva, raw_size, raw))
    def position(rva):
        for _, size, address, raw_size, raw in sections:
            if address <= rva < address + max(size, raw_size):
                return raw + rva - address
        raise ValueError('RVA outside sections')
    def string(rva):
        at = position(rva)
        return data[at:data.index(b'\0', at)]
    imports = []
    rva = struct.unpack_from('<I', data, directories + 8)[0]
    if rva:
        at = position(rva)
        while struct.unpack_from('<I', data, at + 12)[0]:
            imports.append(string(struct.unpack_from('<I', data, at + 12)[0]))
            at += 20
    exports = []
    rva = struct.unpack_from('<I', data, directories)[0]
    if rva:
        at = position(rva)
        base, functions, names, addresses, labels, ordinals = struct.unpack_from('<IIIIII', data, at + 16)
        exports.append((base, data[position(addresses):position(addresses) + functions * 4]))
        for index in range(names):
            exports.append((struct.unpack_from('<H', data, position(ordinals) + index * 2)[0],
                            string(struct.unpack_from('<I', data, position(labels) + index * 4)[0])))
    code = [(name, rva, hashlib.sha256(data[raw:raw + min(size, raw_size)]).digest())
            for name, size, rva, raw_size, raw in sections if name == b'.text']
    signed = struct.unpack_from('<I', data, directories + 4 * 8 + 4)[0] != 0
    managed = struct.unpack_from('<I', data, directories + 14 * 8)[0] != 0
    eligible = symbol_count or any(name.startswith(b'.debug') for name, *_ in sections)
    return signed, managed, eligible, (machine, code, imports, exports)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--strip', type=Path, required=True)
    parser.add_argument('--sevenzip', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    args.strip = args.strip.resolve()
    args.sevenzip = args.sevenzip.resolve()
    project = Path(__file__).resolve().parents[1]
    target = args.directory.resolve()
    if target == project or not target.is_relative_to(project) or not (target / 'VSRenderer.exe').is_file():
        raise ValueError('Expected a Full payload inside the workspace')
    for directory, folders, files in os.walk(target):
        for name in folders + files:
            if (Path(directory) / name).lstat().st_file_attributes & 0x400:
                raise ValueError('Payload reparse point rejected')
    changes = []
    def remove(path, reason):
        if path.is_file():
            changes.append({'path':path.relative_to(target).as_posix(), 'reason':reason, 'saved':path.stat().st_size})
            path.unlink()
    runtime = target / 'runtime'
    # The old source is removable only after checking the current binary/source pair.
    source = runtime / 'awj-source/AWJimage-1.3.1-source.zip'
    if source.is_file():
        if hashlib.sha256(source.read_bytes()).hexdigest() != '82a33e3f54acaca830c37d03d3c671a0728473250e21b529030c44b4e4697124':
            raise ValueError('AWJ corresponding source hash mismatch')
        archive = next((runtime / ('awj.' + suffix) for suffix in ('7z', 'zip') if (runtime / ('awj.' + suffix)).is_file()), None)
        if archive:
            notice = subprocess.check_output([str(args.sevenzip), 'x', '-so', str(archive), 'NOTICE.txt'])
            if b'AWJimage 1.3.1' not in notice:
                raise ValueError('AWJ binary/source version mismatch')
            remove(runtime / 'awj-source/AWJimage_1.2.1_Source.tar.gz', 'superseded AWJ source')
            shutil.copy2(project / 'third_party/runtime-notices/awjimage-NOTICE.txt', runtime / 'awj-source/awjimage-NOTICE.txt')
    models = runtime / 'ncnn/models.json'
    if models.is_file():
        entries = json.loads(models.read_text(encoding='utf-8-sig'))
        if not any('EfficientNet-SceneDetect' in item.get('param', '') for item in entries):
            for path in (runtime / 'ncnn/models/EfficientNet-SceneDetect').glob('*'):
                remove(path, 'not present in NCNN inference catalog')
    for path in (runtime / 'agent').rglob('*'):
        if path.is_file() and (path.name.endswith(('.map', '.d.ts', '.d.mts', '.d.cts'))):
            remove(path, 'non-executable source map or type declaration')
    remove(target / 'ffplay.exe', 'not invoked by the application')
    with tempfile.TemporaryDirectory(prefix='full-trim-', dir=project / 'build') as temporary:
        work = Path(temporary)
        for component in ('awj', 'mkvtoolnix'):
            old, packed = runtime / (component + '.zip'), runtime / (component + '.7z')
            if not old.is_file():
                continue
            if packed.is_file():
                subprocess.run([str(args.sevenzip), 't', str(packed)], check=True, capture_output=True)
                remove(old, 'superseded ZIP tool archive')
                continue
            folder = work / component
            folder.mkdir()
            subprocess.run([str(args.sevenzip), 'x', str(old), '-o' + str(folder), '-y', '-bb0'], check=True, capture_output=True)
            archive = work / (component + '.7z')
            subprocess.run([str(args.sevenzip), 'a', '-t7z', str(archive), '.', '-mx=9', '-md=64m', '-ms=on', '-mmt=4', '-bb0'], cwd=folder, check=True, capture_output=True)
            before = old.stat().st_size
            shutil.copy2(archive, packed)
            old.unlink()
            changes.append({'path':packed.relative_to(target).as_posix(), 'reason':'solid tool archive', 'saved':before-packed.stat().st_size})
        for path in target.rglob('*'):
            if not path.is_file() or path.suffix.lower() not in ('.exe', '.dll', '.pyd'):
                continue
            signed, managed, eligible, fingerprint = pe(path.read_bytes())
            if signed or managed or not eligible:
                continue
            stripped = work / 'stripped.bin'
            shutil.copy2(path, stripped)
            subprocess.run([str(args.strip), '--strip-unneeded', str(stripped)], check=True, capture_output=True)
            if pe(stripped.read_bytes())[3] != fingerprint:
                raise ValueError('PE runtime fingerprint changed: ' + str(path))
            saved = path.stat().st_size - stripped.stat().st_size
            if saved > 1024:
                shutil.copy2(stripped, path)
                changes.append({'path':path.relative_to(target).as_posix(), 'reason':'non-runtime PE symbols', 'saved':saved})
    manifest = runtime / 'ncnn/manifest.json'
    if manifest.is_file():
        info = json.loads(manifest.read_text(encoding='utf-8-sig'))
        info['files'] = {path.relative_to(manifest.parent).as_posix():hashlib.sha256(path.read_bytes()).hexdigest()
                         for path in manifest.parent.rglob('*') if path.is_file() and path != manifest}
        manifest.write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'changes':changes, 'savedBytes':sum(item['saved'] for item in changes)}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f"Full runtime trimmed: {sum(item['saved'] for item in changes)} bytes; {len(changes)} changes")


if __name__ == '__main__':
    main()
