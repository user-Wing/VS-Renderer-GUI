#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <windows.h>
#include <unknwn.h>
#include <dshow.h>

// Public madVR interfaces from the installed developer SDK. No target patching.
template<class Signature> Signature nguMethod(IUnknown* object,int slot){return reinterpret_cast<Signature>((*reinterpret_cast<void***>(object))[slot]);}
inline IUnknown* nguInterface(IUnknown* object,const wchar_t* text){GUID id{};CLSIDFromString(text,&id);IUnknown* out=nullptr;object->QueryInterface(id,reinterpret_cast<void**>(&out));return out;}
inline bool nguMadvrTarget2x(IUnknown* renderer){
    IBasicVideo* video=nullptr;HRESULT destination=renderer->QueryInterface(IID_IBasicVideo,reinterpret_cast<void**>(&video));
    if(SUCCEEDED(destination)){destination=video->SetDestinationPosition(0,0,3840,2160);video->Release();}
    auto* command=nguInterface(renderer,L"{5E9599D1-C5DB-4A84-98A9-09BC5F8F1B79}");if(!command)return false;
    using Send=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,LPWSTR);wchar_t zoom[]=L"200%";
    const auto hr=nguMethod<Send>(command,10)(command,"setZoomMode",zoom);command->Release();return SUCCEEDED(hr) && SUCCEEDED(destination);
}
inline QJsonObject nguMadvrInfo(IUnknown* renderer){
    QJsonObject result;auto* info=nguInterface(renderer,L"{8FAB7F31-06EF-444C-A798-10314E185532}");if(!info)return result;
    using Double=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,double*);
    using Int=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,int*);
    using Size=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,SIZE*);
    using Rect=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,RECT*);
    for(const auto* key:{"refreshRate","renderingTime","averageRenderingTime","renderTime"}){double value=0;const auto hr=nguMethod<Double>(info,8)(info,key,&value);result[key]=QJsonObject{{"hresult",double(hr)},{"value",value}};}
    for(const auto* key:{"droppedFrames","repeatedFrames","presentationGlitches"}){int value=0;const auto hr=nguMethod<Int>(info,4)(info,key,&value);result[key]=QJsonObject{{"hresult",double(hr)},{"value",value}};}
    SIZE size{};if(SUCCEEDED(nguMethod<Size>(info,5)(info,"originalVideoSize",&size)))result["source_size"]=QJsonArray{int(size.cx),int(size.cy)};
    RECT rect{};if(SUCCEEDED(nguMethod<Rect>(info,6)(info,"videoOutputRect",&rect)))result["output_rect"]=QJsonArray{int(rect.left),int(rect.top),int(rect.right),int(rect.bottom)};
    info->Release();return result;
}
inline QJsonArray nguMadvrSettings(IUnknown* settings,const QString& path,int depth=0){
    QJsonArray result;if(depth>5)return result;
    using Enum=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,int,LPCWSTR,LPCWSTR,LPCWSTR,int*,int*,int*);
    using GetString=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,LPCWSTR,int*);
    using GetInt=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,int*);
    for(int kind=0;kind<2;++kind)for(int i=0;i<150;++i){wchar_t id[512]{},name[512]{},type[512]{};int ni=512,nn=512,nt=512;
        if(!nguMethod<Enum>(settings,kind?13:14)(settings,reinterpret_cast<LPCWSTR>(path.utf16()),i,id,name,type,&ni,&nn,&nt))break;
        const QString full=path+"\\"+QString::fromWCharArray(id),t=QString::fromWCharArray(type);QJsonObject item{{"path",full},{"name",QString::fromWCharArray(name)},{"type",t}};
        if(kind)item["children"]=nguMadvrSettings(settings,full,depth+1);
        else {wchar_t value[2048]{};int n=2048,v=0;auto key=reinterpret_cast<LPCWSTR>(full.utf16());
            if(nguMethod<GetString>(settings,9)(settings,key,value,&n))item["value"]=QString::fromWCharArray(value);
            else if(nguMethod<GetInt>(settings,10)(settings,key,&v))item["value"]=v;
            else if(nguMethod<GetInt>(settings,11)(settings,key,&v))item["value"]=bool(v);
        }result.append(item);
    }return result;
}
inline void nguDumpMadvr(IUnknown* renderer,const QString& directory){
    auto* settings=nguInterface(renderer,L"{1C3E03D6-F422-4D31-9424-75936F663BF7}");
    QJsonObject report=nguMadvrInfo(renderer);
    if(settings){report["scaling"]=nguMadvrSettings(settings,"scalingParent");settings->Release();}
    QFile file(QDir(directory).filePath("madvr-info.json"));if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(report).toJson());
    auto* command=nguInterface(renderer,L"{5E9599D1-C5DB-4A84-98A9-09BC5F8F1B79}");
    if(command){using Send=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,const char*,int);nguMethod<Send>(command,5)(command,"keyPress",0x1004a);command->Release();}
}
struct NguMadvrSettingsGuard {
    IUnknown* settings=nullptr;QByteArray backup;QString directory;QMap<QString,QString> original;
    QString get(const QString& key){using Get=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,LPCWSTR,int*);wchar_t value[1024]{};int n=1024;return nguMethod<Get>(settings,9)(settings,reinterpret_cast<LPCWSTR>(key.utf16()),value,&n)?QString::fromWCharArray(value):QString();}
    bool apply(IUnknown* renderer,const QString& path,const QString& family){
        if(family.isEmpty())return true;
        directory=path;settings=nguInterface(renderer,L"{1C3E03D6-F422-4D31-9424-75936F663BF7}");if(!settings)return false;
        using Export=BOOL(STDMETHODCALLTYPE*)(IUnknown*,void**,int*);void* bytes=nullptr;int size=0;
        if(!nguMethod<Export>(settings,4)(settings,&bytes,&size) || !bytes || size<=0)return false;
        backup=QByteArray(static_cast<const char*>(bytes),size);LocalFree(bytes);
        QFile file(QDir(path).filePath("madvr-settings-backup.bin"));if(!file.open(QIODevice::WriteOnly) || file.write(backup)!=backup.size())return false;file.close();
        const QMap<QString,QString> requested{{"lumaUp",QString(family).replace("Ngu","NGU")},{"lumaUpNguLumaDoubleQuality","med"},{"lumaUpNguDoubleActivate","auto"},{"lumaUpNguQuadActivate","auto"}};
        using Set=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,LPCWSTR);QJsonObject changes;bool ok=true;
        for(auto it=requested.cbegin();it!=requested.cend();++it){original[it.key()]=get(it.key());const bool accepted=nguMethod<Set>(settings,6)(settings,reinterpret_cast<LPCWSTR>(it.key().utf16()),reinterpret_cast<LPCWSTR>(it.value().utf16()));const auto readback=get(it.key());changes[it.key()]=QJsonObject{{"before",original[it.key()]},{"requested",it.value()},{"accepted",accepted},{"readback",readback}};ok=ok && accepted && readback==it.value();}
        using SetBool=BOOL(STDMETHODCALLTYPE*)(IUnknown*,LPCWSTR,BOOL);
        const bool keysAccepted=nguMethod<SetBool>(settings,8)(settings,L"keysOnlyIfFocused",FALSE);
        changes["keysOnlyIfFocused"]=QJsonObject{{"requested",false},{"accepted",keysAccepted}};ok=ok && keysAccepted;
        QFile log(QDir(path).filePath("madvr-settings-changes.json"));if(log.open(QIODevice::WriteOnly))log.write(QJsonDocument(changes).toJson());return ok;
    }
    ~NguMadvrSettingsGuard(){
        if(settings && !backup.isEmpty()){using Import=BOOL(STDMETHODCALLTYPE*)(IUnknown*,void*,int);bool ok=nguMethod<Import>(settings,5)(settings,backup.data(),backup.size());for(auto it=original.cbegin();it!=original.cend();++it)ok=ok && get(it.key())==it.value();QFile file(QDir(directory).filePath("madvr-settings-restored.json"));if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"restored",ok}}).toJson());}
        if(settings)settings->Release();
    }
};
