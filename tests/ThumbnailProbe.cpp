#include <windows.h>
#include <initguid.h>
#include <shlobj.h>
#include <thumbcache.h>
#include <propsys.h>
#include <shlwapi.h>
#include <cstdio>
#include <cwchar>

int wmain(int argc,wchar_t **argv) {
    if(argc<2||argc>3)return 2;
    for(auto p=argv[1];*p;++p)if(*p==L'/')*p=L'\\';
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    IShellItem *item=nullptr;IThumbnailCache *cache=nullptr;ISharedBitmap *bitmap=nullptr;
    HRESULT hr=SHCreateItemFromParsingName(argv[1],nullptr,IID_PPV_ARGS(&item));
    if(argc==3 && wcscmp(argv[2],L"legacy")==0){
        PIDLIST_ABSOLUTE pidl=nullptr;PCUITEMID_CHILD child=nullptr;IShellFolder *folder=nullptr;IExtractImage *extract=nullptr;HBITMAP image=nullptr;
        hr=SHParseDisplayName(argv[1],nullptr,&pidl,0,nullptr);
        if(SUCCEEDED(hr))hr=SHBindToParent(pidl,IID_PPV_ARGS(&folder),&child);
        if(SUCCEEDED(hr))hr=folder->GetUIObjectOf(nullptr,1,&child,IID_IExtractImage,nullptr,reinterpret_cast<void**>(&extract));
        wchar_t location[32768];DWORD priority=0,flags=IEIFLAG_QUALITY|IEIFLAG_OFFLINE;SIZE size{256,256};
        std::printf("legacy activation hr=0x%08lx\n",static_cast<unsigned long>(hr));
        if(SUCCEEDED(hr))hr=extract->GetLocation(location,32768,&priority,&size,32,&flags);
        std::printf("legacy location hr=0x%08lx\n",static_cast<unsigned long>(hr));
        if(SUCCEEDED(hr))std::wprintf(L"legacy path=%ls\n",location);
        if(SUCCEEDED(hr)||hr==E_PENDING)hr=extract->Extract(&image);
        BITMAP info{};if(image)GetObject(image,sizeof(info),&info);std::printf("legacy hr=0x%08lx size=%ldx%ld\n",static_cast<unsigned long>(hr),info.bmWidth,info.bmHeight);
        if(image)DeleteObject(image);if(extract)extract->Release();if(folder)folder->Release();CoTaskMemFree(pidl);if(item)item->Release();CoUninitialize();return SUCCEEDED(hr)?0:1;
    }
    if(argc==3 && wcscmp(argv[2],L"factory")==0){
        IShellItemImageFactory *factory=nullptr;HBITMAP image=nullptr;
        if(SUCCEEDED(hr))hr=item->QueryInterface(IID_PPV_ARGS(&factory));
        if(SUCCEEDED(hr))hr=factory->GetImage({256,256},SIIGBF_THUMBNAILONLY,&image);
        BITMAP info{};if(image)GetObject(image,sizeof(info),&info);
        std::printf("factory hr=0x%08lx size=%ldx%ld\n",static_cast<unsigned long>(hr),info.bmWidth,info.bmHeight);
        if(image)DeleteObject(image);if(factory)factory->Release();if(item)item->Release();CoUninitialize();return SUCCEEDED(hr)?0:1;
    }
    if(argc==3){
        const bool registered=wcscmp(argv[2],L"registered")==0;
        auto dll=registered?nullptr:LoadLibraryW(argv[2]);IClassFactory *factory=nullptr;IInitializeWithFile *initialize=nullptr;IThumbnailProvider *provider=nullptr;HBITMAP image=nullptr;WTS_ALPHATYPE alpha{};
        const CLSID clsid={0xd3b7f520,0x726a,0x4af5,{0x93,0x66,0x8c,0xe2,0x68,0x4b,0x61,0x92}};
        auto create=dll?reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(dll,"DllGetClassObject")):nullptr;
        hr=registered?CoCreateInstance(clsid,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&provider)):create?create(clsid,IID_PPV_ARGS(&factory)):HRESULT_FROM_WIN32(GetLastError());
        if(SUCCEEDED(hr)&&factory)hr=factory->CreateInstance(nullptr,IID_PPV_ARGS(&provider));
        std::printf("activation hr=0x%08lx\n",static_cast<unsigned long>(hr));
        if(GetEnvironmentVariableW(L"VSR_THUMB_LEGACY",nullptr,0)){
            IPersistFile *persist=nullptr;IExtractImage *extract=nullptr;
            if(SUCCEEDED(hr))hr=provider->QueryInterface(IID_PPV_ARGS(&persist));
            if(SUCCEEDED(hr))hr=persist->Load(argv[1],STGM_READ);
            if(SUCCEEDED(hr))hr=provider->QueryInterface(IID_PPV_ARGS(&extract));
            wchar_t location[32768];DWORD priority=0,flags=IEIFLAG_QUALITY;SIZE size{256,256};
            if(SUCCEEDED(hr))hr=extract->GetLocation(location,32768,&priority,&size,32,&flags);
            if(SUCCEEDED(hr))hr=extract->Extract(&image);
            if(persist)persist->Release();if(extract)extract->Release();
        }else if(GetEnvironmentVariableW(L"VSR_THUMB_STREAM",nullptr,0)){
            IInitializeWithStream *streamInit=nullptr;IStream *stream=nullptr;
            if(SUCCEEDED(hr))hr=provider->QueryInterface(IID_PPV_ARGS(&streamInit));
            if(SUCCEEDED(hr))hr=SHCreateStreamOnFileEx(argv[1],STGM_READ|STGM_SHARE_DENY_NONE,0,FALSE,nullptr,&stream);
            if(SUCCEEDED(hr)&&GetEnvironmentVariableW(L"VSR_THUMB_MEMORY",nullptr,0)){
                IStream *memory=nullptr;hr=CreateStreamOnHGlobal(nullptr,TRUE,&memory);
                if(SUCCEEDED(hr)){ULARGE_INTEGER bytes;bytes.QuadPart=256ULL*1024*1024;hr=stream->CopyTo(memory,bytes,nullptr,nullptr);stream->Release();stream=memory;LARGE_INTEGER zero{};stream->Seek(zero,STREAM_SEEK_SET,nullptr);}
            }
            if(SUCCEEDED(hr))hr=streamInit->Initialize(stream,STGM_READ);
            if(stream)stream->Release();if(streamInit)streamInit->Release();
        }else{
            if(SUCCEEDED(hr))hr=provider->QueryInterface(IID_PPV_ARGS(&initialize));
            if(SUCCEEDED(hr))hr=initialize->Initialize(argv[1],STGM_READ);
        }
        if(SUCCEEDED(hr)&&!image)hr=provider->GetThumbnail(256,&image,&alpha);
        BITMAP info{};if(image)GetObject(image,sizeof(info),&info);
        std::printf("provider hr=0x%08lx size=%ldx%ld alpha=%u\n",static_cast<unsigned long>(hr),info.bmWidth,info.bmHeight,unsigned(alpha));
        if(image)DeleteObject(image);if(initialize)initialize->Release();if(provider)provider->Release();if(factory)factory->Release();if(dll)FreeLibrary(dll);if(item)item->Release();CoUninitialize();return SUCCEEDED(hr)?0:1;
    }
    if(SUCCEEDED(hr))hr=CoCreateInstance(CLSID_LocalThumbnailCache,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&cache));
    WTS_CACHEFLAGS flags{};WTS_THUMBNAILID id{};
    const auto mode=GetEnvironmentVariableW(L"VSR_THUMB_CACHE_ONLY",nullptr,0)?WTS_INCACHEONLY:GetEnvironmentVariableW(L"VSR_THUMB_SURROGATE",nullptr,0)?WTS_FORCEEXTRACTION|WTS_REQUIRESURROGATE:GetEnvironmentVariableW(L"VSR_THUMB_NORMAL",nullptr,0)?WTS_EXTRACT:GetEnvironmentVariableW(L"VSR_THUMB_INPROC",nullptr,0)?WTS_FORCEEXTRACTION|WTS_EXTRACTINPROC:GetEnvironmentVariableW(L"VSR_THUMB_FRESH",nullptr,0)?WTS_FORCEEXTRACTION|WTS_INSTANCESURROGATE:WTS_FORCEEXTRACTION;
    if(SUCCEEDED(hr))hr=cache->GetThumbnail(item,256,mode,&bitmap,&flags,&id);
    SIZE size{};if(SUCCEEDED(hr))hr=bitmap->GetSize(&size);
    std::printf("thumbnail hr=0x%08lx size=%ldx%ld\n",static_cast<unsigned long>(hr),size.cx,size.cy);
    if(bitmap)bitmap->Release();if(cache)cache->Release();if(item)item->Release();CoUninitialize();
    return SUCCEEDED(hr)&&size.cx>0&&size.cy>0?0:1;
}
