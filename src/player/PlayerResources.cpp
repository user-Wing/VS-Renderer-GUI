#include "player/PlayerResources.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <dxgi1_4.h>
#include <QHash>
#include <QString>
#include <vector>
#include <algorithm>

namespace vsr {
namespace { quint64 ticks(FILETIME value) { ULARGE_INTEGER n{}; n.LowPart=value.dwLowDateTime; n.HighPart=value.dwHighDateTime; return n.QuadPart; } }
struct PlayerResources::Impl {
    PDH_HQUERY query = nullptr; PDH_HCOUNTER gpu = nullptr;
    quint64 lastSystem = 0, lastIdle = 0, lastProcess = 0;
    void initialize() { if (!query && PdhOpenQueryW(nullptr,0,&query)==ERROR_SUCCESS) { PdhAddEnglishCounterW(query,L"\\GPU Engine(*)\\Utilization Percentage",0,&gpu); PdhCollectQueryData(query); } }
    ~Impl() { if(query) PdhCloseQuery(query); }
};
PlayerResources::PlayerResources():impl_(std::make_unique<Impl>()) {}
PlayerResources::~PlayerResources() = default;
ResourceUsage PlayerResources::sample(bool detailed) {
    ResourceUsage out; auto &i=*impl_; FILETIME idle{},kernel{},user{},created{},ended{},pk{},pu{};
    if(GetSystemTimes(&idle,&kernel,&user) && GetProcessTimes(GetCurrentProcess(),&created,&ended,&pk,&pu)) {
        const auto total=ticks(kernel)+ticks(user), process=ticks(pk)+ticks(pu), id=ticks(idle);
        if(i.lastSystem && total>i.lastSystem) { out.cpu=std::clamp(100.0*(1.0-double(id-i.lastIdle)/double(total-i.lastSystem)),0.0,100.0); out.processCpu=std::clamp(100.0*double(process-i.lastProcess)/double(total-i.lastSystem),0.0,100.0); }
        i.lastSystem=total; i.lastIdle=id; i.lastProcess=process;
    }
    MEMORYSTATUSEX memory{}; memory.dwLength=sizeof(memory); GlobalMemoryStatusEx(&memory); out.ram=memory.dwMemoryLoad; out.totalMemoryMiB=memory.ullTotalPhys/1048576;
    PROCESS_MEMORY_COUNTERS process{}; process.cb=sizeof(process); if(GetProcessMemoryInfo(GetCurrentProcess(),&process,sizeof(process))) out.memoryMiB=process.WorkingSetSize/1048576;
    if (!detailed) return out;
    i.initialize();
    if(i.query && i.gpu && PdhCollectQueryData(i.query)==ERROR_SUCCESS) {
        DWORD bytes=0,count=0;
        if(PdhGetFormattedCounterArrayW(i.gpu,PDH_FMT_DOUBLE,&bytes,&count,nullptr)==PDH_MORE_DATA) {
            std::vector<char> buffer(bytes); auto *items=reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W *>(buffer.data());
            if(PdhGetFormattedCounterArrayW(i.gpu,PDH_FMT_DOUBLE,&bytes,&count,items)==ERROR_SUCCESS) {
                QHash<QString,double> engines;
                for(DWORD n=0;n<count;++n) if(items[n].FmtValue.CStatus==PDH_CSTATUS_VALID_DATA || items[n].FmtValue.CStatus==PDH_CSTATUS_NEW_DATA) {
                    const QString name=QString::fromWCharArray(items[n].szName); const int offset=name.indexOf("luid_");
                    if(offset>=0) engines[name.mid(offset)]+=std::max(0.0,items[n].FmtValue.doubleValue);
                }
                for(auto value:engines) out.gpu=std::max(out.gpu,std::min(100.0,value));
            }
        }
    }
    IDXGIFactory1 *factory=nullptr;
    if(SUCCEEDED(CreateDXGIFactory1(IID_IDXGIFactory1,reinterpret_cast<void **>(&factory)))) {
        for(UINT n=0;;++n) { IDXGIAdapter1 *adapter=nullptr; if(factory->EnumAdapters1(n,&adapter)!=S_OK) break;
            IDXGIAdapter3 *budget=nullptr; const GUID id={0x645967A4,0x1392,0x4310,{0xA7,0x98,0x80,0x53,0xCE,0x3E,0x93,0xFD}};
            if(SUCCEEDED(adapter->QueryInterface(id,reinterpret_cast<void **>(&budget)))) { DXGI_QUERY_VIDEO_MEMORY_INFO info{}; if(SUCCEEDED(budget->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info)) && info.Budget) out.vram=std::max(out.vram,100.0*info.CurrentUsage/info.Budget); budget->Release(); }
            adapter->Release();
        } factory->Release();
    }
    return out;
}
}
