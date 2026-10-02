#include "player/PlayerAssociations.h"
#include <QDir>
#include <QSettings>
#include <windows.h>
#include <shlobj.h>
namespace vsr {
QStringList playerVideoExtensions(){return {"mkv","mp4","mov","avi","webm","ts","m2ts","wmv","flv","mpg","mpeg","m4v","vob","ogv"};}
QStringList playerAudioExtensions(){return {"mp3","flac","wav","m4a","ogg","opus","aac","wma","aiff","ape","ac3","dts","eac3","mka","aif","alac","weba"};}
QStringList playerImageExtensions(){return {"jpg","jpeg","jpe","jfif","png","webp","avif","bmp","gif","tif","tiff","heic","heif","jxl"};}
bool registerPlayerAssociations(const QStringList &extensions,const QString &executable,const QString &root) {
    const auto all=playerVideoExtensions()+playerAudioExtensions()+playerImageExtensions();for(const auto &extension:extensions)if(!all.contains(extension))return false;
    QSettings classes(root+"\\Classes",QSettings::NativeFormat),capabilities(root+"\\VSPlayer\\Capabilities",QSettings::NativeFormat),registered(root+"\\RegisteredApplications",QSettings::NativeFormat);
    const auto command='"'+QDir::toNativeSeparators(executable)+"\" \"%1\"";
    const auto iconPath=QDir::toNativeSeparators(executable);
    classes.setValue("VSPlayer.Media/.","VS Player media");classes.setValue("VSPlayer.Media/shell/open/command/.",command);
    classes.setValue("VSPlayer.Media/DefaultIcon/.",iconPath+",-102");
    classes.setValue("VSPlayer.Media/TypeOverlay",iconPath+",-102");
    for(const auto &type:QStringList{"Video","Image","Audio"}) {
        const auto progid="VSPlayer."+type;
        const int resource=type=="Video"?102:type=="Image"?103:101;
        classes.setValue(progid+"/.","VS Player "+type.toLower());
        classes.setValue(progid+"/shell/open/command/.",command);
        classes.setValue(progid+"/DefaultIcon/.",iconPath+QString(",-%1").arg(resource));
        if(type!="Audio")classes.setValue(progid+"/TypeOverlay",iconPath+QString(",-%1").arg(resource));
    }
    classes.setValue("Applications/vs-player.exe/DefaultIcon/.",iconPath+",-101");
    classes.setValue("Applications/vs-player.exe/shell/open/command/.",command);
    capabilities.setValue("ApplicationName","VS Player");capabilities.setValue("ApplicationDescription","VS Player video, audio and image playback");
    capabilities.setValue("ApplicationIcon",iconPath+",-101");
    capabilities.remove("FileAssociations");classes.remove("Applications/vs-player.exe/SupportedTypes");
    for(const auto &extension:all){
        const auto progid=playerImageExtensions().contains(extension)?"VSPlayer.Image":playerVideoExtensions().contains(extension)?"VSPlayer.Video":"VSPlayer.Audio";
        const auto key='.'+extension+"/OpenWithProgids/";
        for(const auto &old:QStringList{"VSPlayer.Media","VSPlayer.Video","VSPlayer.Image","VSPlayer.Audio"})classes.remove(key+old);
        if(extensions.contains(extension)){classes.setValue(key+progid,QString());capabilities.setValue("FileAssociations/."+extension,progid);classes.setValue("Applications/vs-player.exe/SupportedTypes/."+extension,QString());}
    }
    registered.setValue("VS Player","Software\\VSPlayer\\Capabilities");classes.sync();capabilities.sync();registered.sync();
    if(root=="HKEY_CURRENT_USER\\Software")SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);
    return classes.status()==QSettings::NoError && capabilities.status()==QSettings::NoError && registered.status()==QSettings::NoError;
}
}
