from pathlib import Path
import struct
from PIL import Image

root = Path(__file__).resolve().parent
sizes = [(n, n) for n in (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)]
output = root / 'ico'
output.mkdir(exist_ok=True)
for name in ('R1', 'R2', 'R3', 'P1', 'P2', 'P3', 'V1', 'I3'):
    source = Image.open(root / f'{name}.png').convert('RGBA')
    target = output / f'{name}.ico'
    source.save(target, format='ICO', sizes=sizes)
    data = target.read_bytes()
    assert struct.unpack_from('<HHH', data) == (0, 1, len(sizes))
    entries = [struct.unpack_from('<BBBBHHII', data, 6 + 16 * i)
               for i in range(len(sizes))]
    assert {(e[0] or 256, e[1] or 256) for e in entries} == set(sizes)
    assert all(e[5] == 32 and e[7] + e[6] <= len(data) for e in entries)
    icon = Image.open(target)
    assert icon.ico.sizes() == set(sizes)
    for size in sizes:
        frame = icon.ico.getimage(size).convert('RGBA')
        assert frame.size == size
        assert frame.getchannel('A').getextrema()[0] == 0
    print(f'{name}: PNG {source.size}, alpha {source.getchannel("A").getextrema()}, ICO {len(sizes)} sizes, 32-bit, {len(data)} bytes')
