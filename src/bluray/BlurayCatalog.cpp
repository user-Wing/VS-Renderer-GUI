#include "bluray/BlurayCatalog.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QCollator>
#include <QSet>
#include <QDateTime>
#include <QtEndian>
#include <algorithm>
#include <numeric>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#endif

namespace vsr {
namespace {
quint16 u16(const QByteArray &d, int p) { return qFromBigEndian<quint16>(d.constData()+p); }
quint32 u32(const QByteArray &d, int p) { return qFromBigEndian<quint32>(d.constData()+p); }
void put16(QByteArray &d, int p, quint16 v) { qToBigEndian(v,d.data()+p); }
void put32(QByteArray &d, int p, quint32 v) { qToBigEndian(v,d.data()+p); }
QByteArray read(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly)?f.read(8*1024*1024):QByteArray(); }
bool save(const QString &path, const QByteArray &data) { QSaveFile f(path); return f.open(QIODevice::WriteOnly) && f.write(data)==data.size() && f.commit(); }
QString cleanName(QString name) { while(name.endsWith(QChar(0)))name.chop(1);return name; }
void roots(const QString &path, int depth, QStringList &found) {
    const QString child=QDir(path).filePath("BDMV");
    if(QFileInfo(QDir(child).filePath("PLAYLIST")).isDir() && QFileInfo(QDir(child).filePath("STREAM")).isDir()){found<<QFileInfo(child).absoluteFilePath();return;}
    if(QFileInfo(QDir(path).filePath("PLAYLIST")).isDir() && QFileInfo(QDir(path).filePath("STREAM")).isDir()) { found << QFileInfo(path).absoluteFilePath(); return; }
    if(depth==0)return;
    for(const auto &d:QDir(path).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDir::Name)) {
        const auto name=cleanName(d.fileName());
        if(name.isEmpty() || name=="." || name==".." || name.startsWith('.') || QStringList{"BACKUP","STREAM","CLIPINF","AUXDATA","META","CERTIFICATE","BDJO","JAR","SCANS","OST"}.contains(name.toUpper()))continue;
        roots(QDir(path).filePath(name),depth-1,found);
    }
}
QString discName(const QString &bdmv) {
    const QDir meta(QDir(bdmv).filePath("META/DL"));
    for(const auto &raw:meta.entryList({"*.xml"},QDir::Files,QDir::Name)) {
        const auto name=cleanName(raw);
        QXmlStreamReader xml(read(meta.filePath(name)));
        while(!xml.atEnd()){xml.readNext();if(xml.isStartElement() && xml.name()==QStringLiteral("name")){const auto title=xml.readElementText().trimmed();if(!title.isEmpty())return title;}}
    }
    return QFileInfo(QFileInfo(bdmv).absolutePath()).fileName();
}
bool linkDirectory(const QString &link, const QString &target) {
#ifdef Q_OS_WIN
    const auto nativeLink=QDir::toNativeSeparators(link);
    const DWORD attributes=GetFileAttributesW(reinterpret_cast<LPCWSTR>(nativeLink.utf16()));
    if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT) && QFileInfo(link).isDir())return true;
    // A junction references local/optical folders without developer-mode privileges.
    const QString substitute=QStringLiteral("\\??\\")+QDir::toNativeSeparators(QFileInfo(target).absoluteFilePath());
    const QString print=QDir::toNativeSeparators(QFileInfo(target).absoluteFilePath());
    QByteArray names(reinterpret_cast<const char *>(substitute.utf16()),substitute.size()*2);
    names.append(2,'\0');const int printOffset=names.size();names.append(reinterpret_cast<const char *>(print.utf16()),print.size()*2);names.append(2,'\0');
    QByteArray buffer(16+names.size(),'\0');
    auto *b=reinterpret_cast<unsigned char *>(buffer.data());
    qToLittleEndian<quint32>(IO_REPARSE_TAG_MOUNT_POINT,b);qToLittleEndian<quint16>(quint16(8+names.size()),b+4);
    qToLittleEndian<quint16>(0,b+8);qToLittleEndian<quint16>(quint16(substitute.size()*2),b+10);
    qToLittleEndian<quint16>(quint16(printOffset),b+12);qToLittleEndian<quint16>(quint16(print.size()*2),b+14);
    std::copy(names.begin(),names.end(),buffer.begin()+16);
    if(!QDir().mkpath(link))return false;
    const auto native=QDir::toNativeSeparators(link);
    HANDLE h=CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    DWORD returned=0;const bool ok=DeviceIoControl(h,FSCTL_SET_REPARSE_POINT,buffer.data(),DWORD(buffer.size()),nullptr,0,&returned,nullptr);
    CloseHandle(h);return ok && QFileInfo(link).isDir();
