"""Specialized, lossless-weight rewrite of the supplied MoeSR pixel shaders.
Replace zero-padded 1:1 convolutions with integer Texture2D.Load. Not a general HLSL optimizer.
"""
from pathlib import Path
import re,argparse,hashlib,json
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
s=a.source.read_text(encoding='utf-8');blocks=re.split(r'(?=^//!HOOK)',s,flags=re.M)[1:];result=[]
for block in blocks:
 if '//!WIDTH MAIN.w 2 *' in block:
  result.append(block);continue # Pixel shuffle keeps its original implementation.
 directives='\n'.join(l for l in block.splitlines() if l.startswith('//!'))
 assert '//!WIDTH MAIN.w\n' in directives+'\n' and '//!HEIGHT MAIN.h' in directives
 textures='\n'.join(l for l in block.splitlines() if l.startswith('Texture2D'))
 bias=re.search(r'float4 v = (.*);',block).group(1)
 ret=re.search(r'    return ([^\n]+);\n}',block.split('float4 hook()',1)[1]).group(1)
 operations=[]
 for line in block.splitlines():
  if 'v +=' not in line:continue
  match=re.search(r'(\w+)_raw\.SampleLevel\(',line);assert match,line[:200]
  start=match.start();pos=match.end();depth=1
  while depth:
   if line[pos]=='(':depth+=1
   elif line[pos]==')':depth-=1
   pos+=1
  sample=line[match.end():pos-1]
  xy=re.search(r'uv \+ \(float2\((-?[01]\.0f), (-?[01]\.0f)\)',sample)
  if xy:x,y=(int(float(v[:-1])) for v in xy.groups())
  else:
   scalar=re.search(r'uv \+ \(\(?(-?[01]\.0f)\)?\.xx',sample);assert scalar,sample
   x=y=int(float(scalar.group(1)[:-1]))
  line=line[:start]+f'{match.group(1)}_raw.Load(int3(p+int2({x},{y}),0))'+line[pos:]
  operations.append(line.strip())
 assert operations
 result.append(directives+'\n'+textures+'\nfloat4 main(float4 position:SV_Position):SV_Target {\nint2 p=int2(position.xy);\nfloat4 v='+bias+';\n'+'\n'.join(operations)+'\nreturn '+ret+';\n}\n')
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text('// MoeSR 50K: integer-load convolution; original weights and pixel shuffle preserved.\n'+'\n'.join(result),encoding='utf-8')
print(json.dumps({'sourceSha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'outputSha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),'passes':len(blocks)},indent=2))
