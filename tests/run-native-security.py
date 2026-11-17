"""Run the instrumented Native DLL using generated, isolated media fixtures."""
import argparse
import ctypes
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--dll', type=Path, required=True)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--fixtures', type=Path, required=True)
args = parser.parse_args()
args.fixtures.mkdir(parents=True, exist_ok=True)
runtime = args.runtime.resolve()
handles = [os.add_dll_directory(str(runtime)), os.add_dll_directory(str(args.dll.resolve().parent))]
env = {key.upper(): value for key, value in os.environ.items()}
env['PATH'] = str(runtime) + os.pathsep + env.get('PATH', '')
ffmpeg = runtime / 'ffmpeg.exe'
for name, size in [('small.bmp','32x32'), ('4k.bmp','3840x2160'), ('8k.bmp','7680x4320'),
                   ('small.png','32x32'), ('small.jpg','32x32'), ('small.webp','32x32'),
                   ('small.tiff','32x32'), ('small.avif','32x32')]:
    path = args.fixtures / name
    if not path.exists():
        command = [str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-nostdin', '-f', 'lavfi',
                   '-i', 'color=c=red:s='+size, '-frames:v', '1', '-threads', '2']
        if path.suffix == '.avif':
            command += ['-c:v', 'libaom-av1', '-cpu-used', '8']
        subprocess.run(command + ['-y', str(path)], env=env, check=True, timeout=60,
                       creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS | subprocess.CREATE_NO_WINDOW)
seek = args.fixtures / 'seek.mp4'
if not seek.exists():
    subprocess.run([str(ffmpeg), '-hide_banner', '-loglevel', 'error', '-nostdin', '-f', 'lavfi',
                    '-i', 'testsrc2=s=64x64:r=24:d=1', '-c:v', 'libx264', '-threads', '2',
                    '-g', '12', '-bf', '2', '-y', str(seek)], env=env, check=True, timeout=60,
                   creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS | subprocess.CREATE_NO_WINDOW)
dll = ctypes.CDLL(str(args.dll.resolve()))
run = dll.FFF3FP_RunSecurityRegression
run.argtypes = [ctypes.c_char_p]
run.restype = ctypes.c_int
raise SystemExit(run(str(args.fixtures.resolve()).encode('utf-8')))