#else
    if(QFileInfo(link).isDir())return true;
    return QFile::link(target,link);
#endif
}
}

bool BlurayCatalog::parse(const QByteArray &d, BlurayPlaylist *p, QString *error) {
    *p={};
    const auto fail=[&](const QString &s){if(error)*error=s;return false;};
    if(d.size()<58 || d.left(4)!="MPLS" || !QList<QByteArray>{"0100","0200","0300"}.contains(d.mid(4,4)))return fail(QStringLiteral("无效或未支持的 MPLS 版本"));
    const quint32 offset=u32(d,8),mark=u32(d,12);
    if(offset>quint32(d.size()-10))return fail(QStringLiteral("MPLS 播放段越界"));
    const quint32 length=u32(d,int(offset));
    if(length<6 || quint64(offset)+4+length>quint64(d.size()))return fail(QStringLiteral("MPLS 播放段截断"));
    const int end=int(offset+4+length),count=u16(d,int(offset)+6);
    p->header=d.left(int(offset));const int subcount=u16(d,int(offset)+8);
    int pos=int(offset)+10;
    for(int i=0;i<count;++i){
        if(pos+2>end)return fail(QStringLiteral("PlayItem 长度缺失"));
        const int size=u16(d,pos);
        if(size<32 || pos+2+size>end)return fail(QStringLiteral("PlayItem 截断"));
        const QByteArray id=d.mid(pos+2,5);
        if(std::any_of(id.begin(),id.end(),[](char c){return c<'0'||c>'9';}) || d.mid(pos+7,4)!="M2TS")return fail(QStringLiteral("非标准 clip 标识或编码"));
        BlurayPart part;part.clip=QString::fromLatin1(id);part.in=u32(d,pos+14);part.out=u32(d,pos+18);part.multiAngle=(u16(d,pos+11)&0x10)!=0;part.data=d.mid(pos,size+2);
        if(part.out<=part.in)return fail(QStringLiteral("PlayItem 时间范围无效"));
        p->parts<<part;pos+=size+2;
    }
    for(int i=0;i<subcount;++i){
        if(pos+10>end)return fail(QStringLiteral("副播放路径截断"));
        const quint32 size=u32(d,pos);
        if(size<6 || quint64(pos)+4+size>quint64(end))return fail(QStringLiteral("副播放路径越界"));
        // Type 3 contains interactive menu graphics, not programme audio/subtitles.
        if(static_cast<unsigned char>(d[pos+5])!=3)p->subpaths=true;
        pos+=int(size)+4;
    }
    if(mark){
        if(mark>quint32(d.size()-6) || quint64(mark)+4+u32(d,int(mark))>quint64(d.size()))return fail(QStringLiteral("MPLS 章节截断"));
        const int n=u16(d,int(mark)+4);
        if(u32(d,int(mark))<quint32(2+n*14))return fail(QStringLiteral("章节数量越界"));
        for(int i=0;i<n;++i){const int m=int(mark)+6+i*14;const int item=u16(d,m+2);if(item>=count)return fail(QStringLiteral("章节引用不存在的 PlayItem"));p->marks<<BlurayMark{item,u32(d,m+4),d.mid(m,14)};}
    }
    return !p->parts.isEmpty() || fail(QStringLiteral("播放列表为空"));
}

