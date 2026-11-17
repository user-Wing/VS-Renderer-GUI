#include "ThumbnailDecode.h"
#include <shlwapi.h>

int wmain(int argc,wchar_t **argv){
    if(argc!=3)return 2;
    const auto size=wcstoul(argv[2],nullptr,10);if(!size||size>1024)return 2;
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);module=GetModuleHandleW(nullptr);
    IStream *stream=nullptr;HBITMAP bitmap=nullptr;WTS_ALPHATYPE alpha{};
    HRESULT result=SHCreateStreamOnFileEx(argv[1],STGM_READ|STGM_SHARE_DENY_NONE,0,FALSE,nullptr,&stream);
    try{if(SUCCEEDED(result))result=decode(stream,UINT(size),&bitmap,&alpha);}catch(...){result=E_FAIL;}
    if(SUCCEEDED(result)){
        BITMAP info{};GetObject(bitmap,sizeof(info),&info);
        const DWORD header[]{0x56535448,DWORD(info.bmWidth),DWORD(info.bmHeight),DWORD(alpha)};
        const auto out=GetStdHandle(STD_OUTPUT_HANDLE);DWORD written=0;
        const DWORD bytes=DWORD(info.bmWidth*info.bmHeight*4);
        if(!WriteFile(out,header,sizeof(header),&written,nullptr)||written!=sizeof(header)||!WriteFile(out,info.bmBits,bytes,&written,nullptr)||written!=bytes)result=E_FAIL;
    }
    if(bitmap)DeleteObject(bitmap);if(stream)stream->Release();CoUninitialize();return SUCCEEDED(result)?0:1;
}
