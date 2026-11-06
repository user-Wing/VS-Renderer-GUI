// Single-file PlayerLite bootstrapper. Windows 10+ provides tar.exe for tar.xz extraction.
// Each run unpacks beside the launcher, waits for the player, then removes its runtime.
// Qt/player dependencies are embedded; FFmpeg is external. player.ini lives beside this EXE.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct { char marker[8]; uint64_t zipSize; char hash[24]; } PackageFooter;
#pragma pack(pop)

static void fail(const wchar_t *message) {
    MessageBoxW(NULL, message, L"VS Player Lite", MB_OK | MB_ICONERROR);
}
static int exists(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}
static const wchar_t *ffmpegFiles[] = {
    L"ffmpeg.exe", L"ffprobe.exe", L"avcodec-63.dll", L"avformat-63.dll",
    L"avutil-61.dll", L"swresample-7.dll", L"swscale-10.dll", L"avfilter-12.dll"
};
static int checkFfmpeg(const wchar_t *searchPath) {
    wchar_t found[4096], message[2048];
    for (unsigned i = 0; i < sizeof(ffmpegFiles) / sizeof(ffmpegFiles[0]); ++i) {
        DWORD count = SearchPathW(searchPath, ffmpegFiles[i], NULL, 4096, found, NULL);
        if (count && count < 4096 && exists(found)) continue;
        swprintf_s(message, 2048,
            L"需要 FFmpeg shared x64 组件，缺少：%ls\n\n"
            L"请将组件放在 VS-Player.exe 同目录，或将其 bin 目录加入 PATH 环境变量。\n"
            L"需要 ffmpeg.exe、ffprobe.exe，以及 avcodec-63、avformat-63、avutil-61、"
            L"swresample-7、swscale-10、avfilter-12 DLL 和它们的依赖。\n"
            L"仅有静态版 ffmpeg.exe 不能替代这些共享库。", ffmpegFiles[i]);
        fail(message); return 0;
    }
    return 1;
}
static int launchAndWait(const wchar_t *file, wchar_t *cmd, const wchar_t *dir) {
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (!CreateProcessW(file, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, dir, &si, &pi))
        return 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD result = 1;
    GetExitCodeProcess(pi.hProcess, &result);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return result == 0;
}


