#include "player/PlayerAiSubtitles.h"
#include "backend/ThreeFpPlayer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtEndian>
#include <QCryptographicHash>
#include <cmath>

namespace vsr {
PlayerAiSubtitles::PlayerAiSubtitles(QObject *parent):QObject(parent){
    timer_.setInterval(30);connect(&timer_,&QTimer::timeout,this,&PlayerAiSubtitles::pump);
    connect(&worker_,&QProcess::readyReadStandardOutput,this,&PlayerAiSubtitles::receive);
    connect(&worker_,&QProcess::readyReadStandardError,this,[this]{const auto text=QString::fromUtf8(worker_.readAllStandardError().right(2048));if(!text.trimmed().isEmpty())emit status(text);});
    connect(&worker_,&QProcess::finished,this,[this](int code){if(timer_.isActive()){stop();emit status(tr("AI 字幕进程已退出（%1）。").arg(code));}});
    connect(&worker_,&QProcess::errorOccurred,this,[this]{const auto message=worker_.errorString();stop();emit status(message);});
}
PlayerAiSubtitles::~PlayerAiSubtitles(){stop();}
QString PlayerAiSubtitles::validatePackage(const QString &path,QJsonObject *resolved){
    const QFileInfo file(path);const QDir directory(file.isDir()?file.absoluteFilePath():file.absolutePath());
    QFile manifest(directory.filePath("ai-subtitles.json"));
    if(!manifest.open(QIODevice::ReadOnly))return tr("未找到 AI 字幕扩展包 ai-subtitles.json；请在字幕设置中手动配置模型和运行时。");
    if(manifest.size()>65536)return tr("AI 字幕清单过大。");
    auto data=QJsonDocument::fromJson(manifest.readAll()).object();
    if(QFileInfo(data.value("worker").toString()).suffix().compare("py",Qt::CaseInsensitive)!=0)return tr("AI 字幕 worker 必须是 Python 脚本（.py）。");
    const auto fingerprint=[&](){QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData(QJsonDocument(data).toJson(QJsonDocument::Compact));const auto runtime=data.value("runtimeDirectory").toString();if(!runtime.isEmpty()){QFile server(QDir(runtime).filePath("llama-server.exe"));if(server.exists()){if(!server.open(QIODevice::ReadOnly) || !hash.addData(&server))return false;}}for(const auto &key:QStringList{"worker","python","llamaServer"}){const auto path=data.value(key).toString();if(path.isEmpty())continue;QFile executable(path);if(!executable.open(QIODevice::ReadOnly) || !hash.addData(&executable))return false;hash.addData(path.toUtf8());}data.insert("executableFingerprint",QString::fromLatin1(hash.result().toHex()));return true;};
    const int version=data.value("version").toInt(1);
    if(version!=1 && version!=2)return tr("不支持的 AI 字幕包版本。");
    if(version==2){
        for(const auto &key:QStringList{"worker","python","modelDirectory","runtimeDirectory"}){
            auto value=data.value(key).toString();if(value.isEmpty() && key=="runtimeDirectory")continue;
            if(!QFileInfo(value).isAbsolute())value=directory.absoluteFilePath(value);
            if(key=="modelDirectory" || key=="runtimeDirectory"){if(!QFileInfo(value).isDir())return tr("AI 字幕目录缺失：%1").arg(key);}
            else if(!QFileInfo(value).isFile())return tr("AI 字幕文件缺失：%1").arg(key);
            data.insert(key,value);
        }
        if(!fingerprint())return tr("无法校验 AI 字幕执行文件。");if(resolved)*resolved=data;return {};
    }
    for(const auto &key:QStringList{"worker","model","mmproj","python","llamaServer"}){
        auto value=data.value(key).toString();
        if(value.isEmpty() && key=="python")value=QStandardPaths::findExecutable("python");
        if(value.isEmpty() && key=="llamaServer")value=QStandardPaths::findExecutable("llama-server.exe");
        if(!QFileInfo(value).isAbsolute())value=directory.absoluteFilePath(value);
        if(!QFileInfo(value).isFile())return tr("AI 字幕组件缺失：%1。请在字幕设置中配置外置模型和运行时。").arg(key);
        data.insert(key,value);
    }
    if(!fingerprint())return tr("无法校验 AI 字幕执行文件。");if(resolved)*resolved=data;return {};
}
QString PlayerAiSubtitles::start(ThreeFpPlayer *player,const QString &package,const QString &approvedFingerprint){
    stop();QJsonObject data;const auto error=validatePackage(package,&data);if(!error.isEmpty())return error;
    if(!approvedFingerprint.isEmpty() && data.value("executableFingerprint").toString()!=approvedFingerprint)return tr("AI 字幕扩展包在确认后已变化，请重新确认。");
    if(!player->enableAudioTap(true))return tr("当前内核不支持 AI 音频接口，或尚未打开媒体。");
    player_=player;stamped_=false;lines_.clear();
    QStringList arguments{data.value("worker").toString(),"--model-dir",QFileInfo(data.value("model").toString()).absolutePath(),"--model",data.value("model").toString(),"--mmproj",data.value("mmproj").toString(),"--llama-server",data.value("llamaServer").toString(),"--device-backend",data.value("device").toString("auto"),"--window",QString::number(qBound(.5,data.value("window").toDouble(5),30.)),"--step",QString::number(qBound(.5,data.value("step").toDouble(5),30.))};
    if(data.value("version").toInt()==2){
        arguments={data.value("worker").toString(),"--model-dir",data.value("modelDirectory").toString(),"--device",data.value("device").toString("auto"),"--engine",data.value("engine").toString("auto"),"--window",QString::number(qBound(.5,data.value("window").toDouble(5),30.)),"--step",QString::number(qBound(.5,data.value("step").toDouble(5),30.)),"--language",data.value("language").toString("auto")};
        if(!data.value("runtimeDirectory").toString().isEmpty())arguments<<"--runtime-dir"<<data.value("runtimeDirectory").toString();
    }
    worker_.start(data.value("python").toString(),arguments);timer_.start();emit status(tr("AI 字幕：正在检查推理后端与实时性能…"));return {};
}
void PlayerAiSubtitles::stop(){
    timer_.stop();if(player_)player_->enableAudioTap(false);player_=nullptr;captionUntil_=0;emit caption({});
    if(worker_.state()!=QProcess::NotRunning){worker_.closeWriteChannel();worker_.terminate();if(!worker_.waitForFinished(1000)){worker_.kill();worker_.waitForFinished(1000);}}
}
void PlayerAiSubtitles::pump(){
    if(!player_)return;const auto snap=player_->snapshot();
    if(!stamped_ || generation_!=snap.timelineGeneration){generation_=snap.timelineGeneration;stamped_=true;flushed_=false;captionUntil_=0;emit caption({});}
    if(captionUntil_ && snap.position100ns>captionUntil_){captionUntil_=0;emit caption({});}
    if((snap.state!=ThreeFpState::Playing && snap.state!=ThreeFpState::Ended) || worker_.state()!=QProcess::Running)return;
    for(int n=0;n<16;++n){
        AudioTapBlock block;const auto pcm=player_->readAudioTap(block);if(pcm.isEmpty())break;
        if(worker_.bytesToWrite()>512*1024)continue;
        QJsonArray positions;for(unsigned c=0;c<qMin(block.channels,24u);++c)positions.append(block.positions[c]);
        const auto meta=QJsonDocument(QJsonObject{{"generation",double(block.generation)},{"position100ns",double(block.position100ns)},{"frames",int(block.frames)},{"sampleRate",int(block.sampleRate)},{"channels",int(block.channels)},{"sampleFormat",block.sampleFormat},{"positions",positions},{"bytes",pcm.size()}}).toJson(QJsonDocument::Compact);
        QByteArray packet(4,Qt::Uninitialized);qToLittleEndian<quint32>(meta.size(),packet.data());packet+=meta;packet+=pcm;worker_.write(packet);
    }
    if(snap.state==ThreeFpState::Ended && !flushed_){
        const auto meta=QJsonDocument(QJsonObject{{"generation",double(generation_)},{"flush",true},{"bytes",0}}).toJson(QJsonDocument::Compact);
        QByteArray packet(4,Qt::Uninitialized);qToLittleEndian<quint32>(meta.size(),packet.data());packet+=meta;worker_.write(packet);flushed_=true;
    }
}
bool PlayerAiSubtitles::validMessage(const QJsonObject &data){
    const auto type=data.value("type").toString();
    if(type=="status")return data.value("state").isString() && data.value("detail").isString() && data.value("state").toString().size()<=128 && data.value("detail").toString().size()<=2048;
    if(type=="stats"){const auto v=data.value("rtf");return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=0 && v.toDouble()<=1e6;}
    if(type!="final" || !data.value("text").isString() || data.value("text").toString().size()>8192)return false;
    for(const auto &key:{"generation","end100ns"}){const auto value=data.value(key);const double n=value.toDouble(-1);if(!value.isDouble() || !std::isfinite(n) || n<0 || n>9007199254740991. || std::trunc(n)!=n)return false;}
    return true;
}
void PlayerAiSubtitles::receive(){
    lines_+=worker_.readAllStandardOutput();if(lines_.size()>1024*1024){stop();emit status(tr("AI 字幕输出无效。"));return;}
    while(lines_.contains('\n')){
        const int end=lines_.indexOf('\n');const auto data=QJsonDocument::fromJson(lines_.left(end)).object();lines_.remove(0,end+1);
        if(end>65536 || !validMessage(data))continue;
        const auto type=data.value("type").toString();
        if(type=="status")emit status(data.value("state").toString()+": "+data.value("detail").toString());
        if(type=="stats"){
            const double rtf=data.value("rtf").toDouble(-1);
            if(rtf>=0)emit status(tr("AI 字幕性能：RTF %1（≤1 可跟上实时）。").arg(rtf,0,'f',2));
            if(rtf>1){stop();emit status(tr("当前设备的 AI 字幕推理跟不上实时，请选择更小模型或更换 GPU 后端。"));return;}
        }
        if(type=="final" && player_ && stamped_ && quint64(data.value("generation").toDouble())==generation_){
            const auto snap=player_->snapshot();if(snap.timelineGeneration!=generation_)continue;
            const auto endTime=qint64(data.value("end100ns").toDouble());
            if(endTime<snap.position100ns-100000000 || endTime>snap.position100ns+600000000)continue;
            captionUntil_=qMax(endTime,snap.position100ns+50000000);emit caption(data.value("text").toString());
        }
    }
}
}
