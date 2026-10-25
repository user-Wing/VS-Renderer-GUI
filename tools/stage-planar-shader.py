"""Reuse the isolated color runtime when staging the planar VS shader."""
import argparse
import json
from pathlib import Path
import runpy
imports=runpy.run_path(str(Path(__file__).with_name('stage-color.py')))['imports']

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--color',type=Path,required=True)
args=p.parse_args()
mapping={row['source'].lower():row['file'] for row in json.loads((args.color/'runtime.json').read_text())['dlls']}
data=bytearray(args.source.read_bytes())
for offset,name in imports(data):
    if name.lower() in mapping:
        replacement=mapping[name.lower()]
        assert len(replacement)==len(name)
        data[offset:offset+len(name)]=replacement.encode('ascii')
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_bytes(data)
