"""Rebuild the pinned MoeSR FP32 tail convolution schedule (52 passes).

Only passes 40..50 are reordered for texture locality. Weights, FP32 arithmetic,
FP16 surfaces and the pixel shuffle are preserved. This changes rounding order;
see the recorded RGBA error limits, not a claim of bitwise equivalence.
"""
from pathlib import Path
import argparse, hashlib, json, re, runpy, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SHA = '5ecdfa7cfe1576efcd98f7dc31df96b64fd43b24be43fdc5eeef7103a052ed77'
REFERENCE_SHA = '9befd671f24512e74e07121c1dfa6ddafce71f7a936549947b6ddf08ad2a4661'

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--fxc', type=Path, default=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'))
    parser.add_argument('--output', type=Path, default=ROOT/'assets/builtin-shaders/moesr-50k.hlsl.vsrhlsl')
    args = parser.parse_args()
    source = ROOT/'assets/builtin-shaders/moesr-50k.hlsl'
    reference = ROOT/'tests/fixtures/moesr-50k-reference.vsrhlsl'
    assert hashlib.sha256(source.read_bytes()).hexdigest() == SOURCE_SHA
    assert hashlib.sha256(reference.read_bytes()).hexdigest() == REFERENCE_SHA
    codec = runpy.run_path(str(ROOT/'tools/optimize-anime4k-mode-a.py'))
    rows = codec['unpack'](reference.read_bytes())
    blocks = re.split(r'(?=^//!HOOK)', source.read_text(encoding='utf-8'), flags=re.M)[1:]
    assert len(rows) == len(blocks) == 52
    assert [p['save'] for p in rows[40:51]] == [f'L5_{i}' for i in range(8)] + [f'L6_{i}' for i in range(3)]
    with tempfile.TemporaryDirectory() as temporary:
        for i in range(40, 51):
            lines = blocks[i].splitlines()
            operations = [line for line in lines if line.startswith('v +=')]
            assert len(operations) == 72
            reordered = iter(operations[n] for n in sorted(range(72), key=lambda n: (n % 8, n // 8)))
            text = '\n'.join(next(reordered) if line.startswith('v +=') else line for line in lines)
            hlsl = Path(temporary)/f'{i}.hlsl'
            hlsl.write_text(text, encoding='utf-8')
            code = hlsl.with_suffix('.cso')
            subprocess.run([str(args.fxc), '/nologo', '/T', 'ps_5_0', '/E', 'main', '/Ges', '/O3', '/Fo', str(code), str(hlsl)], check=True, stdout=subprocess.DEVNULL)
            rows[i]['code'] = code.read_bytes()
    result = codec['pack'](rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.output.with_suffix(args.output.suffix+'.tmp')
    temporary.write_bytes(result)
    temporary.replace(args.output)
    metadata = dict(semantics='luma-main-prekernel-params-v8', sha256=SOURCE_SHA,
                    optimization='moesr-fp32-tail11-texture-order-v1',
                    referencePackageSha256=REFERENCE_SHA,
                    packageSha256=hashlib.sha256(result).hexdigest())
    Path(str(args.output)+'.json').write_text(json.dumps(metadata, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(metadata))
