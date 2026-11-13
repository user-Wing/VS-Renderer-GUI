from prepare import *
variant=sys.argv[1] if len(sys.argv)>1 else 'base';base=B/('a-'+variant);items=json.loads((base/'manifest.json').read_text());saved={'MAIN':0,'NATIVE':0};sizes=[(1920,1080)];plan=[]
def ev(s):
 st=[]
 for t in s.split():
  if t in ['*','/','+','-','>','<','=']:
   b=st.pop();a=st.pop();st.append({'*':lambda:a*b,'/':lambda:a/b,'+':lambda:a+b,'-':lambda:a-b,'>':lambda:a>b,'<':lambda:a<b,'=':lambda:a==b}[t]())
  elif t.endswith(('.w','.h','.width','.height')):
   name,dim=t.split('.');st.append(((3840,2160) if name=='OUTPUT' else sizes[saved[name]])[dim in ['h','height']])
  else:st.append(float(t))
 assert len(st)==1
 return st[0]
pre=False
for i,p in enumerate(items):
 if p['hook']=='PREKERNEL' and not pre:saved['PREKERNEL']=saved['MAIN'];pre=True
 saved['HOOKED']=saved[p['hook']]
 if not ev(p['when']):continue
 size=(int(ev(p['width'])+.5),int(ev(p['height'])+.5));binding=[saved[n] for n in p['bindings']];idx=len(sizes);sizes.append(size);saved[p['save']]=idx
 hlsl=base/f'pass-{i:02d}.hlsl';dxil=base/f'pass-{i:02d}.dxil';run([sdk/'dxc.exe','-T','ps_6_2','-E','main','-O3']+(['-enable-16bit-types'] if 'half' in variant else [])+['-Fo',dxil,hlsl]);plan.append((i,*size,len(binding),*binding))
output=saved['PREKERNEL' if pre else 'MAIN'];data=bytearray(struct.pack('<II',len(plan),output))
for row in plan:data.extend(struct.pack('<'+'I'*len(row),*row))
(base/'plan.bin').write_bytes(data);print(variant,'active',len(plan),'output',output,sizes[output])
