#include "player/PlayerAudioMetadata.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <algorithm>
#include <limits>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace vsr {
namespace {
QByteArray run(const QString &name,const QStringList &args) {
    QProcess process;
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    process.start(QDir(QCoreApplication::applicationDirPath()).filePath(name),args);
    if(!process.waitForStarted(5000)){qWarning()<<"Metadata process failed to start"<<name<<process.errorString();return {};}
    if(process.state()!=QProcess::NotRunning && !process.waitForFinished(20000)){qWarning()<<"Metadata process timed out"<<name<<process.errorString();process.kill();process.waitForFinished();return {};}
    if(process.exitCode()!=0 || process.exitStatus()!=QProcess::NormalExit){qWarning()<<"Metadata process failed"<<name<<process.exitCode()<<process.readAllStandardError();return {};}
    return process.readAllStandardOutput();
}
qint64 srtTime(const QString &value) {const auto p=value.split(QRegularExpression("[:,.]"));return ((p[0].toLongLong()*3600+p[1].toLongLong()*60+p[2].toLongLong())*1000+p[3].toLongLong())*10000;}
}
QVector<PlayerLyricLine> PlayerAudioMetadata::parseLyrics(QString text,qint64 duration) {
    text.remove(QChar(0xfeff));text.remove('\r');QVector<PlayerLyricLine> lines;
    const QRegularExpression stamp(R"(\[(\d+):(\d{2})(?:[.:](\d{1,3}))?\])");
    const auto offset=QRegularExpression(R"(\[offset:([+-]?\d+)\])",QRegularExpression::CaseInsensitiveOption).match(text);
    const qint64 shift=offset.hasMatch()?offset.captured(1).toLongLong()*10000:0;
    for(const auto &row:text.split('\n')) {
        auto matches=stamp.globalMatch(row);QVector<qint64> times;int end=0;
        while(matches.hasNext()){const auto match=matches.next();times<<qMax<qint64>(0,(match.captured(1).toLongLong()*60000+match.captured(2).toLongLong()*1000+match.captured(3).leftJustified(3,'0').toInt())*10000+shift);end=match.capturedEnd();}
        const auto body=row.mid(end).trimmed();for(const auto at:times)if(!body.isEmpty())lines<<PlayerLyricLine{at,0,body};
    }
    std::stable_sort(lines.begin(),lines.end(),[](const auto &a,const auto &b){return a.start<b.start;});
    for(int i=0;i<lines.size();++i){int next=i+1;while(next<lines.size() && lines[next].start==lines[i].start)++next;lines[i].end=next<lines.size()?lines[next].start:duration>lines[i].start?duration:lines[i].start+10000000;}
    return lines;
}
QJsonObject PlayerAudioMetadata::probe(const QString &path){return QJsonDocument::fromJson(run("ffprobe.exe",{"-v","error","-show_format","-show_streams","-show_chapters","-of","json",path})).object();}
PlayerAudioMetadata PlayerAudioMetadata::read(const QString &path) {
    PlayerAudioMetadata result;
    const auto probe=PlayerAudioMetadata::probe(path);
    if(probe.isEmpty()){result.error=QStringLiteral("无法读取音频元数据(ffprobe)");return result;}
    const auto format=probe.value("format").toObject();
    const qint64 duration=qRound64(format.value("duration").toString().toDouble()*10000000);
    bool audio=false,video=false;int cover=-1,subtitle=-1;QStringList tags;
    const auto collect=[&](const QJsonObject &values){for(const auto &key:values.keys())if(key.contains("lyric",Qt::CaseInsensitive))tags.prepend(values.value(key).toString());for(const auto &key:values.keys())if(key.compare("comment",Qt::CaseInsensitive)==0 && !parseLyrics(values.value(key).toString(),duration).isEmpty())tags<<values.value(key).toString();};
    collect(format.value("tags").toObject());
    for(const auto &value:probe.value("streams").toArray()) {
        const auto stream=value.toObject();collect(stream.value("tags").toObject());const auto type=stream.value("codec_type").toString();
        audio|=type=="audio";if(type=="video"){if(stream.value("disposition").toObject().value("attached_pic").toInt())cover=stream.value("index").toInt();else video=true;}
        if(type=="subtitle" && subtitle<0 && QStringList{"ass","ssa","subrip","mov_text","webvtt","text"}.contains(stream.value("codec_name").toString()))subtitle=stream.value("index").toInt();
    }
    result.audioOnly=audio && !video;
    for(const auto &value:probe.value("chapters").toArray()){const auto c=value.toObject();result.chapters<<QJsonObject{{"start100ns",double(qRound64(c.value("start_time").toString().toDouble()*10000000))},{"title",c.value("tags").toObject().value("title").toString()}};}
    if(!result.audioOnly)return result;
    if(cover>=0)result.cover=QImage::fromData(run("ffmpeg.exe",{"-v","error","-i",path,"-map",QString("0:%1").arg(cover),"-frames:v","1","-f","image2pipe","-c:v","png","pipe:1"}));
    const auto sidecar=QFileInfo(path).absoluteDir().filePath(QFileInfo(path).completeBaseName()+".lrc");QFile file(sidecar);
    if(file.open(QIODevice::ReadOnly)){
        auto text=QString::fromUtf8(file.readAll());text.remove(QChar(0xfeff));text.remove('\r');
        result.lyrics=parseLyrics(text,duration);
        if(result.lyrics.isEmpty() && !text.trimmed().isEmpty())result.lyrics<<PlayerLyricLine{0,duration>0?duration:std::numeric_limits<qint64>::max(),text.trimmed()};
        if(!result.lyrics.isEmpty())result.lyricSource=QFileInfo(sidecar).fileName();
    }
    for(const auto &text:tags)if(result.lyrics.isEmpty()){result.lyrics=parseLyrics(text,duration);if(!result.lyrics.isEmpty())result.lyricSource=QStringLiteral("内嵌歌词标签");}
    if(result.lyrics.isEmpty() && subtitle>=0){
        const auto srt=QString::fromUtf8(run("ffmpeg.exe",{"-v","error","-i",path,"-map",QString("0:%1").arg(subtitle),"-c:s","srt","-f","srt","pipe:1"}));
        const QRegularExpression cue(R"((\d+:\d{2}:\d{2}[,.]\d{3})\s*-->\s*(\d+:\d{2}:\d{2}[,.]\d{3})[^\n]*\n(.*?)(?=\n\s*\n|\z))",QRegularExpression::DotMatchesEverythingOption);
        auto matches=cue.globalMatch(srt);while(matches.hasNext()){const auto m=matches.next();auto body=m.captured(3).trimmed();body.remove(QRegularExpression("<[^>]*>"));result.lyrics<<PlayerLyricLine{srtTime(m.captured(1)),srtTime(m.captured(2)),body};}
        if(!result.lyrics.isEmpty())result.lyricSource=QStringLiteral("容器字幕轨");
    }
    if(result.lyrics.isEmpty())for(const auto &text:tags)if(!text.trimmed().isEmpty()){result.lyrics<<PlayerLyricLine{0,duration,text.trimmed()};result.lyricSource=QStringLiteral("非同步歌词标签");break;}
    return result;
}
}
