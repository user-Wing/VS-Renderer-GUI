#pragma once
#include <windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
class PlaybackScheduling final {
public:
    explicit PlaybackScheduling(const wchar_t *task) noexcept {
        wchar_t enabled[8]{};
        if(GetEnvironmentVariableW(L"VSR_MMCSS",enabled,8) && enabled[0]==L'1'){
            DWORD index=0;task_=AvSetMmThreadCharacteristicsW(task,&index);
            if(task_)AvSetMmThreadPriority(task_,AVRT_PRIORITY_NORMAL);
        }
    }
    ~PlaybackScheduling(){if(task_)AvRevertMmThreadCharacteristics(task_);}
    PlaybackScheduling(const PlaybackScheduling&)=delete;
    PlaybackScheduling& operator=(const PlaybackScheduling&)=delete;
private:
    HANDLE task_=nullptr;
};
