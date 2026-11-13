"""Create a verified GUI + patched Native core source archive (no runtime/user data)."""
from pathlib import Path
import argparse,hashlib,json,subprocess,zipfile
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--native-root',required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];native=Path(a.native_root).resolve();output=Path(a.output).resolve()
if output.exists():raise RuntimeError('Refusing to overwrite '+str(output))
base='VS-Renderer-GUI-audio-unc-20261110/'
selected={}
for top in ['src','assets','docs','tools','tests','patches','plugins','third_party']:
 for f in (root/top).rglob('*'):
  if not f.is_file() or '.git' in f.parts or '__pycache__' in f.parts:continue
  if f.suffix.lower() in {'.exe','.dll','.obj','.pdb','.lib','.zip','.7z','.pyc'}:continue
  selected[base+f.relative_to(root).as_posix()]=f
for name in ['CMakeLists.txt','CMakePresets.json','README.md','LICENSE','THIRD_PARTY_NOTICES.md','project.md','changelog.md','.gitignore','.gitattributes','development/validation.md']:
 selected[base+name]=root/name
for f in (native/'FFF.Native').rglob('*'):
 if not f.is_file() or any(x in f.parts for x in ['.git','x64','Debug','Release','__pycache__']):continue
 if f.suffix.lower() in {'.exe','.dll','.obj','.pdb','.lib','.ilk','.pch','.tlog'}:continue
 selected[base+'native-core/'+f.relative_to(native).as_posix()]=f
for name in ['LICENSE','README.md']:
 if (native/name).is_file():selected[base+'native-core/'+name]=native/name
for evidence in ['audio-unc-20261110','suite-20261110','780m-hlsl-20261111','hlsl-chain-20261111']:
 for f in (root/'development/evidence'/evidence).glob('*'):
  if f.is_file():selected[base+f.relative_to(root).as_posix()]=f
manifest={'guiBase':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'nativeBase':subprocess.check_output(['git','rev-parse','HEAD'],cwd=native,text=True).strip(),'nativePatchOrder':['3fp-vsp-1.1.0.patch','3fp-vsp-1.1.0-network-proxy.patch','3fp-vsp-audio-unc.patch'],'scope':'2026-11-11 780M Mode A fusion, MoeSR FP32 tail11 schedule, bit-exact 1X and shared enhancement Load; measured rejects retained; upstream Native unchanged this round','nativeCore':'Applied source snapshot; do not reapply patches to this snapshot. Standard build script uses a fresh pinned Git checkout.','files':[]}
with zipfile.ZipFile(output,'x',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name,f in sorted(selected.items()):
  data=f.read_bytes();z.writestr(name,data);manifest['files'].append({'path':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
 z.writestr(base+'source-manifest.json',json.dumps(manifest,ensure_ascii=False,indent=2))
with zipfile.ZipFile(output) as z:
 assert z.testzip() is None
 for entry in manifest['files']:assert hashlib.sha256(z.read(entry['path'])).hexdigest()==entry['sha256'],entry['path']
digest=hashlib.sha256(output.read_bytes()).hexdigest();output.with_suffix('.zip.sha256').write_text(digest+'  '+output.name+'\n',encoding='utf-8')
print(json.dumps({'path':str(output),'bytes':output.stat().st_size,'files':len(selected),'sha256':digest,'crcAndEveryFileHashVerified':True},ensure_ascii=False,indent=2))
