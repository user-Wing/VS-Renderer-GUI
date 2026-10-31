from pathlib import Path
import ctypes as c
from ctypes import wintypes as w
import json,os,subprocess,time,statistics,shutil,sys
r=Path.cwd();work=r/'build/upstream-20261031';u=c.WinDLL('user32',use_last_error=True);k=c.WinDLL('kernel32',use_last_error=True);g=c.WinDLL('gdi32',use_last_error=True)
u.SetThreadDpiAwarenessContext.argtypes=[w.HANDLE];u.SetThreadDpiAwarenessContext(c.c_void_p(-4))
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)];u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int];u.GetClientRect.argtypes=[w.HWND,c.POINTER(w.RECT)];u.ClientToScreen.argtypes=[w.HWND,c.POINTER(w.POINT)];u.IsWindowVisible.argtypes=[w.HWND];u.GetDC.argtypes=[w.HWND];u.GetDC.restype=w.HDC;u.ReleaseDC.argtypes=[w.HWND,w.HDC];u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
g.GetPixel.argtypes=[w.HDC,c.c_int,c.c_int];g.GetPixel.restype=w.DWORD
u.SetWindowPos.argtypes=[w.HWND,w.HWND,c.c_int,c.c_int,c.c_int,c.c_int,w.UINT]
callback=c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM);u.EnumWindows.argtypes=[callback,w.LPARAM]
class Process(c.Structure):
 _fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',c.c_size_t),('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('exe',w.WCHAR*260)]
k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE;k.Process32FirstW.argtypes=[w.HANDLE,c.POINTER(Process)];k.Process32NextW.argtypes=[w.HANDLE,c.POINTER(Process)];k.CloseHandle.argtypes=[w.HANDLE];k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE;k.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];k.TerminateProcess.argtypes=[w.HANDLE,w.UINT]
k.GetExitCodeProcess.argtypes=[w.HANDLE,c.POINTER(w.DWORD)]
def descendants(owned):
 h=k.CreateToolhelp32Snapshot(2,0);p=Process();p.size=c.sizeof(p);ok=k.Process32FirstW(h,c.byref(p));pairs=[]
 while ok:pairs.append((p.pid,p.parent));ok=k.Process32NextW(h,c.byref(p))
 k.CloseHandle(h)
 for _ in range(3):
  for pid,parent in pairs:
   if parent in owned:owned.add(pid)
def window(owned):
 found=[]
 @callback
 def visit(hwnd,_):
  pid=w.DWORD();u.GetWindowThreadProcessId(hwnd,c.byref(pid))
  if pid.value in owned and u.IsWindowVisible(hwnd):
   title=c.create_unicode_buffer(1024);u.GetWindowTextW(hwnd,title,1024);rect=w.RECT();u.GetClientRect(hwnd,c.byref(rect));point=w.POINT();u.ClientToScreen(hwnd,c.byref(point))
   if 'VS Player' in title.value and rect.right>600 and rect.bottom>300:found.append((rect.right*rect.bottom,hwnd,pid.value,point.x,point.y,rect.right,rect.bottom))
  return True
 u.EnumWindows(visit,0);return max(found) if found else None
def close(owned,hwnd):
 forced=[];codes={}
 if hwnd:u.PostMessageW(hwnd,0x10,0,0)
 for pid in owned:
  handle=k.OpenProcess(0x101001,False,pid)
  if handle:
   if k.WaitForSingleObject(handle,5000)==0x102:k.TerminateProcess(handle,1);forced.append(pid)
   code=w.DWORD();k.GetExitCodeProcess(handle,c.byref(code));codes[str(pid)]=code.value
   k.CloseHandle(handle)
 return forced,codes
def config(path):
 path.parent.mkdir(parents=True,exist_ok=True);path.write_text('[basic]\nautoplay=true\n[playback]\nremember=false\n[player]\npreset=\nmuted=true\nanimeStage=5\n[decode]\nmode=2\n[color]\nengine=0\n',encoding='utf-8')
