#pragma once

// Shared by the Full player and the C single-file bootstrapper.
#include <windows.h>
#include <shellapi.h>
#include <stdint.h>
#include <wchar.h>

#define VSP_STANDBY_CLASS L"VSPlayer.Standby.v1"
#define VSP_WARM_WINDOW_CLASS L"VSPlayer.WarmWindow.v1"
#define VSP_STANDBY_OPEN 0x56535031
#define VSP_WARM_WINDOW_USED 42

static const wchar_t *vspStartupArguments(void) {
    const wchar_t *command = GetCommandLineW();
    if (*command == L'"') {
        ++command;
        while (*command && *command != L'"') ++command;
        if (*command) ++command;
    } else {
        while (*command && *command != L' ' && *command != L'\t') ++command;
    }
    while (*command == L' ' || *command == L'\t') ++command;
    return command;
}

static void vspStandbyEventName(wchar_t name[64], const wchar_t *exe) {
    wchar_t path[4096];
    wcscpy_s(path, 4096, exe);
    CharLowerBuffW(path, (DWORD)wcslen(path));
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const wchar_t *p = path; *p; ++p) {
        hash ^= *p;
        hash *= UINT64_C(1099511628211);
    }
    swprintf_s(name, 64, L"Local\\VSPlayer.Standby.%016llx", (unsigned long long)hash);
}

static HANDLE vspBeginStandby(const wchar_t *exe) {
    wchar_t name[64];
    vspStandbyEventName(name, exe);
    HANDLE stop = CreateEventW(NULL, TRUE, FALSE, name);
    if (stop && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(stop);
        return NULL;
    }
    return stop;
}

static BOOL vspStandbyExists(const wchar_t *exe) {
    wchar_t name[64];
    vspStandbyEventName(name, exe);
    HANDLE stop = OpenEventW(SYNCHRONIZE, FALSE, name);
    if (!stop) return FALSE;
    CloseHandle(stop);
    return TRUE;
}

static BOOL vspKeepInSession(const wchar_t *directory) {
    wchar_t config[8192], value[16];
    if (swprintf_s(config, 8192, L"%ls\\player.ini", directory) < 0) return FALSE;
    GetPrivateProfileStringW(L"basic", L"standbyInSession", L"true", value, 16, config);
    return _wcsicmp(value, L"false") && wcscmp(value, L"0");
}

static void vspStopStandby(const wchar_t *exe) {
    wchar_t name[64];
    vspStandbyEventName(name, exe);
    HANDLE stop = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    if (stop) {
        SetEvent(stop);
        CloseHandle(stop);
    }
}

static BOOL vspCanForward(const wchar_t *arguments) {
    if (!*arguments) return TRUE;
    wchar_t command[8192];
    if (swprintf_s(command, 8192, L"vsp %ls", arguments) < 0) return FALSE;
    int count = 0;
    wchar_t **values = CommandLineToArgvW(command, &count);
    if (!values) return FALSE;
    BOOL valid = TRUE;
    for (int i = 1; i < count && valid; ++i) {
        const wchar_t *path = values[i];
        const size_t length = wcslen(path);
        const BOOL drive = length >= 3 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/');
        const BOOL unc = length > 3 && path[0] == L'\\' && path[1] == L'\\' && path[2] != L'.' && path[2] != L'?';
        valid = (drive || unc) && !wcschr(path, L'"') &&
            (length < 4 || _wcsicmp(path + length - 4, L".vpy") != 0);
    }
    LocalFree(values);
    return valid;
}

static BOOL vspForwardToStandby(const wchar_t *exe, const wchar_t *arguments, const wchar_t *windowClass) {
    HWND receiver = FindWindowExW(HWND_MESSAGE, NULL, windowClass, exe);
    if (!receiver || wcslen(arguments) >= 4000 || !vspCanForward(arguments)) return FALSE;
    DWORD pid = 0, length = 4096;
    GetWindowThreadProcessId(receiver, &pid);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return FALSE;
    wchar_t image[4096];
    const BOOL matches = QueryFullProcessImageNameW(process, 0, image, &length) &&
        CompareStringOrdinal(image, -1, exe, -1, TRUE) == CSTR_EQUAL;
    CloseHandle(process);
    if (!matches) return FALSE;
    AllowSetForegroundWindow(pid);
    COPYDATASTRUCT data = {VSP_STANDBY_OPEN, (DWORD)((wcslen(arguments) + 1) * sizeof(wchar_t)), (void *)arguments};
    DWORD_PTR accepted = 0;
    return SendMessageTimeoutW(receiver, WM_COPYDATA, 0, (LPARAM)&data,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 350, &accepted) && accepted;
}

typedef struct {
    BOOL (*open)(void *context, const wchar_t *arguments);
    void *context;
} VspStandbyReceiver;

typedef struct {
    const wchar_t *player;
    const wchar_t *directory;
    BOOL inheritRuntimeLock;
    BOOL warmWindowClaimed;
} VspStandbyLaunch;

