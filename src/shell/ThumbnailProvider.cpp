// Lightweight file/stream shell adapter; all media parsing runs in our worker.
#include <windows.h>
#include <initguid.h>
#include <thumbcache.h>
#include <propsys.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <algorithm>
#include <atomic>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>

namespace {
const CLSID providerId={0xd3b7f520,0x726a,0x4af5,{0x93,0x66,0x8c,0xe2,0x68,0x4b,0x61,0x92}};
HMODULE module;
std::atomic<long> objects{0};
#include "ThumbnailProcess.h"
class Thumbnail final:public IThumbnailProvider,public IInitializeWithFile,public IInitializeWithStream,public IExtractImage2,public IPersistFile {
    std::atomic<ULONG> refs{1};std::wstring path;IStream *stream=nullptr;UINT requestedSize=0;
public:
    Thumbnail(){++objects;}~Thumbnail(){if(stream)stream->Release();--objects;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void **out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(id==IID_IUnknown||id==IID_IThumbnailProvider)*out=static_cast<IThumbnailProvider*>(this);
        else if(id==IID_IInitializeWithFile)*out=static_cast<IInitializeWithFile*>(this);
        else if(id==IID_IInitializeWithStream)*out=static_cast<IInitializeWithStream*>(this);
        else if(id==IID_IExtractImage)*out=static_cast<IExtractImage*>(this);
        else if(id==IID_IExtractImage2)*out=static_cast<IExtractImage2*>(this);
        else if(id==IID_IPersistFile||id==IID_IPersist)*out=static_cast<IPersistFile*>(this);
        else return E_NOINTERFACE;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override{const auto n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR value,DWORD mode) override{
        if(!path.empty()||stream)return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);if(!value||!*value)return E_INVALIDARG;
        if(mode&(STGM_WRITE|STGM_READWRITE))return STG_E_ACCESSDENIED;path=value;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Initialize(IStream *value,DWORD mode) override{
        if(!path.empty()||stream)return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);if(!value)return E_INVALIDARG;
        if(mode&(STGM_WRITE|STGM_READWRITE))return STG_E_ACCESSDENIED;stream=value;stream->AddRef();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID *id) override{if(!id)return E_POINTER;*id=providerId;return S_OK;}
    HRESULT STDMETHODCALLTYPE IsDirty() override{return S_FALSE;}
    HRESULT STDMETHODCALLTYPE Load(LPCOLESTR value,DWORD mode) override{return Initialize(value,mode);}
    HRESULT STDMETHODCALLTYPE Save(LPCOLESTR,BOOL) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *value) override{
        if(!value)return E_POINTER;*value=nullptr;if(path.empty())return E_UNEXPECTED;
        const auto bytes=(path.size()+1)*sizeof(wchar_t);*value=static_cast<LPOLESTR>(CoTaskMemAlloc(bytes));if(!*value)return E_OUTOFMEMORY;memcpy(*value,path.c_str(),bytes);return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetLocation(LPWSTR location,DWORD count,DWORD *priority,const SIZE *size,DWORD,DWORD *flags) override{
        if(!location||!size||!flags)return E_POINTER;if((path.empty()&&!stream)||size->cx<=0||size->cy<=0)return E_INVALIDARG;
        auto locationPath=path;
        if(locationPath.empty()&&stream){STATSTG stat{};if(SUCCEEDED(stream->Stat(&stat,STATFLAG_DEFAULT))&&stat.pwcsName){locationPath=stat.pwcsName;CoTaskMemFree(stat.pwcsName);}}
        if(locationPath.size()+1>count)return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        memcpy(location,locationPath.c_str(),(locationPath.size()+1)*sizeof(wchar_t));requestedSize=std::min<LONG>(1024,std::max(size->cx,size->cy));
        if(priority)*priority=0;if(!locationPath.empty()&&!PathIsRelativeW(locationPath.c_str()))*flags|=IEIFLAG_CACHE;else *flags&=~IEIFLAG_CACHE;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Extract(HBITMAP *bitmap) override{WTS_ALPHATYPE alpha{};return requestedSize?GetThumbnail(requestedSize,bitmap,&alpha):E_UNEXPECTED;}
    HRESULT STDMETHODCALLTYPE GetDateStamp(FILETIME *stamp) override{
        if(stamp&&path.empty()&&stream){STATSTG stat{};const auto result=stream->Stat(&stat,STATFLAG_NONAME);if(SUCCEEDED(result))*stamp=stat.mtime;return result;}
        if(!stamp)return E_POINTER;WIN32_FILE_ATTRIBUTE_DATA data{};if(path.empty()||!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&data))return E_FAIL;*stamp=data.ftLastWriteTime;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetThumbnail(UINT size,HBITMAP *bitmap,WTS_ALPHATYPE *alpha) override{
        if(!bitmap||!alpha)return E_POINTER;*bitmap=nullptr;*alpha=WTSAT_UNKNOWN;
        if((path.empty()&&!stream)||!size)return E_INVALIDARG;
        wchar_t temporary[MAX_PATH]{};HRESULT result=E_FAIL;
        try{
            auto inputPath=path;
            if(inputPath.empty()){
                // Shell file streams expose their path; avoid copying large media.
                STATSTG stat{};
                if(SUCCEEDED(stream->Stat(&stat,STATFLAG_DEFAULT))){
                    if(stat.pwcsName){const auto attributes=PathIsRelativeW(stat.pwcsName)?INVALID_FILE_ATTRIBUTES:GetFileAttributesW(stat.pwcsName);if(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY))inputPath=stat.pwcsName;CoTaskMemFree(stat.pwcsName);}
                }
            }
            if(inputPath.empty()){
                // Archive/virtual streams need a bounded file for the isolated worker.
                wchar_t folder[MAX_PATH];if(!GetTempPathW(MAX_PATH,folder)||!GetTempFileNameW(folder,L"vst",0,temporary))return E_FAIL;
                HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_TEMPORARY,nullptr);
                LARGE_INTEGER zero{};bool copied=file!=INVALID_HANDLE_VALUE&&SUCCEEDED(stream->Seek(zero,STREAM_SEEK_SET,nullptr));
                const auto deadline=GetTickCount64()+15000;uint64_t total=0;unsigned char buffer[65536];
                while(copied){ULONG count=0;const auto read=stream->Read(buffer,sizeof(buffer),&count);total+=count;
                    if(FAILED(read)||total>256ULL*1024*1024||GetTickCount64()>deadline){copied=false;break;}
                    if(!count)break;DWORD written=0;if(!WriteFile(file,buffer,count,&written,nullptr)||written!=count){copied=false;break;}
                }
                if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
                if(!copied){DeleteFileW(temporary);return E_FAIL;}inputPath=temporary;
            }
            result=workerThumbnail(inputPath,std::min<UINT>(size,1024),bitmap,alpha);
        }catch(...){result=E_FAIL;}
        if(*temporary)DeleteFileW(temporary);return result;
    }
};
class Factory final:public IClassFactory {
    std::atomic<ULONG> refs{1};
public:
    Factory(){++objects;}~Factory(){--objects;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void **out) override{if(!out)return E_POINTER;*out=nullptr;if(id!=IID_IUnknown&&id!=IID_IClassFactory)return E_NOINTERFACE;*out=static_cast<IClassFactory*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override{const auto n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *outer,REFIID id,void **out) override{if(outer)return CLASS_E_NOAGGREGATION;auto *p=new(std::nothrow) Thumbnail;if(!p)return E_OUTOFMEMORY;const auto result=p->QueryInterface(id,out);p->Release();return result;}
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override{objects+=lock?1:-1;return S_OK;}
};
}
extern "C" BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,void*){if(reason==DLL_PROCESS_ATTACH){module=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
extern "C" __declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(){return objects==0?S_OK:S_FALSE;}
extern "C" __declspec(dllexport) HRESULT WINAPI DllGetClassObject(REFCLSID id,REFIID iid,void **out){if(id!=providerId)return CLASS_E_CLASSNOTAVAILABLE;auto *p=new(std::nothrow) Factory;if(!p)return E_OUTOFMEMORY;const auto result=p->QueryInterface(iid,out);p->Release();return result;}
