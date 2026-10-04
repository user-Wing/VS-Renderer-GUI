#include "player/PlayerWindow.h"
#include "player/PlayerTracks.h"
#include "graph/VpyScriptBuilder.h"
#include "graph/FilterCatalog.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/LavPlayback.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTemporaryDir>
#include <QTest>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QSet>
#include <QUuid>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTreeWidget>
#include <QCursor>
#include <QUrl>
#include "player/PlayerSubtitles.h"
#include "ui/PreviewPane.h"
#include "player/PlayerNetworkInput.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSplitter>
#include <QWheelEvent>
#include <QScopeGuard>
#include <QTimer>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include "player/PlayerCache.h"
#include "player/PlayerLanguage.h"
#include "player/PlayerAssociations.h"
#include "player/PlayerImage.h"
#include "player/PlayerImageTools.h"
#include <QToolButton>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include "player/PlayerMediaMatching.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QImageReader>
#include <QColorSpace>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFontInfo>
#include <QTextLayout>
#include "player/PlayerMenu.h"
#include <windows.h>
#include <cstdio>
using namespace vsr;
class TestPlayer final : public QObject {
    qint64 beforePauseRate_ = 0;
    QByteArray oldIni_; bool hadIni_=false;
    QString configPath() const {return QDir(QCoreApplication::applicationDirPath()).filePath("player.ini");}
    Q_OBJECT
private slots:
    void directOpenWithoutIndex_data() {
        QTest::addColumn<int>("stage");
        QTest::newRow("original")<<-1;QTest::newRow("jinc")<<4;QTest::newRow("d3d11")<<5;
    }
    void directOpenWithoutIndex() {
        QFETCH(int,stage);QTemporaryDir directory;
        QString source=qEnvironmentVariable("VSR_DIRECT_OPEN_SOURCE");
        if(source.isEmpty()) {
            source=directory.filePath("direct.mkv");QProcess encode;
            encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-f","lavfi","-i","testsrc2=size=320x180:rate=48:duration=3","-c:v","ffv1",source});
            QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        }
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("cache/path",directory.filePath("cache"));settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.setValue("decode/video","3FP");settings.setValue("decode/audio","3FP");settings.sync();
        PlayerWindow player;player.show();player.loadPreset(stage<0?QString():QDir(PresetStore::directory()).filePath(QString("builtin/Anime-%1-%2.vpy").arg(stage).arg(stage==4?"Jinc":"D3D11")));
        QSignalSpy scripts(player.server_.get(),&VapourSynthFrameServer::scriptLoaded);
        QElapsedTimer timer;timer.start();QVERIFY(player.openFile(source));
        QVERIFY(player.deferred_.isEmpty());QCOMPARE(player.source_,QFileInfo(source).absoluteFilePath());QVERIFY(player.direct_);
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);
        qInfo()<<"direct-open"<<stage<<"first-frame-ms"<<timer.elapsed()<<"source-bytes"<<QFileInfo(source).size()<<"decode-mode"<<player.snapshot().decodeMode;
        QTest::qWait(150);QCOMPARE(scripts.count(),0);QVERIFY(player.direct_);QCOMPARE(playerCacheBytes(playerCacheDirectory(settings)),0);
    }
    void directNativePerformance() {
        const auto source=qEnvironmentVariable("VSR_PERF_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_PERF_SOURCE for real-media playback measurements.");
        const int seconds=qEnvironmentVariableIntValue("VSR_PERF_SECONDS")>0?qEnvironmentVariableIntValue("VSR_PERF_SECONDS"):60;
        using LogCallback=void(*)(void *,const char *) noexcept;using SetLog=void(*)(LogCallback,void *) noexcept;
        const auto setLog=reinterpret_cast<SetLog>(QLibrary::resolve(qEnvironmentVariable("VSR_3FP_DLL"),"FFF3FP_SetLogCallback"));
        FILE *log=nullptr;
        if(qEnvironmentVariable("VSR_3FP_PROFILE")=="1"){log=std::fopen((qEnvironmentVariable("VSR_PERF_CSV")+".native.log").toUtf8().constData(),"w");QVERIFY(log);QVERIFY(setLog);setLog([](void *context,const char *line) noexcept {std::fprintf(static_cast<FILE *>(context),"%s\n",line);},log);}
        const auto closeLog=qScopeGuard([&]{if(log){setLog(nullptr,nullptr);std::fclose(log);}});
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.setValue("subtitle/visible",true);settings.setValue("decode/mode",2);settings.setValue("player/speed",1.0);settings.sync();
        PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
        auto *surface=player.pane_->surface();const double dpi=surface->devicePixelRatioF();surface->setFixedSize(qRound(3840/dpi),qRound(2160/dpi));
        bool visibleSubtitle=false;
        connect(player.subtitles_.get(),&PlayerSubtitles::imageReady,&player,[&](const QImage &image){
            if(visibleSubtitle || image.isNull())return;
            for(int y=0;y<image.height();y+=4){const auto *row=reinterpret_cast<const QRgb *>(image.constScanLine(y));for(int x=0;x<image.width();x+=4)if(qAlpha(row[x])){visibleSubtitle=true;return;}}
        });
        QElapsedTimer cold;cold.start();QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().swapChainPresents>0 && player.outputSnapshot().presentedVideoFrames>0,20000);const auto firstFrameMs=cold.elapsed();
        player.seekTime(1200000000);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().timelineGeneration>0 && player.position()>=1200000000,20000);
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,5000);QTest::qWait(5000);
        QFile csv(qEnvironmentVariable("VSR_PERF_CSV"));QVERIFY(csv.open(QIODevice::WriteOnly|QIODevice::Truncate));
        csv.write("wall_s,position_s,decoded,accepted,dropped,coalesced,presents,audio_underruns,width,height,decode_mode,scaling_mode,subtitle_visible,first_frame_ms,seek_generation,frame_pts\n");
        const auto first=player.outputSnapshot();QElapsedTimer timer;timer.start();
        for(int sample=0;sample<=seconds;++sample){
            if(sample)QTest::qWait(qMax(0,sample*1000-int(timer.elapsed())));
            RECT rect{};QVERIFY(GetClientRect(reinterpret_cast<HWND>(surface->winId()),&rect));QCOMPARE(rect.right-rect.left,3840L);QCOMPARE(rect.bottom-rect.top,2160L);
            const auto s=player.outputSnapshot();QVERIFY(player.direct_);QCOMPARE(s.state,ThreeFpState::Playing);QCOMPARE(s.decodeMode,2u);QCOMPARE(s.videoScalingMode,1u);
            csv.write(QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16\n").arg(timer.elapsed()/1000.0,0,'f',6).arg(s.position100ns/1e7,0,'f',6).arg(s.decodedVideoFrames).arg(s.presentedVideoFrames).arg(s.droppedVideoFrames).arg(s.coalescedVideoFrames).arg(s.swapChainPresents).arg(s.audioUnderruns).arg(rect.right-rect.left).arg(rect.bottom-rect.top).arg(s.decodeMode).arg(s.videoScalingMode).arg(visibleSubtitle?1:0).arg(firstFrameMs).arg(s.timelineGeneration).arg(s.framePts).toUtf8());csv.flush();
        }
        const auto last=player.outputSnapshot();
        qInfo()<<"native-performance"<<source<<"drops"<<last.droppedVideoFrames-first.droppedVideoFrames<<"coalesced"<<last.coalescedVideoFrames-first.coalescedVideoFrames<<"subtitle"<<visibleSubtitle;
        QVERIFY(qAbs(last.position100ns-first.position100ns-seconds*10000000LL)<2000000);
        QCOMPARE(last.droppedVideoFrames,first.droppedVideoFrames);QCOMPARE(last.coalescedVideoFrames,first.coalescedVideoFrames);
        QCOMPARE(last.audioUnderruns,first.audioUnderruns);
        QCOMPARE(last.timelineGeneration,first.timelineGeneration);
        QVERIFY(player.clip_.fpsDenominator>0);
        QVERIFY(qAbs(double(last.presentedVideoFrames-first.presentedVideoFrames)-seconds*double(player.clip_.fpsNumerator)/player.clip_.fpsDenominator)<3);
        QVERIFY(last.swapChainPresents-first.swapChainPresents+2>=last.presentedVideoFrames-first.presentedVideoFrames);
        if(player.primarySubtitle_>=0)QVERIFY(visibleSubtitle);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(500);const auto paused=player.outputSnapshot();QTest::qWait(1000);QCOMPARE(player.outputSnapshot().presentedVideoFrames,paused.presentedVideoFrames);
        const auto image=player.clock_->capture();QVERIFY(!image.isNull());QSet<QRgb> colors;for(int y=40;y<image.height()-40;y+=40)for(int x=40;x<image.width()-40;x+=40)colors.insert(image.pixel(x,y));QVERIFY(colors.size()>30);
        QVERIFY(image.save(qEnvironmentVariable("VSR_PERF_CSV")+".png"));
    }
    void hardwareNativeAndShaderCapture() {
        const auto source=qEnvironmentVariable("VSR_HARDWARE_CAPTURE_SOURCE");if(source.isEmpty())QSKIP("Set VSR_HARDWARE_CAPTURE_SOURCE for decoder-surface capture checks.");
        ThreeFpApi api;QWidget surface;surface.resize(960,540);surface.show();ThreeFpPlayer native(api,&surface);QVERIFY(native.setDecodeMode(2));
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));QVERIFY(native.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(native.snapshot().state,ThreeFpState::Ready,20000);
        const auto generation=native.snapshot().timelineGeneration;QVERIFY(native.seek(1200000000));QTRY_VERIFY_WITH_TIMEOUT(native.snapshot().timelineGeneration>generation && native.snapshot().swapChainPresents>0,10000);QTest::qWait(500);
        const auto frame=native.snapshot().frameIndex;
        const auto capture=[&]{const auto image=native.capture();QSet<QRgb> colors;for(int y=20;y<image.height()-20;y+=20)for(int x=20;x<image.width()-20;x+=20)colors.insert(image.pixel(x,y));return colors.size()>30?image:QImage{};};
        const auto original=capture();QVERIFY(!original.isNull());QCOMPARE(native.snapshot().videoScalingMode,1u);
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::Jinc2,ThreeFpScalingAlgorithm::Jinc2));QTest::qWait(300);QVERIFY(!capture().isNull());QCOMPARE(native.snapshot().videoScalingMode,0u);
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));native.setView(2,.1f,.1f);QTest::qWait(300);const auto zoomed=capture();QVERIFY(!zoomed.isNull());QVERIFY(zoomed!=original);
        native.setView(1,0,0);QTest::qWait(300);QVERIFY(!capture().isNull());QCOMPARE(native.snapshot().videoScalingMode,1u);QCOMPARE(native.snapshot().frameIndex,frame);
    }
    void mkvTrackNamesAndDefaults() {
        QTemporaryDir directory;QFile caption(directory.filePath("caption.srt"));QVERIFY(caption.open(QIODevice::WriteOnly));caption.write("1\n00:00:00,000 --> 00:00:02,000\nTrack sample\n");caption.close();
        const auto source=directory.filePath("sample.mkv");QProcess encode;encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-f","lavfi","-i","testsrc2=size=128x96:rate=24:duration=2","-f","lavfi","-i","sine=frequency=500:duration=2","-i",caption.fileName(),"-map","0:v","-map","1:a","-map","1:a","-map","1:a","-map","2:s","-map","2:s","-map","2:s","-c:v","ffv1","-c:a","pcm_s16le","-c:s","srt","-metadata:s:a:0","title=粤语","-metadata:s:a:0","language=yue","-metadata:s:a:1","title=中文","-metadata:s:a:1","language=zho","-metadata:s:s:1","title=简体字幕","-metadata:s:s:1","language=zho","-disposition:a:0","forced","-disposition:a:1","default","-disposition:a:2","default+forced","-disposition:s:0","forced","-disposition:s:1","default+forced","-disposition:s:2","default",source});QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("decode/audio","3FP");settings.setValue("decode/video","3FP");settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));settings.sync();
        PlayerWindow player;player.show();QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_,10000);QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().selectedAudioStream,2,5000);QCOMPARE(player.primarySubtitle_,5);
        const auto streams=player.media_.value("streams").toArray();QCOMPARE(playerTrackLabel(streams[1].toObject(),0),QString("Track 0:a:0 - 粤语 - yue - Forced"));QCOMPARE(playerTrackLabel(streams[5].toObject(),1),QString("Track 0:s:1 - 简体字幕 - zho - Default+Forced"));
        auto flagsOff=streams;for(int i=0;i<flagsOff.size();++i){auto item=flagsOff[i].toObject();item["default"]=false;flagsOff[i]=item;}QCOMPARE(playerDefaultTrack(flagsOff,"audio"),1);QCOMPARE(playerDefaultTrack(flagsOff,"subtitle"),4);QCOMPARE(playerDefaultTrack(QJsonArray{},"subtitle"),-1);
        player.selectSubtitle(0,4);QCOMPARE(player.primarySubtitle_,4);player.selectAudio(1);QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().selectedAudioStream,1,5000);
        player.showContextMenu(QPoint(20,20));auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);bool audioLabel=false,subtitleLabel=false;for(auto *list:menu->findChildren<QMenu *>())for(auto *action:list->actions()){audioLabel|=action->text()=="Track 0:a:0 - 粤语 - yue - Forced";subtitleLabel|=action->text()=="Track 0:s:1 - 简体字幕 - zho - Default+Forced";}QVERIFY(audioLabel);QVERIFY(subtitleLabel);menu->close();
        const auto noDefault=directory.filePath("no-default.mkv");encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-i",source,"-map","0","-c","copy","-disposition:a","0","-disposition:s","0","-disposition:a:0","forced","-disposition:s:0","forced",noDefault});QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        QVERIFY(player.openFile(noDefault));QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_,10000);QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().selectedAudioStream,1,5000);QCOMPARE(player.primarySubtitle_,4);
        for(const auto &entry:player.media_.value("streams").toArray())if(entry.toObject().value("type").toString()!="video")QVERIFY(!entry.toObject().value("default").toBool());
    }
    void compressedShaderPipeline() {
        const QDir runtime(QCoreApplication::applicationDirPath());const auto shader="mpv-shaders/AN3223--dotfiles/.config/mpv/shaders/HQ/nlmeans_light.glsl";
        QVERIFY(QFileInfo::exists(runtime.filePath("shaders/mpv-shaders.7z")));QVERIFY(!QFileInfo::exists(runtime.filePath("shaders/"+QString(shader))));
        const auto *definition=FilterCatalog::find("anime4k");QVERIFY(definition);bool listed=false;for(const auto &parameter:definition->parameters)if(parameter.id=="mode"){listed=parameter.choices.contains(shader);QVERIFY(parameter.choices.size()>=915);}QVERIFY(listed);
        FilterGraph graph;const int row=graph.add("anime4k");graph.setParameter(row,"mode",shader);graph.setParameter(row,"scale","1×");
        const auto result=VpyScriptBuilder::build("fixture.mkv",SourceFilter::Ffms2,graph);QVERIFY2(result.errors.isEmpty(),qPrintable(result.errors.join('\n')));
        QString script=result.script;script.replace(QRegularExpression("src = core\\.ffms2\\.Source[^\\n]*"),"src = core.std.BlankClip(width=64, height=48, format=vs.YUV444P16, length=1)");
        VapourSynthFrameServer server;QSignalSpy errors(&server,&VapourSynthFrameServer::errorOccurred),loaded(&server,&VapourSynthFrameServer::scriptLoaded),frames(&server,&VapourSynthFrameServer::frameReady);QTRY_VERIFY_WITH_TIMEOUT(server.available(),10000);server.loadScript(script,runtime.filePath("compressed-shader-test.vpy"));QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),20000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));server.requestFrame(0);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),10000);QVERIFY(errors.isEmpty());QVERIFY(!frames.isEmpty());
    }
    void enhancedFfmpegAvs3Playback() {
        QTemporaryDir directory;const QString source=directory.filePath("sample.avs3");QProcess encode;
        encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-f","lavfi","-i","testsrc2=size=128x128:rate=24","-frames:v","4","-an","-c:v","libuavs3e","-pix_fmt","yuv420p10le","-threads","2","-speed_level","3",source});
        QVERIFY(encode.waitForFinished(30000));QCOMPARE(encode.exitStatus(),QProcess::NormalExit);QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        ThreeFpApi api;PreviewPane pane("AVS3","Test");pane.resize(480,320);pane.setSurfaceActive(true);pane.show();QVERIFY(QTest::qWaitForWindowExposed(&pane));ThreeFpPlayer player(api,pane.surface());
        QVERIFY(player.ready());QVERIFY(player.setDecodeMode(1));QVERIFY2(player.openFile(source),qPrintable(player.lastError()));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,10000);QVERIFY(player.play());
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);QCOMPARE(player.snapshot().videoWidth,128u);QCOMPARE(player.snapshot().videoHeight,128u);
    }
    void lanczos4NativeRender() {
        ThreeFpApi api;PreviewPane pane("Lanczos4","Test");pane.resize(480,320);pane.setSurfaceActive(true);pane.show();QVERIFY(QTest::qWaitForWindowExposed(&pane));ThreeFpPlayer player(api,pane.surface());QVERIFY(player.ready());
        VapourSynthFrame frame;frame.width=64;frame.height=48;frame.format=ThreeFpExternalPixelFormat::Yuv444P16;frame.colorRange=2;frame.colorMatrix=1;frame.totalFrames=1;frame.duration100ns=400000;
        for(int p=0;p<3;++p){frame.planes[p].resize(64*48*2);frame.strides[p]=128;auto *samples=reinterpret_cast<quint16 *>(frame.planes[p].data());for(int y=0;y<48;++y)for(int x=0;x<64;++x)samples[y*64+x]=p==0?((x/3+y/3)%2?14000:50000):32768;}
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Lanczos4,ThreeFpScalingAlgorithm::Lanczos4));QVERIFY(player.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,5000);ThreeFpPixelProbe pixel{};QVERIFY(player.samplePixel(150,150,pixel));QVERIFY(std::isfinite(pixel.red));
        frame.width=1280;frame.height=960;for(int p=0;p<3;++p){frame.planes[p]=QByteArray(1280*960*2,char(128));frame.strides[p]=2560;}QVERIFY(pane.surface()->width()<frame.width);const auto before=player.snapshot().swapChainPresents;QVERIFY(player.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>before,5000);
    }
    void imageRefinementAlgorithms() {
        QTemporaryDir directory;QImage image(32,24,QImage::Format_ARGB32);for(int y=0;y<24;++y)for(int x=0;x<32;++x)image.setPixelColor(x,y,QColor(x%4<2?0:220,y%4<2?0:200,80,128));image.setColorSpace(QColorSpace::SRgb);
        ImageOutput output;output.image=image;output.source=directory.filePath("input.png");QVERIFY(image.save(output.source));output.size=QSize(64,48);output.destination=directory.filePath("output.png");
        QImage jinc,lanczos;for(const auto &algorithm:QStringList{"jinc","lanczos3","lanczos4","anime4k"}){output.resizeAlgorithm=algorithm;const auto error=PlayerImageTools::writeOutput(output);QVERIFY2(error.isEmpty(),qPrintable(algorithm+": "+error));const QImage scaled(output.destination);QCOMPARE(scaled.size(),output.size);QVERIFY(std::abs(scaled.pixelColor(30,20).alpha()-128)<2);QVERIFY(scaled.colorSpace().isValid());if(algorithm=="jinc")jinc=scaled;if(algorithm=="lanczos4")lanczos=scaled;}
        QVERIFY(jinc!=lanczos);output.resizeAlgorithm="lanczos4";output.size=QSize(13,9);QCOMPARE(PlayerImageTools::writeOutput(output),QString());QCOMPARE(QImage(output.destination).size(),output.size);
        QImage wide(20000,2,QImage::Format_RGB32);wide.fill(Qt::cyan);const auto small=PlayerImageTools::resample(wide,QSize(37,3),"jinc");QCOMPARE(small.size(),QSize(37,3));QCOMPARE(small.pixelColor(18,1),QColor(Qt::cyan));
        QImage precise(11,9,QImage::Format_RGBA64);precise.setColorSpace(QColorSpace::SRgb);precise.fill(QColor::fromRgbF(.02,.25,.75,.5));for(const auto &algorithm:QStringList{"jinc","lanczos4"}){const auto resized=PlayerImageTools::resample(precise,QSize(19,17),algorithm);QCOMPARE(resized.depth(),64);QVERIFY(resized.colorSpace().isValid());QVERIFY(std::abs(resized.pixelColor(9,8).alphaF()-.5)<1.0/65535);}
        output.format="avif";for(int bits:{8,10,12,16}){output.image.setText("sourceBitDepth",QString::number(bits));auto args=PlayerImageTools::conversionArguments(output,output.source,directory.path());QCOMPARE(args[args.indexOf("--bit-depth")+1],bits<=10?QString("10"):QString("12"));}
    }
    void imageRefinementCrop() {
        QTemporaryDir directory;QImage image(400,300,QImage::Format_RGB32);image.fill(Qt::cyan);const auto source=directory.filePath("source.png");QVERIFY(image.save(source));PlayerWindow player;player.show();QVERIFY(player.openFile(source));auto *pane=player.findChild<PreviewPane *>();QTRY_VERIFY(!pane->image().isNull());auto *tools=player.findChild<PlayerImageTools *>();QCOMPARE(tools->height(),38);
        tools->findChild<QToolButton *>("imageTool_crop")->click();auto *view=player.findChild<QWidget *>("imageCropView");auto *ratio=player.findChild<QComboBox *>("imageCropRatio");QVERIFY(view);QCOMPARE(ratio->count(),7);ratio->setCurrentIndex(2);auto selection=view->property("selection").toRect();QVERIFY(std::abs(double(selection.width())/selection.height()-16.0/9)<.01);
        const auto screenRect=[&]{const auto fitted=image.size().scaled(view->size(),Qt::KeepAspectRatio);return QRect(QPoint((view->width()-fitted.width())/2,(view->height()-fitted.height())/2),fitted);};
        const auto point=[&](QPoint p){const auto r=screenRect();return QPoint(r.x()+qRound(double(p.x())/image.width()*r.width()),r.y()+qRound(double(p.y())/image.height()*r.height()));};
        ratio->setCurrentIndex(1);selection=view->property("selection").toRect();const auto center=point(selection.center());QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,center);QTest::mouseMove(view,center+QPoint(35,0));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,center+QPoint(35,0));auto moved=view->property("selection").toRect();QCOMPARE(moved.size(),selection.size());QVERIFY(moved.x()>selection.x());
        const auto corner=point(QPoint(moved.x()+moved.width(),moved.y()+moved.height()));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,corner);QTest::mouseMove(view,corner-QPoint(40,40));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,corner-QPoint(40,40));const auto resized=view->property("selection").toRect();QVERIFY(resized.width()<moved.width());QCOMPARE(resized.width(),resized.height());
        ratio->setCurrentIndex(6);player.findChild<QSpinBox *>("imageCropRatioWidth")->setValue(7);player.findChild<QSpinBox *>("imageCropRatioHeight")->setValue(5);selection=view->property("selection").toRect();QVERIFY(std::abs(double(selection.width())/selection.height()-1.4)<.025);view->window()->grab().save("build/image-refined-crop.png");view->window()->close();
    }
    void interpolationLowestWarning() {
        PlayerWindow player;player.show();player.pane_->setSurfaceActive(true);player.timer_->stop();player.setInterpolation(3,true);player.ready_=player.playing_=true;player.qualitySettling_.start();QTest::qWait(2050);player.updateProfile();QTest::qWait(5050);player.submittedFrames_=100;player.skippedFrames_=20;player.updateProfile();QCOMPARE(player.interpolationStage(),3);QVERIFY(player.findChild<QLabel *>("playerInterpolationWarning")->isVisible());
        player.qualityTimer_.invalidate();player.updateProfile();QTest::qWait(5050);player.submittedFrames_+=100;player.updateProfile();QVERIFY(!player.findChild<QLabel *>("playerInterpolationWarning")->isVisible());
        QVERIFY(player.clock_->setScalingAlgorithms(ThreeFpScalingAlgorithm::Lanczos4,ThreeFpScalingAlgorithm::Lanczos4));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/interpolationStart",2);settings.sync();player.source_.clear();player.imageMode_=false;player.setInterpolation(0,true);QTemporaryDir dir;QImage image(16,16,QImage::Format_RGB32);image.fill(Qt::red);const auto path=dir.filePath("source.png");QVERIFY(image.save(path));QVERIFY(player.openFile(path));QCOMPARE(player.interpolationStage(),2);
    }
    void imageToolsBackgroundSave() {
        const bool nativeDisabled=qApp->testAttribute(Qt::AA_DontUseNativeDialogs);qApp->setAttribute(Qt::AA_DontUseNativeDialogs,true);const auto restore=qScopeGuard([=]{qApp->setAttribute(Qt::AA_DontUseNativeDialogs,nativeDisabled);});
        QTemporaryDir directory;QImage image(120,80,QImage::Format_RGB32);image.fill(Qt::cyan);const auto source=directory.filePath("source.png"),target=directory.filePath("另存.png");QVERIFY(image.save(source));
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();QVERIFY(player.openFile(source));QTRY_VERIFY(!pane->image().isNull());auto *tools=player.findChild<PlayerImageTools *>();tools->findChild<QToolButton *>("imageTool_resize")->click();
        auto *width=player.findChild<QSpinBox *>("imageResizeWidth");QVERIFY(width);width->setValue(60);auto *dialog=qobject_cast<QDialog *>(width->window());QVERIFY(dialog);auto *buttons=dialog->findChild<QDialogButtonBox *>();QVERIFY(buttons);
        bool selected=false;QTimer::singleShot(0,this,[&]{for(auto *widget:qApp->topLevelWidgets())if(auto *file=qobject_cast<QFileDialog *>(widget);file && file->isVisible()){file->selectFile(target);selected=true;QMetaObject::invokeMethod(file,"accept");}});
        buttons->button(QDialogButtonBox::Save)->click();QVERIFY(selected);QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(target),10000);QTRY_VERIFY(tools->findChild<QToolButton *>("imageTool_resize")->isEnabled());QCOMPARE(QImage(target).size(),QSize(60,40));QCOMPARE(QImage(source),image);QCOMPARE(pane->image().size(),image.size());
    }
    void imageToolsViewAndDialogs() {
        QTemporaryDir directory;QImage image(120,80,QImage::Format_RGB32);for(int y=0;y<80;++y)for(int x=0;x<120;++x)image.setPixelColor(x,y,y<40?(x<60?Qt::red:Qt::green):(x<60?Qt::blue:Qt::yellow));const auto source=directory.filePath("工具测试.png");QVERIFY(image.save(source));
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();auto *tools=player.findChild<PlayerImageTools *>();QVERIFY(tools);QVERIFY(!tools->isVisible());QVERIFY(player.openFile(source));QTRY_VERIFY(!pane->image().isNull());QVERIFY(tools->isVisible());
        const QStringList names{"edit","rotateLeft","rotateRight","mirror","recycle","wallpaper","resize","crop","convert"};int previous=-1;
        for(const auto &name:names){auto *button=tools->findChild<QToolButton *>("imageTool_"+name);QVERIFY(button);QVERIFY(!button->icon().isNull());QVERIFY(!button->toolTip().isEmpty());QCOMPARE(button->toolButtonStyle(),Qt::ToolButtonIconOnly);QVERIFY(button->x()>previous);previous=button->x();QVERIFY(button->isEnabled());}
        const auto *pixels=pane->image().constBits();tools->findChild<QToolButton *>("imageTool_rotateRight")->click();QCOMPARE(pane->imageDisplaySize(),QSize(80,120));QCOMPARE(pane->image().constBits(),pixels);
        const auto corner=[pane](double x,double y){const auto fitted=pane->imageDisplaySize().scaled(pane->surface()->size(),Qt::KeepAspectRatio);const auto shot=pane->surface()->grab().toImage();const auto dpr=shot.devicePixelRatio();return shot.pixelColor(qRound(((pane->surface()->width()-fitted.width())/2+fitted.width()*x)*dpr),qRound(((pane->surface()->height()-fitted.height())/2+fitted.height()*y)*dpr));};
        QCOMPARE(corner(.25,.25),QColor(Qt::blue));QCOMPARE(corner(.75,.25),QColor(Qt::red));QCOMPARE(corner(.25,.75),QColor(Qt::yellow));QCOMPARE(corner(.75,.75),QColor(Qt::green));
        pane->adoptView(10000,1,1);QCOMPARE(pane->surface()->grab().toImage().pixelColor(pane->surface()->width()*pane->devicePixelRatioF()/2,pane->surface()->height()*pane->devicePixelRatioF()/2),QColor(Qt::blue));pane->adoptView(1,0,0);
        tools->findChild<QToolButton *>("imageTool_rotateLeft")->click();QVERIFY(pane->imageTransform().isIdentity());tools->findChild<QToolButton *>("imageTool_mirror")->click();QCOMPARE(pane->imageDisplaySize(),image.size());QCOMPARE(pane->image().constBits(),pixels);
        tools->findChild<QToolButton *>("imageTool_resize")->click();auto *width=player.findChild<QSpinBox *>("imageResizeWidth");auto *height=player.findChild<QSpinBox *>("imageResizeHeight");QVERIFY(width);QVERIFY(height);width->setValue(60);QCOMPARE(height->value(),40);height->setValue(20);QCOMPARE(width->value(),30);qobject_cast<QDialog *>(width->window())->close();
        tools->findChild<QToolButton *>("imageTool_crop")->click();auto *crop=player.findChild<QWidget *>("imageCropView");QVERIFY(crop);QTest::mousePress(crop,Qt::LeftButton,Qt::NoModifier,QPoint(80,80));QTest::mouseMove(crop,QPoint(250,220));QTest::mouseRelease(crop,Qt::LeftButton,Qt::NoModifier,QPoint(250,220));crop->window()->grab().save("build/image-tools-crop.png");crop->window()->close();
        tools->findChild<QToolButton *>("imageTool_convert")->click();auto *format=player.findChild<QComboBox *>("imageConvertFormat");auto *mode=player.findChild<QComboBox *>("imageConvertMode");QVERIFY(format);QCOMPARE(format->count(),5);QCOMPARE(mode->count(),2);format->setCurrentIndex(format->findData("png"));QVERIFY(!mode->isEnabled());QCOMPARE(player.findChild<QSpinBox *>("imageConvertQuality")->value(),100);format->window()->grab().save("build/image-tools-convert.png");format->window()->close();
        player.grab().save("build/image-tools-toolbar.png");
    }
    void imageToolsExportTransforms() {
        QTemporaryDir directory;QImage image(12,8,QImage::Format_ARGB32);for(int y=0;y<8;++y)for(int x=0;x<12;++x)image.setPixelColor(x,y,QColor(x*20,y*30,100,200));image.setColorSpace(QColorSpace::SRgb);
        ImageOutput output;output.image=image;output.source=directory.filePath("source.png");QVERIFY(image.save(output.source));output.destination=directory.filePath("rotated.png");output.transform=QTransform(0,1,-1,0,0,0);
        QCOMPARE(PlayerImageTools::writeOutput(output),QString());QImage rotated(output.destination);QCOMPARE(rotated,image.transformed(output.transform));
        output.crop=QRect(1,2,4,6);output.destination=directory.filePath("crop.png");QCOMPARE(PlayerImageTools::writeOutput(output),QString());QImage cropped(output.destination);QCOMPARE(cropped,rotated.copy(output.crop));QVERIFY(cropped.colorSpace().isValid());
        output.size=QSize(2,3);output.destination=directory.filePath("resize.png");QCOMPARE(PlayerImageTools::writeOutput(output),QString());QImage resized(output.destination);QCOMPARE(resized.size(),QSize(2,3));QCOMPARE(QImage(output.source),image);
        output.destination=output.source;QVERIFY(!PlayerImageTools::writeOutput(output).isEmpty());QCOMPARE(QImage(output.source),image);
        output.destination=directory.filePath("rotated.png");output.format="invalid";QVERIFY(!PlayerImageTools::writeOutput(output).isEmpty());QCOMPARE(QImage(output.destination),rotated);
    }
    void imageToolsNativeConversion_data() {
        QTest::addColumn<QString>("format");QTest::addColumn<bool>("visual");
        for(const auto &format:QStringList{"avif","webp","jxl","jpgli","png"})QTest::newRow(qPrintable(format))<<format<<false;
        QTest::newRow("webp-visual")<<QString("webp")<<true;
    }
    void imageToolsNativeConversion() {
        QFETCH(QString,format);QFETCH(bool,visual);QTemporaryDir directory;QImage image(128,96,QImage::Format_RGB32);for(int y=0;y<96;++y)for(int x=0;x<128;++x)image.setPixelColor(x,y,QColor(x*2,y*2,96));
        ImageOutput output;output.image=image;output.source=directory.filePath("源 图片.png");QVERIFY(image.save(output.source));output.destination=directory.filePath("转换."+(format=="jpgli"?QString("jpg"):format));output.format=format;output.speed=10;output.quality=visual?75:100;output.visualQuality=visual;
        const auto args=PlayerImageTools::conversionArguments(output,output.source,directory.path());if(format=="avif"){QVERIFY(args.contains("yuv"));QVERIFY(args.contains("420"));}
        const auto error=PlayerImageTools::writeOutput(output);QVERIFY2(error.isEmpty(),qPrintable(error));QVERIFY(QFileInfo(output.destination).size()>0);QCOMPARE(QImage(output.source),image);
        PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded),failed(&loader,&PlayerImage::failed);loader.open(output.destination);QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !failed.isEmpty(),10000);QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));const auto decoded=qvariant_cast<QImage>(loaded.first().first());QCOMPARE(decoded.size(),image.size());
        const auto pixel=decoded.pixelColor(64,48);QVERIFY(std::abs(pixel.red()-128)<20);QVERIFY(std::abs(pixel.green()-96)<20);QVERIFY(std::abs(pixel.blue()-96)<20);
    }
    void directTimelinePreview_data() {
        QTest::addColumn<int>("stage");QTest::addColumn<bool>("resume");
        QTest::newRow("jinc-paused")<<4<<false;QTest::newRow("d3d11-paused")<<5<<false;
        QTest::newRow("jinc-playing")<<4<<true;QTest::newRow("d3d11-playing")<<5<<true;
    }
    void directTimelinePreview() {
        QFETCH(int,stage);QFETCH(bool,resume);QTemporaryDir dir;const auto source=dir.filePath("scrub.mkv");
        QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=8","-c:v","libx264","-preset","ultrafast","-g","96","-threads","2","-y",source});QVERIFY(ffmpeg.waitForFinished(30000));QCOMPARE(ffmpeg.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("playback/remember",false);settings.sync();
        PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath(stage==4?"builtin/Anime-4-Jinc.vpy":"builtin/Anime-5-D3D11.vpy"));QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);
        if(resume){player.togglePlayback();QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,5000);}
        auto *slider=player.findChild<QSlider *>("playerTimeline");QVERIFY(slider);slider->setSliderDown(true);
        const auto first=player.outputSnapshot().presentedVideoFrames;slider->setValue(25000);
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>first && std::abs(player.position()-20000000)<500000,10000);
        QVERIFY(slider->isSliderDown());QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
        const auto second=player.outputSnapshot().presentedVideoFrames;for(int i=0;i<100;++i)slider->setValue(30000+i*400);slider->setValue(75000);
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>second && std::abs(player.position()-60000000)<500000,10000);
        QVERIFY(slider->isSliderDown());QCOMPARE(player.qualityStage(),stage);
        slider->setValue(50000);slider->setSliderDown(false);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position()-40000000)<2000000,10000);
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing,resume,5000);
        if(!resume)QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.outputSnapshot().framePts*10000000.0*player.outputSnapshot().frameTimeBaseNumerator/player.outputSnapshot().frameTimeBaseDenominator-40000000)<500000,10000);
    }
    void embeddedIconsAndTypedAssociations() {
        for(const auto &name:QStringList{"renderer","player"}) {
            QIcon icon(":/icons/"+name+".ico");QVERIFY(!icon.isNull());
            for(const int size:{16,32,48,256})QVERIFY(!icon.pixmap(size,size).isNull());
        }
        for(const auto &name:QStringList{"VSRenderer.exe","vs-player.exe"}) {
            const auto path=QDir(QCoreApplication::applicationDirPath()).filePath(name).toStdWString();
            const auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_AS_DATAFILE);
            QVERIFY(module);const auto release=qScopeGuard([&]{FreeLibrary(module);});
            QVERIFY(FindResourceW(module,MAKEINTRESOURCEW(101),MAKEINTRESOURCEW(14)));
            if(name=="vs-player.exe")for(const int id:{102,103}) {
                QVERIFY(FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(14)));
                const auto icon=static_cast<HICON>(LoadImageW(module,MAKEINTRESOURCEW(id),IMAGE_ICON,32,32,0));
                QVERIFY(icon);DestroyIcon(icon);
            }
        }
        const auto registry="HKEY_CURRENT_USER\\Software\\VSRendererTests\\"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto cleanup=qScopeGuard([&]{QSettings erase(registry,QSettings::NativeFormat);erase.clear();});
        QSettings keys(registry+"\\Classes",QSettings::NativeFormat);
        keys.setValue(".jpg/OpenWithProgids/VSPlayer.Media",QString());
        keys.setValue(".jpg/OpenWithProgids/OtherViewer",QString());
        keys.sync();
        QVERIFY(registerPlayerAssociations({"jpg","mkv","flac"},"C:/Portable Player/vs-player.exe",registry));keys.sync();
        QVERIFY(!keys.contains(".jpg/OpenWithProgids/VSPlayer.Media"));QVERIFY(keys.contains(".jpg/OpenWithProgids/OtherViewer"));
        QVERIFY(keys.contains(".jpg/OpenWithProgids/VSPlayer.Image"));QVERIFY(keys.contains(".mkv/OpenWithProgids/VSPlayer.Video"));
        QVERIFY(keys.contains(".flac/OpenWithProgids/VSPlayer.Audio"));
        QCOMPARE(keys.value("VSPlayer.Image/TypeOverlay").toString(),QString("C:\\Portable Player\\vs-player.exe,-103"));
        QCOMPARE(keys.value("VSPlayer.Video/TypeOverlay").toString(),QString("C:\\Portable Player\\vs-player.exe,-102"));
        QCOMPARE(keys.value("VSPlayer.Image/DefaultIcon/.").toString(),keys.value("VSPlayer.Image/TypeOverlay").toString());
        QCOMPARE(keys.value("VSPlayer.Audio/DefaultIcon/.").toString(),QString("C:\\Portable Player\\vs-player.exe,-101"));
        QVERIFY(!keys.contains("VSPlayer.Image/shellex"));QVERIFY(!keys.contains("VSPlayer.Video/shellex"));
        QCOMPARE(keys.value("VSPlayer.Media/shell/open/command/.").toString(),QString("\"C:\\Portable Player\\vs-player.exe\" \"%1\""));
        QVERIFY(registerPlayerAssociations({},"C:/Portable Player/vs-player.exe",registry));keys.sync();
        QVERIFY(!keys.contains(".jpg/OpenWithProgids/VSPlayer.Image"));QVERIFY(keys.contains(".jpg/OpenWithProgids/OtherViewer"));
    }
    void stillImagesBypassVideoAndBrowse() {
        QTemporaryDir directory;QVERIFY(directory.isValid());
        QImage image(120,80,QImage::Format_RGB32);image.fill(QColor("#ef4020"));
        for(const auto &name:QStringList{"01.JPG","02.png","04.bmp"})QVERIFY(image.save(directory.filePath(name)));
        QProcess webp;webp.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-i",directory.filePath("02.png"),"-y",directory.filePath("03.webp")});QVERIFY(webp.waitForFinished(15000));QCOMPARE(webp.exitCode(),0);
        QFile broken(directory.filePath("05.jpg"));QVERIFY(broken.open(QIODevice::WriteOnly));broken.write("invalid JPEG");broken.close();
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/wheel","volume");settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));settings.sync();
        PlayerWindow player;player.show();player.activateWindow();QTest::qWait(100);
        auto *pane=player.findChild<PreviewPane *>();QVERIFY(pane);
        QVERIFY(player.openFile(directory.filePath("01.JPG")));
        QTRY_COMPARE_WITH_TIMEOUT(pane->image().size(),image.size(),15000);
        QVERIFY(!player.findChild<QSlider *>("playerTimeline")->isEnabled());
        const auto volume=player.findChild<QSlider *>("playerVolume")->value();
        const QPointF point(100,100);QWheelEvent wheel(point,pane->surface()->mapToGlobal(point.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(pane->surface(),&wheel);QVERIFY(pane->zoom()>1);QCOMPARE(player.findChild<QSlider *>("playerVolume")->value(),volume);
        for(const auto &name:QStringList{"02.png","03.webp","04.bmp"}) {
            QTest::keyClick(pane->surface(),Qt::Key_Right);QVERIFY(player.windowTitle().endsWith(name));
            QTRY_COMPARE_WITH_TIMEOUT(pane->image().size(),image.size(),15000);QCOMPARE(pane->zoom(),1.0f);
        }
        QTest::keyClick(pane->surface(),Qt::Key_Left);QVERIFY(player.windowTitle().endsWith("03.webp"));
        QVERIFY(player.openFile(directory.filePath("05.jpg")));QTRY_VERIFY_WITH_TIMEOUT(player.findChild<QLabel *>("playerStatus")->text().startsWith(QStringLiteral("图片解码失败：")),15000);
        QVERIFY(player.openFile(directory.filePath("02.png")));QTRY_COMPARE_WITH_TIMEOUT(pane->image().size(),image.size(),15000);
        player.togglePlayback();QTest::qWait(300);QCOMPARE(player.skippedFrames(),quint64(0));
        QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
        QProcess video;const auto videoPath=directory.filePath("video.mkv");video.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=160x96:rate=24:duration=1","-c:v","ffv1","-y",videoPath});QVERIFY(video.waitForFinished(15000));QCOMPARE(video.exitCode(),0);
        player.loadPreset({});QVERIFY(player.openFile(videoPath));QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,160u,15000);QVERIFY(pane->image().isNull());QVERIFY(player.findChild<QSlider *>("playerTimeline")->isEnabled());
        QVERIFY(player.openFile(directory.filePath("01.JPG")));QTRY_COMPARE_WITH_TIMEOUT(pane->image().size(),image.size(),15000);QVERIFY(!player.findChild<QSlider *>("playerTimeline")->isEnabled());
    }
    void jpegMetadataAndOrientation() {
        QTemporaryDir directory;QImage original(120,80,QImage::Format_RGB32);original.fill(QColor("#2244ee"));original.setColorSpace(QColorSpace(QColorSpace::SRgb));const auto path=directory.filePath("oriented.jpg");QVERIFY(original.save(path));QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));auto bytes=file.readAll();file.close();
        const auto exif=QByteArray::fromHex("45786966000049492a0008000000010012010300010000000600000000000000");QByteArray marker;marker.append(char(0xff));marker.append(char(0xe1));marker.append(char((exif.size()+2)>>8));marker.append(char((exif.size()+2)&255));marker+=exif;bytes.insert(2,marker);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);file.close();
        QImageReader reference(path);reference.setAutoTransform(true);const auto expected=reference.read();QVERIFY(!expected.isNull());PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded);QSignalSpy errors(&loader,&PlayerImage::failed);loader.open(path);QTRY_VERIFY(!loaded.isEmpty() || !errors.isEmpty());QVERIFY(errors.isEmpty());const auto actual=qvariant_cast<QImage>(loaded.first().first());QCOMPARE(actual.size(),QSize(80,120));QCOMPARE(actual.size(),expected.size());QCOMPARE(actual.colorSpace(),expected.colorSpace());QCOMPARE(actual.pixelColor(40,60),expected.pixelColor(40,60));
    }
    void imageInfoAndDeepZoom() {
        QTemporaryDir directory;QImage image(40000,64,QImage::Format_RGB32);image.fill(QColor("#dd5522"));const auto path=directory.filePath("wide.png");QVERIFY(image.save(path));
        PlayerWindow player;player.show();player.activateWindow();auto *pane=player.findChild<PreviewPane *>();QVERIFY(player.openFile(path));QTRY_COMPARE(pane->image().size(),image.size());
        QTest::keyClick(pane->surface(),Qt::Key_Tab);QLabel *info=nullptr;for(auto *label:player.findChildren<QLabel *>())if(label->text().contains(QStringLiteral("输入图片：")))info=label;QVERIFY(info);QVERIFY(info->text().contains(QStringLiteral("解码器：")));QVERIFY(info->text().contains(QStringLiteral("输出：")));QVERIFY(info->text().contains(QStringLiteral("未启用")));
        pane->adoptView(1024,0,0);QWheelEvent wheel(QPointF(pane->surface()->rect().center()),pane->surface()->mapToGlobal(pane->surface()->rect().center()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(pane->surface(),&wheel);QVERIFY(pane->zoom()>1024);pane->adoptView(pane->zoom(),-1,0);emit pane->viewChanged(pane->zoom(),-1,0);QTest::qWait(100);const auto captured=pane->surface()->grab().toImage();const auto pixel=captured.pixelColor(captured.width()/2,captured.height()/2);QVERIFY(pixel.red()>180 && pixel.green()>40);QVERIFY(info->text().contains(QString::number(pane->zoom()*100,'f',1)));captured.save("build/image-deep-zoom.png");
    }
    void externalNameMatching() {
        QVERIFY(playerMediaMatchScore("S01E01-crf12-xxx.mkv","S01E01-CN.ass")>1);
        QCOMPARE(playerMediaMatchScore("S01E01-crf12-xxx.mkv","S01E02-CN.ass"),0.0);
        QVERIFY(playerMediaMatchScore("Movie Title-crf12-1080p.mkv","Movie Title-CN.ass")>.65);
        QCOMPARE(playerMediaMatchScore("Show-01.mkv","Show-02.flac"),0.0);
        QCOMPARE(playerMediaMatchScore("Completely Different.mkv","S01E01-CN.ass"),0.0);
    }
    void nativeEqualizerProcessesAudio() {
        QTemporaryDir directory;const auto path=directory.filePath("tone.wav");QProcess ffmpeg;ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","sine=frequency=1000:sample_rate=48000:duration=15","-ac","2","-y",path});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        ThreeFpApi api;QVERIFY(api.available());QWidget surface;surface.resize(320,180);surface.show();ThreeFpConfiguration config{};config.size=sizeof(config);config.version=api.apiVersion();config.outputWindow=reinterpret_cast<void *>(surface.winId());config.decodeMode=1;config.sdrPeakNits=100;config.sdrPaperWhiteNits=203;void *handle=nullptr;QCOMPARE(api.create(&config,&handle),ThreeFpResult::Success);const auto cleanup=qScopeGuard([&]{api.destroy(handle);});
        struct Levels {quint32 size=sizeof(Levels),version=1,channels=0,reserved=0;float values[8]{};};
        using PeaksFn=ThreeFpResult (*)(void *,Levels *);QLibrary library(api.libraryPath());QVERIFY(library.load());auto peaks=reinterpret_cast<PeaksFn>(library.resolve("FFF3FP_GetAudioPeakLevels"));QVERIFY(peaks);
        const auto snapshot=[&]{ThreeFpSnapshot value{};value.size=sizeof(value);value.version=8;api.snapshot(handle,&value);return value;};
        const auto peak=[&]{Levels levels;peaks(handle,&levels);return qMax(levels.values[0],levels.values[1]);};
        QCOMPARE(api.open(handle,path.toUtf8().constData()),ThreeFpResult::Success);QTRY_COMPARE_WITH_TIMEOUT(snapshot().state,ThreeFpState::Ready,10000);QCOMPARE(api.setVolume(handle,.3f,0),ThreeFpResult::Success);QCOMPARE(api.play(handle),ThreeFpResult::Success);QTRY_VERIFY(peak()>.005f);const float baseline=peak();
        float gains[10]{};gains[4]=6;QCOMPARE(api.setAudioEffects(handle,true,gains,1,0),ThreeFpResult::Success);QTRY_VERIFY_WITH_TIMEOUT(peak()>baseline*1.6f,5000);const auto boosted=peak();QVERIFY(boosted<baseline*2.5f);
        QCOMPARE(api.setAudioEffects(handle,true,gains,.5f,0),ThreeFpResult::Success);QTRY_VERIFY_WITH_TIMEOUT(peak()<boosted*.7f && peak()>baseline*.7f,5000);
        QCOMPARE(api.setAudioEffects(handle,true,gains,.5f,1000000),ThreeFpResult::Success);QCOMPARE(api.seek(handle,0),ThreeFpResult::Success);QTRY_VERIFY_WITH_TIMEOUT(snapshot().audioInsertedSilenceFrames>=4000,5000);const auto inserted=snapshot().audioInsertedSilenceFrames;
        QCOMPARE(api.setAudioEffects(handle,false,gains,1,-1000000),ThreeFpResult::Success);const auto position=snapshot().position100ns;QTest::qWait(300);QVERIFY(snapshot().position100ns>position);QCOMPARE(snapshot().state,ThreeFpState::Playing);
        const auto media=QJsonDocument::fromJson(api.mediaInfo(handle).toUtf8()).object();int channels=0;for(const auto &entry:media.value("streams").toArray())if(entry.toObject().value("type").toString()=="audio")channels=entry.toObject().value("outputChannels").toInt();QCOMPARE(channels,2);
        qInfo()<<"Native EQ peak"<<baseline<<boosted<<"delay silence frames"<<inserted;
        for(int count:{6,8}) {
            const auto surround=directory.filePath(QString("surround%1.wav").arg(count));ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","sine=frequency=500:sample_rate=48000:duration=2","-ac",QString::number(count),"-y",surround});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
            QCOMPARE(api.setAudioEffects(handle,false,gains,1,0),ThreeFpResult::Success);QCOMPARE(api.open(handle,surround.toUtf8().constData()),ThreeFpResult::Success);QTRY_COMPARE(snapshot().state,ThreeFpState::Ready);const auto details=QJsonDocument::fromJson(api.mediaInfo(handle).toUtf8()).object();int input=0,output=0;for(const auto &entry:details.value("streams").toArray())if(entry.toObject().value("type").toString()=="audio"){input=entry.toObject().value("channels").toInt();output=entry.toObject().value("outputChannels").toInt();}QCOMPARE(input,count);QVERIFY(output==count || output==2);qInfo()<<"WASAPI channel policy"<<input<<output;
        }
    }
    void audioTracksEffectsAndDrops() {
        QTemporaryDir directory;const auto source=directory.filePath("S01E01-crf12-test.mkv"),external=directory.filePath("S01E01.wav"),subtitle=directory.filePath("S01E01-CN.srt");
        QProcess ffmpeg;const auto executable=QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");
        ffmpeg.start(executable,{"-v","error","-f","lavfi","-i","testsrc2=size=160x96:rate=24:duration=20","-f","lavfi","-i","sine=frequency=1000:duration=20","-f","lavfi","-i","sine=frequency=170:duration=20","-map","0:v","-map","1:a","-map","2:a","-c:v","ffv1","-c:a","pcm_s16le","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        ffmpeg.start(executable,{"-v","error","-f","lavfi","-i","sine=frequency=1000:duration=20","-ac","2","-y",external});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        QFile caption(subtitle);QVERIFY(caption.open(QIODevice::WriteOnly));caption.write("1\n00:00:00,000 --> 00:00:20,000\nMatched caption\n");caption.close();
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("decode/audio","3FP");settings.setValue("decode/video","3FP");settings.setValue("player/renderer","VS");settings.setValue("player/speed",1.0);settings.setValue("player/preset",QString());settings.sync();
        PlayerWindow player;player.show();player.activateWindow();player.loadPreset({});QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().isExternalAudio==1,15000);const auto title=player.windowTitle();
        auto *pane=player.findChild<PreviewPane *>();
        const auto menu=[&](){QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(20,20),pane->surface()->mapToGlobal(QPoint(20,20)));QApplication::sendEvent(pane->surface(),&context);return player.findChild<QMenu *>("playerContextMenu");};
        auto *root=menu();QVERIFY(root);QMenu *audio=nullptr,*subtitles=nullptr;int ai=-1,si=-1;for(int i=0;i<root->actions().size();++i){auto *a=root->actions()[i];if(a->text()==QStringLiteral("音频设置")){audio=a->menu();ai=i;}if(a->text()==QStringLiteral("字幕设置")){subtitles=a->menu();si=i;}}QVERIFY(audio && subtitles);QCOMPARE(si,ai+1);
        auto *tracks=audio->actions().first()->menu();QVERIFY(tracks);QVERIFY(tracks->actions().size()>=4);tracks->actions()[1]->trigger();QTRY_VERIFY(player.snapshot().isExternalAudio==0 && player.snapshot().selectedAudioStream==2);
        audio->actions().last()->trigger();root->close();QTRY_VERIFY(player.findChild<QDialog *>("playerEqualizer"));auto *eq=player.findChild<QDialog *>("playerEqualizer");QVERIFY(!eq->isModal());QCOMPARE(eq->findChildren<QSlider *>().size(),12);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);auto *toggle=eq->findChild<QCheckBox *>();toggle->setChecked(true);eq->findChild<QSlider *>("equalizerBand4")->setValue(6);const auto before=player.position();QTRY_VERIFY_WITH_TIMEOUT(player.position()>before,3000);QCOMPARE(player.snapshot().state,ThreeFpState::Playing);QCOMPARE(player.windowTitle(),title);eq->grab().save("build/audio-equalizer.png");eq->close();
        root=menu();audio=nullptr;for(auto *a:root->actions())if(a->text()==QStringLiteral("音频设置"))audio=a->menu();QVERIFY(audio);auto *sync=audio->actions()[1]->menu();sync->actions()[1]->trigger();root->close();QTest::qWait(100);root=menu();for(auto *a:root->actions())if(a->text()==QStringLiteral("音频设置"))audio=a->menu();QVERIFY(audio->actions()[1]->menu()->actions().last()->text().contains("0.1"));root->close();
        QMimeData captions;captions.setUrls({QUrl::fromLocalFile(subtitle)});const auto captionTop=pane->surface()->mapTo(&player,QPoint(60,pane->surface()->height()/5));QDragEnterEvent captionEnter(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionEnter);QDragMoveEvent captionMove(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionMove);QVERIFY(player.findChild<QLabel *>("playerDropHint")->isVisible());QVERIFY(player.findChild<QLabel *>("playerDropHint")->text().contains(QStringLiteral("次字幕")));QDropEvent captionDrop(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionDrop);QCOMPARE(player.windowTitle(),title);root=menu();for(auto *a:root->actions())if(a->text()==QStringLiteral("字幕设置"))subtitles=a->menu();QVERIFY(subtitles->actions()[1]->menu()->actions().last()->isChecked());root->close();
        QMimeData mime;mime.setUrls({QUrl::fromLocalFile(external)});const auto top=pane->surface()->mapTo(&player,QPoint(40,pane->surface()->height()/4)),bottom=pane->surface()->mapTo(&player,QPoint(40,pane->surface()->height()*3/4));
        QDragEnterEvent enter(top,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&enter);QDragMoveEvent move(top,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&move);QVERIFY(player.findChild<QLabel *>("playerStatus")->text().contains(QStringLiteral("作为外部音频加载")));
        QDropEvent drop(top,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&drop);QTRY_COMPARE(player.snapshot().isExternalAudio,1u);QCOMPARE(player.windowTitle(),title);
        QDragEnterEvent enter2(bottom,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&enter2);QDragMoveEvent move2(bottom,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&move2);QVERIFY(player.findChild<QLabel *>("playerStatus")->text().contains(QStringLiteral("单独播放音频")));
        QDropEvent drop2(bottom,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&drop2);QTRY_VERIFY(player.windowTitle().endsWith("S01E01.wav"));QTRY_COMPARE(player.snapshot().selectedVideoStream,-1);
    }
    void avifGridAndAlpha() {
        PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded),errors(&loader,&PlayerImage::failed);
        const auto wide=QFINDTESTDATA("fixtures/wide-grid.avif");QVERIFY(!wide.isEmpty());
        loader.open(wide);QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),30000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QCOMPARE(qvariant_cast<QImage>(loaded.takeFirst().first()).size(),QSize(40000,64));
        const auto alpha=QFINDTESTDATA("fixtures/alpha-10bit.avif");QVERIFY(!alpha.isEmpty());
        loader.open(alpha);QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);QVERIFY(errors.isEmpty());
        const auto image=qvariant_cast<QImage>(loaded.takeFirst().first());QCOMPARE(image.size(),QSize(128,96));QVERIFY(image.pixelColor(20,20).alpha()<240);QVERIFY(image.pixelColor(100,20).alpha()>250);
        // An obsolete decode must never replace the most recently requested image.
        loader.open(wide);loader.open(alpha);QTRY_COMPARE_WITH_TIMEOUT(loaded.size(),1,15000);QCOMPARE(qvariant_cast<QImage>(loaded.first().first()).size(),QSize(128,96));
    }
    void avifGbrColors() {
        PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded),errors(&loader,&PlayerImage::failed);
        loader.open(QFINDTESTDATA("fixtures/gbr-10bit.avif"));QTRY_VERIFY(!loaded.isEmpty() || !errors.isEmpty());QVERIFY(errors.isEmpty());const auto image=qvariant_cast<QImage>(loaded.first().first());QCOMPARE(image.size(),QSize(128,96));
        for(int y=0;y<96;y+=7)for(int x=0;x<128;x+=7){const auto pixel=image.pixelColor(x,y);QVERIFY(qAbs(pixel.red()-x*2)<=1);QVERIFY(qAbs(pixel.green()-y*2)<=1);QVERIFY(qAbs(pixel.blue()-96)<=1);QCOMPARE(pixel.alpha(),255);}
    }
    void additionalStillCodecs() {
        QTemporaryDir directory;QImage source(128,96,QImage::Format_RGB32);source.fill(QColor("#3184da"));QVERIFY(source.save(directory.filePath("source.png")));
        PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded),errors(&loader,&PlayerImage::failed);
        for(const auto &suffix:QStringList{"tiff","jxl","gif","jp2","tga"}) {
            const auto path=directory.filePath("image."+suffix);QVERIFY(PlayerImage::supports(path));
            QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-i",directory.filePath("source.png"),"-frames:v","1","-y",path});
            QVERIFY(encoder.waitForFinished(15000));QVERIFY2(encoder.exitCode()==0,encoder.readAllStandardError().constData());
            loader.open(path);QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
            const auto image=qvariant_cast<QImage>(loaded.takeFirst().first());QCOMPARE(image.size(),source.size());QVERIFY(image.pixelColor(30,30).blue()>image.pixelColor(30,30).red());
        }
    }
    void contextMenuRevealAndChecks() {
        PlayerMenu menu(nullptr);auto *boolean=menu.addAction("Boolean");boolean->setCheckable(true);boolean->setChecked(true);
        auto *submenu=PlayerMenu::add(&menu,"Nested");submenu->addAction("Item");
        menu.popup(QPoint(200,200));QVERIFY(menu.testAttribute(Qt::WA_TranslucentBackground));
        QCOMPARE(menu.property("menuReveal").toDouble(),0.0);QTest::qWait(70);const auto reveal=menu.property("menuReveal").toDouble();QVERIFY(reveal>0 && reveal<1);
        QTRY_COMPARE(menu.property("menuReveal").toDouble(),1.0);
        const auto geometry=menu.actionGeometry(boolean);const auto image=menu.grab().toImage();const auto ratio=menu.devicePixelRatioF();
        image.save("build/menu-check-test.png");
        const auto color=image.pixelColor(qRound((geometry.left()+19)*ratio),qRound((geometry.center().y()-4)*ratio));QVERIFY2(color.blue()>color.red(),qPrintable(color.name()+" "+QString::number(ratio)+" "+QString::number(geometry.left())+" "+QString::number(geometry.center().y())));
        submenu->popup(QPoint(400,200));QCOMPARE(submenu->property("menuReveal").toDouble(),0.0);QTRY_COMPARE(submenu->property("menuReveal").toDouble(),1.0);submenu->close();menu.close();
    }
    void suppliedStillImage() {
        const auto path=qEnvironmentVariable("VSR_TEST_IMAGE");if(path.isEmpty())QSKIP("Set VSR_TEST_IMAGE to verify a real large image");
        PlayerWindow player;player.show();player.activateWindow();
        auto *pane=player.findChild<PreviewPane *>();QVERIFY(player.openFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(!pane->image().isNull() || player.findChild<QLabel *>("playerStatus")->text().startsWith(QStringLiteral("图片解码失败：")),180000);
        QVERIFY2(!pane->image().isNull(),qPrintable(player.findChild<QLabel *>("playerStatus")->text()));
        qInfo()<<"Decoded real image"<<pane->image().size()<<pane->image().sizeInBytes()<<pane->image().text("decodeMilliseconds")<<pane->image().text("decoder");
        if(!qEnvironmentVariable("VSR_TEST_IMAGE_THUMBNAIL").isEmpty()) {
            QImage sampled(316,653,QImage::Format_RGB32);const auto &decoded=pane->image();
            for(int y=0;y<sampled.height();++y)for(int x=0;x<sampled.width();++x)sampled.setPixelColor(x,y,decoded.pixelColor(qint64(x)*decoded.width()/sampled.width(),qint64(y)*decoded.height()/sampled.height()));
            QVERIFY(sampled.save(qEnvironmentVariable("VSR_TEST_IMAGE_THUMBNAIL")));
            if(!qEnvironmentVariable("VSR_TEST_IMAGE_COMPARE_THUMBNAIL").isEmpty()) {
                const QImage original(qEnvironmentVariable("VSR_TEST_IMAGE_COMPARE_THUMBNAIL"));QCOMPARE(sampled.size(),original.size());qint64 error=0,bottomError=0;int count=0,bottomCount=0;
                for(int y=0;y<sampled.height();++y)for(int x=0;x<sampled.width();++x){const auto a=sampled.pixelColor(x,y),b=original.pixelColor(x,y);const int delta=qAbs(a.red()-b.red())+qAbs(a.green()-b.green())+qAbs(a.blue()-b.blue());error+=delta;count+=3;if(y>sampled.height()*2/3){bottomError+=delta;bottomCount+=3;}}
                qInfo()<<"Original pixel sample mean error"<<double(error)/count<<"bottom third"<<double(bottomError)/bottomCount;QVERIFY(double(error)/count<30);QVERIFY(double(bottomError)/bottomCount<30);
            }
        }
        if(qEnvironmentVariableIsSet("VSR_TEST_IMAGE_REFERENCE")) {
            QElapsedTimer timer;timer.start();QImageReader reader(path);reader.setAutoTransform(true);const auto reference=reader.read();const auto milliseconds=timer.elapsed();QVERIFY(!reference.isNull());QCOMPARE(reference.size(),pane->image().size());
            for(int y=1;y<8;++y)for(int x=1;x<8;++x){const QPoint at(x*reference.width()/8,y*reference.height()/8);QCOMPARE(reference.pixelColor(at),pane->image().pixelColor(at));}
            qInfo()<<"Qt JPEG reference decode ms"<<milliseconds<<"49 original pixels identical";
        }
        QTest::qWait(300);const auto captured=pane->surface()->grab().toImage();QVERIFY(!captured.isNull());
        QSet<QRgb> colors;for(int y=20;y<captured.height()-20;y+=20)for(int x=20;x<captured.width()-20;x+=20)colors.insert(captured.pixel(x,y));QVERIFY(colors.size()>8);
        pane->adoptView(2,.2f,.2f);emit pane->viewChanged(2,.2f,.2f);QTest::qWait(100);QVERIFY(pane->surface()->grab().toImage()!=captured);
        QVERIFY(!player.findChild<QSlider *>("playerTimeline")->isEnabled());
    }
    void animeStageSelection_data() {
        QTest::addColumn<int>("stage");QTest::addColumn<QString>("format");
        QTest::newRow("CNN-enhanced-420-10")<<0<<QString("yuv420p10le");
        QTest::newRow("CNN-422-10")<<1<<QString("yuv422p10le");
        QTest::newRow("no-CNN-enhanced-444-10")<<2<<QString("yuv444p10le");
        QTest::newRow("no-CNN-420-10")<<3<<QString("yuv420p10le");
        QTest::newRow("Jinc")<<4<<QString("yuv420p10le");
        QTest::newRow("D3D11")<<5<<QString("yuv420p10le");
    }
    void animeStageSelection() {
        QFETCH(int,stage);QFETCH(QString,format);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/animeStage",stage);settings.setValue("basic/autoplay",false);settings.setValue("performance/predecode",false);settings.sync();
        QTemporaryDir dir;const auto video=dir.filePath("ten-bit.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=2","-c:v","ffv1","-pix_fmt",format,"-y",video});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.resize(900,600);player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);QCOMPARE(player.qualityStage(),stage);
        if(stage<4)QVERIFY(player.outputSnapshot().videoWidth>320u);else QCOMPARE(player.outputSnapshot().videoWidth,320u);
        player.seekFrame(17);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,17,5000);QCOMPARE(player.qualityStage(),stage);
        // An imported custom VPY must not inherit the Anime starting stage.
        const auto custom=dir.filePath("custom.vpy");QVERIFY(PresetStore::write(custom,"import vapoursynth as vs\nvs.core.std.BlankClip(width=160,height=90,length=48,fpsnum=24,format=vs.YUV444P10,color=[400,500,520]).set_output()\n"));player.loadPreset(custom);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,160u,10000);QCOMPARE(player.qualityStage(),0);
    }
    void fixedAnimePresets_data() { animeStageSelection_data(); }
    void fixedAnimePresets() {
        QFETCH(int,stage);QFETCH(QString,format);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/animeStage",(stage+1)%6);settings.setValue("basic/autoplay",false);settings.setValue("performance/predecode",false);settings.setValue("render/upscale",7);settings.setValue("render/downscale",7);settings.sync();
        QTemporaryDir dir;const auto video=dir.filePath("manual.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=20","-c:v","ffv1","-pix_fmt",format,"-y",video});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.resize(900,600);player.show();
        const QStringList names{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy"};const auto path=QDir(PresetStore::directory()).filePath("builtin/"+names[stage]);
        QFile script(path);QVERIFY(script.open(QIODevice::ReadOnly));const auto text=script.readAll();QVERIFY(text.contains("_stage = "+QByteArray::number(stage)));QVERIFY(text.contains("_anime = _stage < 4"));QVERIFY(!text.contains("clip.width < 3840"));script.close();const auto restore=qScopeGuard([&]{if(stage==0)PresetStore::write(path,QString::fromUtf8(text));});
        if(stage==0){auto slow=QString::fromUtf8(text);slow.replace("clip.set_output(0)","import time\ndef _slow(n, f):\n    time.sleep(0.08)\n    return f\nclip = core.std.ModifyFrame(clip, clip, _slow)\nclip.set_output(0)");QVERIFY(PresetStore::write(path,slow));}
        player.loadPreset(path);QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);QCOMPARE(player.qualityStage(),stage);
        if(stage<4)QVERIFY(player.outputSnapshot().videoWidth>320u);else QCOMPARE(player.outputSnapshot().videoWidth,320u);
        if(stage==4)QVERIFY(player.outputSnapshot().videoScalingMode!=1u);if(stage==5)QVERIFY(player.findChild<QLabel *>("playerStatus")->text().contains(QStringLiteral("D3D11 原生直通")));
        player.resize(960,640);player.seekFrame(17);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,17,10000);QCOMPARE(player.qualityStage(),stage);
        auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QMenu *builtins=nullptr;for(auto *child:menu->findChildren<QMenu *>())if(child->title()==QStringLiteral("开发者内置"))builtins=child;QVERIFY(builtins);QCOMPARE(builtins->actions().size(),8);int checked=0;for(auto *action:builtins->actions())if(action->isChecked()){++checked;QVERIFY(action->text().startsWith(QStringLiteral("手动")));}QCOMPARE(checked,1);menu->close();
        if(stage==0){player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);QTest::qWait(9500);QVERIFY(player.skippedFrames()>10);QCOMPARE(player.qualityStage(),0);player.togglePlayback();}
    }
    void builtinInterpolation() {
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("performance/predecode",false);settings.setValue("player/renderer","VS");settings.setValue("render/upscale",7);settings.setValue("render/downscale",7);settings.sync();
        QTemporaryDir dir;const auto video=dir.filePath("interpolation.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=160x96:rate=24:duration=30","-c:v","ffv1","-y",video});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);
        for(int stage=0;stage<4;++stage){player.setInterpolation(stage);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);QCOMPARE(player.interpolationStage(),stage);QCOMPARE(player.outputSnapshot().videoWidth,160u);QCOMPARE(player.outputSnapshot().videoHeight,96u);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);player.seekFrame(3);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,3,10000);QVERIFY(std::abs(player.position()-625000)<50000);}
        auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QMenu *builtins=nullptr,*interpolation=nullptr;for(auto *child:menu->findChildren<QMenu *>()){if(child->title()==QStringLiteral("开发者内置"))builtins=child;if(child->title()==QStringLiteral("补帧"))interpolation=child;}QVERIFY(builtins);QCOMPARE(builtins->actions()[0]->text(),QStringLiteral("Anime · 自动切换"));QCOMPARE(builtins->actions()[1]->text(),QStringLiteral("Realistic.vpy"));QVERIFY(interpolation);int checked=0;for(auto *action:interpolation->actions())if(action->isChecked())++checked;QCOMPARE(checked,1);menu->close();
        player.setInterpolation(-1);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);QCOMPARE(player.interpolationStage(),-1);QCOMPARE(player.qualityStage(),0);QVERIFY(player.outputSnapshot().videoWidth>160u);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
    }
    void interpolationAdaptiveFallback() {
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("performance/predecode",false);settings.sync();
        QTemporaryDir dir;const auto video=dir.filePath("fallback.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=160x96:rate=48:duration=90","-c:v","ffv1","-y",video});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.show();const QDir builtin(QDir(PresetStore::directory()).filePath("builtin"));QMap<QString,QString> originals;
        const auto restore=qScopeGuard([&]{for(auto it=originals.begin();it!=originals.end();++it)PresetStore::write(it.key(),it.value());});
        for(const auto &name:QStringList{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"}){const auto path=builtin.filePath(name);QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));originals.insert(path,QString::fromUtf8(file.readAll()));file.close();QVERIFY(PresetStore::write(path,"import vapoursynth as vs\nimport time\ncore=vs.core\nclip=core.std.BlankClip(width=160,height=96,length=8640,fpsnum=96,format=vs.YUV444P16)\ndef slow(n,f):\n    time.sleep(0.08)\n    return f\nclip=core.std.ModifyFrame(clip,clip,slow)\nclip.set_output()\n"));}
        player.setInterpolation(0,true);QVERIFY(player.openFile(video));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,15000);
        for(int stage=1;stage<4;++stage)QTRY_COMPARE_WITH_TIMEOUT(player.interpolationStage(),stage,15000);
        QTRY_VERIFY_WITH_TIMEOUT(player.findChild<QLabel *>("playerInterpolationWarning")->isVisible(),15000);QCOMPARE(player.interpolationStage(),3);QCOMPARE(player.snapshot().state,ThreeFpState::Playing);player.togglePlayback();
        // A load failure also walks the automatic chain while retaining pause state.
        for(auto it=originals.begin();it!=originals.end();++it)QVERIFY(PresetStore::write(it.key(),"raise RuntimeError('forced unavailable interpolation')\n"));
        player.setInterpolation(0,true);QTRY_COMPARE_WITH_TIMEOUT(player.interpolationStage(),3,10000);QTRY_VERIFY_WITH_TIMEOUT(player.findChild<QLabel *>("playerInterpolationWarning")->isVisible(),10000);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
    }
    void imageAssociations() {
        const auto registry="HKEY_CURRENT_USER\\Software\\VSRendererTests\\"+QUuid::createUuid().toString(QUuid::WithoutBraces);const auto cleanup=qScopeGuard([&]{QSettings erase(registry,QSettings::NativeFormat);erase.clear();});
        const auto extensions=playerImageExtensions();QVERIFY(extensions.contains("jpg"));QVERIFY(extensions.contains("avif"));QVERIFY(registerPlayerAssociations(extensions,"C:/Portable Player/vs-player.exe",registry));QSettings keys(registry+"\\Classes",QSettings::NativeFormat);
        for(const auto &extension:extensions)QVERIFY(keys.contains('.'+extension+"/OpenWithProgids/VSPlayer.Image"));QVERIFY(!keys.contains(".mkv/OpenWithProgids/VSPlayer.Video"));QVERIFY(registerPlayerAssociations({},"C:/Portable Player/vs-player.exe",registry));keys.sync();QVERIFY(!keys.contains(".avif/OpenWithProgids/VSPlayer.Image"));
    }
    void vvcSourceFallback_data() {
        QTest::addColumn<QString>("presetName");QTest::addColumn<QString>("decoder");
        QTest::newRow("ffms-3fp")<<QString()<<QString("3FP");
        QTest::newRow("ffms-lav")<<QString()<<QString("LAV");
        QTest::newRow("anime")<<QString("Anime")<<QString("3FP");
        QTest::newRow("realistic")<<QString("Realistic")<<QString("3FP");
        QTest::newRow("custom-resize")<<QString("resize")<<QString("3FP");
    }
    void vvcSourceFallback() {
        const QString source=qEnvironmentVariable("VSR_VVC_TEST_FILE");
        if(source.isEmpty())QSKIP("Set VSR_VVC_TEST_FILE to verify the real VVC regression");
        QVERIFY(QFileInfo::exists(source));QFETCH(QString,presetName);QFETCH(QString,decoder);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("decode/video",decoder);settings.setValue("decode/audio",decoder);settings.setValue("basic/autoplay",false);settings.sync();
        QTemporaryDir dir;PlayerWindow player;player.show();
        if(presetName=="resize") {
            FilterGraph graph;const int row=graph.add("resize");graph.setParameter(row,"width",640);graph.setParameter(row,"height",268);
            const auto path=dir.filePath("resize.vpy");QVERIFY(PresetStore::write(path,PresetStore::create(graph,SourceFilter::Ffms2,{},"VVC")));player.loadPreset(path);
        } else if(!presetName.isEmpty())player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/"+presetName+".vpy"));
        QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);
        if(presetName!="Anime")QCOMPARE(player.outputSnapshot().videoWidth,presetName=="resize"?640u:1920u);
        else QVERIFY(player.outputSnapshot().videoWidth>0);
        if(presetName=="resize")QCOMPARE(player.outputSnapshot().videoHeight,268u);
        QVERIFY(player.snapshot().state==ThreeFpState::Ready || player.snapshot().state==ThreeFpState::Paused);
        player.seekFrame(37);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,37,10000);
        QVERIFY(std::abs(player.position()-15416667)<1000);
        player.togglePlayback();QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,5000);
        QTRY_VERIFY_WITH_TIMEOUT(player.position()>20000000,5000);
        QVERIFY(player.snapshot().decodedAudioFrames>0);
        player.togglePlayback();QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Paused,5000);
        // Switching back to a saved FFMS2 preset must also retain VVC support.
        player.loadPreset(QString());QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,1920u,20000);
        player.seekFrame(61);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,61,10000);
    }
    void wheelZoomResetButton() {
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();QVERIFY(pane);pane->setSurfaceActive(true);
        auto *reset=player.findChild<QPushButton *>("playerResetZoom");QVERIFY(reset);QVERIFY(!reset->isVisible());
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/wheel","zoom");settings.sync();
        const QPointF point(pane->surface()->width()/3,pane->surface()->height()/3);
        QWheelEvent wheel(point,pane->surface()->mapToGlobal(point.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(pane->surface(),&wheel);
        QVERIFY(pane->zoom()>1);QVERIFY(reset->isVisible());QVERIFY(!pane->pan().isNull());QTest::mouseClick(reset,Qt::LeftButton);QCOMPARE(pane->zoom(),1.0f);QCOMPARE(pane->pan(),QPointF());QVERIFY(!reset->isVisible());
        player.toggleFullscreen();QApplication::sendEvent(pane->surface(),&wheel);QVERIFY(reset->isVisible());QTest::mouseClick(reset,Qt::LeftButton);QCOMPARE(pane->zoom(),1.0f);player.toggleFullscreen();
    }
    void initTestCase() { QCoreApplication::setOrganizationName("VSRendererTests"); QCoreApplication::setApplicationName("VSPlayerTests"); QCoreApplication::setApplicationVersion(VSR_VERSION); QFile file(configPath());hadIni_=file.exists();if(file.open(QIODevice::ReadOnly))oldIni_=file.readAll(); }
    void init() {QSettings settings(configPath(),QSettings::IniFormat);settings.clear();settings.setValue("player/core","3FP");settings.setValue("player/renderer","VS");settings.setValue("performance/cpu",100);settings.setValue("performance/gpu",100);settings.setValue("performance/ram",100);settings.setValue("performance/vram",100);settings.sync();}
    void cleanupTestCase() {QFile file(configPath());if(hadIni_){QVERIFY(file.open(QIODevice::WriteOnly));file.write(oldIni_);}else file.remove();QSettings().clear();}
    void playbackAndPreset() {
        QSettings settings(configPath(),QSettings::IniFormat); const auto oldCore = settings.value("player/core"); settings.setValue("player/core", QStringLiteral("3FP"));
        QTemporaryDir dir; const QString video = dir.filePath(QStringLiteral("测试 空格.mkv"));
        const QString metadata = dir.filePath("chapters.txt");
        QFile chapters(metadata); QVERIFY(chapters.open(QIODevice::WriteOnly)); chapters.write(";FFMETADATA1\n[CHAPTER]\nTIMEBASE=1/1000\nSTART=0\nEND=4000\ntitle=Opening\n[CHAPTER]\nTIMEBASE=1/1000\nSTART=4000\nEND=12000\ntitle=Second chapter\n"); chapters.close();
        QProcess ffmpeg; ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe", {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=12","-f","lavfi","-i","sine=frequency=440:duration=12","-i",metadata,"-map","0:v","-map","1:a","-map_metadata","2","-map_chapters","2","-c:v","libx264","-preset","ultrafast","-g","24","-c:a","aac","-y",video});
        QVERIFY(ffmpeg.waitForFinished(20000)); QCOMPARE(ffmpeg.exitCode(), 0);
        FilterGraph graph; int row = graph.add("resize"); graph.setParameter(row,"width",160); graph.setParameter(row,"height",90);
        const QString preset = dir.filePath("preview.vpy"); QVERIFY(PresetStore::write(preset, PresetStore::create(graph, SourceFilter::Ffms2, "Z:/missing.mkv", "test")));
        PlayerWindow player; player.show(); QVERIFY(QTest::qWaitForWindowExposed(&player)); player.loadPreset(preset); QVERIFY(player.openFile(video));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames > 2, 15000);
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state, ThreeFpState::Playing, 5000);
        QCOMPARE(player.outputSnapshot().videoWidth, 160u); QCOMPARE(player.outputSnapshot().videoHeight, 90u);
        auto *timeline = player.findChild<QSlider *>("playerTimeline"); QVERIFY(timeline); QTRY_COMPARE(timeline->property("chapters").toJsonArray().size(), 2);
        player.togglePlayback(); QTRY_COMPARE(player.snapshot().state, ThreeFpState::Paused);
        player.seekFrame(37); QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position() - 15416667) < 1000, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex, 37, 5000);
        auto *time = player.findChild<QLineEdit *>("playerPosition"); time->setText("00:00:03.000"); QTest::keyClick(time, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position() - 30000000) < 500000, 5000);
        QCOMPARE(PlayerWindow::normalizedRate(.03), .1); QCOMPARE(PlayerWindow::normalizedRate(20), 16.); QCOMPARE(PlayerWindow::normalizedRate(1.126), 1.15);
        QTest::qWait(500); beforePauseRate_ = player.position(); player.setRate(2); QTest::qWait(100); QVERIFY(std::abs(player.position() - beforePauseRate_) < 500000); player.togglePlayback(); QTRY_COMPARE(player.snapshot().state, ThreeFpState::Playing);
        QTest::qWait(350); auto before = player.position(); QTest::qWait(750); auto advance = player.position() - before;
        QVERIFY2(advance > 11000000 && advance < 20000000, qPrintable(QString("2x clock advanced %1").arg(advance)));
        QVERIFY(player.snapshot().decodedAudioFrames > 0); QVERIFY(player.snapshot().audioPosition100ns > 0);
        player.togglePlayback(); QTRY_COMPARE(player.snapshot().state, ThreeFpState::Paused);
        player.setRate(.1); player.togglePlayback(); QTRY_COMPARE(player.snapshot().state, ThreeFpState::Playing); QTest::qWait(300); before = player.position(); QTest::qWait(700);
        advance = player.position() - before; QVERIFY2(advance > 300000 && advance < 1300000, qPrintable(QString("0.1x clock advanced %1").arg(advance)));
        player.togglePlayback(); QTRY_COMPARE(player.snapshot().state, ThreeFpState::Paused);
        auto *speed = player.findChild<QDoubleSpinBox *>("playerSpeed"); auto *edit = speed->findChild<QLineEdit *>();
        edit->setText("20"); QTest::keyClick(edit,Qt::Key_Return); QCOMPARE(speed->value(),16.);
        edit->setText("0.03"); QTest::keyClick(edit,Qt::Key_Return); QCOMPARE(speed->value(),.1);
        edit->setText("1.126"); QTest::keyClick(edit,Qt::Key_Return); QCOMPARE(speed->value(),1.15);
        player.setRate(1); player.seekTime(20000000);
        QTRY_VERIFY(std::abs(player.position() - 20000000) < 500000);
        for (auto *button : player.findChildren<QPushButton *>()) if (button->text() == "›K") { button->click(); break; }
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 20500000, 5000);
        QTest::keyClick(&player, Qt::Key_Tab); player.grab().save("build/mingw-release/vs-player-layout.png");
        FilterGraph interpolation; const int rife = interpolation.add("rife"); interpolation.setParameter(rife,"inference_scale",QStringLiteral("2 - 半宽半高"));
        const QString interpolatePreset = dir.filePath("rife.vpy"); QVERIFY(PresetStore::write(interpolatePreset,PresetStore::create(interpolation,SourceFilter::Ffms2,"Z:/missing.mkv","RIFE")));
        player.loadPreset(interpolatePreset);
        QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,320u,15000);
        player.seekFrame(7); QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,7,15000);
        QVERIFY(std::abs(player.position()-1458333) < 1000);
        settings.setValue("player/core", oldCore);
    }
    void lavPlayer() {
        QSettings settings(configPath(),QSettings::IniFormat); const auto oldCore = settings.value("player/core"); settings.setValue("player/core", QStringLiteral("LAV")); QCOMPARE(settings.value("player/core").toString(), QStringLiteral("LAV"));
        QTemporaryDir dir; const QString video = dir.filePath("lav.mkv");
        QProcess ffmpeg; ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe", {"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24:duration=6","-f","lavfi","-i","sine=frequency=440:duration=6","-c:v","libx264","-preset","ultrafast","-c:a","aac","-y",video});
        QVERIFY(ffmpeg.waitForFinished(15000)); QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player; player.show(); QVERIFY(player.openFile(video));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames > 3,15000);
        QTRY_VERIFY(player.position() > 5000000);
        QTRY_COMPARE(player.findChild<QPushButton *>("playerDecoder")->text(), QString("LAV"));
        player.togglePlayback(); player.seekFrame(25); QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,25,5000);
        player.setRate(2); player.togglePlayback(); QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,5000);
        QTRY_VERIFY(player.findChild<QPushButton *>("playerDecoder")->text().startsWith("3FP"));
        settings.setValue("player/core",oldCore);
    }
    void playerInteractionAndConfiguration() {
        PlayerWindow player;player.show();player.activateWindow();QTest::qWait(150);
        auto *speed=player.findChild<QDoubleSpinBox *>("playerSpeed");auto *button=player.findChild<QPushButton *>("playerSpeedPopupButton");QVERIFY(button);button->click();
        auto *popup=player.findChild<QWidget *>("playerSpeedPopup");QVERIFY(popup && popup->isVisible());
        QPushButton *increase=nullptr;for(auto *b:popup->findChildren<QPushButton *>())if(b->text()=="›")increase=b;QVERIFY(increase);
        increase->click();increase->click();increase->click();QCOMPARE(speed->value(),1.15);QVERIFY(popup->isVisible());popup->close();QTest::qWait(30);
        auto *pane=player.findChild<PreviewPane *>();QVERIFY(pane);pane->surface()->setFocus();
        const auto original=player.geometry();QTest::keyClick(&player,Qt::Key_Return);QTRY_VERIFY(player.isFullScreen());
        QVERIFY(!(GetWindowLongPtrW(reinterpret_cast<HWND>(player.winId()),GWL_STYLE)&WS_CAPTION));
        QTest::keyClick(&player,Qt::Key_Return);QTRY_VERIFY(!player.isFullScreen());QCOMPARE(player.geometry().size(),original.size());
        QTest::keyClick(&player,Qt::Key_Return);QTRY_VERIFY(player.isFullScreen());QTest::keyClick(&player,Qt::Key_Escape);QTRY_VERIFY(!player.isFullScreen());
        QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QVERIFY(menu->isVisible());QMenu *presets=nullptr;for(auto *action:menu->actions())if(action->text()==QStringLiteral("VapourSynth 预设"))presets=action->menu();QVERIFY(presets);QVERIFY(presets->actions().first()->menu());QCOMPARE(presets->actions().first()->menu()->actions().size(),2);menu->close();
        QTemporaryDir dir;const auto file=dir.filePath("all.ini");QVERIFY(player.saveConfiguration(file));
        QSettings edit(file,QSettings::IniFormat);edit.setValue("performance/cpu",75);edit.setValue("decode/output","p010le");edit.setValue("subtitle/style/color","#123456");edit.setValue("player/speed",1.35);edit.sync();
        QVERIFY(player.loadConfiguration(file));QCOMPARE(speed->value(),1.35);QVERIFY(player.saveConfiguration(dir.filePath("roundtrip.ini")));QSettings result(dir.filePath("roundtrip.ini"),QSettings::IniFormat);QCOMPARE(result.value("performance/cpu").toInt(),75);QCOMPARE(result.value("decode/output").toString(),"p010le");QCOMPARE(result.value("subtitle/style/color").toString(),"#123456");
        player.grab().save("build/player-interaction-layout.png");
    }
    void slowVpyCountsSkippedFrames() {
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("performance/predecode",false);settings.sync();
        QTemporaryDir dir; const auto source=dir.filePath("source.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","color=blue:size=160x90:rate=24:duration=8","-c:v","ffv1","-y",source});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        const auto script=dir.filePath("slow.vpy");QFile vpy(script);QVERIFY(vpy.open(QIODevice::WriteOnly));vpy.write("import vapoursynth as vs\nimport time\nc=vs.core.std.BlankClip(width=160,height=90,length=192,format=vs.YUV444P16,fpsnum=24,color=[10000,30000,40000])\ndef slow(n):\n    time.sleep(0.10)\n    return c\nvs.core.std.FrameEval(c,eval=slow).set_output()\n");vpy.close();
        PlayerWindow player;player.show();player.loadPreset(script);QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>5,10000);QTRY_VERIFY_WITH_TIMEOUT(player.skippedFrames()>3,10000);
        player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::keyClick(&player,Qt::Key_Tab);QTest::qWait(100);bool found=false;for(auto *label:player.findChildren<QLabel *>())if(label->text().contains(QStringLiteral("VS 跳过"))){found=true;QVERIFY(label->text().contains(QStringLiteral("GPU：")));}QVERIFY(found);
        const auto dropped=player.skippedFrames();player.seekTime(0);QTest::qWait(30);QVERIFY(player.skippedFrames()<=dropped+1);
    }
    void subtitleOverlay() {
        QTemporaryDir dir;const auto path=dir.filePath("caption.srt");QFile srt(path);QVERIFY(srt.open(QIODevice::WriteOnly));srt.write("1\n00:00:00,000 --> 00:00:05,000\nSubtitle test\n\n");srt.close();
        PlayerSubtitles subtitles;QSignalSpy spy(&subtitles,&PlayerSubtitles::imageReady);QSignalSpy errors(&subtitles,&PlayerSubtitles::errorOccurred);
        subtitles.load(0,path,-1,"srt",{{"size",72},{"color","#ffffff"}});subtitles.render(10000000,QSize(640,360),QSize(1920,1080),true);QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(),10000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        const auto image=qvariant_cast<QImage>(spy.last().first());int visible=0;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y)))++visible;QVERIFY(visible>100);
        image.save("build/subtitle-overlay.png");spy.clear();subtitles.render(10000000,QSize(640,360),QSize(1920,1080),false);QTRY_VERIFY(!spy.isEmpty());const auto hidden=qvariant_cast<QImage>(spy.last().first());for(int y=0;y<hidden.height();++y)for(int x=0;x<hidden.width();++x)QVERIFY(qAlpha(hidden.pixel(x,y))==0);
    }
    void subtitleOnlyChangedFrames() {
        QTemporaryDir directory;QFile caption(directory.filePath("caption.srt"));QVERIFY(caption.open(QIODevice::WriteOnly));
        caption.write("1\n00:00:00,000 --> 00:00:02,000\nFirst cue\n\n2\n00:00:02,000 --> 00:00:04,000\nSecond cue\n");caption.close();
        PlayerSubtitles subtitles;QSignalSpy frames(&subtitles,&PlayerSubtitles::imageReady),errors(&subtitles,&PlayerSubtitles::errorOccurred);
        subtitles.load(0,caption.fileName(),-1,"subrip",{});
        const auto render=[&](qint64 at,const QSize &size=QSize(640,360),bool visible=true){subtitles.render(at,size,QSize(640,360),visible);};
        render(5000000);QTRY_COMPARE_WITH_TIMEOUT(frames.size(),1,5000);QVERIFY(errors.isEmpty());
        const auto first=qvariant_cast<QImage>(frames.last().first());render(10000000);QTest::qWait(200);QCOMPARE(frames.size(),1);
        render(25000000);QTRY_COMPARE(frames.size(),2);QVERIFY(qvariant_cast<QImage>(frames.last().first())!=first);
        render(45000000);QTRY_COMPARE(frames.size(),3);const auto empty=qvariant_cast<QImage>(frames.last().first());
        for(int y=0;y<empty.height();++y)for(int x=0;x<empty.width();++x)QCOMPARE(qAlpha(empty.pixel(x,y)),0);
        render(5000000);QTRY_COMPARE(frames.size(),4);QCOMPARE(qvariant_cast<QImage>(frames.last().first()),first);
        render(5000000,QSize(320,180));QTRY_COMPARE(frames.size(),5);QCOMPARE(qvariant_cast<QImage>(frames.last().first()).size(),QSize(320,180));
        render(5000000,QSize(320,180),false);QTRY_COMPARE(frames.size(),6);
        render(10000000,QSize(320,180),false);QTest::qWait(200);QCOMPARE(frames.size(),6);
    }
    void embeddedTextSubtitleSeek_data() {
        QTest::addColumn<QString>("codec");QTest::newRow("ass")<<QString("ass");QTest::newRow("subrip")<<QString("srt");
    }
    void embeddedTextSubtitleSeek() {
        QFETCH(QString,codec);QTemporaryDir directory;QFile caption(directory.filePath("caption.srt"));QVERIFY(caption.open(QIODevice::WriteOnly));
        caption.write("1\n00:00:01,000 --> 00:00:04,000\nFirst cue\n\n2\n00:00:40,000 --> 00:00:45,000\nLast cue\n");caption.close();
        const auto source=directory.filePath("embedded.mkv");QProcess encode;
        encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-f","lavfi","-i","color=black:size=128x96:rate=24:duration=60","-i",caption.fileName(),"-c:v","ffv1","-c:s",codec,source});
        QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        PlayerSubtitles subtitles;QSignalSpy frames(&subtitles,&PlayerSubtitles::imageReady),errors(&subtitles,&PlayerSubtitles::errorOccurred);subtitles.load(0,source,1,codec,{});
        const auto visible=[&](qint64 at) {
            subtitles.render(at,QSize(640,360),QSize(128,96),true);
            if(frames.isEmpty())return false;
            const auto image=qvariant_cast<QImage>(frames.last().first());
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y)))return true;
            return false;
        };
        QTRY_VERIFY_WITH_TIMEOUT(visible(20000000),5000);QVERIFY(errors.isEmpty());frames.clear();
        QTRY_VERIFY_WITH_TIMEOUT(visible(420000000),5000);QVERIFY(errors.isEmpty());frames.clear();
        QTRY_VERIFY_WITH_TIMEOUT(visible(20000000),5000);QVERIFY(errors.isEmpty());
    }
    void bundledMadvr() {
        QTemporaryDir dir;const QString source=dir.filePath("sample.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=10","-f","lavfi","-i","sine=duration=10","-c:v","libx264","-preset","ultrafast","-c:a","aac","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);QWidget surface;surface.resize(640,360);surface.show();
        LavPlayback lav;QVERIFY2(lav.open(source,reinterpret_cast<void *>(surface.winId())),qPrintable(lav.error()));QVERIFY(lav.madvrActive());lav.resizeVideo(640,360);QVERIFY(lav.play());QTest::qWait(350);QVERIFY(lav.position()>0);QVERIFY(lav.pause());
        lav.seek(0);QImage image;QTRY_VERIFY_WITH_TIMEOUT(!(image=lav.capture()).isNull(),8000);QVERIFY2(!image.isNull(),qPrintable(lav.error()));QSet<QRgb> colors;for(int y=0;y<image.height();y+=8)for(int x=0;x<image.width();x+=8)colors.insert(image.pixel(x,y));QVERIFY(colors.size()>30);image.save("build/madvr-capture.png");
        QImage caption(640,360,QImage::Format_ARGB32_Premultiplied);caption.fill(Qt::transparent);for(int y=20;y<80;++y)for(int x=20;x<150;++x)caption.setPixel(x,y,qRgba(255,0,0,255));lav.setSubtitle(caption);lav.play();QTest::qWait(300);lav.pause();image=lav.capture();QVERIFY(!image.isNull());image.save("build/madvr-osd.png");
        QVERIFY(lav.open(source,reinterpret_cast<void *>(surface.winId())));QVERIFY(lav.play());QTest::qWait(150);QVERIFY(lav.position()>0);lav.pause();
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/renderer","madVR");settings.sync();
        PlayerWindow player;player.show();QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.position()>2000000,10000);QCOMPARE(player.outputSnapshot().presentedVideoFrames,0u);
        player.activateWindow();player.findChild<PreviewPane *>()->surface()->setFocus();QTest::keyClick(&player,Qt::Key_Tab);QTest::qWait(550);bool found=false;for(auto *label:player.findChildren<QLabel *>())if(label->text().contains("madshi video renderer") && label->text().contains(QStringLiteral("CPU："))){found=true;QVERIFY(label->text().contains(QStringLiteral("VS 处理：未启用")));}QVERIFY(found);
        player.loadPreset("test.vpy");QVERIFY(player.findChild<QLabel *>("playerStatus")->text().contains(QStringLiteral("预设不生效")));
        player.togglePlayback();player.setRate(2);player.togglePlayback();QTest::qWait(250);const auto at=player.position();QTest::qWait(650);QVERIFY(player.position()-at>9000000);QTRY_VERIFY(player.snapshot().audioPosition100ns>0);player.togglePlayback();
    }
    void outputFormats() {
        QWidget surface;surface.setAttribute(Qt::WA_NativeWindow);surface.resize(64,64);surface.show();ThreeFpApi api;ThreeFpPlayer player(api,&surface);
        VapourSynthFrame frame;frame.width=frame.height=64;frame.format=ThreeFpExternalPixelFormat::Yuv444P16;frame.duration100ns=416667;frame.totalFrames=20;frame.colorMatrix=1;
        const quint16 values[]{30000,12000,45000};for(int p=0;p<3;++p){frame.strides[p]=128;frame.planes[p].resize(64*128);auto *data=reinterpret_cast<quint16 *>(frame.planes[p].data());std::fill(data,data+4096,values[p]);}
        QVERIFY(player.submitFrame(frame));QTRY_VERIFY(player.snapshot().presentedVideoFrames>0);const auto base=player.capture().pixelColor(32,32);
        for(const auto &format:QStringList{"p010le","p016le","yuv444p10le","yuv444p16le","nv12","rgb48le","rgb24"}) {QVERIFY2(player.setOutputFormat(format),qPrintable(format));++frame.frameIndex;QVERIFY2(player.submitFrame(frame),qPrintable(format+": "+player.lastError()));QTRY_COMPARE(player.snapshot().frameIndex,frame.frameIndex);const auto image=player.capture();QVERIFY(!image.isNull());const auto pixel=image.pixelColor(32,32);QVERIFY2(std::abs(pixel.red()-base.red())<6 && std::abs(pixel.green()-base.green())<6 && std::abs(pixel.blue()-base.blue())<6,qPrintable(format));QCOMPARE(player.snapshot().videoWidth,64u);QCOMPARE(player.snapshot().videoHeight,64u);}
        QImage caption(64,64,QImage::Format_ARGB32_Premultiplied);caption.fill(Qt::transparent);for(int y=20;y<40;++y)for(int x=20;x<40;++x)caption.setPixel(x,y,qRgba(255,255,255,255));QVERIFY(player.setSubtitle(caption));QTest::qWait(300);const auto captured=player.capture();captured.save("build/output-caption.png");int white=0;for(int y=0;y<captured.height();++y)for(int x=0;x<captured.width();++x){const auto c=captured.pixelColor(x,y);if(c.red()>240 && c.green()>240 && c.blue()>240)++white;}QVERIFY(white>30);
    }
    void mountedSubtitleAndScreenshots() {
        QTemporaryDir dir;const auto source=dir.filePath("capture-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".mkv");const auto path=dir.filePath("external.srt");QFile srt(path);QVERIFY(srt.open(QIODevice::WriteOnly));srt.write("1\n00:00:00,000 --> 00:00:20,000\nExternal subtitle\n\n");srt.close();
        QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","color=blue:size=320x180:rate=24:duration=8","-i",path,"-c:v","ffv1","-c:s","srt","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.show();QVERIFY(player.openFile(source));QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);player.togglePlayback();QTest::qWait(500);
        auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);
        QMenu *tracks=nullptr,*capture=nullptr;for(auto *action:menu->actions()){if(action->text()==QStringLiteral("字幕设置"))tracks=action->menu();if(action->text()==QStringLiteral("图像截取"))capture=action->menu();}QVERIFY(tracks && capture);auto *primary=tracks->actions().first()->menu();QVERIFY(primary);QCOMPARE(primary->actions().size(),2);QVERIFY(primary->actions().last()->isChecked());
        capture->actions().first()->trigger();QTRY_VERIFY_WITH_TIMEOUT(player.findChildren<QProcess *>("playerScreenshotProcess").isEmpty(),15000);QTest::qWait(800);capture->actions().last()->trigger();menu->close();const QDir images(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto name=QFileInfo(source).completeBaseName();QTRY_COMPARE_WITH_TIMEOUT(images.entryList({name+"*.png"},QDir::Files).size(),2,10000);
        QImage original,display;for(const auto &file:images.entryList({name+"*.png"},QDir::Files)){if(file.contains("-VS.png"))original.load(images.filePath(file));else display.load(images.filePath(file));}QCOMPARE(original.size(),QSize(320,180));QCOMPARE(display.size(),pane->surface()->size()*pane->surface()->devicePixelRatioF());int light=0;for(int y=0;y<display.height();++y)for(int x=0;x<display.width();++x){const auto c=display.pixelColor(x,y);if(c.red()>180 && c.green()>180 && c.blue()>180)++light;}QVERIFY(light>100);
        for(const auto &file:images.entryList({name+"*.png"},QDir::Files))QFile::rename(images.filePath(file),QDir::current().filePath("build/"+file));
    }
    void embeddedPgs() {
        const QString source="D:/Animation Enhance/MyGO BDRemux/01.mkv";if(!QFileInfo::exists(source))QSKIP("Local test media is unavailable");
        PlayerSubtitles subtitles;QSignalSpy spy(&subtitles,&PlayerSubtitles::imageReady);QSignalSpy errors(&subtitles,&PlayerSubtitles::errorOccurred);subtitles.load(0,source,2,"hdmv_pgs_subtitle",{});bool found=false;
        for(int seconds=0;seconds<80 && !found;seconds+=2){spy.clear();subtitles.render(seconds*10000000LL,QSize(640,360),QSize(1920,1080),true);QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(),10000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));const auto image=qvariant_cast<QImage>(spy.last().first());for(int y=0;y<image.height()&&!found;++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))){found=true;image.save("build/embedded-pgs.png");break;}}
        QVERIFY(found);
    }
    void realAnime4kStatistics() {
        const QString source="D:/Animation Enhance/MyGO BDRemux/01.mkv";const auto preset=QDir(QCoreApplication::applicationDirPath()).filePath("../../dist/VS-Renderer-GUI-windows-x64/vpy/Anime4K Default.vpy");if(!QFileInfo::exists(source) || !QFileInfo::exists(preset))QSKIP("Local test media or preset is unavailable");
        QVERIFY(PresetStore::load(preset,source).script.contains("clip = _vsr_sharpen_chain"));
        PlayerWindow player;player.show();player.loadPreset(preset);QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>5,30000);QTest::qWait(1500);player.activateWindow();QTRY_VERIFY(player.isActiveWindow());player.findChild<PreviewPane *>()->surface()->setFocus();QTest::keyClick(&player,Qt::Key_Tab);QTest::qWait(550);QString text;for(auto *label:player.findChildren<QLabel *>())if(label->text().contains(QStringLiteral("VS 跳过")))text=label->text();QVERIFY(!text.isEmpty());QVERIFY(text.contains(QRegularExpression(QStringLiteral("GPU：\\d+\\.\\d+%"))));QVERIFY(player.skippedFrames()>0);QCOMPARE(player.outputSnapshot().videoWidth,3840u);QCOMPARE(player.outputSnapshot().videoHeight,2160u);qInfo().noquote()<<text;player.grab().save("build/player-real-tab-layout.png");player.togglePlayback();
        QTest::qWait(2500);const auto paused=player.outputSnapshot().swapChainPresents;QTest::qWait(1000);QCOMPARE(player.outputSnapshot().swapChainPresents,paused);for(auto *label:player.findChildren<QLabel *>())if(label->text().contains(QStringLiteral("VS 跳过")))qInfo().noquote()<<"Paused (extreme preset):"<<label->text();
    }
    void bundledLav() {
        QTemporaryDir dir; const QString source = dir.filePath("sample.mkv"); QVERIFY(QFile::copy(":/startup/warmup.mkv",source));
        LavPlayback lav; QVERIFY2(lav.open(source), qPrintable(lav.error())); QVERIFY(lav.duration() > 0);
        QVERIFY(lav.play()); QTest::qWait(150); QVERIFY(lav.position() > 0); QVERIFY(lav.pause()); QVERIFY(lav.seek(0));
        lav.volume(.4f, true);
    }
    void fullscreenAndFolderPlaylist() {
        QTemporaryDir dir;QVERIFY(QDir().mkpath(dir.filePath("child")));QVERIFY(QFile::copy(":/startup/warmup.mkv",dir.filePath("one.mkv")));QVERIFY(QFile::copy(":/startup/warmup.mkv",dir.filePath("child/two.mkv")));
        QFile note(dir.filePath("notes.txt"));QVERIFY(note.open(QIODevice::WriteOnly));note.write("test");note.close();
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());
        auto *pane=player.findChild<PreviewPane *>();auto *placeholder=pane->findChild<QLabel *>();QVERIFY(placeholder);
        QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(10,10),placeholder->mapToGlobal(QPoint(10,10)));QApplication::sendEvent(placeholder,&context);
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu && menu->isVisible());QCOMPARE(menu->actions()[0]->text(),QStringLiteral("打开文件…"));QCOMPARE(menu->actions()[1]->text(),QStringLiteral("打开文件夹…"));QCOMPARE(menu->actions()[2]->text(),QStringLiteral("打开链接…"));menu->close();
        QVERIFY(player.openFolder(dir.path()));auto *list=player.findChild<QTreeWidget *>("playerPlaylist");QVERIFY(list);QCOMPARE(list->topLevelItemCount(),3);auto *folder=list->topLevelItem(0);QVERIFY(folder->data(0,Qt::UserRole+1).toBool());folder->setExpanded(true);QTRY_COMPARE(folder->childCount(),1);QCOMPARE(folder->child(0)->text(0),QString("two.mkv"));
        player.activateWindow();QTRY_VERIFY(player.isActiveWindow());pane->surface()->setFocus();QTest::keyClick(&player,Qt::Key_Return);QTRY_VERIFY(player.isFullScreen());
        QCursor::setPos(player.mapToGlobal(player.rect().center()));auto *controls=player.findChild<QWidget *>("playerControls");QVERIFY(controls);QTRY_VERIFY_WITH_TIMEOUT(!controls->isVisible(),4000);
        QCursor::setPos(player.mapToGlobal(QPoint(player.width()/2,player.height()-8)));QTRY_VERIFY(controls->isVisible());
        QCursor::setPos(player.mapToGlobal(QPoint(player.width()-4,player.height()/2)));QTRY_VERIFY(player.findChild<QWidget *>("playerPlaylistPanel")->isVisible());
        QCursor::setPos(player.mapToGlobal(player.rect().center()));QTRY_VERIFY(!player.findChild<QWidget *>("playerPlaylistPanel")->isVisible());
        QTest::keyClick(&player,Qt::Key_Return);QTRY_VERIFY(!player.isFullScreen());QVERIFY(controls->isVisible());
    }
    void playlistDockAndWheel() {
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());
        auto *pane=player.findChild<PreviewPane *>();auto *panel=player.findChild<QWidget *>("playerPlaylistPanel");
        QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(player.centralWidget()->width()-200,100)));QTRY_VERIFY(panel->isVisible());
        QCursor::setPos(player.mapToGlobal(QPoint(80,100)));QTRY_VERIFY(!panel->isVisible());
        QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QAction *pin=nullptr;QMenu *wheel=nullptr;
        for(auto *action:menu->actions()){if(action->text()==QStringLiteral("播放列表"))pin=action;if(action->text()==QStringLiteral("鼠标滚轮"))wheel=action->menu();}QVERIFY(pin && wheel);
        const int width=player.width();pin->trigger();menu->close();QTRY_VERIFY(panel->isVisible());QVERIFY(player.width()>width);QCOMPARE(panel->parentWidget(),player.findChild<QSplitter *>("playerVideoSplit"));
        auto *split=player.findChild<QSplitter *>("playerVideoSplit");split->setSizes({700,400});QTest::qWait(100);QVERIFY(panel->width()>350);
        player.toggleFullscreen();QVERIFY(player.isFullScreen());QVERIFY(panel->parentWidget()==player.centralWidget());player.toggleFullscreen();QTRY_VERIFY(panel->isVisible());QCOMPARE(panel->parentWidget(),split);
        player.findChild<QPushButton *>("playerPlaylistClose")->click();QTRY_VERIFY(!panel->isVisible());
        auto *volume=player.findChild<QSlider *>("playerVolume");volume->setValue(50);
        QWheelEvent up(QPointF(20,20),pane->surface()->mapToGlobal(QPoint(20,20)),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(pane->surface(),&up);QCOMPARE(volume->value(),55);QCOMPARE(pane->zoom(),1.f);
        // Reopen a fresh menu after the old popup was destroyed.
        QApplication::sendEvent(pane->surface(),&event);menu=player.findChild<QMenu *>("playerContextMenu");for(auto *action:menu->actions())if(action->text()==QStringLiteral("鼠标滚轮"))wheel=action->menu();wheel->actions().last()->trigger();menu->close();
        QApplication::sendEvent(pane->surface(),&up);QVERIFY(pane->zoom()>1);QCOMPARE(volume->value(),55);
    }
    void alistRedirectRangeProxy() {
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));const QByteArray media("abcdefghijklmnopqrstuvwxyz");int apiCalls=0,redirects=0;
        connect(&server,&QTcpServer::newConnection,&server,[&]{while(server.hasPendingConnections()){auto *socket=server.nextPendingConnection();connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{auto bytes=socket->property("request").toByteArray()+socket->readAll();socket->setProperty("request",bytes);if(socket->property("done").toBool() || !bytes.contains("\r\n\r\n"))return;
            const auto target=bytes.split(' ')[1];QByteArray body,header;
            if(target=="/api/fs/get") {const auto length=QRegularExpression("Content-Length: (\\d+)",QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(bytes)).captured(1).toInt();if(bytes.size()-bytes.indexOf("\r\n\r\n")-4<length)return;++apiCalls;body=QJsonDocument(QJsonObject{{"code",200},{"data",QJsonObject{{"raw_url",QString("http://127.0.0.1:%1/redirect").arg(server.serverPort())}}}}).toJson();header="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";}
            else if(target=="/redirect"){++redirects;header="HTTP/1.1 302 Found\r\nLocation: /raw\r\n";}
            else if(target=="/raw"){const auto range=QRegularExpression("Range: bytes=(\\d+)-(\\d*)",QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(bytes));const int start=range.hasMatch()?range.captured(1).toInt():0,end=range.hasMatch()&&!range.captured(2).isEmpty()?range.captured(2).toInt():media.size()-1;body=media.mid(start,end-start+1);header="HTTP/1.1 206 Partial Content\r\nContent-Type: video/x-matroska\r\nAccept-Ranges: bytes\r\nContent-Range: bytes "+QByteArray::number(start)+"-"+QByteArray::number(end)+"/26\r\n";}
            else {body="<html>AList</html>";header="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";}
            socket->setProperty("done",true);socket->write(header+"Connection: close\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);socket->disconnectFromHost();});}});
        PlayerNetworkInput input;QSignalSpy ready(&input,&PlayerNetworkInput::ready),errors(&input,&PlayerNetworkInput::errorOccurred);input.open(QUrl(QString("http://127.0.0.1:%1/OpenShare/test.mkv").arg(server.serverPort())));QTRY_COMPARE_WITH_TIMEOUT(ready.size(),1,5000);QVERIFY(errors.isEmpty());QCOMPARE(apiCalls,1);QVERIFY(redirects>=1);
        QNetworkAccessManager client;QNetworkRequest request(QUrl(ready.first().first().toString()));request.setRawHeader("Range","bytes=5-9");auto *reply=client.get(request);QSignalSpy finished(reply,&QNetworkReply::finished);QTRY_COMPARE(finished.size(),1);QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),206);QCOMPARE(reply->readAll(),QByteArray("fghij"));reply->deleteLater();input.cancel();
    }
    void builtinProfilesAndNative4k() {
        const QString source="D:/Animation Enhance/GBC 108048/Girls.Band.Cry.03.AV1.FLAC.1080p48F_2026.09.26-19.25.11.mkv";if(!QFileInfo::exists(source))QSKIP("Local 4K48 AV1 media unavailable");
        PlayerWindow player;player.show();const auto builtin=QDir(PresetStore::directory()).filePath("builtin");player.loadPreset(QDir(builtin).filePath("Anime.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>20,20000);QCOMPARE(player.outputSnapshot().videoWidth,3840u);QCOMPARE(player.skippedFrames(),0u);
        player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::keyClick(&player,Qt::Key_Tab);QTest::qWait(1000);const auto first=player.outputSnapshot();QTest::qWait(8000);const auto last=player.outputSnapshot();
        const auto presented=last.presentedVideoFrames-first.presentedVideoFrames,dropped=last.droppedVideoFrames-first.droppedVideoFrames,coalesced=last.coalescedVideoFrames-first.coalescedVideoFrames;qInfo()<<"4K48 native direct: presented"<<presented<<"dropped"<<dropped<<"coalesced"<<coalesced<<"decode"<<last.decodeMode;
        QVERIFY(presented>300);QVERIFY(double(dropped+coalesced)/(presented+dropped+coalesced)<.05);
        player.togglePlayback();QTest::qWait(1000);const auto paused=player.outputSnapshot().swapChainPresents;QTest::qWait(700);QCOMPARE(player.outputSnapshot().swapChainPresents,paused);
        player.loadPreset(QDir(builtin).filePath("Realistic.vpy"));QTest::qWait(300);QCOMPARE(player.outputSnapshot().videoWidth,3840u);
    }
    void animeTargetAndAdaptiveFallback() {
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("performance/predecode",false);settings.sync();
        QTemporaryDir dir;const auto video=dir.filePath("wide.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=640x360:rate=48:duration=60","-c:v","libx264","-preset","ultrafast","-y",video});QVERIFY(ffmpeg.waitForFinished(20000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.show();const auto preset=QDir(PresetStore::directory()).filePath("builtin/Anime.vpy");QFile file(preset);QVERIFY(file.open(QIODevice::ReadOnly));const auto original=file.readAll();file.close();const auto restore=qScopeGuard([&]{QFile saved(preset);if(saved.open(QIODevice::WriteOnly))saved.write(original);});
        player.loadPreset(preset);QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().videoWidth>640 && player.outputSnapshot().videoWidth<1920,15000);QVERIFY(player.outputSnapshot().videoHeight<=1080);
        auto *pane=player.findChild<PreviewPane *>();const auto bounds=pane->surface()->size()*pane->surface()->devicePixelRatioF();QVERIFY(int(player.outputSnapshot().videoWidth)<=bounds.width());QVERIFY(int(player.outputSnapshot().videoHeight)<=bounds.height());
        player.togglePlayback();player.seekFrame(6);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,6,5000);
        auto script=QString::fromUtf8(original);script.replace("clip.set_output(0)","import time\ndef _slow(n, f):\n    time.sleep(0.08)\n    return f\nclip = core.std.ModifyFrame(clip, clip, _slow)\nclip.set_output(0)");QVERIFY(PresetStore::write(preset,script));player.loadPreset(preset);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,10000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);
        for(int n=0;n<4;++n){player.resize(1100+n*30,620+n*15);QTest::qWait(1500);QCOMPARE(player.qualityStage(),0);}
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,10000);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(2500);QCOMPARE(player.qualityStage(),0);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);
        QTRY_COMPARE_WITH_TIMEOUT(player.qualityStage(),1,12000);QVERIFY(player.outputSnapshot().videoWidth!=640u);
        for(int stage=2;stage<=3;++stage){QTRY_COMPARE_WITH_TIMEOUT(player.qualityStage(),stage,15000);QVERIFY(player.outputSnapshot().videoWidth!=640u);}
        QTRY_COMPARE_WITH_TIMEOUT(player.qualityStage(),4,15000);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>10,5000);QCOMPARE(player.outputSnapshot().videoWidth,640u);
        player.togglePlayback();QTest::qWait(100);qInfo()<<"Anime fallback CNN+enhancement -> CNN -> no CNN+enhancement -> no CNN -> Jinc verified";
        QVERIFY(!QFileInfo::exists(video+".ffindex"));QVERIFY(!QFileInfo::exists(video+".lwi"));
    }
    void suppliedRemoteLinks() {
        if(!qEnvironmentVariableIsSet("VSR_REMOTE_SMOKE"))QSKIP("Explicit remote smoke test; set VSR_REMOTE_SMOKE=1");
        const QStringList links{
            "https://modelscope.cn/datasets/ARXChem/Animations-List/resolve/master/K-ON%21/%5BVCB-Studio%5D%20K-ON%21%20%5BMa10p_1080p%5D/%5BVCB-Studio%5D%20K-ON%21%20%5B01%5D%5BMa10p_1080p%5D%5Bx265_flac_2aac%5D.mkv",
            "http://192.168.3.14:5244/OpenShare/139Cloud-%E5%BD%B1%E8%A7%86%E8%B5%84%E6%BA%90/Adachi%20to%20Shimamura%202160/Adachi.to.Shimamura.BDRip.2160p.AV1-10bit.FLAC.01-HDRFixed.mkv"};
        for(const auto &link:links){
            PlayerNetworkInput input;QSignalSpy ready(&input,&PlayerNetworkInput::ready),errors(&input,&PlayerNetworkInput::errorOccurred);input.open(QUrl(link));QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty() || !errors.isEmpty(),30000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
            QNetworkAccessManager client;QNetworkRequest request(QUrl(ready.first().first().toString()));request.setRawHeader("Range","bytes=0-1023");auto *reply=client.get(request);QSignalSpy finished(reply,&QNetworkReply::finished);QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,30000);QCOMPARE(reply->error(),QNetworkReply::NoError);const auto data=reply->readAll();QCOMPARE(data.left(4),QByteArray::fromHex("1a45dfa3"));QCOMPARE(data.size(),1024);reply->deleteLater();
            qInfo()<<QUrl(link).host()<<"resolved + Matroska Range verified";
        }
        {PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Realistic.vpy"));QVERIFY(player.openFile(links.first()));for(int seconds=0;seconds<40 && player.outputSnapshot().presentedVideoFrames<=5;++seconds){QTest::qWait(1000);if(seconds%5==0)qInfo().noquote()<<"Native loading:"<<player.findChild<QLabel *>("playerStatus")->text();}QVERIFY(player.outputSnapshot().presentedVideoFrames>5);qInfo()<<"ModelScope native playback"<<player.outputSnapshot().videoWidth<<player.outputSnapshot().videoHeight;
        QVERIFY(player.openFile(links.last()));QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,3840u,40000);QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,10000);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>5,10000);qInfo()<<"AList native playback"<<player.outputSnapshot().videoWidth<<player.outputSnapshot().videoHeight;
        }

    }
    void uiFontsApplyAndPersist() {
        const auto oldFont=qApp->font();const auto oldFallback=QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Han);
        const auto restore=qScopeGuard([&]{qApp->setFont(oldFont);QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,oldFallback);});
        const QString latin="Times New Roman",chinese="SimSun";QVERIFY(QFontDatabase::families().contains(latin));QVERIFY(QFontDatabase::families().contains(chinese));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/language","zh_CN");settings.sync();
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QAction *action=nullptr;for(auto *a:menu->actions())if(a->text()==QStringLiteral("设置…"))action=a;QVERIFY(action);menu->close();bool applied=false;
        QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);const auto close=qScopeGuard([dialog]{dialog->reject();});dialog->findChild<QFontComboBox *>("playerChineseFont")->setCurrentFont(QFont(chinese));dialog->findChild<QFontComboBox *>("playerLatinFont")->setCurrentFont(QFont(latin));dialog->findChild<QPushButton *>("playerApplySettings")->click();
            QCOMPARE(QFontInfo(player.font()).family(),latin);QCOMPARE(QFontInfo(dialog->font()).family(),latin);QCOMPARE(QFontInfo(player.findChild<QLabel *>("playerStatus")->font()).family(),latin);QCOMPARE(QFontInfo(dialog->findChild<QPushButton *>("playerCancelSettings")->font()).family(),latin);QCOMPARE(dialog->findChild<QFontComboBox *>("playerChineseFont")->currentFont().family(),chinese);QCOMPARE(dialog->findChild<QFontComboBox *>("playerLatinFont")->currentFont().family(),latin);
            QTextLayout layout(QStringLiteral("应用"),player.font());layout.beginLayout();layout.createLine();layout.endLayout();QVERIFY(!layout.glyphRuns().isEmpty());QCOMPARE(layout.glyphRuns().first().rawFont().familyName(),chinese);dialog->grab().save("build/player-ui-fonts.png");applied=true;});
        action->trigger();QVERIFY(applied);settings.sync();QCOMPARE(settings.value("theme/chineseFont").toString(),chinese);QCOMPARE(settings.value("theme/latinFont").toString(),latin);
        PlayerWindow reopened;reopened.show();QCOMPARE(QFontInfo(reopened.findChild<QLabel *>("playerStatus")->font()).family(),latin);
    }
    void colorSettingsPreserveVsChain() {
        QTemporaryDir dir;const auto video=dir.filePath("color.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",video));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("playback/remember",false);settings.sync();
        FilterGraph graph;const auto preset=dir.filePath("color.vpy");QVERIFY(PresetStore::write(preset,PresetStore::create(graph,SourceFilter::Ffms2,{},{})));
        PlayerWindow player;player.show();player.loadPreset(preset);QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);
        QSignalSpy reloads(player.server_.get(),&VapourSynthFrameServer::scriptLoaded);const auto at=player.position();const auto original=player.settings_->value("player/preset");
        bool edited=false;QTimer::singleShot(100,&player,[&]{
            auto *dialog=player.findChild<QDialog *>("playerColorSettings");QVERIFY(dialog);const auto close=qScopeGuard([dialog]{dialog->reject();});
            auto *engine=dialog->findChild<QComboBox *>("colorEngine");auto *icc=dialog->findChild<QComboBox *>("color_icc");auto *tone=dialog->findChild<QComboBox *>("color_tone");QVERIFY(engine && icc && tone);
            QCOMPARE(engine->currentIndex(),0);QVERIFY(!tone->isEnabled());engine->setCurrentIndex(1);QVERIFY(tone->isEnabled());icc->setCurrentIndex(0);
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            QTRY_COMPARE_WITH_TIMEOUT(player.output_->colorStatus().activeEngine,1u,20000);QCOMPARE(reloads.count(),0);QCOMPARE(player.position(),at);QCOMPARE(player.settings_->value("player/preset"),original);
            QTest::qWait(150);
            QVERIFY(dialog->grab().save("build/player-color-settings.png"));edited=true;
        });player.showColorSettings();QVERIFY(edited);
        QSettings saved(configPath(),QSettings::IniFormat);QCOMPARE(saved.value("color/engine").toInt(),1);QCOMPARE(saved.value("color/icc").toInt(),0);
        player.settings_->setValue("color/engine",0);player.applyColorSettings();QTRY_COMPARE(player.output_->colorStatus().activeEngine,0u);QCOMPARE(reloads.count(),0);QCOMPARE(player.position(),at);
        bool ordered=false;QTimer::singleShot(100,&player,[&]{
            auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);const auto close=qScopeGuard([dialog]{dialog->reject();});dialog->findChild<QListWidget *>()->setCurrentRow(4);
            auto *color=dialog->findChild<QPushButton *>("playerColorConfig");QVERIFY(color);QPushButton *videoButton=nullptr,*audioButton=nullptr;
            for(auto *button:dialog->findChildren<QPushButton *>()){if(button->text().contains("LAV Video"))videoButton=button;if(button->text().contains("LAV Audio"))audioButton=button;}
            QVERIFY(videoButton && audioButton);QVERIFY(color->mapTo(dialog,QPoint()).y()<videoButton->mapTo(dialog,QPoint()).y());QVERIFY(videoButton->mapTo(dialog,QPoint()).y()<audioButton->mapTo(dialog,QPoint()).y());ordered=true;
        });player.showSettings();QVERIFY(ordered);
    }
    void generatedColorMatrixUsesVsProps() {
        FilterGraph graph;graph.add("ccd");const auto generated=VpyScriptBuilder::build("stub.mkv",SourceFilter::Ffms2,graph).script;
        const auto start=generated.indexOf("def _vsr_matrix(c):"),finish=generated.indexOf("\nclip = src",start);QVERIFY(start>=0 && finish>start);const auto helper=generated.mid(start,finish-start);
        VapourSynthFrameServer server;QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),20000);QVERIFY2(server.available(),qPrintable(server.errorString()));
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        for(const auto primary:{1,9}) {
            loaded.clear();errors.clear();frames.clear();
            const auto script=QString("import vapoursynth as vs\ncore=vs.core\n%1\nc=core.std.BlankClip(width=64,height=48,length=2,format=vs.RGBS,color=[.4,.2,.1])\nc=core.std.SetFrameProps(c,_Matrix=0,_Primaries=%2,_Transfer=16,_ColorRange=0)\nc=core.resize.Bicubic(c,format=vs.YUV444P16,matrix=_vsr_matrix(c))\nc.set_output()\n").arg(helper).arg(primary);
            server.loadScript(script,"matrix-color-test.vpy");QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty()||!errors.isEmpty(),20000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));server.requestFrame(0);QTRY_VERIFY(!frames.isEmpty());
            const auto frame=qvariant_cast<VapourSynthFrame>(frames.first().first());QCOMPARE(frame.colorMatrix,unsigned(primary));QCOMPARE(frame.colorPrimaries,unsigned(primary));QCOMPARE(frame.colorTransfer,16u);
        }
    }
    void settingsPagesAndLanguage() {
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();
        QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QAction *settingsAction=nullptr;for(auto *action:menu->actions())if(action->text()==QStringLiteral("设置…"))settingsAction=action;QVERIFY(settingsAction);menu->close();
        bool edited=false;QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");if(!dialog)return;auto *categories=dialog->findChild<QListWidget *>();if(!categories || categories->count()!=8){dialog->reject();return;}
            auto *load=dialog->findChild<QPushButton *>("playerLoadSettings"),*save=dialog->findChild<QPushButton *>("playerSaveSettings"),*cancel=dialog->findChild<QPushButton *>("playerCancelSettings"),*ok=dialog->findChild<QPushButton *>("playerConfirmSettings"),*apply=dialog->findChild<QPushButton *>("playerApplySettings");QVERIFY(load && save && cancel && ok && apply);QVERIFY(load->x()<save->x());QVERIFY(save->x()+save->width()<cancel->x());QVERIFY(cancel->x()<ok->x() && ok->x()<apply->x());QVERIFY(cancel->text().contains("&N"));QVERIFY(ok->text().contains("&Y"));QVERIFY(apply->text().contains("&A"));
            categories->setCurrentRow(7);dialog->findChild<QPushButton *>("playerSelectImages")->click();auto *formats=dialog->findChild<QListWidget *>("playerAssociationFormats");for(int row=0;row<formats->count();++row){const auto *item=formats->item(row);QCOMPARE(item->checkState()==Qt::Checked,playerImageExtensions().contains(item->data(Qt::UserRole).toString()));}dialog->grab().save("build/player-settings-footer-images.png");
            dialog->findChild<QComboBox *>("playerLanguage")->setCurrentIndex(1);dialog->findChild<QCheckBox *>("playerAutoplay")->setChecked(false);dialog->findChild<QSpinBox *>("playerOpacity")->setValue(90);dialog->findChild<QPushButton *>("playerBackground")->setText("#172839");dialog->findChild<QSpinBox *>("playerCtrlSeconds")->setValue(15);dialog->findChild<QSpinBox *>("playerCtrlAltSeconds")->setValue(45);dialog->findChild<QComboBox *>("playerAudioDecoder")->setCurrentIndex(1);auto *anime=dialog->findChild<QComboBox *>("playerAnimeStage");QVERIFY(anime);QCOMPARE(anime->count(),6);anime->setCurrentIndex(3);edited=true;dialog->findChild<QPushButton *>("playerConfirmSettings")->click();});settingsAction->trigger();QVERIFY(edited);
        QSettings saved(configPath(),QSettings::IniFormat);QCOMPARE(saved.value("basic/language").toString(),"en_US");QCOMPARE(saved.value("decode/video").toString(),"3FP");QCOMPARE(saved.value("decode/audio").toString(),"LAV");QCOMPARE(saved.value("player/animeStage").toInt(),3);QVERIFY(!saved.value("basic/autoplay").toBool());QVERIFY(std::abs(player.windowOpacity()-.9)<.01);QVERIFY(player.styleSheet().contains("#172839"));
        QContextMenuEvent englishEvent(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&englishEvent);menu=player.findChild<QMenu *>("playerContextMenu");bool english=false;for(auto *action:menu->actions())if(action->text()=="Settings…"){settingsAction=action;english=true;}QVERIFY(english);menu->close();
        QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);auto *categories=dialog->findChild<QListWidget *>();QCOMPARE(categories->item(0)->text(),"General");QCOMPARE(categories->item(6)->text(),"Cache");QCOMPARE(categories->item(7)->text(),"File associations");dialog->grab().save("build/player-settings-english.png");dialog->reject();});settingsAction->trigger();
    }
    void applySettingsKeepsPlaybackState() {
        QTemporaryDir directory;const auto video=directory.filePath("apply.mkv");QProcess ffmpeg;
        ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=160x96:rate=24:duration=8","-c:v","ffv1","-y",video});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.setValue("player/preset","");settings.sync();
        PlayerWindow player;player.show();player.loadPreset({});QVERIFY(player.openFile(video));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,15000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);player.seekTime(20000000);QTRY_VERIFY(qAbs(player.position()-20000000)<1000000);
        for(const bool resume:{false,true}) {
            if(resume){player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);}
            const auto at=player.position();auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);
            auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QAction *action=nullptr;for(auto *candidate:menu->actions())if(candidate->text()==QStringLiteral("设置…"))action=candidate;QVERIFY(action);menu->close();bool applied=false;
            QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);const auto close=qScopeGuard([dialog]{dialog->reject();});auto *button=dialog->findChild<QPushButton *>("playerApplySettings");QVERIFY(button);dialog->findChild<QSpinBox *>("playerOpacity")->setValue(95);auto *start=dialog->findChild<QComboBox *>("playerInterpolationStart");QVERIFY(start);QCOMPARE(start->count(),4);start->setCurrentIndex(2);button->click();QCOMPARE(player.settings_->value("player/interpolationStart").toInt(),2);QVERIFY(dialog->isVisible());QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);QTRY_VERIFY(qAbs(player.position()-at)<10000000);QTest::qWait(250);if(resume){QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);}else{QVERIFY(player.snapshot().state!=ThreeFpState::Playing);const auto pausedAt=player.position();QTest::qWait(300);QCOMPARE(player.position(),pausedAt);}QVERIFY(qAbs(player.windowOpacity()-.95)<.01);applied=true;});
            action->trigger();QVERIFY(applied);
        }
    }
    void indexCacheAndBitdepthResize() {
        QTemporaryDir dir;QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("cache/path",dir.filePath("indexes"));settings.setValue("basic/autoplay",false);settings.setValue("performance/resizeBeforeEnhance",true);settings.sync();
        const auto source=dir.filePath("tenbit.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=1280x720:rate=24:duration=3","-pix_fmt","yuv420p10le","-c:v","ffv1","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        PlayerWindow player;player.resize(1000,500);player.show();auto *surface=player.pane_->surface();const auto dpi=surface->devicePixelRatioF();surface->setFixedSize(qRound(640/dpi),qRound(360/dpi));player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);QVERIFY(player.outputSnapshot().videoWidth<1280u);QVERIFY(playerCacheBytes(playerCacheDirectory(settings))>0);QVERIFY(!QFileInfo::exists(source+".ffindex"));QVERIFY(!QFileInfo::exists(source+".lwi"));
        const auto first=playerIndexPath(settings,source,"ffindex");QVERIFY(QFileInfo::exists(first));const auto stamp=QFileInfo(first).lastModified();QTest::qWait(500);player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,10000);QCOMPARE(QFileInfo(first).lastModified(),stamp);
        FilterGraph graph;const auto preset=dir.filePath("lsmas.vpy");QVERIFY(PresetStore::write(preset,PresetStore::create(graph,SourceFilter::Lsmas,{},{})));player.loadPreset(preset);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().videoWidth,1280u,15000);QVERIFY(QFileInfo::exists(playerIndexPath(settings,source,"lwi")));
        QFile unrelated(dir.filePath("indexes/keep.txt"));QVERIFY(unrelated.open(QIODevice::WriteOnly));unrelated.write("keep");unrelated.close();QVERIFY(clearPlayerIndexes(playerCacheDirectory(settings)));QCOMPARE(playerCacheBytes(playerCacheDirectory(settings)),0);QVERIFY(unrelated.exists());
    }
    void autoplayRememberAndKeys() {
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("playback/ctrlSeconds",10);settings.setValue("playback/ctrlAltSeconds",60);settings.sync();QTemporaryDir dir;const auto source=dir.filePath("remember.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24:duration=80","-c:v","libx264","-preset","ultrafast","-g","24","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        FilterGraph graph;const auto preset=dir.filePath("local.vpy");QVERIFY(PresetStore::write(preset,PresetStore::create(graph,SourceFilter::Ffms2,{},{})));
        {PlayerWindow player;player.show();player.activateWindow();player.loadPreset(preset);QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);QTest::qWait(500);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);QVERIFY(player.position()<1000000);player.seekFrame(72);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,72,5000);}
        PlayerWindow player;player.show();player.activateWindow();player.loadPreset(preset);QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,72,15000);QVERIFY(std::abs(player.position()-30000000)<50000);
        auto *pane=player.findChild<PreviewPane *>();pane->surface()->setFocus();QTRY_VERIFY(player.isActiveWindow());QTest::keyClick(pane->surface(),Qt::Key_Right);QTRY_VERIFY(std::abs(player.position()-40000000)<50000);QTest::keyClick(pane->surface(),Qt::Key_Right,Qt::ControlModifier);QTRY_VERIFY(std::abs(player.position()-140000000)<50000);QTest::keyClick(pane->surface(),Qt::Key_Right,Qt::ControlModifier|Qt::AltModifier);QTRY_VERIFY(std::abs(player.position()-740000000)<50000);
        FilterGraph interpolation;const int rife=interpolation.add("rife");interpolation.setParameter(rife,"inference_scale",QStringLiteral("2 - 半宽半高"));const auto doubled=dir.filePath("double.vpy");QVERIFY(PresetStore::write(doubled,PresetStore::create(interpolation,SourceFilter::Ffms2,{},{})));player.loadPreset(doubled);QTRY_COMPARE_WITH_TIMEOUT(player.outputSnapshot().frameIndex,3552,15000);QVERIFY(std::abs(player.position()-740000000)<50000);
    }
    void independentLavAudioAndAssociations() {
        QTemporaryDir dir;const auto source=dir.filePath("split.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24:duration=6","-f","lavfi","-i","sine=duration=6","-c:v","libx264","-preset","ultrafast","-c:a","aac","-y",source});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        LavPlayback audio;QVERIFY2(audio.open(source,nullptr,false,true),qPrintable(audio.error()));QVERIFY(audio.play());QTest::qWait(500);QVERIFY(audio.position()>1000000);QVERIFY(audio.pause());
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("decode/video","3FP");settings.setValue("decode/audio","LAV");settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>4,15000);QTRY_VERIFY(player.findChild<QPushButton *>("playerDecoder")->text().startsWith("3FP"));player.togglePlayback();
        const auto registry="HKEY_CURRENT_USER\\Software\\VSRendererTests\\"+QUuid::createUuid().toString(QUuid::WithoutBraces);const auto cleanup=qScopeGuard([&]{QSettings erase(registry,QSettings::NativeFormat);erase.clear();});QVERIFY(registerPlayerAssociations({"mkv","flac"},"C:/Portable Player/vs-player.exe",registry));QSettings keys(registry+"\\Classes",QSettings::NativeFormat);QCOMPARE(keys.value("VSPlayer.Media/shell/open/command/.").toString(),QString("\"C:\\Portable Player\\vs-player.exe\" \"%1\""));QVERIFY(keys.contains(".mkv/OpenWithProgids/VSPlayer.Video"));QVERIFY(registerPlayerAssociations({},"C:/Portable Player/vs-player.exe",registry));keys.sync();QVERIFY(!keys.contains(".mkv/OpenWithProgids/VSPlayer.Video"));
    }
    void audioOnlyAndRemember() {
        QTemporaryDir dir;const auto source=dir.filePath("audio.flac");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","sine=duration=8","-c:a","flac","-y",source});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.sync();
        {PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().duration100ns>0,15000);player.seekTime(30000000);QTRY_VERIFY(std::abs(player.position()-30000000)<50000);QVERIFY(!player.findChild<QLabel *>("playerStatus")->text().contains("执行失败"));}
        PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position()-30000000)<50000,5000);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
        settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.sync();PlayerWindow autoplay;autoplay.show();QVERIFY(autoplay.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(autoplay.position()>=30000000 && autoplay.position()<40000000,5000);QTRY_COMPARE(autoplay.snapshot().state,ThreeFpState::Playing);autoplay.togglePlayback();
    }
    void nativeD3D11Output() {
        const QString source="D:/Animation Enhance/GBC 108048/Girls.Band.Cry.02.AV1.FLAC.1080p48F.mkv";if(!QFileInfo::exists(source))QSKIP("Local AV1 file unavailable");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("render/upscale",7);settings.setValue("render/downscale",7);settings.sync();PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Realistic.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>10,20000);qInfo()<<"D3D11 native scaling mode"<<player.outputSnapshot().videoScalingMode;QCOMPARE(player.outputSnapshot().videoScalingMode,1u);player.togglePlayback();
        ThreeFpApi api;QWidget surface;surface.resize(960,540);surface.show();ThreeFpPlayer native(api,&surface);native.setDecodeMode(2);QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));QVERIFY(native.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(native.snapshot().state,ThreeFpState::Ready,15000);QVERIFY(native.seek(600000000));QTest::qWait(700);const auto image=native.capture();QVERIFY(!image.isNull());QSet<QRgb> colors;for(int y=30;y<image.height()-30;y+=30)for(int x=30;x<image.width()-30;x+=30)colors.insert(image.pixel(x,y));QVERIFY(colors.size()>8);QCOMPARE(native.snapshot().videoScalingMode,1u);
    }
    void httpVideoAndPausedGpu() {
        QTemporaryDir dir;const auto video=dir.filePath("remote.mkv");QProcess ffmpeg;ffmpeg.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=8","-c:v","libx264","-preset","ultrafast","-y",video});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        QFile file(video);QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));int requests=0;
        connect(&server,&QTcpServer::newConnection,&server,[&]{while(server.hasPendingConnections()){auto *socket=server.nextPendingConnection();connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{auto header=socket->property("header").toByteArray()+socket->readAll();socket->setProperty("header",header);if(!header.contains("\r\n\r\n"))return;++requests;const auto range=QRegularExpression("Range: bytes=(\\d+)-(\\d*)",QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(header));const qint64 start=range.hasMatch()?range.captured(1).toLongLong():0;const qint64 end=range.hasMatch()&&!range.captured(2).isEmpty()?qMin(range.captured(2).toLongLong(),bytes.size()-1):bytes.size()-1;QByteArray reply=range.hasMatch()?"HTTP/1.1 206 Partial Content\r\n":"HTTP/1.1 200 OK\r\n";reply+="Accept-Ranges: bytes\r\nContent-Type: video/x-matroska\r\nConnection: close\r\nContent-Length: "+QByteArray::number(end-start+1)+"\r\n";if(range.hasMatch())reply+="Content-Range: bytes "+QByteArray::number(start)+"-"+QByteArray::number(end)+"/"+QByteArray::number(bytes.size())+"\r\n";socket->write(reply+"\r\n"+bytes.mid(start,end-start+1));socket->disconnectFromHost();});}});
        const auto url=QString("http://127.0.0.1:%1/video%20name.mkv?token=test").arg(server.serverPort());
        FilterGraph graph;const auto preset=dir.filePath("remote.vpy");QVERIFY(PresetStore::write(preset,"raise Exception('Network must bypass VS')\n"));
        PlayerWindow player;player.show();player.loadPreset(preset);QVERIFY(player.openFile(url));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>3,15000);QVERIFY(requests>1);QVERIFY(player.windowTitle().contains("video name"));
        player.togglePlayback();player.seekFrame(24);QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position()-10000000)<50000,5000);QTRY_COMPARE(player.findChild<QLineEdit *>("playerFrame")->text(),"24");QCOMPARE(player.outputSnapshot().videoWidth,320u);
        const auto sub=dir.filePath("caption.srt");QFile subtitle(sub);QVERIFY(subtitle.open(QIODevice::WriteOnly));subtitle.write("1\n00:00:00,000 --> 00:00:08,000\nStatic paused caption\n");subtitle.close();QVERIFY(player.openFile(sub));QTest::qWait(1200);const auto presents=player.outputSnapshot().swapChainPresents;QTest::qWait(1000);QCOMPARE(player.outputSnapshot().swapChainPresents,presents);
        QVERIFY(PresetStore::write(preset,PresetStore::create(graph,SourceFilter::Lsmas,{},{})));QVERIFY(player.openFile(QUrl::fromLocalFile(video).toString()));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>3,15000);
    }
};
QTEST_MAIN(TestPlayer)
#include "TestPlayer.moc"