def trial(label,exe,enabled=True,cold=False,single=False):
 env=os.environ.copy();env['PATH']=str(r/'dist/VS-GUI-Full')+';'+env['PATH'];env.pop('VSR_3FP_DLL',None)
 env['FFF_NO_PREFETCH']='0' if enabled else '1'
 if not single:
  env['VSR_PLAYER_CONFIG']=str(work/'startup-config'/f'{label}.ini');config(Path(env['VSR_PLAYER_CONFIG']))
  env['VSR_3FP_DLL']=str(work/('FFF.Native-before.dll' if label.startswith('old') else 'native/FFF.Native.dll'))
 else:
  config(exe.parent/'player.ini');env['LOCALAPPDATA']=str(work/('cold-'+label if cold else 'warm-'+label.split('-')[0]));Path(env['LOCALAPPDATA']).mkdir(exist_ok=True)
  with exe.open('rb') as file:file.seek(-24,2);tag=file.read(24).decode('ascii')
  cold=not (Path(env['LOCALAPPDATA'])/'VSPlayerLite'/tag/'.ready').is_file()
 with (work/(label+'.stderr.txt')).open('wb') as output:
  start=time.perf_counter();process=subprocess.Popen([str(exe),str(work/'startup-green.mp4')],env=env,stdout=output,stderr=subprocess.STDOUT,creationflags=0x08000000);spawn=(time.perf_counter()-start)*1000;owned={process.pid};first=None;frame=None;hwnd=None;last_scan=0
  try:
   while time.perf_counter()-start<15:
    now=time.perf_counter()
    if single and not hwnd and now-last_scan>.025:descendants(owned);last_scan=now
    view=window(owned)
    if view:
     _,hwnd,pid,x,y,width,height=view
     if first is None:
      first=(time.perf_counter()-start)*1000;u.SetWindowPos(hwnd,w.HWND(-1),0,0,0,0,0x13)
     dc=u.GetDC(None);green=0
     for fraction in (.35,.45,.55):
      color=g.GetPixel(dc,x+width//2,y+int(height*fraction));red=color&255;blue=(color>>16)&255;value=(color>>8)&255;green+=value>red+45 and value>blue+45
     u.ReleaseDC(None,dc)
     if green>=2:frame=(time.perf_counter()-start)*1000;break
    time.sleep(.005)
  finally:forced,codes=close(owned,hwnd);process.wait(timeout=5)
 row={'case':label,'host':str(exe),'pid':process.pid,'prefetch':enabled,'coldExtraction':cold,'single':single,'spawnMs':spawn,'visibleWindowMs':first,'greenFrameMs':frame,'forcedStops':forced,'exitCodes':codes,'success':frame is not None and not forced and all(value==0 for value in codes.values())};print(json.dumps(row),flush=True);return row
def main():
 mode=sys.argv[1] if len(sys.argv)>1 else 'full';rows=[]
 if mode=='full':
  old=r/'dist/VS-GUI-Full/vs-player.exe'
  for i in range(3):rows.append(trial('old-full-'+str(i),old,False))
  for i in range(8):
   for enabled in ((False,True) if i%2==0 else (True,False)):rows.append(trial(f'new-full-{i}-{int(enabled)}',r/'build/mingw-release/vs-player.exe',enabled))
 else:
  for variant,source in [('old',work/'VS-Player-before.exe'),('new',r/'dist/VS-Player.exe')]:
   exe=work/('single-'+variant)/'VS-Player.exe';exe.parent.mkdir(exist_ok=True);shutil.copy2(source,exe);rows.append(trial(variant+'single-first',exe,True,False,True))
   if variant=='old':
    for i in range(3):rows.append(trial(f'oldsingle-{i}',exe,False,False,True))
   else:
    for i in range(8):
     for enabled in ((False,True) if i%2==0 else (True,False)):rows.append(trial(f'newsingle-{i}-{int(enabled)}',exe,enabled,False,True))
    for i in range(3):rows.append(trial(f'newsingle-cold-{i}',exe,True,True,True))
 (work/(mode+'-startup.json')).write_text(json.dumps(rows,indent=2),encoding='utf-8')
 if not all(row['success'] for row in rows):raise SystemExit('Startup trials failed')
if __name__=='__main__':main()
