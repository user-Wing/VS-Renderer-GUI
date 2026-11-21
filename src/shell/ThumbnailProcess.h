#pragma once

// The Explorer-side DLL only launches the bounded worker and receives BGRA pixels.
HRESULT workerThumbnail(const std::wstring &path,const std::wstring &extension,UINT size,HBITMAP *bitmap,WTS_ALPHATYPE *alpha){
    wchar_t dllPath[32768];const auto count=GetModuleFileNameW(module,dllPath,32768);if(!count||count==32768)return E_FAIL;
    const auto folder=std::wstring(dllPath).substr(0,std::wstring(dllPath).find_last_of(L"\\/")+1);
    const auto worker=folder+L"vsr-thumbnail-worker.exe";
    if(path.find(L'"')!=std::wstring::npos)return E_INVALIDARG;
    auto command=L"\""+worker+L"\" \""+path+L"\" "+std::to_wstring(size);
    if(!extension.empty())command+=L" "+extension;
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};HANDLE read=nullptr,write=nullptr;
    if(!CreatePipe(&read,&write,&security,0))return HRESULT_FROM_WIN32(GetLastError());
    SetHandleInformation(read,HANDLE_FLAG_INHERIT,0);
    HANDLE null=CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);
    HANDLE job=CreateJobObjectW(nullptr,nullptr);PROCESS_INFORMATION process{};
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdOutput=write;startup.StartupInfo.hStdError=null;startup.StartupInfo.hStdInput=null;
    SIZE_T length=0;InitializeProcThreadAttributeList(nullptr,1,0,&length);std::vector<unsigned char> attributes(length);
    startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    bool initialized=InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&length)!=FALSE;
    HANDLE handles[]{write,null};JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    HRESULT result=E_FAIL;
    do {
        if(!initialized||!job||null==INVALID_HANDLE_VALUE||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))break;
        if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr))break;
        if(!CreateProcessW(worker.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED|EXTENDED_STARTUPINFO_PRESENT,nullptr,folder.c_str(),&startup.StartupInfo,&process))break;
        if(!AssignProcessToJobObject(job,process.hProcess)){TerminateProcess(process.hProcess,1);break;}
        ResumeThread(process.hThread);CloseHandle(write);write=nullptr;
        std::vector<unsigned char> data;const auto deadline=GetTickCount64()+20000;
        bool done=false;
        while(GetTickCount64()<deadline){
            DWORD available=0;if(!PeekNamedPipe(read,nullptr,0,nullptr,&available,nullptr)){done=GetLastError()==ERROR_BROKEN_PIPE;break;}
            if(available){
                if(data.size()+available>16+1024*1024*4)break;
                const auto start=data.size();data.resize(start+available);DWORD received=0;
                if(!ReadFile(read,data.data()+start,available,&received,nullptr))break;data.resize(start+received);
            }else if(WaitForSingleObject(process.hProcess,10)==WAIT_OBJECT_0){done=true;break;}
        }
        if(!done||data.size()<16)break;
        DWORD header[4];memcpy(header,data.data(),sizeof(header));
        if(header[0]!=0x56535448||!header[1]||!header[2]||header[1]>size||header[2]>size||data.size()!=16+size_t(header[1])*header[2]*4)break;
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=LONG(header[1]);info.bmiHeader.biHeight=-LONG(header[2]);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        void *pixels=nullptr;auto image=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!image)break;
        memcpy(pixels,data.data()+16,data.size()-16);*bitmap=image;
        *alpha=header[3]==WTSAT_RGB?WTSAT_RGB:WTSAT_ARGB;result=S_OK;
    }while(false);
    if(initialized)DeleteProcThreadAttributeList(startup.lpAttributeList);
    if(job)CloseHandle(job);if(process.hThread)CloseHandle(process.hThread);if(process.hProcess){WaitForSingleObject(process.hProcess,1000);CloseHandle(process.hProcess);}
    if(null!=INVALID_HANDLE_VALUE)CloseHandle(null);if(write)CloseHandle(write);CloseHandle(read);return result;
}