static HANDLE vspStartPlayer(const VspStandbyLaunch *launch, const wchar_t *arguments) {
    wchar_t command[8192];
    if (swprintf_s(command, 8192, L"\"%ls\" %ls", launch->player, arguments) < 0) return NULL;
    STARTUPINFOW startup = {0};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(launch->player, command, NULL, NULL, launch->inheritRuntimeLock,
            0, NULL, launch->directory, &startup, &process)) return NULL;
    if (wcscmp(arguments, L"--warm-window") && wcscmp(arguments, L"--standby"))
        AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    return process.hProcess;
}

static BOOL vspOpenStandbyPlayer(void *context, const wchar_t *arguments) {
    VspStandbyLaunch *launch = (VspStandbyLaunch *)context;
    if (vspForwardToStandby(launch->player, arguments, VSP_WARM_WINDOW_CLASS)) {
        launch->warmWindowClaimed = TRUE;
        return TRUE;
    }
    // Full already has a direct launch path. Lite can still avoid re-extracting.
    if (!launch->inheritRuntimeLock) return FALSE;
    const HANDLE player = vspStartPlayer(launch, arguments);
    if (!player) return FALSE;
    CloseHandle(player);
    return TRUE;
}

static LRESULT CALLBACK vspStandbyWindow(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW *create = (const CREATESTRUCTW *)lParam;
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    if (message == WM_COPYDATA) {
        const COPYDATASTRUCT *data = (const COPYDATASTRUCT *)lParam;
        if (!data || data->dwData != VSP_STANDBY_OPEN || !data->lpData ||
            data->cbData < sizeof(wchar_t) || data->cbData > 4000 * sizeof(wchar_t) ||
            data->cbData % sizeof(wchar_t) != 0) return FALSE;
        const wchar_t *arguments = (const wchar_t *)data->lpData;
        const size_t characters = data->cbData / sizeof(wchar_t);
        if (wmemchr(arguments, L'\0', characters) != arguments + characters - 1 ||
            !vspCanForward(arguments)) return FALSE;
        const VspStandbyReceiver *receiver = (const VspStandbyReceiver *)GetWindowLongPtrW(window, GWLP_USERDATA);
        return receiver && receiver->open(receiver->context, arguments);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static HWND vspCreateStandbyReceiver(const wchar_t *exe, const wchar_t *windowClass, VspStandbyReceiver *receiver) {
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSW type = {0};
    type.lpfnWndProc = vspStandbyWindow;
    type.hInstance = instance;
    type.lpszClassName = windowClass;
    if (!RegisterClassW(&type)) return NULL;
    return CreateWindowExW(0, windowClass, exe, 0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, instance, receiver);
}

static BOOL vspRunStandby(const wchar_t *exe, HANDLE stop, VspStandbyLaunch *launch, const wchar_t *initialArguments) {
    if (WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) return TRUE;
    VspStandbyReceiver receiver = {vspOpenStandbyPlayer, launch};
    const HWND window = vspCreateStandbyReceiver(exe, VSP_STANDBY_CLASS, &receiver);
    if (!window) return FALSE;
    if (initialArguments) {
        const HANDLE player = vspStartPlayer(launch, initialArguments);
        if (!player) {
            DestroyWindow(window);
            return FALSE;
        }
        CloseHandle(player);
    }
    wchar_t pid[16];
    swprintf_s(pid, 16, L"%lu", GetCurrentProcessId());
    SetEnvironmentVariableW(L"VSR_PLAYER_WARM_PARENT", pid);
    const BOOL budget = SetProcessWorkingSetSizeEx(GetCurrentProcess(), 256 * 1024, 9500000, QUOTA_LIMITS_HARDWS_MAX_ENABLE);
    HANDLE handles[2] = {stop, budget ? vspStartPlayer(launch, L"--warm-window") : NULL};
    for (;;) {
        const DWORD count = handles[1] ? 2 : 1;
        const DWORD signaled = MsgWaitForMultipleObjects(count, handles, FALSE, INFINITE, QS_ALLINPUT);
        if (signaled == WAIT_OBJECT_0 || signaled == WAIT_FAILED) break;
        if (count == 2 && signaled == WAIT_OBJECT_0 + 1) {
            DWORD code = 0;
            GetExitCodeProcess(handles[1], &code);
            CloseHandle(handles[1]);
            handles[1] = code == VSP_WARM_WINDOW_USED && WaitForSingleObject(stop, 0) == WAIT_TIMEOUT
                ? vspStartPlayer(launch, L"--warm-window") : NULL;
        } else {
            MSG message;
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
            if (launch->warmWindowClaimed) {
                launch->warmWindowClaimed = FALSE;
                if (handles[1]) CloseHandle(handles[1]);
                // The foreground child owns its media. Prepare the next idle
                // window while playback is open rather than after it closes.
                handles[1] = budget && WaitForSingleObject(stop, 0) == WAIT_TIMEOUT
                    ? vspStartPlayer(launch, L"--warm-window") : NULL;
            }
        }
    }
    if (IsWindow(window)) DestroyWindow(window);
    if (handles[1]) {
        WaitForSingleObject(handles[1], 300);
        CloseHandle(handles[1]);
    }
    return TRUE;
}
