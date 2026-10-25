"""Measure complete VS output, retaining original frames in the RIFE helper.

Native Jinc/D3D11 presentation must be measured separately in the player.
Run each case in a fresh process to release Vulkan contexts between cases.
"""
import argparse
from collections import deque
import json
import os
from pathlib import Path
import re
import time

import vapoursynth as vs

p = argparse.ArgumentParser()
p.add_argument('--source', required=True)
p.add_argument('--case', choices=['pre-a4k-final-a4k', 'pre-a4k-jinc',
                                 'pre-a4k-d3d11', 'rife-final-a4k'], required=True)
p.add_argument('--threads', type=int, default=1)
p.add_argument('--shader', choices=['cnn', 'no-cnn', 'copy'], default='cnn')
p.add_argument('--sampling', choices=['420', '444'], default='444')
p.add_argument('--filter', default='ewa_lanczos')
p.add_argument('--plugin', help='Optional experimental planar Shader DLL')
p.add_argument('--lanes', type=int, choices=[1,2], default=1)
p.add_argument('--reduced-output', action='store_true', help='Resize the entire clip to inference resolution; original frames also lose resolution')
p.add_argument('--planar420', action='store_true')
p.add_argument('--phase', choices=['pipeline', 'rife', 'anime'], default='pipeline')
p.add_argument('--queue', type=int, default=4)
p.add_argument('--frames', type=int, default=480)
p.add_argument('--start', type=int, default=1440)
p.add_argument('--output', required=True)
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
if args.plugin:
    dll_directory = os.add_dll_directory(str(root / '.deps/color/ucrt64/bin'))
core = vs.core
if args.plugin:
    core.std.LoadPlugin(str(Path(args.plugin).resolve()))
core.max_cache_size = 1024
core.num_threads = 8
builder = (root / 'src/graph/VpyScriptBuilder.cpp').read_text(encoding='utf-8')
for helper in re.findall(r'R"PY\((.*?)\)PY"', builder, re.S):
    exec(helper)
model = str(Path(vs.__file__).parent / 'plugins/models/rife-v4.26')
shader = (root / 'assets' / ('anime4k-a-fast.glsl' if args.shader == 'cnn' else 'anime4k-no-cnn.glsl')).read_text(encoding='utf-8')
if args.shader == 'copy':
    shader = '//!HOOK MAIN\n//!BIND HOOKED\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}\n'
src = core.lsmas.LWLibavSource(args.source, cachedir=str(Path(args.output).parent))
src = core.resize.Point(src, format=vs.YUV420P16 if args.sampling == '420' else vs.YUV444P16)


def anime(clip):
    plugin = core.vsrplacebo if args.plugin else core.placebo
    options = {'subsampling': 1} if args.plugin and args.planar420 else {}
    if args.lanes == 2:
        branches = [plugin.Shader(core.std.SelectEvery(clip, cycle=2, offsets=n),
                    shader_s=shader, width=3840, height=2160, filter=args.filter, **options) for n in range(2)]
        return core.std.Interleave(branches)
    return plugin.Shader(clip, shader_s=shader, width=3840, height=2160, filter=args.filter, **options)


if args.phase == 'rife':
    clip = _vsr_rife(src, model, 2, 0, args.threads, True, 2)
elif args.phase == 'anime':
    clip = anime(src)
elif args.case.startswith('pre-a4k'):
    high = anime(src)
    clip = _vsr_rife(core.resize.Bicubic(high, width=960, height=540), model, 2, 0, args.threads, True, 1) if args.reduced_output else _vsr_rife(high, model, 2, 0, args.threads, True, 4)
    if args.case.endswith('final-a4k'):
        clip = anime(clip)
else:
    clip = anime(_vsr_rife(core.resize.Bicubic(src, width=960, height=540), model, 2, 0, args.threads, True, 1) if args.reduced_output else _vsr_rife(src, model, 2, 0, args.threads, True, 2))
start = min(args.start, clip.num_frames - args.frames - 32)
# Warm shader compilation and inference before the timed interval.
for n in range(start, start + 24):
    clip.get_frame(n).close()
first = start + 24
pending = deque()
times = []
t0 = time.perf_counter()
for n in range(first, first + args.frames):
    pending.append(clip.get_frame_async(n))
    if len(pending) >= args.queue:
        pending.popleft().result().close()
        times.append(time.perf_counter() - t0)
while pending:
    pending.popleft().result().close()
    times.append(time.perf_counter() - t0)
elapsed = time.perf_counter() - t0
result = dict(vars(args), width=clip.width, height=clip.height,
              fps_num=clip.fps_num, fps_den=clip.fps_den,
              elapsed=elapsed, output_fps=args.frames / elapsed,
              realtime=elapsed <= args.frames * clip.fps_den / clip.fps_num,
              completion_seconds=times,
              scope='VS complete frames; excludes Native upload and presentation')
Path(args.output).write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps({k: v for k, v in result.items() if k != 'completion_seconds'}), flush=True)
