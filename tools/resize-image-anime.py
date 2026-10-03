"""Offline single-image Anime4K export; viewing never runs a VS graph."""
import sys
import subprocess
from pathlib import Path
import vapoursynth as vs

source, destination, shader, ffmpeg = sys.argv[1:5]
width, height = map(int, sys.argv[5:7])
core = vs.core
core.num_threads = 4
clip = core.ffms2.Source(source, cache=False, alpha=True)
yuv = core.resize.Bicubic(clip, format=vs.YUV444P16, matrix_s="709",
                           range_in_s="full", range_s="full")
result = core.placebo.Shader(yuv, shader=shader, width=width, height=height,
                             filter="ewa_lanczos", antiring=0.5)
result = core.resize.Point(result, format=vs.RGB48, matrix_in_s="709",
                            range_in_s="full", range_s="full")
frame = result.get_frame(0)
alpha = clip.get_frame(0).props.get("_Alpha")
alpha = (core.std.BlankClip(width=width, height=height, format=vs.GRAY16, color=65535)
         if alpha is None else core.resize.Lanczos(core.std.ModifyFrame(
             core.std.BlankClip(width=clip.width, height=clip.height, format=alpha.format.id),
             clips=clip, selector=lambda n, f: f.props["_Alpha"]),
             width=width, height=height, format=vs.GRAY16, filter_param_a=4))
raw = Path(destination).with_suffix(".raw")
with raw.open("wb") as output:
    for plane in (1, 2, 0):
        output.write(frame[plane].tobytes())
    output.write(alpha.get_frame(0)[0].tobytes())
subprocess.run([ffmpeg, "-v", "error", "-f", "rawvideo", "-pixel_format", "gbrap16le",
                "-video_size", f"{width}x{height}", "-i", str(raw), "-frames:v", "1",
                "-y", destination], check=True)
