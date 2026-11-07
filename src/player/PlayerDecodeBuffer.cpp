#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QMap>
#include <QTimer>
#include <windows.h>
#include <algorithm>

namespace vsr {
void PlayerWindow::updateDecodeBuffer(){
    if(bufferTimer_.isValid() && bufferTimer_.elapsed()<500)return;bufferTimer_.restart();
    const auto snap=clock_->snapshot();
    if(!ready_ || !direct_ || imageMode_ || minimalMode_ || networkSource() || snap.videoHeight<2160 || snap.decodeMode!=1)return;
    if(snap.timelineGeneration!=decodePoolGeneration_){
        decodePoolGeneration_=snap.timelineGeneration;decodeBurstFrom_=decodeBurstUntil_=0;bitrateTimer_.invalidate();
        if(bitrateProbe_){bitrateProbe_->disconnect(this);bitrateProbe_->kill();bitrateProbe_->deleteLater();bitrateProbe_=nullptr;}
    }
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(!GlobalMemoryStatusEx(&memory))return;
    const quint64 appBudget=memory.ullTotalPhys*std::clamp(settings_->value("performance/ram",50).toInt(),1,100)/100;
    quint64 bytes=appBudget/2;
    const auto chosen=settings_->value("performance/bufferMiB",0).toULongLong();if(chosen)bytes=qMin(bytes,qMin(chosen,bytes/1048576)*1048576);
    const quint64 frameBytes=quint64(snap.videoWidth)*snap.videoHeight*6; // Conservative 16-bit 4:4:4 estimate; Native charges actual strides/pipeline.
    const quint64 resident=qMin(decodePoolBytes_,quint64(snap.queuedVideoFrames)*frameBytes);
    bytes=qMin(bytes,memory.ullAvailPhys*3/4+resident);
    const quint64 used=usage_.memoryMiB*1048576,other=used>resident?used-resident:0;
    bytes=qMin(bytes,appBudget>other?appBudget-other:0);
    const bool enabled=settings_->value("performance/predecode",true).toBool();
    const bool burst=enabled && snap.position100ns>=decodeBurstFrom_ && snap.position100ns<decodeBurstUntil_;
    const unsigned frames=enabled?(burst?48u:8u):2u;
    const auto pipeline=quint64(qMax(0,media_.value("videoDecoderDelayFrames").toInt()));
    // The configured RAM percentage is a ceiling, never a reservation target.
    bytes=qMin(bytes,frameBytes*(frames+pipeline+1));
    if(burst)bytes=qMin(bytes,quint64(4)*1024*1024*1024);
    if(bytes!=decodePoolBytes_ || frames!=decodePoolFrames_){clock_->setDecodeBuffer(bytes,frames);decodePoolBytes_=bytes;decodePoolFrames_=frames;}
    if(!enabled || bitrateProbe_ || (bitrateTimer_.isValid() && bitrateTimer_.elapsed()<5000))return;
    bitrateTimer_.restart();const auto path=source_;const auto at=snap.position100ns;const auto generation=snap.timelineGeneration;
    const double average=snap.duration100ns>0?double(QFileInfo(path).size())*8/(snap.duration100ns/1e7):double(snap.videoBitRate);
    auto *probe=new QProcess(this);bitrateProbe_=probe;probe->setProperty("source",path);
    connect(probe,&QProcess::readyReadStandardOutput,this,[probe]{auto bytes=probe->property("packets").toByteArray()+probe->readAllStandardOutput();if(bytes.size()>2*1024*1024){probe->kill();return;}probe->setProperty("packets",bytes);});
    connect(probe,&QProcess::finished,this,[this,probe,path,at,average,generation](int code){
        const auto output=probe->property("packets").toByteArray()+probe->readAllStandardOutput();bitrateProbe_=nullptr;probe->deleteLater();if(code || clock_->snapshot().timelineGeneration!=generation || source_!=path || qAbs(position()-at)>40*10000000LL)return;
        QMap<qint64,quint64> seconds;
        for(const auto &line:output.split('\n')){double pts=-1;quint64 size=0;for(const auto &field:line.split('|')){if(field.startsWith("pts_time="))pts=field.mid(9).toDouble();if(field.startsWith("size="))size=field.mid(5).toULongLong();}if(pts>=at/1e7 && pts<=at/1e7+30)seconds[qint64(pts)]+=size;}
        if(average>0)for(auto it=seconds.cbegin();it!=seconds.cend();++it)if(double(it.value())*8>average*2.5){
            const auto begin=qMax(qint64(0),(it.key()-15)*10000000),end=(it.key()+2)*10000000;
            if(end<=position())continue;
            if(decodeBurstUntil_>position() && begin>decodeBurstUntil_)break;
            if(decodeBurstUntil_<=position()){decodeBurstFrom_=begin;decodeBurstUntil_=end;}
            else {decodeBurstFrom_=qMin(decodeBurstFrom_,begin);decodeBurstUntil_=qMax(decodeBurstUntil_,end);}
            // Merge overlapping prefetch windows, including the tail of the
            // same spike. A later disjoint spike is handled by the next probe.
        }
    });
    connect(probe,&QProcess::errorOccurred,this,[this,probe](QProcess::ProcessError error){if(error==QProcess::FailedToStart){bitrateProbe_=nullptr;probe->deleteLater();}});
    QTimer::singleShot(20000,probe,[probe]{if(probe->state()!=QProcess::NotRunning)probe->kill();});
    const auto bundled=QDir(QCoreApplication::applicationDirPath()).filePath("ffprobe.exe");
    QStringList args{"-v","error","-select_streams","v:0","-read_intervals",QString::number(at/1e7,'f',3)+"%+30","-show_packets","-show_entries","packet=pts_time,size,flags","-of","compact=p=0",path};
    probe->start(QFileInfo::exists(bundled)?bundled:QString("ffprobe.exe"),args);
}
}
