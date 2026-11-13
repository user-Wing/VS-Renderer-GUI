"""Package the measured 1X integer-load variant for the existing CustomShader API.

First run convert-anime4k-fast.py. The original shaders and licenses stay intact.
OUTPUT becomes NATIVE in the fallback source and package expressions, retaining
the original 1X executor's geometry. This is not used for Fast/noCNN presets.
"""
from pathlib import Path
import argparse, hashlib, json, os, runpy, subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SHA = 'c9e323af492aab4de88997ebbaa15700815a4713d634cf04bb4814bc6ffca2ad'
MANIFEST_SHA = '88c2a82940eb756f657b7a850a1308952ab06754072b8acda0ce18a04afe7d3b'

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--shaders', type=Path, default=ROOT/'build/anime4k-native/shaders')
    parser.add_argument('--output', type=Path, default=ROOT/'assets/builtin-shaders/anime4k-1x.glsl')
    args = parser.parse_args()
    source = ROOT/'assets/anime4k-a-fast.glsl'
    assert hashlib.sha256(source.read_bytes()).hexdigest() == SOURCE_SHA
    assert hashlib.sha256((args.shaders/'manifest.json').read_bytes()).hexdigest() == MANIFEST_SHA
    codec = runpy.run_path(str(ROOT/'tools/optimize-anime4k-mode-a.py'))
    manifest = json.loads((args.shaders/'manifest.json').read_text(encoding='utf-8'))['passes']
    tools = ROOT/'.deps/color/ucrt64/bin'
    env = dict(os.environ, PATH=str(tools)+os.pathsep+os.environ['PATH'])
    fxc = 'C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
    import tempfile
    rows = []
    with tempfile.TemporaryDirectory() as temporary:
        for hook in ['MAIN', 'PREKERNEL']:
            for item in manifest:
                d = item['directives']
                if item['program'] != 0 or d['HOOK'][0] != hook:
                    continue
                i = item['pass']
                p = dict(hook='MAIN', save=d.get('SAVE',['MAIN'])[0],
                         width=d.get('WIDTH',['MAIN.w'])[0].replace('OUTPUT.','NATIVE.'),
                         height=d.get('HEIGHT',['MAIN.h'])[0].replace('OUTPUT.','NATIVE.'),
                         when=d.get('WHEN',['1'])[0].replace('OUTPUT.','NATIVE.'), bindings=item['bindings'])
                p['code'] = (args.shaders/f'pass-{i:02}.cso').read_bytes()
                # Shuffle and resampling retain their original filtering.
                if i not in [19,20,21,26]:
                    text = (args.shaders/f'pass-{i:02}.frag').read_text(encoding='utf-8')
                    for name in p['bindings']:
                        text = text.replace(f'#define {name}_texOff(o) {name}_tex(uv+(o)*{name}_pt)',
                            f'#define {name}_texOff(o) texelFetch({name}_raw,clamp(ivec2(gl_FragCoord.xy)+ivec2(o),ivec2(0),textureSize({name}_raw,0)-1),0)')
                        text = text.replace(f'{name}_tex({name}_pos)',f'{name}_texOff(vec2(0))')
                    base = Path(temporary)/str(i)
                    base.with_suffix('.frag').write_text(text,encoding='utf-8')
                    commands = [[str(tools/'glslc.exe'),'-fshader-stage=frag',str(base.with_suffix('.frag')),'-o',str(base.with_suffix('.spv'))],
                                [str(tools/'spirv-cross.exe'),str(base.with_suffix('.spv')),'--hlsl','--shader-model','50','--output',str(base.with_suffix('.hlsl'))],
                                [fxc,'/nologo','/T','ps_5_0','/E','main','/Fo',str(base.with_suffix('.cso')),str(base.with_suffix('.hlsl'))]]
                    for command in commands:
                        subprocess.run(command,env=env,check=True,stdout=subprocess.DEVNULL)
                    p['code'] = base.with_suffix('.cso').read_bytes()
                rows.append(p)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(source.read_text(encoding='utf-8').replace('OUTPUT.','NATIVE.'),encoding='utf-8')
    result = codec['pack'](rows)
    Path(str(args.output)+'.vsrhlsl').write_bytes(result)
    metadata = dict(semantics='luma-main-prekernel-params-v8',sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
                    optimization='anime4k-1x-clamped-load-v1', originalSourceSha256=SOURCE_SHA,
                    packageSha256=hashlib.sha256(result).hexdigest())
    Path(str(args.output)+'.vsrhlsl.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(metadata))