QByteArray BlurayCatalog::slice(const BlurayPlaylist &p, int first, int count) {
    if(first<0 || count<1 || first+count>p.parts.size() || p.subpaths)return {};
    QByteArray list(10,'\0');put16(list,6,quint16(count));
    for(int i=first;i<first+count;++i)list+=p.parts[i].data;
    put32(list,0,quint32(list.size()-4));
    QByteArray marks(6,'\0');int n=0;
    for(const auto &mark:p.marks)if(mark.item>=first && mark.item<first+count){auto data=mark.data;put16(data,2,quint16(mark.item-first));marks+=data;++n;}
    put32(marks,0,quint32(marks.size()-4));put16(marks,4,quint16(n));
    auto header=p.header;put32(header,8,quint32(header.size()));put32(header,12,quint32(header.size()+list.size()));put32(header,16,0);
    return header+list+marks;
}

BlurayScan BlurayCatalog::scan(const QString &path, const QJsonObject &profile) {
    BlurayScan result;result.source=QFileInfo(path).absoluteFilePath();QStringList discs;
    roots(QFileInfo(path).isFile()?QFileInfo(path).absolutePath():path,7,discs);
    QCollator collator;collator.setNumericMode(true);std::sort(discs.begin(),discs.end(),[&](const QString &a,const QString &b){return collator.compare(a,b)<0;});
    const double minimum=profile.value("minimumMinutes").toDouble(15),maximum=profile.value("maximumMinutes").toDouble(45);
    int episode=profile.value("firstEpisode").toInt(1);
    const auto entries=profile.value("entries").toArray();
    for(const auto &disc:discs){
        QVector<BlurayTitle> candidates;QSet<QString> seen,episodePlaylists;
        const QDir dir(QDir(disc).filePath("PLAYLIST"));
        for(const auto &raw:dir.entryList({"*.mpls"},QDir::Files,QDir::Name)){
            const auto name=cleanName(raw);
            BlurayPlaylist p;QString error;p.path=dir.filePath(name);const auto bytes=read(p.path);const auto file=p.path;
            if(!parse(bytes,&p,&error)){result.warnings<<file+": "+error;continue;}p.path=file;
            const auto add=[&](int first,int count,bool suggested,const QString &label){
                if(first<0 || count<1 || first+count>p.parts.size()){result.warnings<<file+QStringLiteral(": 模板 PlayItem 范围无效");return;}
                BlurayTitle title;title.disc=disc;title.playlist=file;title.firstItem=first;title.itemCount=count;title.suggested=suggested;title.label=label;QString signature;
                for(int i=first;i<first+count;++i){const auto &part=p.parts[i];title.ticks+=part.out-part.in;title.clips<<part.clip;signature+=QString::fromLatin1(QCryptographicHash::hash(part.data,QCryptographicHash::Sha256).toHex());
                    const QFileInfo stream(QDir(disc).filePath("STREAM/"+part.clip+".m2ts"));
                    if(!stream.isFile() || stream.size()<192)title.warning=QStringLiteral("引用视频缺失或为空；请下载完整 BD");
                    if(!QFileInfo(QDir(disc).filePath("CLIPINF/"+part.clip+".clpi")).isFile())title.warning=QStringLiteral("CLPI 缺失，无法保证裁切/定位");
                    if(part.multiAngle)title.warning=QStringLiteral("多角度节目：原始 MPLS 可 remux，拆分需手动确认");
                }
                if(p.subpaths)title.warning=QStringLiteral("含副播放路径：保留原始 MPLS，禁用拆集");
                for(const auto &m:p.marks)if(m.item>=first && m.item<first+count && m.data[1]==1)++title.chapters;
                if(!seen.contains(signature)){seen.insert(signature);candidates<<title;}
            };
            if(!entries.isEmpty()){
                for(const auto &e:entries){const auto o=e.toObject();if(o.value("playlist").toString()!=name)continue;const auto match=o.value("discContains").toString();if(!match.isEmpty() && !disc.contains(match,Qt::CaseInsensitive))continue;add(o.value("firstItem").toInt(),o.value("itemCount").toInt(p.parts.size()),true,o.value("label").toString());}
                continue;
            }
            int episodic=0;for(const auto &part:p.parts){const double minutes=(part.out-part.in)/2700000.0;if(minutes>=minimum && minutes<=maximum)++episodic;}
            if(profile.value("splitPlayItems").toBool(true) && episodic>=2 && !p.subpaths && std::none_of(p.parts.begin(),p.parts.end(),[](const auto &part){return part.multiAngle;})){
                episodePlaylists.insert(file);
                for(int i=0;i<p.parts.size();++i){const double minutes=(p.parts[i].out-p.parts[i].in)/2700000.0;if(minutes>=minimum && minutes<=maximum)add(i,1,true,{});}
            }
            add(0,p.parts.size(),(!profile.value("splitPlayItems").toBool(true) || episodic<2) && !p.subpaths && p.parts.size()>0 &&
                std::accumulate(p.parts.begin(),p.parts.end(),qint64(0),[](qint64 sum,const auto &part){return sum+part.out-part.in;})>=qint64(minimum*2700000),{});
        }
        for(auto &title:candidates){
            if(!episodePlaylists.isEmpty() && !episodePlaylists.contains(title.playlist))title.suggested=false;
            QSet<QString> unique;for(const auto &clip:title.clips)unique.insert(clip);
            if(title.itemCount>3 && unique.size()*2<title.itemCount && title.ticks/title.itemCount<5*60*45000){title.suggested=false;title.warning=QStringLiteral("大量重复短片，疑似菜单/循环；请手动确认");}
            if(title.label.isEmpty())title.label=title.suggested?QStringLiteral("%1 · 候选第 %2 集").arg(discName(disc)).arg(episode++):QStringLiteral("%1 · %2（完整节目）").arg(discName(disc),QFileInfo(title.playlist).fileName());
            result.titles<<title;
        }
    }
    if(discs.isEmpty())result.warnings<<QStringLiteral("未找到 BDMV/PLAYLIST 与 STREAM。请选择光盘根目录或包含多卷 BD 的上级目录。");
    return result;
}

