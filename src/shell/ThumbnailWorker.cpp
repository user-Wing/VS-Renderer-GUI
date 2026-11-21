#include "ThumbnailDecode.h"
#include "image/RawExtensions.h"
#include <shlwapi.h>

namespace {
bool isRaw(const wchar_t *extension){
    if(!extension||!*extension)return false;
    char ascii[8]{};
    return WideCharToMultiByte(CP_UTF8,0,extension,-1,ascii,sizeof(ascii),nullptr,nullptr)>0&&
        std::any_of(std::begin(vsr::rawExtensions),std::end(vsr::rawExtensions),
            [&](const char *value){return _stricmp(ascii,value)==0;});
}

HRESULT rawThumbnail(const wchar_t *path,UINT size,HBITMAP *bitmap,WTS_ALPHATYPE *alpha){
    wchar_t runtime[32768];const auto count=GetModuleFileNameW(module,runtime,32768);
    if(!count||count==32768)return E_FAIL;
    const auto folder=std::wstring(runtime).substr(0,std::wstring(runtime).find_last_of(L"\\/")+1);
    const auto library=LoadLibraryExW((folder+L"FFF.Native.dll").c_str(),nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!library)return HRESULT_FROM_WIN32(GetLastError());
    using Decode=int (*)(const char*,void (*)(void*,const uint16_t*,int,int,int),void*);
    const auto decodeNative=reinterpret_cast<Decode>(GetProcAddress(library,"VSR3FP_DecodeRawImage"));
    struct Result{UINT size;HBITMAP image=nullptr;} result{size};
    const int utf8Size=WideCharToMultiByte(CP_UTF8,0,path,-1,nullptr,0,nullptr,nullptr);
    std::string filename(utf8Size,'\0');
    if(utf8Size)WideCharToMultiByte(CP_UTF8,0,path,-1,filename.data(),utf8Size,nullptr,nullptr);
    const int status=decodeNative&&utf8Size?decodeNative(filename.c_str(),
        [](void *opaque,const uint16_t *source,int width,int height,int stride){
            auto &result=*static_cast<Result*>(opaque);
            if(!source||width<=0||height<=0||width>32768||height>32768||stride<width*6)return;
            const double scale=std::min(1.0,double(result.size)/std::max(width,height));
            const int w=std::max(1,int(std::lround(width*scale))),h=std::max(1,int(std::lround(height*scale)));
            void *pixels=nullptr;
            result.image=thumbnailBitmap(w,h,&pixels);
            if(!result.image)return;
            auto *dst=static_cast<uint32_t*>(pixels);
            for(int y=0;y<h;++y){
                const auto *row=reinterpret_cast<const uint16_t*>(reinterpret_cast<const char*>(source)+size_t(y)*height/h*stride);
                for(int x=0;x<w;++x){
                    const auto *rgb=row+3*(size_t(x)*width/w);
                    *dst++=0xff000000u|uint32_t(rgb[0]>>8)<<16|uint32_t(rgb[1]>>8)<<8|(rgb[2]>>8);
                }
            }
        },&result):-1;
    FreeLibrary(library);
    if(status||!result.image){if(result.image)DeleteObject(result.image);return E_FAIL;}
    *bitmap=result.image;*alpha=WTSAT_RGB;
    return S_OK;
}
}

int wmain(int argc,wchar_t **argv){
    if(argc!=3&&argc!=4)return 2;
    const auto size=wcstoul(argv[2],nullptr,10);if(!size||size>1024)return 2;
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);module=GetModuleHandleW(nullptr);
    IStream *stream=nullptr;HBITMAP bitmap=nullptr;WTS_ALPHATYPE alpha{};
    HRESULT result=SHCreateStreamOnFileEx(argv[1],STGM_READ|STGM_SHARE_DENY_NONE,0,FALSE,nullptr,&stream);
    try{
        if(SUCCEEDED(result)){
            const auto *suffix=PathFindExtensionW(argv[1]);
            const bool raw=isRaw(argc==4?argv[3]:suffix&&*suffix?suffix+1:nullptr);
            result=raw?rawThumbnail(argv[1],UINT(size),&bitmap,&alpha):E_FAIL;
            if(FAILED(result))result=decode(stream,UINT(size),&bitmap,&alpha);
        }
    }catch(...){result=E_FAIL;}
    if(SUCCEEDED(result)){
        BITMAP info{};GetObject(bitmap,sizeof(info),&info);
        const DWORD header[]{0x56535448,DWORD(info.bmWidth),DWORD(info.bmHeight),DWORD(alpha)};
        const auto out=GetStdHandle(STD_OUTPUT_HANDLE);DWORD written=0;
        const DWORD bytes=DWORD(info.bmWidth*info.bmHeight*4);
        if(!WriteFile(out,header,sizeof(header),&written,nullptr)||written!=sizeof(header)||!WriteFile(out,info.bmBits,bytes,&written,nullptr)||written!=bytes)result=E_FAIL;
    }
    if(bitmap)DeleteObject(bitmap);if(stream)stream->Release();CoUninitialize();return SUCCEEDED(result)?0:1;
}
