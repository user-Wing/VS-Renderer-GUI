#define WIN32_LEAN_AND_MEAN
#include "../src/player/PlayerStandbyWin.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR arguments, int show) {
    (void)instance;
    (void)previous;
    (void)arguments;
    (void)show;

    wchar_t self[4096], player[4096];
    const DWORD length = GetModuleFileNameW(NULL, self, 4096);
    if (!length || length >= 4096) return 1;
    wchar_t *name = wcsrchr(self, L'\\');
    if (!name) return 1;
    *name = L'\0';
    // The Full player and the standard single-file launcher use the same
    // case-insensitive filename. The event identifies this installation.
    if (swprintf_s(player, 4096, L"%ls\\vs-player.exe", self) < 0) return 1;
    vspStopStandby(player);
    const ULONGLONG started = GetTickCount64();
    while (vspStandbyExists(player)) {
        if (GetTickCount64() - started >= 10000) {
            MessageBoxW(NULL, L"VS Player standby is still shutting down.", L"VS Player", MB_OK | MB_ICONWARNING);
            return 1;
        }
        Sleep(25);
    }
    return 0;
}
