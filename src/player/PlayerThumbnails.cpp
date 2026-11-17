#include "player/PlayerThumbnails.h"
#include "player/PlayerAssociations.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <windows.h>
#include <shlobj.h>
#include <QSet>

namespace vsr {
namespace {
const QString clsid="{D3B7F520-726A-4AF5-9366-8CE2684B6192}";
const QStringList handlers{"/shellex/{E357FCCD-A995-4576-B01F-234630154E96}/.","/shellex/{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}/."};
void notify(const QString &root){if(root=="HKEY_CURRENT_USER\\Software")SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST|SHCNF_FLUSH,nullptr,nullptr);}
// Read the import table without loading DLLs into the running player's process.
QStringList imports(const QString &path){
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};
    const auto read=[&](qint64 offset,void *data,qint64 bytes){return offset>=0&&file.seek(offset)&&file.read(static_cast<char*>(data),bytes)==bytes;};
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    if(!read(0,&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||!read(dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt.FileHeader.NumberOfSections>96)return {};
    QList<IMAGE_SECTION_HEADER> sections;
    const auto sectionOffset=qint64(dos.e_lfanew)+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
    for(int n=0;n<nt.FileHeader.NumberOfSections;++n){IMAGE_SECTION_HEADER section{};if(!read(sectionOffset+n*sizeof(section),&section,sizeof(section)))return {};sections<<section;}
    const auto offset=[&](DWORD rva)->qint64{for(const auto &s:sections)if(rva>=s.VirtualAddress&&quint64(rva)-s.VirtualAddress<s.SizeOfRawData)return qint64(s.PointerToRawData)+rva-s.VirtualAddress;return -1;};
    QStringList result;const auto start=offset(nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);if(start<0)return result;
    for(int n=0;n<512;++n){IMAGE_IMPORT_DESCRIPTOR entry{};if(!read(start+n*sizeof(entry),&entry,sizeof(entry))||!entry.Name)break;
        const auto at=offset(entry.Name);if(at<0||!file.seek(at))break;auto name=file.read(256);const auto end=name.indexOf('\0');if(end<1)break;name.truncate(end);
        const auto text=QString::fromLatin1(name).toLower();if(text.contains('/')||text.contains('\\'))break;result<<text;
    }
    return result;
}
}
QStringList thumbnailExtensions(){return playerVideoExtensions()+QStringList{"jxl","avif","webp","png","jpg","jpeg","bmp","gif","tif","tiff","heic","heif","qoi","jp2","j2k","exr"};}
bool registerThumbnailProvider(const QStringList &extensions,const QString &library,const QString &root){
    if(extensions.isEmpty()||!QFileInfo(library).isFile())return false;
    for(const auto &extension:extensions)if(!thumbnailExtensions().contains(extension))return false;
    QSettings classes(root+"\\Classes",QSettings::NativeFormat),backup(root+"\\VSPlayer\\ThumbnailBackup",QSettings::NativeFormat);
    classes.setValue("CLSID/"+clsid+"/.","VS Player thumbnails");
    classes.setValue("CLSID/"+clsid+"/InprocServer32/.",QDir::toNativeSeparators(QFileInfo(library).absoluteFilePath()));
    classes.setValue("CLSID/"+clsid+"/InprocServer32/ThreadingModel","Apartment");
    // Per-user COM activation uses the small adapter; decoding stays in the worker.
    classes.setValue("CLSID/"+clsid+"/DisableProcessIsolation",1);
    classes.remove("CLSID/"+clsid+"/AppID");
    classes.remove("AppID/"+clsid);
    for(const auto &extension:extensions){
        for(const auto &prefix:QStringList{"."+extension,"SystemFileAssociations/."+extension}){
            for(int n=0;n<handlers.size();++n){
                const auto key=prefix+handlers[n],saved=(n?"legacy/":QString())+prefix;
                if(!backup.contains(saved+"/saved")){
                    backup.setValue(saved+"/saved",true);backup.setValue(saved+"/existed",classes.contains(key));backup.setValue(saved+"/value",classes.value(key));
                }
                classes.setValue(key,clsid);
            }
        }
    }
    classes.sync();backup.sync();notify(root);return classes.status()==QSettings::NoError&&backup.status()==QSettings::NoError;
}
bool unregisterThumbnailProvider(const QString &root){
    QSettings classes(root+"\\Classes",QSettings::NativeFormat),backup(root+"\\VSPlayer\\ThumbnailBackup",QSettings::NativeFormat);
    for(const auto &extension:thumbnailExtensions())for(const auto &prefix:QStringList{"."+extension,"SystemFileAssociations/."+extension}){
        for(int n=0;n<handlers.size();++n){
            const auto key=prefix+handlers[n],saved=(n?"legacy/":QString())+prefix;
            if(classes.value(key).toString().compare(clsid,Qt::CaseInsensitive)==0){
                if(backup.value(saved+"/existed").toBool())classes.setValue(key,backup.value(saved+"/value"));else classes.remove(prefix+handlers[n].left(handlers[n].size()-2));
            }
        }
    }
    classes.remove("CLSID/"+clsid);classes.remove("AppID/"+clsid);backup.clear();classes.sync();backup.sync();notify(root);return classes.status()==QSettings::NoError&&backup.status()==QSettings::NoError;
}
bool installPlayerThumbnails(const QStringList &extensions,QString *error){
    const auto fail=[&](const QString &text){if(error)*error=text;return false;};
    if(extensions.isEmpty())return fail(QCoreApplication::translate("PlayerWindow","请先勾选需要缩略图的视频或图片格式。"));
    QDir source(QCoreApplication::applicationDirPath());
    QStringList names{"vsr-thumbnail.dll","vsr-thumbnail-worker.exe","avutil-61.dll","swresample-7.dll","avcodec-63.dll","avformat-63.dll","swscale-10.dll"};
    const auto locate=[&](const QString &name,const QString &parent=QString()){
        for(const auto &folder:QStringList{parent,source.absolutePath(),QFileInfo(playerAssociationExecutable()).absolutePath()})if(!folder.isEmpty()){const auto path=QDir(folder).filePath(name);if(QFileInfo(path).isFile())return path;}
        return QStandardPaths::findExecutable(name);
    };
    QStringList paths;QCryptographicHash hash(QCryptographicHash::Sha256);
    QSet<QString> discovered(names.cbegin(),names.cend());QHash<QString,QString> resolved;
    wchar_t systemPath[MAX_PATH];GetSystemDirectoryW(systemPath,MAX_PATH);const auto system=QDir(QString::fromWCharArray(systemPath));
    for(int index=0;index<names.size();++index){
        const auto name=names[index];const auto path=resolved.contains(name)?resolved[name]:locate(name);
        QFile file(path);if(!file.open(QIODevice::ReadOnly))return fail(QCoreApplication::translate("PlayerWindow","缺少缩略图组件：%1").arg(name));
        hash.addData(name.toUtf8());if(!hash.addData(&file))return fail(file.errorString());paths<<path;
        for(const auto &dependency:imports(path)){
            if(discovered.contains(dependency)||dependency.startsWith("api-ms-")||dependency.startsWith("ext-ms-")||system.exists(dependency))continue;
            discovered.insert(dependency);names<<dependency;resolved[dependency]=locate(dependency,QFileInfo(path).absolutePath());
        }
    }
    const auto directory=QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("VSPlayer/thumbnails/"+QString::fromLatin1(hash.result().toHex().left(20)));
    if(!QDir().mkpath(directory))return fail(QCoreApplication::translate("PlayerWindow","无法创建缩略图组件目录。"));
    for(int i=0;i<names.size();++i){const auto target=QDir(directory).filePath(names[i]);if(!QFileInfo(target).isFile()&&!QFile::copy(paths[i],target))return fail(QCoreApplication::translate("PlayerWindow","无法安装缩略图组件：%1").arg(names[i]));}
    if(!registerThumbnailProvider(extensions,QDir(directory).filePath(names[0])))return fail(QCoreApplication::translate("PlayerWindow","缩略图处理器注册失败。"));
    return true;
}
}
