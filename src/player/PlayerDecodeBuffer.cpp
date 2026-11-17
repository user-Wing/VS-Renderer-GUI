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
#include <cmath>

namespace vsr {
void PlayerWindow::updateDecodeBuffer(){
    if(bufferTimer_.isValid() && bufferTimer_.elapsed()<500)return;bufferTimer_.restart();
    const auto snap=clock_->snapshot();
    const auto resetPrediction=[&]{
        ++bitrateProbeEpoch_;decodeBurstFrom_=decodeBurstUntil_=0;bitrateTimer_.invalidate();
        if(bitrateProbe_){
            auto *probe=bitrateProbe_;bitrateProbe_=nullptr;probe->disconnect(this);
            if(probe->state()==QProcess::NotRunning)probe->deleteLater();
            else {connect(probe,&QProcess::finished,probe,&QObject::deleteLater);probe->kill();}
        }
    };
    const bool applicable=ready_ && direct_ && !imageMode_ && !minimalMode_ && !networkSource() &&
        snap.videoHeight>=2160 && snap.videoWidth>0 && snap.decodeMode==1 &&
        snap.state==ThreeFpState::Playing && !seekPending_ && snap.playbackRate==1.0;
    if(!applicable || snap.timelineGeneration!=decodePoolGeneration_ || snap.selectedVideoStream!=decodePoolStream_){
        resetPrediction();decodePoolStream_=snap.selectedVideoStream;
        decodePoolGeneration_=snap.timelineGeneration;
    }
    if(!applicable){
        if((decodePoolBytes_ || decodePoolFrames_) && clock_->setDecodeBuffer(0,0)){decodePoolBytes_=0;decodePoolFrames_=0;}
        return;
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
    if(!enabled || !bytes)resetPrediction();
    const bool burst=enabled && snap.position100ns>=decodeBurstFrom_ && snap.position100ns<decodeBurstUntil_;
    const unsigned frames=enabled?(burst?48u:8u):2u;
    const auto pipeline=quint64(qMax(0,media_.value("videoDecoderDelayFrames").toInt()));
    // The configured RAM percentage is a ceiling, never a reservation target.
    bytes=qMin(bytes,frameBytes*(frames+pipeline+1));
    if(burst)bytes=qMin(bytes,quint64(4)*1024*1024*1024);
    // Older DLLs interpret zero bytes as the default pool. One byte gives them
    // their minimum demand queue without accidentally enabling a large pool.
    if(!bytes && media_.value("decodeBufferPolicyVersion").toInt()<2)bytes=1;
    if(bytes!=decodePoolBytes_ || frames!=decodePoolFrames_){
        if(clock_->setDecodeBuffer(bytes,frames)){decodePoolBytes_=bytes;decodePoolFrames_=frames;}
    }
    if(!enabled || !bytes || !media_.contains("timelineOrigin100ns") || snap.selectedVideoStream<0 ||
        bitrateProbe_ || (bitrateTimer_.isValid() && bitrateTimer_.elapsed()<5000))return;
    bitrateTimer_.restart();const auto path=source_;const auto at=snap.position100ns;const auto generation=snap.timelineGeneration;
    const auto stream=snap.selectedVideoStream;const auto epoch=bitrateProbeEpoch_;
    const double origin=media_.value("timelineOrigin100ns").toDouble()/1e7;
    const double average=double(snap.videoBitRate);
    if(!std::isfinite(origin) || average<=0)return;
    auto *probe=new QProcess(this);bitrateProbe_=probe;probe->setProperty("source",path);
    const auto appendOutput=[probe]{
        const auto chunk=probe->readAllStandardOutput();auto packets=probe->property("packets").toByteArray();
        if(probe->property("rejected").toBool() || chunk.size()>2*1024*1024-packets.size()){
            probe->setProperty("rejected",true);probe->kill();return;
        }
        packets+=chunk;probe->setProperty("packets",packets);
    };
    connect(probe,&QProcess::readyReadStandardOutput,this,appendOutput);
    connect(probe,&QProcess::readyReadStandardError,this,[probe]{
        const auto bytes=probe->property("errorBytes").toLongLong()+probe->readAllStandardError().size();
        probe->setProperty("errorBytes",bytes);if(bytes>65536){probe->setProperty("rejected",true);probe->kill();}
    });
    connect(probe,&QProcess::finished,this,[this,probe,path,at,average,generation,stream,epoch,origin,appendOutput](int code,QProcess::ExitStatus status){
        appendOutput();const auto output=probe->property("packets").toByteArray();
        const bool current=bitrateProbe_==probe;if(current)bitrateProbe_=nullptr;probe->deleteLater();
        const auto now=clock_->snapshot();
        if(!current || code || status!=QProcess::NormalExit || probe->property("rejected").toBool() ||
            bitrateProbeEpoch_!=epoch || now.timelineGeneration!=generation || now.selectedVideoStream!=stream ||
            now.state!=ThreeFpState::Playing || now.playbackRate!=1.0 || seekPending_ || source_!=path ||
            qAbs(double(now.position100ns)-double(at))>40*1e7)return;
        QMap<qint64,quint64> seconds;
        for(const auto &line:output.split('\n')){
            double pts=-1;quint64 size=0;bool timeOk=false,sizeOk=false;
            for(const auto &field:line.split('|')){if(field.startsWith("pts_time="))pts=field.mid(9).toDouble(&timeOk)-origin;if(field.startsWith("size="))size=field.mid(5).toULongLong(&sizeOk);}
            if(timeOk && sizeOk && std::isfinite(pts) && pts>=0 && pts>=at/1e7 && pts<=at/1e7+30 && pts<9e11 &&
                size<=quint64(INT_MAX) && seconds[qint64(pts)]<=UINT64_MAX-size)seconds[qint64(pts)]+=size;
        }
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
    connect(probe,&QProcess::errorOccurred,this,[this,probe](QProcess::ProcessError error){if(error==QProcess::FailedToStart){if(bitrateProbe_==probe)bitrateProbe_=nullptr;probe->deleteLater();}});
    QTimer::singleShot(20000,probe,[probe]{if(probe->state()!=QProcess::NotRunning){probe->setProperty("rejected",true);probe->kill();}});
    const auto bundled=QDir(QCoreApplication::applicationDirPath()).filePath("ffprobe.exe");
    QStringList args{"-v","error","-select_streams",QString::number(stream),"-read_intervals",QString::number(at/1e7+origin,'f',3)+"%+30","-show_packets","-show_entries","packet=pts_time,size","-of","compact=p=0",path};
    probe->start(QFileInfo::exists(bundled)?bundled:QString("ffprobe.exe"),args);
}
}