QString BlurayCatalog::prepare(const BlurayTitle &title, QString *error) {
    if(QFileInfo(title.playlist).suffix().compare("m2ts",Qt::CaseInsensitive)==0 && QFileInfo(title.playlist).isFile())return title.playlist;
    BlurayPlaylist p;const auto bytes=read(title.playlist);
    if(!parse(bytes,&p,error))return {};
    const bool complex=p.subpaths || std::any_of(p.parts.begin(),p.parts.end(),[](const auto &part){return part.multiAngle;});
    if(complex && (title.firstItem!=0 || title.itemCount!=p.parts.size())){
        if(error)*error=QStringLiteral("多角度/副播放路径不能自动拆分，请选择完整 MPLS");return {};
    }
    const auto data=complex?bytes:slice(p,title.firstItem,title.itemCount);
    if(data.isEmpty()){if(error)*error=QStringLiteral("无法生成节目播放描述");return {};}
    QByteArray identity=title.disc.toUtf8()+data;
    for(const auto &clip:title.clips){QFileInfo f(QDir(title.disc).filePath("STREAM/"+clip+".m2ts"));identity+=QByteArray::number(f.size())+QByteArray::number(f.lastModified().toMSecsSinceEpoch());}
    const QString key=QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex());
    const QDir root(QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("bluray/"+key));
    const QString bdmv=root.filePath("BDMV");QDir().mkpath(bdmv+"/PLAYLIST");
    for(const auto &dir:QStringList{"STREAM","CLIPINF"})if(!linkDirectory(bdmv+"/"+dir,QDir(title.disc).filePath(dir))){if(error)*error=QStringLiteral("视频/CLPI 目录无法建立本地引用。网络盘请先通过 ModelScope Manager 下载完整 BD 到本地磁盘。");return {};}
    for(const auto &name:QStringList{"index.bdmv","MovieObject.bdmv"}){const auto d=read(QDir(title.disc).filePath(name));if(!d.isEmpty())save(bdmv+"/"+name,d);}
    const QString playlist=bdmv+"/PLAYLIST/00000.mpls";
    if(!save(playlist,data)){if(error)*error=QStringLiteral("保存播放描述失败");return {};}
    return playlist;
}

