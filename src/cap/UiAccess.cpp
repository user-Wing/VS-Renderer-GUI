#include "UiAccess.h"
#include <QCoreApplication>
#include <QDir>
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <QProcess>
#include <QCryptographicHash>

namespace cap {
int runStartupTaskCommand(bool enabled,const QString &user){
    const auto id=QString::fromLatin1(QCryptographicHash::hash(user.toUtf8(),QCryptographicHash::Sha256).toHex().left(12));
    const auto name="VSRenderer-VSCap-"+id;QProcess task;
    QStringList args=enabled?QStringList{"/Create","/TN",name,"/SC","ONLOGON","/RL","HIGHEST","/IT","/RU",user,"/TR","\""+QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+"\"","/F"}:QStringList{"/Delete","/TN",name,"/F"};
    task.start("schtasks.exe",args);if(!task.waitForFinished(30000))return 1;return task.exitCode();
}
bool configureElevatedStartup(bool enabled,QString *error){
    const auto executable=QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
    const auto user=qEnvironmentVariable("USERDOMAIN")+"\\"+qEnvironmentVariable("USERNAME");
    const auto arguments=QString("--startup-task %1 \"%2\"").arg(enabled?"enable":"disable",user).toStdWString();
    SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.lpVerb=L"runas";launch.lpFile=executable.c_str();launch.lpParameters=arguments.c_str();launch.nShow=SW_HIDE;
    if(!ShellExecuteExW(&launch)){*error=QString::number(GetLastError());return false;}
    const auto waited=WaitForSingleObject(launch.hProcess,45000);DWORD result=1;if(waited==WAIT_OBJECT_0)GetExitCodeProcess(launch.hProcess,&result);CloseHandle(launch.hProcess);
    if(result)*error=QStringLiteral("计划任务配置失败（%1）").arg(result);return result==0;
}
bool hasUiAccess() {
    HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
    DWORD value=0,size=0;const bool granted=GetTokenInformation(token,TokenUIAccess,&value,sizeof(value),&size) && value;
    CloseHandle(token);return granted;
}

bool restartWithUiAccess(QString *error) {
    HANDLE own=nullptr;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_DUPLICATE,&own)){
        *error=QString::number(GetLastError());return false;
    }
    TOKEN_ELEVATION elevation{};DWORD size=0;
    const bool elevated=GetTokenInformation(own,TokenElevation,&elevation,sizeof(elevation),&size) && elevation.TokenIsElevated;
    const auto executable=QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
    const auto directory=QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).toStdWString();
    const auto arguments=(QString("--uiaccess-launch --wait-parent %1").arg(GetCurrentProcessId())).toStdWString();
    if(!elevated){
        CloseHandle(own);
        SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.lpVerb=L"runas";launch.lpFile=executable.c_str();
        launch.lpParameters=arguments.c_str();launch.lpDirectory=directory.c_str();launch.nShow=SW_HIDE;
        if(ShellExecuteExW(&launch))return true;
        *error=QString::number(GetLastError());return false;
    }

    // UAC approval is required before this path. A same-session system token
    // supplies SeTcbPrivilege only while creating our own user's UIAccess token.
    // No system process is modified and no driver or certificate is installed.
    DWORD session=0;ProcessIdToSessionId(GetCurrentProcessId(),&session);
    HANDLE system=nullptr;
    const auto processes=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(processes!=INVALID_HANDLE_VALUE){
        PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
        if(Process32FirstW(processes,&entry))do{
            DWORD candidate=0;
            if(_wcsicmp(entry.szExeFile,L"winlogon.exe") || !ProcessIdToSessionId(entry.th32ProcessID,&candidate) || candidate!=session)continue;
            const auto process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!process)continue;
            HANDLE token=nullptr;
            if(OpenProcessToken(process,TOKEN_QUERY|TOKEN_DUPLICATE,&token)){
                PRIVILEGE_SET privilege{};privilege.PrivilegeCount=1;privilege.Control=PRIVILEGE_SET_ALL_NECESSARY;
                LookupPrivilegeValueW(nullptr,SE_TCB_NAME,&privilege.Privilege[0].Luid);privilege.Privilege[0].Attributes=SE_PRIVILEGE_ENABLED;
                BOOL allowed=FALSE;
                if(PrivilegeCheck(token,&privilege,&allowed) && allowed)
                    DuplicateTokenEx(token,TOKEN_QUERY|TOKEN_IMPERSONATE,nullptr,SecurityImpersonation,TokenImpersonation,&system);
                CloseHandle(token);
            }
            CloseHandle(process);if(system)break;
        }while(Process32NextW(processes,&entry));
        CloseHandle(processes);
    }
    if(!system){CloseHandle(own);*error=QStringLiteral("无法取得同一桌面会话的 UIAccess 启动权限");return false;}
    HANDLE primary=nullptr;BOOL started=FALSE;DWORD code=ERROR_ACCESS_DENIED;
    if(SetThreadToken(nullptr,system)){
        if(DuplicateTokenEx(own,TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_DEFAULT,nullptr,SecurityImpersonation,TokenPrimary,&primary)){
            DWORD enabled=1;
            if(SetTokenInformation(primary,TokenUIAccess,&enabled,sizeof(enabled))){
                auto command=L"\""+executable+L"\" "+arguments;
                STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.lpDesktop=const_cast<wchar_t*>(L"winsta0\\default");
                PROCESS_INFORMATION child{};
                started=CreateProcessAsUserW(primary,executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&startup,&child);
                code=GetLastError();
                if(started){CloseHandle(child.hThread);CloseHandle(child.hProcess);}
            }else code=GetLastError();
            CloseHandle(primary);
        }else code=GetLastError();
        RevertToSelf();
    }else code=GetLastError();
    CloseHandle(system);CloseHandle(own);
    if(!started)*error=QStringLiteral("UIAccess 启动失败（Windows %1）").arg(code);
    return started;
}
}
