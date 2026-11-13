#pragma once
#include "backend/ExportPipeline.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QDateTime>
#include <memory>

namespace vsr::exportjobs {
inline QString root() { const auto override=qEnvironmentVariable("VSR_EXPORT_ROOT");return override.isEmpty()?QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("VSRenderer/export-jobs"):override; }
inline QJsonObject read(const QString &path) {QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>8*1024*1024)return {};return QJsonDocument::fromJson(f.readAll()).object();}
inline bool write(const QString &path,const QJsonObject &o) {QSaveFile f(path);const auto bytes=QJsonDocument(o).toJson();return f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size()&&f.commit();}
inline QStringList strings(const QJsonValue &v){QStringList s;for(const auto &x:v.toArray())s<<x.toString();return s;}
inline QJsonObject planJson(const ExportPlan &p){return {{"ffmpeg",p.ffmpegProgram},{"ffmpegArgs",QJsonArray::fromStringList(p.ffmpegArguments)},{"vspipe",p.vspipeProgram},{"vspipeArgs",QJsonArray::fromStringList(p.vspipeArguments)}};}
inline ExportPlan plan(const QJsonObject &o){return {o["ffmpeg"].toString(),strings(o["ffmpegArgs"]),o["vspipe"].toString(),strings(o["vspipeArgs"]),{}};}
inline QStringList jobs(const QString &base){auto ids=QDir(base).entryList({"job-*"},QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);QStringList ordered;for(const auto &v:read(QDir(base).filePath("order.json"))["ids"].toArray()){const auto id=v.toString();if(ids.removeOne(id))ordered<<id;}return ordered+ids;}
inline QString workerProgram(){const auto override=qEnvironmentVariable("VSR_EXPORT_WORKER");return override.isEmpty()?QDir(QCoreApplication::applicationDirPath()).filePath("vs-export-worker.exe"):override;}
inline bool ensureWorker(){
    QDir().mkpath(root());if(!QFileInfo::exists(workerProgram()))return false;
    QProcess process;process.setProgram(workerProgram());process.setArguments({"--root",root()});process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process.setStandardInputFile(QProcess::nullDevice());process.setStandardOutputFile(QProcess::nullDevice());process.setStandardErrorFile(QProcess::nullDevice());
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->inheritHandles=false;});
#endif
    return process.startDetached();
}
inline bool command(const QString &id,const QString &action){if(!id.startsWith("job-")||id.contains('/')||id.contains('\\'))return false;const auto dir=QDir(root()).filePath(id);return write(QDir(dir).filePath("command-"+QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json"),{{"action",action}});}
inline bool busy(const QString &s){return s=="waiting"||s=="running"||s=="paused"||s=="stopping";}

// Owns BOTH producer and consumer, their pipe and frozen scripts. No UI parent
// process, temporary directory lifetime or GUI event loop participates in encoding.
class Worker final : public QObject {
    QString base_,active_;ExportPipeline pipeline_;QJsonObject status_;QTimer timer_;bool queuePaused_=false;
    void persist(){if(!active_.isEmpty()){status_["updated"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);status_["workerPid"]=double(QCoreApplication::applicationPid());write(QDir(base_).filePath(active_+"/status.json"),status_);}}
    void log(const QString &text){if(active_.isEmpty())return;QFile f(QDir(base_).filePath(active_+"/worker.log"));if(f.open(QIODevice::Append)&&f.size()<16*1024*1024)f.write((text+'\n').toUtf8());}
public:
    explicit Worker(QString base):base_(std::move(base)){
        connect(&pipeline_,&ExportPipeline::logMessage,this,[this](const QString &s){log(s);});
        connect(&pipeline_,&ExportPipeline::durationKnown,this,[this](double d){status_["duration"]=d;persist();});
        connect(&pipeline_,&ExportPipeline::progress,this,[this](qint64 f,qint64 us){status_["frames"]=double(f);status_["microseconds"]=double(us);});
        connect(&pipeline_,&ExportPipeline::finished,this,[this](bool ok,const QString &message){status_["state"]=status_["state"]=="stopping"?"stopped":ok?"completed":"failed";status_["message"]=message;persist();active_.clear();});
        // A dead worker is NOT resumable encoding. Keep the partial output and
        // require an explicit restart from frame zero. Waiting jobs remain queued.
        for(const auto &id:jobs(base_)){const auto path=QDir(base_).filePath(id+"/status.json");auto s=read(path);const auto state=s["state"].toString();if(state=="running"||state=="paused"||state=="stopping"){s["state"]="failed";s["message"]="编码进程已中断；保留部分输出，请从头重做（不支持断点续接）。";write(path,s);}}
        queuePaused_=read(QDir(base_).filePath("queue.json"))["paused"].toBool();
        timer_.setInterval(500);connect(&timer_,&QTimer::timeout,this,[this]{tick();});timer_.start();QTimer::singleShot(0,this,[this]{tick();});
    }
    void tick(){
        for(const auto &id:jobs(base_)){
            const QDir dir(QDir(base_).filePath(id));auto s=id==active_?status_:read(dir.filePath("status.json"));
            if(s.isEmpty())s={{"state","waiting"},{"frames",0},{"microseconds",0}};
            for(const auto &file:dir.entryList({"command-*.json"},QDir::Files,QDir::Name)){
                const auto action=read(dir.filePath(file))["action"].toString();
                if(action=="stop"){
                    if(id==active_){status_["state"]="stopping";pipeline_.cancel();s=status_;}
                    else if(s["state"]=="waiting")s["state"]="stopped";
                }else if(action=="restart" && !busy(s["state"].toString())){
                    auto m=read(dir.filePath("job.json"));auto p=plan(m["plan"].toObject());p.ffmpegArguments.removeAll("-n");if(!p.ffmpegArguments.contains("-y"))p.ffmpegArguments.prepend("-y");m["plan"]=planJson(p);if(write(dir.filePath("job.json"),m))s={{"state","waiting"},{"frames",0},{"microseconds",0}};
                }else if(action=="pause"||action=="resume"){
                    const bool paused=action=="pause";
                    if(id!=active_ || pipeline_.setPaused(paused)){queuePaused_=paused;write(QDir(base_).filePath("queue.json"),{{"paused",paused}});if(id==active_){status_["state"]=paused?"paused":"running";s=status_;}}
                }else if(action=="remove" && id!=active_){s["state"]="stopped";s["hidden"]=true;}
                write(dir.filePath("status.json"),s);QFile::remove(dir.filePath(file));
            }
            if(id==active_){status_=s;persist();continue;}
            if(!active_.isEmpty()||queuePaused_||s["hidden"].toBool()||s["state"]!="waiting")continue;
            const auto m=read(dir.filePath("job.json"));if(m.isEmpty())continue;
            active_=id;status_=s;status_["state"]="running";status_["duration"]=m["duration"];status_["started"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);persist();
            if(!pipeline_.start(plan(m["plan"].toObject())) && !active_.isEmpty()){status_["state"]="failed";status_["message"]="无法启动编码进程；请检查worker.log与依赖。";persist();active_.clear();}
        }
        persist();
    }
};
}
