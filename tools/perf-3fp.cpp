#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include "3FP/Api/FFF.Player.Api.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

// Isolated native playback probe. It never opens player.ini or writes media.
int wmain(int argc, wchar_t** argv) {
    if (argc != 7) {
        std::fwprintf(stderr, L"Usage: perf-3fp DLL VIDEO WIDTH HEIGHT SECONDS ALGORITHM\n");
        return 2;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const std::wstring dllPath = argv[1];
    SetDllDirectoryW(dllPath.substr(0, dllPath.find_last_of(L"\\/")).c_str());
    const auto dll = LoadLibraryW(argv[1]);
    if (!dll) { std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError()); return 3; }
#define LOAD(name) const auto name = reinterpret_cast<decltype(&FFF3FP_##name)>(GetProcAddress(dll, "FFF3FP_" #name)); if (!name) return 4
    LOAD(Create); LOAD(Open); LOAD(Play); LOAD(Pause); LOAD(Seek); LOAD(SetLogCallback);
    LOAD(SetScalingAlgorithms); LOAD(GetSnapshot); LOAD(GetRenderTargetInfo); LOAD(GetLastError); LOAD(Destroy);
    LOAD(GetApiVersion);
    LOAD(SetVolume);
#undef LOAD
    SetLogCallback([](void*, const char* line) noexcept { std::fprintf(stderr, "%s\n", line); }, nullptr);
    const int width = _wtoi(argv[3]), height = _wtoi(argv[4]);
    const double seconds = _wtof(argv[5]);
    const auto window = CreateWindowExW(0, L"STATIC", L"3FP performance probe",
        WS_POPUP | WS_VISIBLE, 0, 0, width, height, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    FFF3FPConfiguration config{};
    config.size = sizeof(config); config.version = GetApiVersion(); config.outputWindow = window;
    if(const auto adapter=std::getenv("VSR_3FP_ADAPTER"))config.preferredAdapterIndex=std::atoi(adapter);
    config.decodeMode = FFF3FPDecodeMode::D3D11;
    if (const auto mode = std::getenv("VSR_3FP_DECODE_MODE"))
        config.decodeMode = static_cast<FFF3FPDecodeMode>(std::atoi(mode));
    config.colorMode = FFF3FPColorMode::MapToSdr;
    config.sdrPeakNits = 100; config.hdrPeakNits = 1000; config.sdrPaperWhiteNits = 203;
    config.videoScalingQuality = FFF3FPVideoScalingQuality::HighQuality;
    FFF3FPHandle player{};
    const auto result = Create(&config, &player);
    if (result != FFFResult::Success) { std::fprintf(stderr, "Create failed: %d\n", int(result)); return 5; }
    SetVolume(player, 1.0f, 1);
    if(const auto height=std::getenv("VSR_3FP_PRESCALE")) {
        const auto setPreScale=reinterpret_cast<decltype(&FFF3FP_SetSoftwarePreScale)>(GetProcAddress(dll,"FFF3FP_SetSoftwarePreScale"));
        if(!setPreScale || setPreScale(player,std::atoi(height))!=FFFResult::Success)return 8;
    }
    const auto algorithm = static_cast<FFF3FPScalingAlgorithm>(_wtoi(argv[6]));
    SetScalingAlgorithms(player, algorithm, static_cast<FFF3FPScalingAlgorithm>(_wtoi(argv[6]) & 255));
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, nullptr, 0, nullptr, nullptr);
    std::string path(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, path.data(), bytes, nullptr, nullptr);
    const auto start = std::chrono::steady_clock::now();
    if (Open(player, path.c_str()) != FFFResult::Success) {
        char error[2048]{}; GetLastError(player, error, sizeof(error), nullptr);
        std::fprintf(stderr, "Open failed: %s\n", error); Destroy(player); return 7;
    }
    bool playing = false, measuring = false, failed = false;
    auto measurement = start;
    double first = -1, nextSample = 0;
    std::puts("wall_s,position_s,decoded,accepted,dropped,coalesced,presents,present_wait_ms,lock_wait_ms,transfer_ms,convert_ms,decode_mode,scaling_mode,swap_w,swap_h,dest_w,dest_h,output_bits,seek_generation,first_frame_s,cpu_s,queued_video_frames");
    for (;;) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        RECT rect{};
        if (!GetClientRect(window, &rect) || rect.right != width || rect.bottom != height) {
            std::fprintf(stderr, "Output window changed: %ldx%ld, expected %dx%d. Discard this trial.\n",
                rect.right, rect.bottom, width, height); failed = true; break;
        }
        FFF3FPSnapshot s{}; s.size = sizeof(s); s.version = 8;
        if (GetSnapshot(player, &s) != FFFResult::Success) return 6;
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - start).count();
        if (s.swapChainPresents && s.presentedVideoFrames && first < 0) first = elapsed;
        if (s.state == FFF3FPState::Failed || elapsed > seconds + 65) {
            char error[2048]{}; GetLastError(player, error, sizeof(error), nullptr);
            std::fprintf(stderr, "%s\n", error);
            std::fprintf(stderr, "Playback failed/timed out: state=%u\n", unsigned(s.state)); failed = true; break;
        }
        if (s.state == FFF3FPState::Ready && !playing) {
            const double seekSeconds=std::getenv("VSR_PERF_SEEK_SECONDS")?std::atof(std::getenv("VSR_PERF_SEEK_SECONDS")):10.0;
            Seek(player, static_cast<long long>(std::min(seekSeconds,std::max(0.0,s.duration100ns/1e7-seconds-6))*10000000)); Play(player); playing = true;
        }
        if (playing && !measuring && s.timelineGeneration && s.presentedVideoFrames > 1) {
            measurement = now; measuring = true;
        }
        const double wall = std::chrono::duration<double>(now - measurement).count();
        if (measuring && wall >= nextSample) {
            FILETIME created{}, exited{}, kernel{}, user{};
            GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
            const double cpuSeconds = (double(kernel.dwHighDateTime) * 4294967296.0 +
                kernel.dwLowDateTime + double(user.dwHighDateTime) * 4294967296.0 +
                user.dwLowDateTime) / 1e7;
            FFF3FPRenderTargetInfo t{}; t.size = sizeof(t); t.version = 1;
            GetRenderTargetInfo(player, &t);
            std::printf("%.6f,%.6f,%llu,%llu,%llu,%llu,%llu,%.4f,%.4f,%.4f,%.4f,%u,%u,%u,%u,%u,%u,%u,%llu,%.6f,%.6f,%u\n",
                wall, s.position100ns / 1e7, s.decodedVideoFrames, s.presentedVideoFrames,
                s.droppedVideoFrames, s.coalescedVideoFrames, s.swapChainPresents,
                s.presentWait100ns / 1e4, s.deviceLockWait100ns / 1e4,
                s.hardwareTransfer100ns / 1e4, s.softwareConvert100ns / 1e4,
                unsigned(s.decodeMode), unsigned(s.videoScalingMode), t.swapWidth, t.swapHeight,
                t.destWidth, t.destHeight, t.outputBitDepth, s.timelineGeneration, first, cpuSeconds, s.queuedVideoFrames);
            std::fflush(stdout); nextSample += 1;
        }
        if (measuring && wall >= seconds + 5) break;
        Sleep(2);
    }
    Pause(player); Destroy(player); DestroyWindow(window); FreeLibrary(dll);
    return playing && !failed ? 0 : 7;
}
