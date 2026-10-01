"""Exercise the generated interpolation helpers against the bundled VS runtime."""
import argparse
import json
import pathlib
import time

import vapoursynth as vs

parser = argparse.ArgumentParser()
parser.add_argument('--benchmark', action='store_true')
parser.add_argument('--source', help='Optional benchmark media (at least 16 frames)')
parser.add_argument('--media-regression', help='Check UV after skipped-frame requests in actual media')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
builder = (root / 'src/graph/VpyScriptBuilder.cpp').read_text(encoding='utf-8')
helpers = builder.split('R"PY(', 1)[1].split(')PY"', 1)[0]
core = vs.core
exec(helpers)


def moving_clip(width=320, height=180, length=12):
    base = core.std.BlankClip(width=width, height=height, length=length,
                              format=vs.YUV420P10, fpsnum=24000, fpsden=1001)
    square = core.std.BlankClip(base, width=32, height=32, color=[700, 512, 512])
    def moving(n):
        left = 16 + n * 4
        return core.std.AddBorders(square, left=left, right=width-left-32,
                                   top=32, bottom=height-64, color=[100, 512, 512])
    return core.std.FrameEval(base, eval=moving)


def pixels(frame):
    return tuple(bytes(frame[p]) for p in range(len(frame)))


def verify(name, source, output, factor):
    assert output.num_frames == source.num_frames * factor, name
    assert output.fps_num * source.fps_den == source.fps_num * factor * output.fps_den, name
    assert output.width == source.width and output.height == source.height, name
    for n in range(source.num_frames):
        expected = source.get_frame(n)
        actual = output.get_frame(n * factor)
        assert pixels(actual) == pixels(expected), (name, n, 'original changed')
        assert actual.props['_DurationNum'] * output.fps_num == actual.props['_DurationDen'] * output.fps_den
    for n in [1, output.num_frames - 1, min(3, output.num_frames - 1), 1]:
        frame = output.get_frame(n)
        assert frame.props['_DurationNum'] * output.fps_num == frame.props['_DurationDen'] * output.fps_den
    assert pixels(output.get_frame(output.num_frames - 1)) == pixels(source.get_frame(source.num_frames - 1))
    print(name, 'PASS: originals, count, FPS, duration, random seek, tail', flush=True)


source = moving_clip()
mv = _vsr_mvtools(source, block=16, pel=1, overlap=False,
                  chroma=False, searchparam=2, blend=False)
verify('MVTools', source, mv, 2)
assert pixels(mv.get_frame(1)) != pixels(source.get_frame(0)), 'MVTools did not interpolate'
for pel, block, overlap, chroma in [(2, 8, True, True), (1, 32, False, False)]:
    verify(f'MVTools pel={pel} block={block}', source,
           _vsr_mvtools(source, block, pel, overlap, chroma, 2, False), 2)
one = core.std.BlankClip(source, length=1)
verify('MVTools one frame', one, _vsr_mvtools(one, 16, 1, False, False, 2, False), 2)
tiny = core.std.BlankClip(source, width=64, height=36, length=3)
verify('MVTools tiny preview', tiny, _vsr_mvtools(tiny, 16, 1, False, False, 2, False), 2)
cut = core.std.BlankClip(source, length=3, color=[100, 512, 512]) + core.std.BlankClip(source, length=3, color=[900, 512, 512])
cut_out = _vsr_mvtools(cut, 16, 1, False, False, 2, False)
assert pixels(cut_out.get_frame(5)) == pixels(cut.get_frame(2)), 'scene cut should hold source'
print('MVTools scene cut PASS', flush=True)
for fmt, colors in [(vs.YUV420P8, [100, 100, 180]),
                    (vs.YUV420P10, [400, 400, 720]),
                    (vs.YUV420P16, [25600, 25600, 46080])]:
    colored = core.std.BlankClip(width=320, height=180, format=fmt, length=12,
                                 fpsnum=24, color=colors)
    expected = pixels(colored.get_frame(0))
    for chroma in [False, True]:
        for scale in [1, 2, 4]:
            out = _vsr_mvtools(colored, 16, 1, False, chroma, 2, False, scale)
            # Seek/drop patterns and simultaneous requests must retain valid UV.
            requests = [out.get_frame_async(n) for n in [17, 2, 7, 1, 21, 9, 4, 3]]
            for request in requests:
                assert pixels(request.result()) == expected, (fmt, chroma, scale, 'invalid chroma')
    print(f'MVTools colored {fmt} PASS: UV preserved during concurrent skipped/seek frames', flush=True)
