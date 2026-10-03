"""Isolate the optional color DLLs from Qt/VS's GCC and shaderc runtime names."""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path


def imports(data):
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not a PE file')
    count, opt_size = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    opt = pe + 24
    if struct.unpack_from('<H', data, opt)[0] != 0x20b:
        raise ValueError('Expected PE32+ x64')
    sections = []
    for n in range(count):
        header = opt + opt_size + n * 40
        virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', data, header + 8)
        sections.append((rva, max(virtual_size, raw_size), raw))

    def offset(rva):
        for start, size, raw in sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise ValueError('Import RVA outside PE sections')

    rva = struct.unpack_from('<I', data, opt + 120)[0]
    if not rva:
        return []
    cursor, result = offset(rva), []
    while any(data[cursor:cursor + 20]):
        name_offset = offset(struct.unpack_from('<I', data, cursor + 12)[0])
        end = data.index(0, name_offset)
        result.append((name_offset, data[name_offset:end].decode('ascii')))
        cursor += 20
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dependencies', type=Path, required=True)
    parser.add_argument('--bridge', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    binaries = {file.name.lower(): file for file in (args.dependencies / 'ucrt64/bin').glob('*.dll')}
    required = {'libplacebo-360.dll'}
    pending = list(required)
    while pending:
        for _, name in imports(binaries[pending.pop()].read_bytes()):
            key = name.lower()
            if key in binaries and key not in required:
                required.add(key)
                pending.append(key)
            elif not (key in binaries or key.startswith(('api-ms-', 'ext-ms-')) or (Path('C:/Windows/System32') / name).exists()):
                raise ValueError(f'Missing color dependency: {name}')
    mapping = {key: 'v3_' + binaries[key].name[3:] for key in required}
    args.output.mkdir(parents=True, exist_ok=True)
    records = []
    for file, name in [(binaries[key], mapping[key]) for key in sorted(required)] + [(args.bridge, 'vsr-color.dll')]:
        original = file.read_bytes()
        data = bytearray(original)
        for pos, dependency in imports(data):
            replacement = mapping.get(dependency.lower())
            if replacement:
                assert len(replacement) == len(dependency)
                data[pos:pos + len(dependency)] = replacement.encode('ascii')
        destination = args.output / name
        destination.write_bytes(data)
        records.append({'file': name, 'source': file.name, 'sha256': hashlib.sha256(data).hexdigest(),
                        'original_sha256': hashlib.sha256(original).hexdigest()})
    licenses = args.dependencies / 'ucrt64/share/licenses'
    shutil.copytree(licenses, args.output / 'licenses', dirs_exist_ok=True)
    (args.output / 'runtime.json').write_text(json.dumps({'dlls': records, 'modification': 'PE import names isolated with v3_ prefix; executable code unchanged'}, indent=2), encoding='utf-8')
    print(f'Color runtime: {len(records)} DLLs staged in {args.output}')


if __name__ == '__main__':
    main()