QJsonObject BlurayCatalog::feedback(const BlurayScan &scan) {
    QJsonArray titles,metadata,warnings;QSet<QString> files;
    QStringList discs;roots(scan.source,7,discs);
    for(const auto &disc:discs)for(const auto &dir:QStringList{"PLAYLIST","CLIPINF"})for(const auto &name:QDir(QDir(disc).filePath(dir)).entryList(QDir::Files,QDir::Name))files.insert(QDir(disc).filePath(dir+"/"+cleanName(name)));
    for(const auto &title:scan.titles){QJsonArray clips;for(const auto &c:title.clips){clips<<c;if(title.itemCount>0)files.insert(QDir(title.disc).filePath("CLIPINF/"+c+".clpi"));}if(title.itemCount>0)files.insert(title.playlist);
        titles<<QJsonObject{{"disc",title.disc},{"playlist",QFileInfo(title.playlist).fileName()},{"label",title.label},{"firstItem",title.firstItem},{"itemCount",title.itemCount},{"duration",time(title.ticks)},{"clips",clips},{"warning",title.warning}};}
    qint64 total=0;for(const auto &file:files){const auto bytes=read(file);if(total+bytes.size()>8*1024*1024)continue;total+=bytes.size();metadata<<QJsonObject{{"path",file},{"base64",QString::fromLatin1(bytes.toBase64())}};}
    for(const auto &w:scan.warnings)warnings<<w;
    return {{"schemaVersion",1},{"source",scan.source},{"titles",titles},{"metadata",metadata},{"warnings",warnings}};
}
QString BlurayCatalog::mkvmerge() {
    const QDir app(QCoreApplication::applicationDirPath());
    for(const auto &file:QStringList{app.filePath("runtime/mkvtoolnix/mkvmerge.exe"),app.filePath("mkvmerge.exe"),QStandardPaths::findExecutable("mkvmerge"),QStringLiteral("C:/PortableSoft/Mkvtoolnix/mkvmerge.exe")})if(QFileInfo(file).isFile())return file;
    return {};
}
QJsonObject BlurayCatalog::metadata(const QString &path) {
    BlurayPlaylist p;QString error;if(!parse(read(path),&p,&error))return {};
    QJsonArray chapters;QVector<qint64> offsets; qint64 ticks=0;
    for(const auto &part:p.parts){offsets<<ticks;ticks+=part.out-part.in;}
    for(const auto &mark:p.marks)if(mark.data[1]==1 && mark.time>=p.parts[mark.item].in && mark.time<p.parts[mark.item].out){const qint64 at=offsets[mark.item]+mark.time-p.parts[mark.item].in;chapters<<QJsonObject{{"start100ns",double(at)*10000000.0/45000.0},{"title",QString("Chapter %1").arg(chapters.size()+1)}};}
    QJsonObject languages;
    for(const auto &part:p.parts){
        if(part.multiAngle)continue;
        const auto &d=part.data;const int stn=34;
        if(stn+16>d.size())continue;const int end=stn+2+u16(d,stn);if(end>d.size())continue;
        const int count=static_cast<unsigned char>(d[stn+4])+static_cast<unsigned char>(d[stn+5])+static_cast<unsigned char>(d[stn+6]);int pos=stn+16;
        for(int i=0;i<count && pos<end;++i){const int len=static_cast<unsigned char>(d[pos++]);if(pos+len>=end || len<3)break;const int type=static_cast<unsigned char>(d[pos]);const int pid=type==1?u16(d,pos+1):-1;pos+=len;const int size=static_cast<unsigned char>(d[pos++]);if(pos+size>end || size<1)break;const int codec=static_cast<unsigned char>(d[pos]);QString lang;
            if((codec==0x90 || codec==0x91) && size>=4)lang=QString::fromLatin1(d.mid(pos+1,3));
            else if((codec==0x03 || codec==0x04 || (codec>=0x80 && codec<=0x86) || codec==0xa1 || codec==0xa2 || codec==0x92) && size>=5)lang=QString::fromLatin1(d.mid(pos+2,3));
            if(pid>=0 && !lang.isEmpty())languages.insert(QString::number(pid),lang);pos+=size;
        }
    }
    return {{"chapters",chapters},{"languages",languages},{"duration100ns",double(ticks)*10000000.0/45000.0}};
}
QString BlurayCatalog::time(qint64 ticks) {const qint64 ms=ticks/45;return QString("%1:%2:%3.%4").arg(ms/3600000,2,10,QChar('0')).arg(ms/60000%60,2,10,QChar('0')).arg(ms/1000%60,2,10,QChar('0')).arg(ms%1000,3,10,QChar('0'));}
}
