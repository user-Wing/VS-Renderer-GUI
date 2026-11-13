from pathlib import Path
import re,struct,json,subprocess,os,sys
R=Path(__file__).resolve().parents[3];B=R/'build/hlsl-benchmark';B.mkdir(parents=True,exist_ok=True);tools=R/'.deps/color/ucrt64/bin';sdk=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64');env=dict(os.environ);env['PATH']=str(tools)+';'+env['PATH']
def run(args):
 p=subprocess.run(list(map(str,args)),env=env,capture_output=True)
 if p.returncode:raise RuntimeError(p.stderr.decode(errors='replace')+p.stdout.decode(errors='replace'))
def unpack(path):
 data=path.read_bytes();at=8
 def u():
  nonlocal at
  n=struct.unpack_from('<I',data,at)[0];at+=4;return n
 def s():
  nonlocal at
  n=u();v=data[at:at+n];at+=n;return v
 out=[]
 for i in range(u()):
  p=dict(zip(['hook','save','width','height','when'],[s().decode() for _ in range(5)]));p['bindings']=[s().decode() for _ in range(u())];p['code']=s();out.append(p)
 assert at==len(data)
 return out
def pack(passes,path):
 out=bytearray(b'VSRHLSL1'+struct.pack('<I',len(passes)))
 def s(v):
  if isinstance(v,str):v=v.encode()
  out.extend(struct.pack('<I',len(v)));out.extend(v)
 for p in passes:
  for k in ['hook','save','width','height','when']:s(p[k])
  out.extend(struct.pack('<I',len(p['bindings'])))
  for name in p['bindings']:s(name)
  s(p['code'])
 path.write_bytes(out)
def parse(source):
 blocks=[]
 for b in re.split(r'(?=^//!DESC )',source,flags=re.M):
  if '//!HOOK' not in b:continue
  d={}
  for k,v in re.findall(r'^//!(\w+)\s+([^\n]+)',b,re.M):d.setdefault(k,[]).append(v.strip())
  body='\n'.join(x for x in b.splitlines() if not x.startswith('//!'))
  blocks.append((d,body))
 return sorted(blocks,key=lambda b:{'LUMA':0,'MAIN':1,'PREKERNEL':2}[b[0]['HOOK'][0]])
def emit(p,body,components,loads=False):
 h='#version 450\nlayout(location=0) in vec2 uv;\nlayout(location=0) out vec4 outColor;\n'
 for i,n in enumerate(p['bindings']):
  h+=f'layout(set=0,binding={i}) uniform sampler2D {n}_raw;\n#define {n}_size vec2(textureSize({n}_raw,0))\n#define {n}_pt (1.0 / {n}_size)\n#define {n}_pos uv\n#define {n}_tex(p) textureLod({n}_raw,(p),0.0)\n#define {n}_gather(p,c) textureGather({n}_raw,(p),c)\n'
  h+=f'#define {n}_texOff(o) '+(f'texelFetch({n}_raw,clamp(ivec2(gl_FragCoord.xy)+ivec2(o),ivec2(0),textureSize({n}_raw,0)-1),0)' if loads else f'{n}_tex(uv+(o)*{n}_pt)')+'\n'
 if 'HOOKED' in p['bindings'] and p['hook'] not in p['bindings']:
  n=p['hook'];h+=f'#define {n}_texOff(o) HOOKED_texOff(o)\n#define {n}_tex(p) HOOKED_tex(p)\n#define {n}_size HOOKED_size\n#define {n}_pt HOOKED_pt\n#define {n}_pos HOOKED_pos\n'
 if p['hook'] in p['bindings'] and 'HOOKED' not in p['bindings']:
  n=p['hook'];h+=f'#define HOOKED_texOff(o) {n}_texOff(o)\n#define HOOKED_tex(p) {n}_tex(p)\n#define HOOKED_size {n}_size\n#define HOOKED_pt {n}_pt\n#define HOOKED_pos {n}_pos\n'
 out={1:'vec4(v.r,0,0,1)',2:'vec4(v.rg,0,1)',3:'vec4(v.rgb,1)',4:'v'}[components]
 return h+body+'\nvoid main(){vec4 v=hook();outColor='+out+';}\n'
if __name__=='__main__':
 model=sys.argv[1] if len(sys.argv)>1 else 'a';variant=sys.argv[2] if len(sys.argv)>2 else 'base'
 source=R/('assets/builtin-shaders/anime4k-mode-a.glsl' if model=='a' else 'assets/anime4k-a-fast.glsl');blocks=parse(source.read_text(encoding='utf-8-sig'));passes=[];dest=B/(model+'-'+variant);dest.mkdir(exist_ok=True)
 original=unpack(R/'tests/fixtures/anime4k-mode-a-reference.vsrhlsl') if model=='a' else None
 for i,(d,body) in enumerate(blocks):
  hook=d['HOOK'][0];p={k:d.get(k.upper(),[v])[0] for k,v in dict(hook=hook,save=hook,width='HOOKED.w',height='HOOKED.h',when='1').items()};p['bindings']=[n for n in d['BIND'] if n!='NATIVE'];p['desc']=d['DESC'][0];p['components']=int(d.get('COMPONENTS',['4'])[0]);p['cnn']='conv2d' in p['save'];
  if original:
   for k in ['hook','save','width','height','when','bindings']:assert original[i][k]==p[k],(i,k,original[i][k],p[k])
  base=dest/f'pass-{i:02d}';frag=emit(p,body,p['components'],loads=('load' in variant and p['cnn']));base.with_suffix('.frag').write_text(frag,encoding='utf-8')
  run([tools/'glslc.exe','-fshader-stage=frag',base.with_suffix('.frag'),'-o',base.with_suffix('.spv')]);run([tools/'spirv-cross.exe',base.with_suffix('.spv'),'--hlsl','--shader-model','50','--output',base.with_suffix('.hlsl')])
  hs=base.with_suffix('.hlsl').read_text();
  if 'half' in variant and p['cnn']:
   # Only tensor arithmetic; pixel coordinates/texture sizes remain full precision.
   hs=re.sub(r'\bfloat4x4\b','min16float4x4',hs);hs=re.sub(r'\bfloat4\b','min16float4',hs);hs=hs.replace('Texture2D<min16float4>','Texture2D<float4>');hs=re.sub(r'min16float4 outColor : SV_Target0','float4 outColor : SV_Target0',hs)
   base.with_suffix('.hlsl').write_text(hs)
  run([sdk/'fxc.exe','/nologo','/T','ps_5_0','/E','main','/Ges','/Fo',base.with_suffix('.cso'),base.with_suffix('.hlsl')]);p['code']=base.with_suffix('.cso').read_bytes();passes.append(p)
 pack(passes,dest/'pipeline.vsrhlsl');(dest/'manifest.json').write_text(json.dumps([{k:v for k,v in p.items() if k!='code'} for p in passes],indent=2));print(model,variant,len(passes),flush=True)
