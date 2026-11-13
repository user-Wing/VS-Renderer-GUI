"""Rebuild the measured FP32 3-channel reconstruction/depth-shuffle fusion.

Only the pinned built-in Mode A model is accepted. No global importer or Native
ABI change. The original shader and license stay intact; all other DXBC passes
are copied byte-for-byte from the reference fixture. Requires Windows SDK fxc.
"""
from pathlib import Path
import argparse,hashlib,json,re,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
SOURCE_SHA='27264a412fbebf203253f280d39acc4cacd42be58bc01d0cf719d61dd6d12adf'
REFERENCE_SHA='c3dfb102dd3f650b5d941f124d5394d19b5002fae1775208d079ff10641eb8c7'
def unpack(data):
    assert data[:8]==b'VSRHLSL1';at=8
    def uint():
        nonlocal at
        v=struct.unpack_from('<I',data,at)[0];at+=4;return v
    def field():
        nonlocal at
        n=uint();assert at+n<=len(data);v=data[at:at+n];at+=n;return v
    rows=[]
    for _ in range(uint()):
        p=dict(zip(['hook','save','width','height','when'],[field().decode() for _ in range(5)]));p['bindings']=[field().decode() for _ in range(uint())];p['code']=field();rows.append(p)
    assert at==len(data);return rows
def pack(rows):
    out=bytearray(b'VSRHLSL1'+struct.pack('<I',len(rows)))
    def field(v):
        if isinstance(v,str):v=v.encode()
        out.extend(struct.pack('<I',len(v)));out.extend(v)
    for p in rows:
        for k in ['hook','save','width','height','when']:field(p[k])
        out.extend(struct.pack('<I',len(p['bindings'])))
        for n in p['bindings']:field(n)
        field(p['code'])
    return bytes(out)
def generate(source,rows):
    blocks=[]
    for b in re.split(r'(?=^//!DESC )',source,flags=re.M):
        if '//!HOOK' in b:blocks.append(b)
    blocks.sort(key=lambda b:{'LUMA':0,'MAIN':1,'PREKERNEL':2}[re.search(r'^//!HOOK (\w+)',b,re.M)[1]])
    assert len(rows)==len(blocks)==49
    group=[rows[i] for i in [33,34,35,36]]
    assert [p['save'] for p in group]==['conv2d_last_tf','conv2d_last_tf1','conv2d_last_tf2','MAIN']
    assert len(set(p['when'] for p in group))==1
    assert all(p['bindings']==group[0]['bindings'] for p in group[:3]);assert len(group[0]['bindings'])==14
    terms=[re.findall(r'(?:vec4 result =|result \+=) mat4\(([^()]+)\) \* (g_\d+);',blocks[i]) for i in [33,34,35]]
    assert all(len(t)==28 for t in terms)
    p=group[0].copy();p.update(save='MAIN',width='conv2d_tf.w 2 *',height='conv2d_tf.h 2 *');p['bindings']=['MAIN']+p['bindings']
    h='// Generated from bloc97 Anime4K, MIT/Unlicense; see anime4k-mode-a.glsl.\n// FP32 arithmetic. Preserve the original FP16 intermediate rounding.\n'
    for i,name in enumerate(p['bindings']):h+=f'Texture2D<float4> {name}_raw:register(t{i});\n'
    h+='SamplerState smp:register(s0);\nfloat4 main(float4 pos:SV_Position):SV_Target{\nint2 dst=int2(pos.xy),p=dst/2;uint q=(dst.y&1)*2+(dst.x&1);uint w,h;MAIN_raw.GetDimensions(w,h);float3 result=0;\n'
    macros=dict(re.findall(r'^#define (g_\d+) (.+)',blocks[33],re.M))
    for k in range(28):
        name=terms[0][k][1];assert all(t[k][1]==name for t in terms);macro=macros[name];tex=re.search(r'(\w+)_tex\(',macro)[1];neg='max(-' in macro
        h+=f'float4 v{k}=max({"-" if neg else ""}{tex}_raw.Load(int3(p,0)),0);\n'
        weights=[[x.strip() for x in t[k][0].split(',')] for t in terms]
        assert all(len(w)==16 for w in weights)
        for c in range(4):
            values=['float4('+','.join(x+'f' for x in a[c*4:c*4+4])+')[q]' for a in weights]
            h+=f'result=mad(float3({",".join(values)}),v{k}.{"xyzw"[c]},result);\n'
    bias=[[x.strip() for x in re.findall(r'result \+= vec4\(([^()]+)\);',blocks[i])[0].split(',')] for i in [33,34,35]]
    h+='result+=float3('+','.join('float4('+','.join(x+'f' for x in a)+')[q]' for a in bias)+');\n'
    h+='result=f16tof32(f32tof16(result));\nreturn MAIN_raw.SampleLevel(smp,pos.xy/(float2(w,h)*2),0)+float4(result,result.b);\n}\n'
    return p,h
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--fxc',type=Path,default=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'));parser.add_argument('--output',type=Path,default=ROOT/'assets/builtin-shaders/anime4k-mode-a.glsl.vsrhlsl');args=parser.parse_args()
    source=ROOT/'assets/builtin-shaders/anime4k-mode-a.glsl';reference=ROOT/'tests/fixtures/anime4k-mode-a-reference.vsrhlsl';assert hashlib.sha256(source.read_bytes()).hexdigest()==SOURCE_SHA;assert hashlib.sha256(reference.read_bytes()).hexdigest()==REFERENCE_SHA
    rows=unpack(reference.read_bytes());p,hlsl=generate(source.read_text(),rows);hlslpath=ROOT/'assets/builtin-shaders/source/anime4k-mode-a-reconstruct-fused.hlsl';hlslpath.write_text(hlsl,encoding='utf-8')
    with tempfile.TemporaryDirectory() as tmp:
        code=Path(tmp)/'fusion.cso';subprocess.run([str(args.fxc),'/nologo','/T','ps_5_0','/E','main','/Ges','/O3','/Fo',str(code),str(hlslpath)],check=True);p['code']=code.read_bytes()
    result=pack(rows[:33]+[p]+rows[37:]);args.output.parent.mkdir(parents=True,exist_ok=True);temp=args.output.with_suffix(args.output.suffix+'.tmp');temp.write_bytes(result);temp.replace(args.output)
    metadata={'semantics':'luma-main-prekernel-params-v8','sha256':SOURCE_SHA,'optimization':'mode-a-fp32-reconstruct-shuffle-fusion-v1','packageSha256':hashlib.sha256(result).hexdigest(),'referencePackageSha256':REFERENCE_SHA}
    Path(str(args.output)+'.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8');print(json.dumps(metadata))
