// Single-file PlayerLite bootstrapper. Windows 10+ provides tar.exe for ZIP extraction.
// First run extracts bundled Qt/player components once; subsequent launches reuse the cache.
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR args, int show) {
    (void)instance; (void)previous; (void)show;
    wchar_t self[4096], originalDir[4096], cacheParent[4096], cache[4096];
    wchar_t archive[4096], ready[4096], player[4096], command[8192], envPath[32768];
    if (!GetModuleFileNameW(NULL, self, 4096)) { fail(L"Cannot resolve executable path."); return 1; }
    wcscpy_s(originalDir, 4096, self);
    wchar_t *last = wcsrchr(originalDir, L'\\');
    if (!last) { fail(L"Invalid executable directory."); return 1; }
    *last = 0;
    wchar_t originalPath[28000] = { 0 }, externalPath[32768];
    GetEnvironmentVariableW(L"PATH", originalPath, 28000);
    swprintf_s(externalPath, 32768, L"%ls;%ls", originalDir, originalPath);
    if (!checkFfmpeg(externalPath)) return 2;
    HANDLE input = CreateFileW(self, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (input == INVALID_HANDLE_VALUE) { fail(L"Cannot open single-file package."); return 1; }
    LARGE_INTEGER size, pos;
    PackageFooter footer = { 0 };
    DWORD bytes = 0;
    if (!GetFileSizeEx(input, &size) || size.QuadPart <= (LONGLONG)sizeof(footer)) goto invalid;
    pos.QuadPart = size.QuadPart - sizeof(footer);
    if (!SetFilePointerEx(input, pos, NULL, FILE_BEGIN) ||
        !ReadFile(input, &footer, sizeof(footer), &bytes, NULL) || bytes != sizeof(footer) ||
        memcmp(footer.marker, "VSLITE02", 8) || footer.zipSize >= (uint64_t)pos.QuadPart) goto invalid;
    wchar_t hash[25];
    for (int i = 0; i < 24; ++i) {
        if (!((footer.hash[i] >= '0' && footer.hash[i] <= '9') ||
              (footer.hash[i] >= 'a' && footer.hash[i] <= 'f'))) goto invalid;
        hash[i] = (wchar_t)footer.hash[i];
    }
    hash[24] = 0;
    if (!GetEnvironmentVariableW(L"LOCALAPPDATA", cacheParent, 3900)) goto invalid;
    wcscat_s(cacheParent, 4096, L"\\VSPlayerLite");
    CreateDirectoryW(cacheParent, NULL);
    swprintf_s(cache, 4096, L"%ls\\%ls", cacheParent, hash);
    swprintf_s(archive, 4096, L"%ls\\payload.zip", cache);
    swprintf_s(ready, 4096, L"%ls\\.ready", cache);
    swprintf_s(player, 4096, L"%ls\\vs-player.exe", cache);
    wchar_t mutexName[100];
    swprintf_s(mutexName, 100, L"Local\\VSPlayerLite-%ls", hash);
    HANDLE mutex = CreateMutexW(NULL, FALSE, mutexName);
    if (mutex) WaitForSingleObject(mutex, INFINITE);
    if (!exists(ready) || !exists(player)) {
        CreateDirectoryW(cache, NULL);
        HANDLE output = CreateFileW(archive, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (output == INVALID_HANDLE_VALUE) {
            if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
            CloseHandle(input);
            fail(L"Cannot create PlayerLite cache."); return 1;
        }
        pos.QuadPart = size.QuadPart - sizeof(footer) - (LONGLONG)footer.zipSize;
        int copied = !!SetFilePointerEx(input, pos, NULL, FILE_BEGIN);
        char buf[65536];
        uint64_t remaining = footer.zipSize;
        while (copied && remaining) {
            DWORD chunk = remaining > sizeof(buf) ? sizeof(buf) : (DWORD)remaining;
            DWORD readBytes = 0, written = 0;
            copied = !!ReadFile(input, buf, chunk, &readBytes, NULL) &&
                readBytes == chunk && !!WriteFile(output, buf, readBytes, &written, NULL) &&
                written == readBytes;
            remaining -= readBytes;
        }
        CloseHandle(output);
        if (!copied) {
            if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
            CloseHandle(input);
            fail(L"Cannot extract PlayerLite archive."); return 1;
        }
        swprintf_s(command, 8192, L"tar.exe -xf \"%ls\" -C \"%ls\"", archive, cache);
        int unpacked = launchAndWait(NULL, command, originalDir);
        DeleteFileW(archive);
        if (!unpacked || !exists(player)) {
            if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
            CloseHandle(input);
            fail(L"Cannot unpack PlayerLite (Windows tar.exe is required)."); return 1;
        }
        HANDLE marker = CreateFileW(ready, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (marker != INVALID_HANDLE_VALUE) CloseHandle(marker);
    }
    if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
    CloseHandle(input);

    // Keep the launcher directory ahead of PATH; cache supplies Qt/common DLLs.
    swprintf_s(envPath, 32768, L"%ls;%ls", cache, externalPath);
    SetEnvironmentVariableW(L"PATH", envPath);
    wchar_t config[4096];
    swprintf_s(config, 4096, L"%ls\\player.ini", originalDir);
    SetEnvironmentVariableW(L"VSR_PLAYER_CONFIG", config);
    if (!exists(config)) {
        HANDLE settings = CreateFileW(config, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
        if (settings == INVALID_HANDLE_VALUE && !exists(config)) {
            fail(L"Cannot create player.ini beside VS-Player.exe. Check folder permissions.");
            return 1;
        }
        if (settings != INVALID_HANDLE_VALUE) {
            static const char defaults[] = "[basic]\r\nlanguage=zh_CN\r\n";
            DWORD written = 0;
            int ok = !!WriteFile(settings, defaults, sizeof(defaults) - 1, &written, NULL);
            CloseHandle(settings);
            if (!ok || written != sizeof(defaults) - 1) {
                fail(L"Cannot initialize player.ini.");
                return 1;
            }
            SetEnvironmentVariableW(L"VSR_PLAYER_NEW_CONFIG", L"1");
        }
    }
    swprintf_s(command, 8192, L"\"%ls\" %ls", player, args ? args : L"");
    STARTUPINFOW startup = { 0 };
    PROCESS_INFORMATION process = { 0 };
    startup.cb = sizeof(startup);
    if (!CreateProcessW(player, command, NULL, NULL, FALSE, 0, NULL, originalDir, &startup, &process)) {
        fail(L"Cannot start VS Player. Check the FFmpeg shared x64 components and the extracted runtime.");
        return 1;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
invalid:
    CloseHandle(input);
    fail(L"Invalid PlayerLite single-file package.");
    return 1;
}
