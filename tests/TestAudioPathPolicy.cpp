#include "backend/LocalMediaPath.h"
#include "backend/AudioFormatPolicy.h"
#include <iostream>
#include <fstream>
#include <filesystem>
int main(int argc,char **argv) {
    int failures=0,checks=0;
    auto check=[&](bool ok,const char *name){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<name<<'\n';}};
    for(const auto *p:{LR"(C:\media\movie.mkv)",LR"(Z:\电影 空格\片.mkv)",LR"(\\NAS\share\中文 空格.mkv)",L"//NAS/share/movie.mkv",LR"(\\?\UNC\NAS\share\movie.mkv)"})check(vsr_native::regularMediaPathSyntax(p),"regular syntax");
    for(const auto *p:{LR"(\\.\pipe\evil)",LR"(\\?\GLOBALROOT\Device\Harddisk0)",LR"(\\?\C:\file.mkv)",LR"(\??\C:\file.mkv)",LR"(\\NAS\pipe\evil)",LR"(\\NAS\IPC$\evil)",LR"(\\NAS\share)",LR"(\\NAS\share\)",LR"(C:\media\file.mkv:secret)",LR"(C:\media\NUL.mkv)",LR"(C:\media\COM1)",L"http://host/movie",LR"(C:\media\file. )"})check(!vsr_native::regularMediaPathSyntax(p),"reject unsafe syntax");
    const auto r=vsr_native::exclusiveSampleRates(48000,44100);check(r[0]==48000 && r[1]==44100,"48k before 44.1k mix");
    const auto r2=vsr_native::exclusiveSampleRates(44100,48000);check(r2[0]==44100 && r2[1]==48000,"44.1k before 48k mix");
    const auto r3=vsr_native::exclusiveSampleRates(96000,44100);check(r3[0]==96000,"96k source first");
    wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
    auto root=std::filesystem::path(temp)/(L"vsp-audio-path-"+std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directory(root);auto file=root/L"中文 空格.wav";std::ofstream(file)<<"fixture";
    std::wstring normalized;std::string error;
    check(vsr_native::normalizeRegularMediaPath(file.wstring(),normalized,error),"existing unicode file");
    check(!vsr_native::normalizeRegularMediaPath(root.wstring(),normalized,error),"directory rejected");
    check(!vsr_native::normalizeRegularMediaPath((root/L"missing.wav").wstring(),normalized,error),"missing rejected");
    auto link=root/L"link.wav";
    if(CreateSymbolicLinkW(link.c_str(),file.c_str(),SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))check(!vsr_native::normalizeRegularMediaPath(link.wstring(),normalized,error),"symlink rejected");
    auto longDir=root;for(int i=0;i<7;++i){longDir/=L"long-path-component-abcdefghijklmnopqrstuvwxyz";CreateDirectoryW((L"\\\\?\\"+longDir.wstring()).c_str(),nullptr);}
    auto longFile=longDir/L"media.wav";const auto longName=L"\\\\?\\"+longFile.wstring();
    HANDLE fixture=CreateFileW(longName.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);check(fixture!=INVALID_HANDLE_VALUE,"create long-path fixture");if(fixture!=INVALID_HANDLE_VALUE)CloseHandle(fixture);
    check(vsr_native::normalizeRegularMediaPath(longFile.wstring(),normalized,error),"long local path");
    if(argc>1){const int n=MultiByteToWideChar(CP_UTF8,0,argv[1],-1,nullptr,0);std::wstring unc(n,L'\0');MultiByteToWideChar(CP_UTF8,0,argv[1],-1,unc.data(),n);unc.resize(n-1);check(vsr_native::normalizeRegularMediaPath(unc,normalized,error),"real SMB file");if(!error.empty())std::cerr<<error<<'\n';}
    DeleteFileW(longName.c_str());for(int i=0;i<7;++i){RemoveDirectoryW((L"\\\\?\\"+longDir.wstring()).c_str());longDir=longDir.parent_path();}
    std::filesystem::remove_all(root);
    std::cout<<"checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;
}
