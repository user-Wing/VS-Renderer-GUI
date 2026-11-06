"""Stage the GLSL import tools beside the player without a Python runtime dependency."""
import argparse
from pathlib import Path
import shutil
import filecmp

parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, required=True, help='Application runtime directory')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = root / '.deps/color/ucrt64'
files = ('glslc.exe', 'spirv-cross.exe', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')
if not all((source / 'bin' / name).is_file() for name in files):
    print('GLSL import tools are not installed; native HLSL and cached shaders remain available.')
else:
    target = args.output / 'runtime/shader-tools'
    target.mkdir(parents=True, exist_ok=True)
    for name in files:
        if not (target / name).is_file() or not filecmp.cmp(source / 'bin' / name, target / name, shallow=False):
            shutil.copy2(source / 'bin' / name, target / name)
    for name in ('shaderc', 'glslang', 'spirv-tools', 'spirv-cross', 'libgcc', 'libstdc++', 'libwinpthread', 'winpthreads'):
        license_path = source / 'share/licenses' / name
        if license_path.is_dir():
            shutil.copytree(license_path, target / 'licenses' / name, dirs_exist_ok=True)
    shutil.copy2(root / 'third_party/color/packages.json', target / 'packages.json')
    print('Staged GLSL import tools: ' + str(target))