for scale in [2, 4]:
    verify(f'MVTools scale={scale}', source,
           _vsr_mvtools(source, 16, 1, False, False, 2, False, scale), 2)

models = pathlib.Path(vs.__file__).parent / 'plugins/models'
# Prove that original output times never request the generated RGB/inference branch.
real_core = core
class InferenceTrap:
    def RIFE(self, c, **kwargs):
        doubled = real_core.std.Interleave([c, c])
        def reject(n, f):
            raise RuntimeError('inference branch requested for original frame')
        return real_core.std.ModifyFrame(doubled, clips=doubled, selector=reject)
class CoreProxy:
    rife = InferenceTrap()
    def __getattr__(self, key):
        return getattr(real_core, key)
core = CoreProxy()
direct = _vsr_rife(source, '', 2, 0, 1, False, 1)
for n in range(source.num_frames):
    assert pixels(direct.get_frame(n * 2)) == pixels(source.get_frame(n))
core = real_core
print('RIFE original branch bypass PASS (inference trap)', flush=True)
for model in ['rife-v4.26', 'rife-v4.26-heavy']:
    for scale in [1, 2, 4]:
        verify(f'{model} scale={scale}', source,
               _vsr_rife(source, str(models / model), 2, 0, 1, False, scale), 2)
verify('RIFE one frame', one, _vsr_rife(one, str(models / 'rife-v4.26'), 2, 0, 1, False, 1), 2)
for factor in [3, 4]:
    verify(f'RIFE {factor}x', source,
           _vsr_rife(source, str(models / 'rife-v4.26'), factor, 0, 1, False, 1), factor)

if args.media_regression:
    media = core.lsmas.LWLibavSource(source=args.media_regression)
    out = _vsr_mvtools(media, 16, 1, False, False, 2, False)
    u = core.std.PlaneStats(out, plane=1)
    v = core.std.PlaneStats(out, plane=2)
    rows = []
    for n in [771, 770, 1201, 7, 771]:
        n = min(n, out.num_frames - 1)
        uv = [u.get_frame(n).props['PlaneStatsAverage'], v.get_frame(n).props['PlaneStatsAverage']]
        assert all(value > 0 for value in uv), (n, uv, 'zero chroma')
        rows.append(dict(frame=n, u=uv[0], v=uv[1]))
    print('Actual media skipped-frame UV PASS', json.dumps(rows), flush=True)

if args.benchmark:
    source = core.lsmas.LWLibavSource(source=args.source) if args.source else moving_clip(1920, 1080, 16)
    assert source.num_frames >= 16
    for n in range(16):
        source.get_frame(n)
    rows = []
    cases = [('MVTools block=16', lambda: _vsr_mvtools(source, 16, 1, False, False, 2, False)),
             ('MVTools block=32', lambda: _vsr_mvtools(source, 32, 1, False, False, 2, False)),
             ('MVTools half size', lambda: _vsr_mvtools(source, 16, 1, False, False, 2, False, 2))]
    for scale in [1, 2, 4]:
        cases.append((f'RIFE scale={scale}', lambda scale=scale: _vsr_rife(
            source, str(models / 'rife-v4.26'), 2, 0, 1, False, scale)))
    for name, build in cases:
        out = build()
        out.get_frame(1)
        start = time.perf_counter()
        for n in range(3, 27, 2):
            out.get_frame(n)
        elapsed = time.perf_counter() - start
        rows.append(dict(name=name, inserted_fps=12 / elapsed, seconds=elapsed))
    print(json.dumps(rows, indent=2), flush=True)
