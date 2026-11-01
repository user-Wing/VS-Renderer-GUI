"""Compile the bundled Anime4K and Full player enhancement GLSL for D3D11.

Requires glslc/spirv-cross in .deps/color/ucrt64/bin and the Windows SDK fxc.
The upstream shader and its license remain in assets/anime4k-a-fast.glsl.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import runpy
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, default=root / 'build/anime4k-native/shaders')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
tools = root / '.deps/color/ucrt64/bin'
env = os.environ.copy()
env['PATH'] = str(tools) + os.pathsep + env['PATH']
fxc = Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
helper = runpy.run_path(str(root/'assets/gpu-sharpen.py'))['_vsr_sharpen_shader']
enhance = '\n'.join(helper(mode, 0.3, 2) for mode in ('sharpen_edges', 'enhance_detail'))
# The helper puts DESC after HOOK; normalize directive order for block splitting.
enhance = enhance.replace('//!HOOK LUMA\n//!BIND HOOKED\n//!DESC VS Renderer GPU sharpen',
                          '//!DESC VS Renderer GPU sharpen\n//!HOOK LUMA\n//!BIND HOOKED')
(args.output/'enhance.glsl').write_text(enhance,encoding='utf-8')
programs = [('fast', root/'assets/anime4k-a-fast.glsl'),
            ('nocnn', root/'assets/anime4k-no-cnn.glsl'), ('enhance', args.output/'enhance.glsl')]
blocks = []
for program, (_, source) in enumerate(programs):
    blocks += [(program, block) for block in re.split(r'(?=^//!DESC )', source.read_text(encoding='utf-8'), flags=re.M)[1:]]
manifest = []
embedded = ['// Generated from Anime4K (Copyright (c) 2019-2021 bloc97, MIT/Unlicense)',
            '// and assets/gpu-sharpen.py (VS Renderer, MIT). Keep original notices.', '#pragma once']
for index, (program, block) in enumerate(blocks):
    directives = {}
    for name, value in re.findall(r'^//!(\w+)\s+([^\n]+)', block, flags=re.M):
        directives.setdefault(name, []).append(value.strip())
    body = '\n'.join(line for line in block.splitlines() if not line.startswith('//!'))
    names = sorted(set(re.findall(r'\b([A-Za-z_][A-Za-z_0-9]*)_(?:texOff|tex|pos|pt|size)\b', body)))
    header = '#version 450\nlayout(location=0) in vec2 uv;\nlayout(location=0) out vec4 outColor;\n'
    for binding, name in enumerate(names):
        header += f'layout(set=0,binding={binding}) uniform sampler2D {name}_raw;\n'
        header += f'#define {name}_size vec2(textureSize({name}_raw,0))\n'
        header += f'#define {name}_pt (1.0 / {name}_size)\n#define {name}_pos uv\n'
        header += f'#define {name}_tex(p) textureLod({name}_raw,(p),0.0)\n'
        header += f'#define {name}_texOff(o) {name}_tex(uv+(o)*{name}_pt)\n'
    base = args.output / f'pass-{index:02d}'
    base.with_suffix('.frag').write_text(header + body + '\nvoid main(){outColor=hook();}\n', encoding='utf-8')
    commands = [
        [str(tools/'glslc.exe'), '-fshader-stage=frag', str(base.with_suffix('.frag')), '-o', str(base.with_suffix('.spv'))],
        [str(tools/'spirv-cross.exe'), str(base.with_suffix('.spv')), '--hlsl', '--shader-model', '50', '--output', str(base.with_suffix('.hlsl'))],
        [str(fxc), '/nologo', '/T', 'ps_5_0', '/E', 'main', '/Fo', str(base.with_suffix('.cso')), str(base.with_suffix('.hlsl'))],
    ]
    for command in commands:
        subprocess.run(command, env=env, check=True)
    bytecode = base.with_suffix('.cso').read_bytes()
    embedded.append(f'inline constexpr unsigned char anime4kPass{index}[] = {{')
    embedded.extend(','.join(str(x) for x in bytecode[n:n+32])+',' for n in range(0, len(bytecode), 32))
    embedded.append('};')
    manifest.append({'pass': index, 'program': program, 'directives': directives, 'bindings': names, 'bytecode': base.with_suffix('.cso').name})
specs = []
for item in manifest:
    index, d = item['pass'], item['directives']
    # MAIN hooks run in source order; PREKERNEL runs after all MAIN hooks.
    hook = d['HOOK'][0]
    assert hook in ('LUMA', 'MAIN', 'PREKERNEL')
    hooked = 'LUMA' if hook == 'LUMA' else 'MAIN'
    strings = [hook, d.get('SAVE', [hooked])[0],
               d.get('WIDTH', [hooked+'.w'])[0], d.get('HEIGHT', [hooked+'.h'])[0], d.get('WHEN', ['1'])[0]]
    bindings = ','.join(json.dumps(x) for x in item['bindings'])
    specs.append('{anime4kPass%d,sizeof(anime4kPass%d),%s,{%s},%d,%d}' %
                 (index, index, ','.join(json.dumps(x) for x in strings), bindings, len(item['bindings']), item['program']))
embedded += ['inline constexpr Anime4KPass anime4kPasses[] = {', ',\n'.join(specs), '};']
(args.output/'Anime4KFastShaders.h').write_text('\n'.join(embedded)+'\n', encoding='utf-8')
(args.output/'manifest.json').write_text(json.dumps({'programs': [{'name': name, 'source': source.name,
    'sourceSha256': hashlib.sha256(source.read_bytes()).hexdigest()} for name, source in programs], 'passes': manifest}, indent=2)+'\n', encoding='utf-8')
print(f'Compiled {len(manifest)} passes into Anime4KFastShaders.h.')
