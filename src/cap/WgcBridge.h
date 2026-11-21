#pragma once
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
// C ABI only: the MSVC C++/WinRT helper never exposes C++/Qt objects or allocations.
using VsrWgcReceiver = void (__cdecl *)(const uint16_t *,uint32_t,uint32_t,uint32_t,void *);
using VsrWgcCapture = HRESULT (__cdecl *)(HMONITOR,VsrWgcReceiver,void *,wchar_t *,uint32_t);
#endif
