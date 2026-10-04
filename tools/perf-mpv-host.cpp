#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>

int wmain(int argc, wchar_t** argv) {
    if (argc != 5) return 2; // mpv executable, media, width, height
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HWND window = CreateWindowExW(0, L"STATIC", L"mpv performance probe",
        WS_POPUP | WS_VISIBLE, 0, 0, _wtoi(argv[3]), _wtoi(argv[4]),
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\" --no-config --vo=gpu --gpu-api=d3d11 --gpu-context=d3d11 --hwdec=d3d11va --scale=bilinear --cscale=bilinear --dscale=bilinear --start=120 --sid=no --video-sync=audio --script=C:/Private/VS-Renderer-GUI-dev/tools/perf-mpv.lua --wid=";
    command += std::to_wstring(reinterpret_cast<std::uintptr_t>(window));
    command += L" \"" + std::wstring(argv[2]) + L"\"";
    STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &si, &process)) return 3;
    std::fprintf(stderr, "MPV_PID=%lu\n", process.dwProcessId); std::fflush(stderr);
    while (WaitForSingleObject(process.hProcess, 2) == WAIT_TIMEOUT) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    DWORD result{}; GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hThread); CloseHandle(process.hProcess); DestroyWindow(window);
    return int(result);
}
