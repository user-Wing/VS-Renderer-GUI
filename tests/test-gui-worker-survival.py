"""Kill a real Qt ExportWindow test process while its detached worker runs.
No video codec: both sides are bounded byte-stream fixtures.
"""
from pathlib import Path
import tempfile,os,sys,time,subprocess,json,shutil
app,worker,fixture=map(lambda p:Path(p).resolve(),sys.argv[1:4])
with tempfile.TemporaryDirectory(prefix='vsr-gui-death-') as tmp:
 r=Path(tmp);shutil.copy2(fixture,r/'vspipe.exe');shutil.copy2(fixture,r/'muxer.exe');shutil.copy2(app,r/'gui.exe');(r/'source.mkv').write_bytes(b'fixture')
 # The resolver intentionally prefers the application's bundled producer over PATH.
 # Give this GUI its own application directory so it cannot select the real vspipe.
 env=dict(os.environ,VSR_EXPORT_ROOT=str(r/'jobs'),VSR_EXPORT_WORKER=str(worker),VSR_GUI_WORKER_FIXTURE=str(r),QT_QPA_PLATFORM='offscreen');env['PATH']=str(r)+';'+str(app.parent)+';'+env['PATH']
 p=subprocess.Popen([str(r/'gui.exe'),'submitForCrashFixture','-o',str(r/'gui.txt')+',txt'],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE);pid=None
 def wait(fn):
  until=time.monotonic()+25
  while time.monotonic()<until:
   value=fn()
   if value:return value
   time.sleep(.05)
  raise AssertionError('timeout: '+((r/'gui.txt').read_text(errors='replace') if (r/'gui.txt').exists() else 'GUI exit='+str(p.poll())))
 def status():
  for f in (r/'jobs').glob('job-*/status.json'):
   try:return json.loads(f.read_text())
   except (ValueError,FileNotFoundError):pass
  return {}
 try:
  s=wait(lambda:status() if status().get('state')=='running' else None);pid=int(s['workerPid']);p.kill();p.communicate(timeout=5)
  wait(lambda:status().get('state')=='completed');expected=b''.join(bytes([i])*512 for i in range(60));assert (r/'out.mkv').read_bytes()==expected
  print('PASS real Qt ExportWindow submitted task; GUI forcibly killed; detached worker completed exact 30720-byte payload')
 finally:
  if p.poll() is None:p.kill();p.wait()
  if pid:subprocess.run(['taskkill','/PID',str(pid),'/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  time.sleep(.3)
