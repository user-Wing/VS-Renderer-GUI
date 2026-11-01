"""Compare the native GPU probe with the original GLSL on an OpenGL context.

Run after building tests/Anime4KFastProbe.cpp. Requires moderngl and numpy.
"""
from pathlib import Path
import argparse
import json
import re
import runpy
import subprocess
import moderngl
import numpy as np

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--directory', type=Path, default=root/'build/anime4k-native')
args = parser.parse_args()
work = args.directory
ctx = moderngl.create_standalone_context(require=450)
helper=runpy.run_path(str(root/'assets/gpu-sharpen.py'))['_vsr_sharpen_shader']
enhance='\n'.join(helper(mode,0.3,2) for mode in ('sharpen_edges','enhance_detail'))
enhance=enhance.replace('//!HOOK LUMA\n//!BIND HOOKED\n//!DESC VS Renderer GPU sharpen','//!DESC VS Renderer GPU sharpen\n//!HOOK LUMA\n//!BIND HOOKED')
sources=[(root/'assets/anime4k-a-fast.glsl').read_text(encoding='utf-8'),(root/'assets/anime4k-no-cnn.glsl').read_text(encoding='utf-8'),enhance]
blocks=[(program,block) for program,source in enumerate(sources) for block in re.split(r'(?=^//!DESC )',source,flags=re.M)[1:]]
passes = []
vertex = '''#version 450
out vec2 uv;
void main(){uv=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(uv*2.0-1.0,0,1);}
'''
for program,block in blocks:
    d = {}
    for name, value in re.findall(r'^//!(\w+)\s+([^\n]+)', block, flags=re.M):
        d.setdefault(name, []).append(value.strip())
    body = '\n'.join(line for line in block.splitlines() if not line.startswith('//!'))
    names = sorted(set(re.findall(r'\b([A-Za-z_][A-Za-z_0-9]*)_(?:texOff|tex|pos|pt|size)\b', body)))
    header = '#version 450\nin vec2 uv;\nlayout(location=0) out vec4 color;\n'
    for name in names:
        header += f'uniform sampler2D {name}_raw;\n#define {name}_size vec2(textureSize({name}_raw,0))\n'
        header += f'#define {name}_pt (1.0/{name}_size)\n#define {name}_pos uv\n'
        header += f'#define {name}_tex(p) textureLod({name}_raw,(p),0.0)\n#define {name}_texOff(o) {name}_tex(uv+(o)*{name}_pt)\n'
    p = ctx.program(vertex_shader=vertex,fragment_shader=header+body+'\nvoid main(){color=hook();}\n')
    passes.append((program,d,names,p,ctx.vertex_array(p,[])))
