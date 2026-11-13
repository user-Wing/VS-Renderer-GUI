"""Bounded IPC/lifecycle regression, no codec/GPU/performance benchmark."""
from pathlib import Path
import os,sys,json,time,subprocess,tempfile,hashlib
worker=Path(sys.argv[1]).resolve();fixture=Path(sys.argv[2]).resolve()
with tempfile.TemporaryDirectory(prefix='vsr-worker-test-') as tmp:
 root=Path(tmp); stub=root/'pipe.py'
 stub.write_text('''import sys,time,os,subprocess
from pathlib import Path
mode=sys.argv[1]
if mode=='produce':
 for n in range(60):
  sys.stdout.buffer.write(bytes([n])*512);sys.stdout.buffer.flush();time.sleep(.04)
elif mode=='consume':
 data=bytearray()
 while True:
  b=sys.stdin.buffer.read(512)
  if not b:break
  data+=b
  print('frame='+str(len(data)//512)+'\\nout_time_us='+str(len(data)*1000),flush=True)
 Path(sys.argv[2]).write_bytes(data)
elif mode=='ui':
 p=subprocess.Popen([sys.argv[2],'--root',sys.argv[3]],creationflags=subprocess.DETACHED_PROCESS|subprocess.CREATE_NEW_PROCESS_GROUP,close_fds=True)
 Path(sys.argv[3],'spawned.json').write_text(str(p.pid))
 time.sleep(120)
''')
 def atomic(path,o):
  t=path.with_suffix('.tmp');t.write_text(json.dumps(o));os.replace(t,path)
 def make(id):
  d=root/id;d.mkdir();out=d/'result.bin';m={'version':1,'source':'fixture','output':str(out),'duration':30,'inputs':[],'plan':{'ffmpeg':str(fixture),'ffmpegArgs':['consume',str(out)],'vspipe':str(fixture),'vspipeArgs':['produce']}}
  atomic(d/'job.json',m);return d
 def status(d):
  try:return json.loads((d/'status.json').read_text())
  except (FileNotFoundError,json.JSONDecodeError):return {}
 def wait(fn,seconds=15):
  until=time.monotonic()+seconds
  while time.monotonic()<until:
   if fn():return
   time.sleep(.08)
  raise AssertionError('timeout')
 def cmd(d,a):atomic(d/('command-'+str(time.time_ns())+'.json'),{'action':a})
 def start():return subprocess.Popen([str(worker),'--root',str(root)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 children=[];ui=None;pid=None
 try:
  first=make('job-001');ui=subprocess.Popen([sys.executable,str(stub),'ui',str(worker),str(root)]);wait(lambda:(root/'spawned.json').exists());pid=int((root/'spawned.json').read_text());wait(lambda:status(first).get('state')=='running');ui.kill();ui.wait();wait(lambda:status(first).get('state')=='completed');expected=b''.join(bytes([n])*512 for n in range(60));assert (first/'result.bin').read_bytes()==expected
  print('PASS UI process killed; detached worker and producer/consumer completed exact payload')
  second=make('job-002');wait(lambda:status(second).get('state')=='running');cmd(second,'pause');wait(lambda:status(second).get('state')=='paused');time.sleep(.15);assert status(second)['state']=='paused';cmd(second,'resume');wait(lambda:status(second).get('state')=='completed');assert (second/'result.bin').read_bytes()==expected
  print('PASS pause/resume and next queued job without UI')
  third=make('job-003');wait(lambda:status(third).get('state')=='running');cmd(third,'stop');wait(lambda:status(third).get('state')=='stopped');assert len((third/'result.bin').read_bytes())<len(expected);cmd(third,'restart');wait(lambda:status(third).get('state')=='completed');assert (third/'result.bin').read_bytes()==expected
  print('PASS stop drains partial output; explicit restart starts from zero')
  fourth=make('job-004');wait(lambda:status(fourth).get('state')=='running');subprocess.run(['taskkill','/PID',str(pid),'/F'],stdout=subprocess.DEVNULL,check=True);pid=None;time.sleep(.3);p=start();children.append(p);wait(lambda:status(fourth).get('state')=='failed');assert '中断' in status(fourth).get('message','');cmd(fourth,'restart');wait(lambda:status(fourth).get('state')=='completed');assert (fourth/'result.bin').read_bytes()==expected
  print('PASS dead worker marks interrupted task failed, never resumes fragments')
 finally:
  if ui and ui.poll() is None:ui.kill();ui.wait()
  if pid:subprocess.run(['taskkill','/PID',str(pid),'/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  for p in children:
   if p.poll() is None:p.kill()
   p.wait(timeout=5)
  time.sleep(.3)
print('PASS 4 lifecycle scenarios; no media encoding or performance measurements')
