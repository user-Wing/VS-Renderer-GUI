#pragma once
#include <windows.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QSettings>
#include <QElapsedTimer>
#include <cmath>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif
namespace vsr {
// Read-only counters: reuse installed vendor libraries and the existing PawnIO
// service. Never install a driver, change its ACL, or request elevation here.
class PowerSensors {
    QLibrary pawn_,nvml_;
    HANDLE handle_=nullptr;
    using Open=HRESULT(__stdcall *)(HANDLE*);
    using Load=HRESULT(__stdcall *)(HANDLE,const unsigned char*,size_t);
    using Execute=HRESULT(__stdcall *)(HANDLE,const char*,const uint64_t*,size_t,uint64_t*,size_t,size_t*);
    using Close=HRESULT(__stdcall *)(HANDLE);
    Execute execute_=nullptr;Close close_=nullptr;
    using NvInit=int(*)();using NvCount=int(*)(unsigned*);
    using NvHandle=int(*)(unsigned,void**);using NvPower=int(*)(void*,unsigned*);
    NvInit shutdown_=nullptr;NvCount count_=nullptr;NvHandle device_=nullptr;NvPower power_=nullptr;
    bool initialized_=false,nvReady_=false;
    uint64_t energyRegister_=0;uint32_t previous_=0;double joules_=0;
    QElapsedTimer timer_;double lastCpu_=-1;
    QString cpuStatus_=QStringLiteral("N/A");
    bool read(uint64_t address,uint64_t &value) {
        size_t returned=0;return execute_ && SUCCEEDED(execute_(handle_,"ioctl_read_msr",&address,1,&value,1,&returned)) && returned==1;
    }
    void initialize() {
        initialized_=true;
        wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
        nvml_.setFileName(QDir(QString::fromWCharArray(system)).filePath("nvml.dll"));
        const auto init=reinterpret_cast<NvInit>(nvml_.resolve("nvmlInit_v2"));
        shutdown_=reinterpret_cast<NvInit>(nvml_.resolve("nvmlShutdown"));
        count_=reinterpret_cast<NvCount>(nvml_.resolve("nvmlDeviceGetCount_v2"));
        device_=reinterpret_cast<NvHandle>(nvml_.resolve("nvmlDeviceGetHandleByIndex_v2"));
        power_=reinterpret_cast<NvPower>(nvml_.resolve("nvmlDeviceGetPowerUsage"));
        nvReady_=init && shutdown_ && count_ && device_ && power_ && init()==0;
        unsigned a=0,b=0,c=0,d=0;
#ifdef _MSC_VER
        int registers[4]{};__cpuid(registers,0);b=registers[1];
#else
        __cpuid(0,a,b,c,d);
#endif
        const bool amd=b==0x68747541,intel=b==0x756e6547;
#ifdef _MSC_VER
        __cpuid(registers,1);a=registers[0];
#else
        __cpuid(1,a,b,c,d);
#endif
        unsigned family=(a>>8)&15;if(family==15)family+=(a>>20)&255;
        if(!intel && !(amd && (family==0x17 || family==0x19)))return;
        QSettings installed("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO",QSettings::NativeFormat);
        const auto directory=installed.value("InstallLocation",qEnvironmentVariable("ProgramFiles")+"/PawnIO").toString();
        pawn_.setFileName(QDir(directory).filePath("PawnIOLib.dll"));
        const auto open=reinterpret_cast<Open>(pawn_.resolve("pawnio_open"));
        const auto load=reinterpret_cast<Load>(pawn_.resolve("pawnio_load"));
        execute_=reinterpret_cast<Execute>(pawn_.resolve("pawnio_execute"));close_=reinterpret_cast<Close>(pawn_.resolve("pawnio_close"));
        if(!open || !load || !execute_ || !close_)return;
        const auto result=open(&handle_);
        if(FAILED(result)){handle_=nullptr;if(result==E_ACCESSDENIED)cpuStatus_=QStringLiteral("N/A（PawnIO 需要管理员权限）");return;}
        QFile module(amd?":/pawnio/AMDFamily17.bin":":/pawnio/IntelMSR.bin");
        if(!module.open(QIODevice::ReadOnly))return;
        const auto bytes=module.readAll();if(FAILED(load(handle_,reinterpret_cast<const unsigned char*>(bytes.constData()),size_t(bytes.size()))))return;
        energyRegister_=amd?0xC001029B:0x611;
    }
public:
    ~PowerSensors(){if(handle_ && close_)close_(handle_);if(nvReady_)shutdown_();}
    void sample(double &cpu,double &gpu,QString &cpuStatus) {
        if(!initialized_)initialize();cpuStatus=cpuStatus_;
        if(energyRegister_){
            DWORD_PTR process=0,system=0;GetProcessAffinityMask(GetCurrentProcess(),&process,&system);
            const auto affinity=process & (~process+1);const auto old=affinity?SetThreadAffinityMask(GetCurrentThread(),affinity):0;
            uint64_t units=0,energy=0;
            if(old && (joules_>0 || read(energyRegister_==0x611?0x606:0xC0010299,units)) && read(energyRegister_,energy)){
                if(joules_==0)joules_=std::ldexp(1.0,-int((units>>8)&31));
                if(timer_.isValid()){
                    const double seconds=timer_.nsecsElapsed()/1e9;
                    if(seconds>=.2){lastCpu_=uint32_t(uint32_t(energy)-previous_)*joules_/seconds;previous_=uint32_t(energy);timer_.restart();}
                }else{previous_=uint32_t(energy);timer_.start();}
            }
            if(old)SetThreadAffinityMask(GetCurrentThread(),old);
        }
        cpu=lastCpu_;
        if(nvReady_){unsigned count=0;if(count_(&count)==0)for(unsigned i=0;i<count;++i){void *device=nullptr;unsigned mw=0;if(device_(i,&device)==0 && power_(device,&mw)==0)gpu=(gpu<0?0:gpu)+mw/1000.;}}
    }
};
}