copy = ctx.program(vertex_shader=vertex,fragment_shader='#version 450\nin vec2 uv;out vec4 color;uniform sampler2D src;void main(){color=textureLod(src,uv,0);}')
copy_vao = ctx.vertex_array(copy,[])
extract=ctx.program(vertex_shader=vertex,fragment_shader='''#version 450
in vec2 uv;out vec4 color;uniform sampler2D src;
void main(){color=vec4(dot(textureLod(src,uv,0).rgb,vec3(0.2126,1.0-0.2126-0.0722,0.0722))* (219.0/255.0)+16.0/255.0,0,0,1);}''')
merge=ctx.program(vertex_shader=vertex,fragment_shader='''#version 450
in vec2 uv;out vec4 color;uniform sampler2D src;uniform sampler2D enhanced;
void main(){vec4 p=textureLod(src,uv,0);float oldY=dot(p.rgb,vec3(0.2126,1.0-0.2126-0.0722,0.0722))*(219.0/255.0)+16.0/255.0;p.rgb+=(textureLod(enhanced,uv,0).r-oldY)/(219.0/255.0);color=p;}''')
extract_vao,merge_vao=ctx.vertex_array(extract,[]),ctx.vertex_array(merge,[])
w,h = 64,40
y,x = np.mgrid[:h,:w]
data = np.stack([((x//3+y//5)%2)*0.65+x/w*0.3, y/h*0.8+0.1, ((x*x+y*7)%37)/40, np.ones_like(x)],axis=-1).astype('f2')
(work/'input.rgba16').write_bytes(data.tobytes())
results = []
# The probe compares shader processing plus bilinear resize. The player's
# existing native Jinc final scaler is checked separately by the Lite tests.
for algorithm in (9,10,11,12,13,14):
 for scale in (0.75,1,1.25,1.5,2,3,4,5):
    size = (round(w*scale),round(h*scale))
    target=(w,h) if algorithm>=13 else size
    selected_program=1 if algorithm in (11,12) else 0
    is_enhanced=algorithm in (10,11)
    def texture(size, pixels=None, dtype='f2'):
        t=ctx.texture(size,4,pixels,dtype=dtype);t.filter=(moderngl.LINEAR,moderngl.LINEAR);t.repeat_x=t.repeat_y=False;return t
    source=texture((w,h),data.tobytes());saved={'MAIN':source,'NATIVE':source};resources=[source];fbos=[];executed=[]
    if is_enhanced:
        luma=texture((w,h));resources.append(luma);fbo=ctx.framebuffer([luma]);fbos.append(fbo);fbo.use();source.use(0);extract['src'].value=0;extract_vao.render(vertices=3);saved['LUMA']=luma
    def evaluate(expr):
        stack=[]
        for token in expr.split():
            if token in ('*','/','>','<'):
                b,a=stack.pop(),stack.pop()
                stack.append({'*':lambda:a*b,'/':lambda:a/b,'>':lambda:float(a>b),'<':lambda:float(a<b)}[token]())
            elif token.endswith(('.w','.h')):
                name,axis=token.rsplit('.',1);stack.append((target if name=='OUTPUT' else saved[name].size)[axis=='h'])
            else:stack.append(float(token))
        assert len(stack)==1
        return stack[0]
    for hook in ('LUMA','MAIN','PREKERNEL'):
        if hook=='LUMA' and not is_enhanced:continue
        hooked='LUMA' if hook=='LUMA' else 'MAIN'
        for i,(program,d,names,p,vao) in enumerate(passes):
            if program!=(2 if hook=='LUMA' else selected_program) or d['HOOK'][0]!=hook or not evaluate(d.get('WHEN',['1'])[0]):continue
            saved['HOOKED']=saved[hooked]
            out=texture((round(evaluate(d.get('WIDTH',[hooked+'.w'])[0])),round(evaluate(d.get('HEIGHT',[hooked+'.h'])[0]))));resources.append(out)
            fbo=ctx.framebuffer([out]);fbos.append(fbo);fbo.use()
            for binding,name in enumerate(names):saved[name].use(binding);p[name+'_raw'].value=binding
            vao.render(vertices=3);saved[d.get('SAVE',[hooked])[0]]=out;executed.append(i)
        if hook=='LUMA':
            rgb=texture((w,h));resources.append(rgb);fbo=ctx.framebuffer([rgb]);fbos.append(fbo);fbo.use();source.use(0);saved['LUMA'].use(1);merge['src'].value=0;merge['enhanced'].value=1;merge_vao.render(vertices=3);saved['MAIN']=rgb
    final=texture(size,dtype='f4');resources.append(final);fbo=ctx.framebuffer([final]);fbos.append(fbo);fbo.use()
    saved['MAIN'].use(0);copy['src'].value=0;copy_vao.render(vertices=3)
    reference=np.frombuffer(fbo.read(components=4,dtype='f4'),dtype='f4').reshape(size[1],size[0],4)
    native=work/f'native-{algorithm}-{scale}.rgba32'
    subprocess.run([str(work/'probe.exe'),str(w),str(h),str(size[0]),str(size[1]),str(work/'input.rgba16'),str(native),str(algorithm)],check=True)
    actual=np.fromfile(native,dtype='f4').reshape(reference.shape)
    diff=np.abs(actual[...,:3]-reference[...,:3]);assert np.isfinite(actual).all()
    result={'algorithm':algorithm,'scale':scale,'size':size,'passes':executed,'maxRgbError':float(diff.max()),'meanRgbError':float(diff.mean())}
    results.append(result);print(json.dumps(result),flush=True)
    assert diff.max()<0.005 and diff.mean()<0.0005, result
    for f in fbos:f.release()
    for t in resources:t.release()
(work/'pixel-equivalence.json').write_text(json.dumps({'renderer':ctx.info['GL_RENDERER'],'results':results},indent=2)+'\n')
