"""Stage only the tested API4 additions; never copy VSRepo's API3 DLL directory wholesale."""
import argparse, pathlib, shutil, hashlib, json
root=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
source=root/'.deps/vs-python/Lib/site-packages';target=args.output/'Lib/site-packages'
files=['havsfunc.py','adjust.py','nnedi3_resample.py']
folders=['vsutil','mvsfunc']
records={}
for name in files:
    p=source/name
    if p.is_file():
        dest=target/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest);records[name]=hashlib.sha256(p.read_bytes()).hexdigest()
for name in folders:
    if (source/name).is_dir():shutil.copytree(source/name,target/name,dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
p=source/'vapoursynth/plugins/vsrepo/fft3dfilter.dll'
if p.is_file():
    dest=target/'vapoursynth/plugins/fft3dfilter.dll';dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest);records[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
p=source/'vapoursynth/plugins/vsrepo/MiscFilters.dll'
if p.is_file():
    dest=target/'vapoursynth/plugins/MiscFilters.dll';dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest);records[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
for pattern in ['havsfunc-*.dist-info','vsutil-*.dist-info','mvsfunc-*.dist-info']:
    for p in source.glob(pattern):shutil.copytree(p,target/p.name,dirs_exist_ok=True)
(args.output/'vs-extra-filters.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
