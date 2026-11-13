#pragma once
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

// Only explicitly opened filesystem media is accepted. No URL, NT namespace,
// named pipe, DOS device, alternate stream, directory or reparse target.
namespace vsr_native {
inline bool regularMediaPathSyntax(std::wstring path) {
    std::replace(path.begin(),path.end(),L'/',L'\\');
    if(path.empty() || path.find(L'\0')!=std::wstring::npos)return false;
    std::wstring upper=path;
    std::transform(upper.begin(),upper.end(),upper.begin(),[](wchar_t c){return std::towupper(c);});
    if(upper.rfind(L"\\\\?\\UNC\\",0)==0)path=L"\\\\"+path.substr(8);
    else if(upper.rfind(L"\\\\?\\",0)==0 || upper.rfind(L"\\\\.\\",0)==0 || upper.rfind(L"\\??\\",0)==0)return false;
    const bool unc=path.rfind(L"\\\\",0)==0;
    if(unc) {
        const auto serverEnd=path.find(L'\\',2);
        if(serverEnd==std::wstring::npos || serverEnd==2)return false;
        const auto shareEnd=path.find(L'\\',serverEnd+1);
        if(shareEnd==std::wstring::npos || shareEnd==serverEnd+1 || shareEnd+1>=path.size())return false;
    }
    for(size_t start=0;start<path.size();) {
        const auto end=path.find(L'\\',start);
        auto part=path.substr(start,end==std::wstring::npos?path.size()-start:end-start);
        if(!part.empty()) {
            const bool drive=!unc && start==0 && part.size()==2 && std::iswalpha(part[0]) && part[1]==L':';
            if(!drive && part.find_first_of(L":*?\"<>|")!=std::wstring::npos)return false;
            if(part!=L"." && part!=L".." && (part.back()==L' ' || part.back()==L'.'))return false;
            auto base=part.substr(0,part.find(L'.'));
            std::transform(base.begin(),base.end(),base.begin(),[](wchar_t c){return std::towupper(c);});
            if(base==L"CON" || base==L"PRN" || base==L"AUX" || base==L"NUL" || base==L"CONIN$" || base==L"CONOUT$" || base==L"PIPE" || base==L"IPC$")return false;
            if(base.size()==4 && (base.substr(0,3)==L"COM" || base.substr(0,3)==L"LPT") && ((base[3]>=L'1' && base[3]<=L'9') || base[3]==L'¹' || base[3]==L'²' || base[3]==L'³'))return false;
        }
        if(end==std::wstring::npos)break;
        start=end+1;
    }
    return true;
}
inline std::wstring stripLongPrefix(std::wstring path) {
    if(path.size()>=8 && _wcsnicmp(path.c_str(),L"\\\\?\\UNC\\",8)==0)return L"\\\\"+path.substr(8);
    if(path.rfind(L"\\\\?\\",0)==0)return path.substr(4);
    return path;
}
inline bool normalizeRegularMediaPath(std::wstring path,std::wstring &normalized,std::string &error) {
    normalized.clear();error.clear();
    if(!regularMediaPathSyntax(path)){error="URLs, device namespaces, pipes and alternate streams are disabled.";return false;}
    std::replace(path.begin(),path.end(),L'/',L'\\');
    path=stripLongPrefix(path);
    std::vector<wchar_t> full(32768);
    const auto length=GetFullPathNameW(path.c_str(),static_cast<DWORD>(full.size()),full.data(),nullptr);
    if(!length || length>=full.size()){error="The media path is invalid or too long.";return false;}
    path.assign(full.data(),length);
    if(!regularMediaPathSyntax(path)){error="The normalized media path is not allowed.";return false;}
    const auto native=path.rfind(L"\\\\",0)==0?L"\\\\?\\UNC\\"+path.substr(2):L"\\\\?\\"+path;
    const auto attributes=GetFileAttributesW(native.c_str());
    if(attributes==INVALID_FILE_ATTRIBUTES || (attributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))) {
        error="Select an existing regular media file; check SMB access and permissions. Windows error "+std::to_string(GetLastError());return false;
    }
    HANDLE file=CreateFileW(native.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE){error="Cannot inspect the media file. Windows error "+std::to_string(GetLastError());return false;}
    BY_HANDLE_FILE_INFORMATION info{};
    const bool regular=GetFileType(file)==FILE_TYPE_DISK && GetFileInformationByHandle(file,&info) && !(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));
    // Validate every ancestor, not just the final component. SMB DFS/reparse
    // shares deliberately remain unsupported rather than widening access.
    bool parents=true;
    size_t root=3;
    if(path.rfind(L"\\\\",0)==0){const auto server=path.find(L'\\',2);root=path.find(L'\\',server+1)+1;}
    for(size_t end=path.find(L'\\',root);end!=std::wstring::npos;end=path.find(L'\\',end+1)) {
        const auto parent=path.substr(0,end);
        const auto longParent=parent.rfind(L"\\\\",0)==0?L"\\\\?\\UNC\\"+parent.substr(2):L"\\\\?\\"+parent;
        const auto a=GetFileAttributesW(longParent.c_str());
        if(a==INVALID_FILE_ATTRIBUTES || (a&FILE_ATTRIBUTE_REPARSE_POINT)){parents=false;break;}
    }
    CloseHandle(file);
    if(!regular || !parents){error="Device and reparse paths are disabled.";return false;}
    normalized=path.size()>=248?native:path;
    return true;
}
}