// Only descend into directories created beside this launcher. Never follow junctions.
static void removeRuntime(const wchar_t *root) {
    wchar_t pattern[4096]; WIN32_FIND_DATAW entry;
    swprintf_s(pattern,4096,L"%ls\\*",root);
    HANDLE find=FindFirstFileW(pattern,&entry);
    if(find!=INVALID_HANDLE_VALUE){
        do {if(!wcscmp(entry.cFileName,L".") || !wcscmp(entry.cFileName,L".."))continue;
            wchar_t path[4096];swprintf_s(path,4096,L"%ls\\%ls",root,entry.cFileName);
            if(entry.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
                if(entry.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)RemoveDirectoryW(path);
                else removeRuntime(path);
            } else {SetFileAttributesW(path,FILE_ATTRIBUTE_NORMAL);DeleteFileW(path);}
        }while(FindNextFileW(find,&entry));FindClose(find);
    }
    RemoveDirectoryW(root);
}
static void cleanStaleRuntimes(const wchar_t *parent){
    wchar_t pattern[4096];WIN32_FIND_DATAW entry;
    swprintf_s(pattern,4096,L"%ls\\.vs-player-run-*",parent);
    HANDLE find=FindFirstFileW(pattern,&entry);if(find==INVALID_HANDLE_VALUE)return;
    do {if(!(entry.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || (entry.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;
        wchar_t root[4096],lockPath[4096];swprintf_s(root,4096,L"%ls\\%ls",parent,entry.cFileName);swprintf_s(lockPath,4096,L"%ls\\.vsp-runtime-lock",root);
        HANDLE lock=CreateFileW(lockPath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
        if(lock!=INVALID_HANDLE_VALUE){CloseHandle(lock);removeRuntime(root);}
    }while(FindNextFileW(find,&entry));FindClose(find);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR args, int show) {
    (void)instance;(void)previous;(void)show;
    wchar_t self[4096],parent[4096],runtime[4096]={0},archive[4096],player[4096],command[8192],envPath[32768],lockPath[4096];
    HANDLE input=INVALID_HANDLE_VALUE,lock=INVALID_HANDLE_VALUE;int result=1;
    if(!GetModuleFileNameW(NULL,self,4096)){fail(L"Cannot resolve executable path.");return 1;}
    wcscpy_s(parent,4096,self);wchar_t *last=wcsrchr(parent,L'\\');if(!last)return 1;*last=0;
    wchar_t originalPath[28000]={0},externalPath[32768];GetEnvironmentVariableW(L"PATH",originalPath,28000);swprintf_s(externalPath,32768,L"%ls;%ls",parent,originalPath);
    if(!checkFfmpeg(externalPath))return 2;
    input=CreateFileW(self,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(input==INVALID_HANDLE_VALUE){fail(L"Cannot open single-file package.");return 1;}
    LARGE_INTEGER size,pos;PackageFooter footer={0};DWORD bytes=0;
    if(!GetFileSizeEx(input,&size) || size.QuadPart<=(LONGLONG)sizeof(footer))goto invalid;
    pos.QuadPart=size.QuadPart-sizeof(footer);
    if(!SetFilePointerEx(input,pos,NULL,FILE_BEGIN) || !ReadFile(input,&footer,sizeof(footer),&bytes,NULL) || bytes!=sizeof(footer) || memcmp(footer.marker,"VSLITE03",8) || footer.zipSize>=(uint64_t)pos.QuadPart)goto invalid;
    wchar_t hash[25];for(int i=0;i<24;++i){if(!((footer.hash[i]>='0' && footer.hash[i]<='9') || (footer.hash[i]>='a' && footer.hash[i]<='f')))goto invalid;hash[i]=footer.hash[i];}hash[24]=0;
    cleanStaleRuntimes(parent);
    swprintf_s(runtime,4096,L"%ls\\.vs-player-run-%ls-%lu",parent,hash,GetCurrentProcessId());
    if(!CreateDirectoryW(runtime,NULL)){runtime[0]=0;fail(L"Cannot create temporary runtime beside VS-Player.exe. Check folder permissions.");goto done;}
    swprintf_s(lockPath,4096,L"%ls\\.vsp-runtime-lock",runtime);
    SECURITY_ATTRIBUTES lockAttributes={sizeof(SECURITY_ATTRIBUTES),NULL,TRUE};
    lock=CreateFileW(lockPath,GENERIC_READ|GENERIC_WRITE,0,&lockAttributes,CREATE_NEW,FILE_ATTRIBUTE_HIDDEN,NULL);
    if(lock==INVALID_HANDLE_VALUE)goto done;
    swprintf_s(archive,4096,L"%ls\\payload.tar.xz",runtime);swprintf_s(player,4096,L"%ls\\vs-player.exe",runtime);
    HANDLE output=CreateFileW(archive,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE){fail(L"Cannot write runtime archive.");goto done;}
    pos.QuadPart=size.QuadPart-sizeof(footer)-(LONGLONG)footer.zipSize;
    int copied=!!SetFilePointerEx(input,pos,NULL,FILE_BEGIN);char buffer[131072];uint64_t remaining=footer.zipSize;
    while(copied && remaining){DWORD chunk=remaining>sizeof(buffer)?sizeof(buffer):(DWORD)remaining,read=0,written=0;
        copied=!!ReadFile(input,buffer,chunk,&read,NULL) && read==chunk && !!WriteFile(output,buffer,read,&written,NULL) && written==read;remaining-=read;}
    CloseHandle(output);if(!copied){fail(L"Cannot read runtime payload.");goto done;}
    wchar_t tar[4096];GetSystemDirectoryW(tar,4000);wcscat_s(tar,4096,L"\\tar.exe");
    swprintf_s(command,8192,L"\"%ls\" -xf \"%ls\" -C \"%ls\"",tar,archive,runtime);
    int unpacked=launchAndWait(tar,command,parent);DeleteFileW(archive);
    if(!unpacked || !exists(player)){fail(L"Cannot unpack PlayerLite. Windows tar.exe with XZ support is required.");goto done;}
    swprintf_s(envPath,32768,L"%ls;%ls",runtime,externalPath);SetEnvironmentVariableW(L"PATH",envPath);
    wchar_t config[4096];swprintf_s(config,4096,L"%ls\\player.ini",parent);SetEnvironmentVariableW(L"VSR_PLAYER_CONFIG",config);SetEnvironmentVariableW(L"VSR_SINGLE_PLAYER_PATH",self);
    if(!exists(config))SetEnvironmentVariableW(L"VSR_PLAYER_NEW_CONFIG",L"1");
    swprintf_s(command,8192,L"\"%ls\" %ls",player,args?args:L"");STARTUPINFOW startup={0};PROCESS_INFORMATION process={0};startup.cb=sizeof(startup);
    // The child retains the lock if the bootstrapper crashes; another launch must
    // not remove resources while that player is still running.
    if(!CreateProcessW(player,command,NULL,NULL,TRUE,0,NULL,parent,&startup,&process)){fail(L"Cannot start VS Player. Check FFmpeg shared x64 dependencies.");goto done;}
    CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);DWORD exitCode=1;GetExitCodeProcess(process.hProcess,&exitCode);CloseHandle(process.hProcess);result=(int)exitCode;goto done;
invalid:
    fail(L"Invalid single-file package.");
done:
    if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);
    if(lock!=INVALID_HANDLE_VALUE)CloseHandle(lock);
    if(runtime[0])removeRuntime(runtime);
    return result;
}
