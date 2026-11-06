"""One still RGB24 frame through VideoEnhancer's NCNN Vulkan wrapper."""
import argparse
import ctypes
import importlib.util
import os
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--model', required=True)
p.add_argument('--scale', type=int, required=True, choices=(1, 2, 3, 4))
p.add_argument('--width', type=int, required=True)
p.add_argument('--height', type=int, required=True)
p.add_argument('--gpu', type=int, default=0)
p.add_argument('--input', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parent
dll_directory = os.add_dll_directory(str(root))
spec = importlib.util.spec_from_file_location('upscale_ncnn_vulkan_wrapper', next(root.glob('*.pyd')))
wrapped = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wrapped)
model = root / 'models' / a.model
data = a.input.read_bytes()
if a.width <= 0 or a.height <= 0 or len(data) != a.width * a.height * 3:
    raise ValueError('Invalid RGB24 input')
engine = wrapped.UPSCALEWrapped(a.gpu, False, 4)
engine.set_parameters(64, a.scale)
if engine.load(str(model), str(model.with_suffix('.bin'))) != 0:
    raise RuntimeError('NCNN model loading failed')
source = wrapped.UPSCALEImage(data, a.width, a.height, 3)
# The wrapper borrows the Python byte buffers; retain both until inference ends.
output_data = bytes(len(data) * a.scale ** 2)
output = wrapped.UPSCALEImage(output_data, a.width * a.scale, a.height * a.scale, 3)
if engine.process(source, output) != 0:
    raise RuntimeError('NCNN inference failed')
a.output.write_bytes(output.get_data())
# This wheel faults during DLL process-detach after a successful inference.
# The one-frame child has closed its output; bypass DLL teardown on Windows.
if not ctypes.windll.kernel32.TerminateProcess(ctypes.c_void_p(-1), 0):
    raise ctypes.WinError()
