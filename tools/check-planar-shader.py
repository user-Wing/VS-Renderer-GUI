"""Compare the planar backend against the original packed Shader on real pixels."""
import argparse
import json
import os
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--source', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
dll_path = os.add_dll_directory(str(root / 'build/mingw-release/color'))
import vapoursynth as vs

core = vs.core
assert hasattr(core, 'vsrplacebo'), 'Planar plugin must actually be loaded'
source = core.lsmas.LWLibavSource(args.source, cachedir=str(Path(args.output).resolve().parent))
source = core.resize.Point(source, format=vs.YUV444P16)
shader = (root / 'assets/anime4k-a-fast.glsl').read_text(encoding='utf-8')
rows = []
for text, width, height in [('//!HOOK MAIN\n//!BIND HOOKED\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}', 320, 180),
                            (shader, 640, 360)]:
    small = core.resize.Bicubic(source, width=320, height=180)
    old = core.placebo.Shader(small, shader_s=text, width=width, height=height)
    new = core.vsrplacebo.Shader(small, shader_s=text, width=width, height=height)
    for n in (0, 24, 72):
        a, b = old.get_frame(n), new.get_frame(n)
        planes = []
        for plane in range(3):
            values_a = memoryview(a[plane]).cast('B').cast('H')
            values_b = memoryview(b[plane]).cast('B').cast('H')
            differences = [abs(x-y) for x,y in zip(values_a, values_b)]
            planes.append({'max': max(differences), 'mae': sum(differences)/len(differences)})
        rows.append({'frame': n, 'width': width, 'planes': planes})
        # Packed RGB48 and planar R16 use different GPU rounding paths.
        # Bound maximum error to half an 8-bit code value, without claiming bit identity.
        Path(args.output).write_text(json.dumps(rows, indent=2), encoding='utf-8')
        assert all(p['max'] <= 128 and p['mae'] < 32 for p in planes), rows[-1]
    output420 = core.vsrplacebo.Shader(small, shader_s=text, width=width, height=height, subsampling=1)
    frame = output420.get_frame(24)
    assert frame.format.id == vs.YUV420P16
    assert frame.width == width and frame.height == height
Path(args.output).write_text(json.dumps(rows, indent=2), encoding='utf-8')
print('Planar 444 pixels match packed Shader; 420 output format and dimensions verified')
