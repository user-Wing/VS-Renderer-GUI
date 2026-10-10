#include "player/PlayerWindow.h"
#include "bluray/BlurayCatalog.h"
#include "player/PlayerInfoPanel.h"
#include "player/PlayerDiscMenu.h"
#include "player/PlayerTracks.h"
#include "graph/VpyScriptBuilder.h"
#include "graph/FilterCatalog.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/FrameTimeline.h"
#include "backend/LavPlayback.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"
#include <QApplication>
#include <QMouseEvent>
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
#include <QPointer>
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
#include <QScreen>
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
#include <QEventLoop>
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
#include "player/PlayerPng.h"
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
#include <cmath>
#include <cstring>
#include <thread>
#include <QScopeGuard>
using namespace vsr;
class TestPlayer final : public QObject {
    qint64 beforePauseRate_ = 0;
    QByteArray oldIni_; bool hadIni_=false;
    QString configPath() const {return QDir(QCoreApplication::applicationDirPath()).filePath("player.ini");}
    Q_OBJECT
private slots:
    void imageStartupBudget() {
        QTemporaryDir dir;const auto path=dir.filePath("startup.png");QImage image(960,640,QImage::Format_RGB32);image.fill(QColor(71,127,193));QVERIFY(image.save(path));
        const auto nativeBefore=GetModuleHandleW(L"FFF.Native.dll");
        QElapsedTimer elapsed;elapsed.start();PlayerWindow player;const auto construction=elapsed.elapsed();player.show();QVERIFY(player.openFile(path));
        auto *pane=player.findChild<PreviewPane *>();QTRY_VERIFY_WITH_TIMEOUT(!pane->image().isNull(),10000);
        qInfo()<<"image-startup-ms"<<elapsed.elapsed()<<"construction-ms"<<construction<<"decode-ms"<<pane->image().text("decodeMilliseconds");
        QCOMPARE(pane->image().size(),image.size());QCOMPARE(pane->image().pixelColor(123,321),image.pixelColor(123,321));
        QCOMPARE(GetModuleHandleW(L"FFF.Native.dll"),nativeBefore);QVERIFY(!player.server_->initializing());QVERIFY(!player.server_->available());
    }
    void hevc444HardwareFallback() {
        const auto path=qEnvironmentVariable("VSR_HEVC444_SOURCE");if(path.isEmpty())QSKIP("Set VSR_HEVC444_SOURCE for NVDEC regression");
        ThreeFpApi api;PreviewPane pane("NVDEC","Test");pane.resize(1920,1080);pane.show();pane.setSurfaceActive(true);ThreeFpPlayer player(api,pane.surface());player.setMuted(true);player.setDecodeMode(2);
        QElapsedTimer elapsed;elapsed.start();QVERIFY(player.openFile(path));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,30000);QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>8,30000);
        qInfo()<<"hevc-444-first-ms"<<elapsed.elapsed()<<"mode"<<player.snapshot().decodeMode<<"media"<<player.mediaInfo();
        QCOMPARE(player.snapshot().decodeMode,2u);QVERIFY(player.mediaInfo().contains("cuda",Qt::CaseInsensitive));
        QVERIFY(player.seek(10*10000000LL));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().position100ns>=10*10000000LL,10000);QVERIFY(player.pause());
    }
    void directStartupClock_data(){QTest::addColumn<int>("stage");QTest::newRow("original")<<-1;QTest::newRow("D3D11")<<5;QTest::newRow("Jinc")<<4;}
    void directStartupClock(){
        QFETCH(int,stage);QTemporaryDir directory;auto source=qEnvironmentVariable("VSR_STARTUP_CLOCK_SOURCE");
        if(source.isEmpty()){source=directory.filePath("silent-48.mkv");QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=640x360:rate=7001/146:duration=16","-c:v","libx264","-preset","ultrafast","-g","48","-threads","2","-y",source});QVERIFY(encoder.waitForFinished(20000));QCOMPARE(encoder.exitCode(),0);}
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.setValue("decode/mode",qEnvironmentVariableIntValue("VSR_STARTUP_CLOCK_CPU")?1:2);settings.setValue("player/speed",1.0);settings.setValue("player/muted",true);settings.sync();
        PlayerWindow player;player.show();player.loadPreset(stage<0?QString():QDir(PresetStore::directory()).filePath(QString("builtin/Anime-%1-%2.vpy").arg(stage).arg(stage==5?"D3D11":"Jinc")));QElapsedTimer cold;cold.start();QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,20000);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,15000);qInfo()<<"startup-clock-first-frame-ms"<<cold.elapsed()<<"source-fps"<<double(player.clip_.fpsNumerator)/player.clip_.fpsDenominator<<"display-hz"<<player.screen()->refreshRate()<<"decode-mode"<<player.snapshot().decodeMode;QVERIFY(player.direct_);QVERIFY(player.clip_.fpsNumerator>0 && player.clip_.fpsDenominator>0);
        bool correct=true;const double fps=double(player.clip_.fpsNumerator)/player.clip_.fpsDenominator;
        const auto measure=[&](const char *phase){const auto first=player.outputSnapshot();QElapsedTimer wall;wall.start();for(int i=0;i<6;++i){QTest::qWait(500);const auto s=player.outputSnapshot();qInfo()<<"startup-clock"<<phase<<"wall"<<wall.elapsed()/1000.<<"position"<<s.position100ns/1e7<<"pts"<<s.framePts<<"accepted"<<s.presentedVideoFrames<<"presents"<<s.swapChainPresents<<"generation"<<s.timelineGeneration;}const auto last=player.outputSnapshot();const double seconds=wall.elapsed()/1000.,media=(last.position100ns-first.position100ns)/1e7,accepted=double(last.presentedVideoFrames-first.presentedVideoFrames);const double frameTime=double(last.framePts)*last.frameTimeBaseNumerator/last.frameTimeBaseDenominator;correct &= qAbs(media-seconds)<.25 && qAbs(accepted-seconds*fps)<fps*.25 && qAbs(frameTime-last.position100ns/1e7)<.15;qInfo()<<"startup-clock-summary"<<phase<<"wall"<<seconds<<"media"<<media<<"accepted-fps"<<accepted/seconds<<"frame-clock-error"<<frameTime-last.position100ns/1e7<<"dropped"<<last.droppedVideoFrames-first.droppedVideoFrames<<"coalesced"<<last.coalescedVideoFrames-first.coalescedVideoFrames;QCOMPARE(last.state,ThreeFpState::Playing);QCOMPARE(last.timelineGeneration,first.timelineGeneration);};
        measure("open");const auto generation=player.snapshot().timelineGeneration;player.seekTime(50000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().timelineGeneration>generation,10000);measure("seek");player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(200);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);measure("resume");QVERIFY2(correct,"Media time and accepted frame cadence must follow wall time at 1x from initial playback, seek and resume");
    }
    void overlayControlsStayUsableDuringPlayback_data(){QTest::addColumn<bool>("vsPlayback");QTest::newRow("native")<<false;QTest::newRow("VS")<<true;}
    void overlayControlsStayUsableDuringPlayback(){
        QFETCH(bool,vsPlayback);
        QTemporaryDir directory;const auto video=directory.filePath("overlays.mkv");QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=640x360:rate=24:duration=30","-f","lavfi","-i","sine=duration=30","-c:v","mpeg4","-q:v","4","-g","24","-c:a","pcm_s16le","-y",video});QVERIFY(encoder.waitForFinished(20000));QCOMPARE(encoder.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.setValue("player/muted",true);settings.setValue("info/audioDetailed",true);settings.sync();PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());if(vsPlayback){const auto preset=directory.filePath("overlay.vpy");FilterGraph graph;QVERIFY(PresetStore::write(preset,PresetStore::create(graph,SourceFilter::Ffms2,{},{})));player.loadPreset(preset);}QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>8,20000);QTest::keyClick(&player,Qt::Key_Tab);QTRY_VERIFY(player.info_->isVisible() && player.audioInfo_->isVisible());
        const auto checkFooter=[&](QWidget *panel){auto *area=panel->findChild<QScrollArea *>("playerInfoTextArea");auto *slider=panel->findChild<QSlider *>("playerInfoOpacity");QVERIFY(area && slider);QVERIFY(slider->geometry().top()-area->geometry().bottom()>=qRound(panel->height()*.05));QVERIFY(panel->geometry().bottom()<player.controls_->geometry().top());QCursor::setPos(slider->mapToGlobal(slider->rect().center()));player.updateChrome();QVERIFY(!player.playlistPanel_->isVisible());QCOMPARE(QApplication::widgetAt(QCursor::pos()),slider);QTest::mouseClick(slider,Qt::LeftButton,Qt::NoModifier,QPoint(slider->width()-18,slider->height()/2));QVERIFY(slider->value()>50);};checkFooter(player.info_);checkFooter(player.audioInfo_);
        player.chromeTimer_->stop();const auto viewport=player.pane_->surface()->size();const auto before=player.outputSnapshot();const auto start=player.pos();QSignalSpy redraw(player.pane_,&PreviewPane::redrawRequested),view(player.pane_,&PreviewPane::viewChanged),reload(player.server_.get(),&VapourSynthFrameServer::scriptLoaded);
        for(int n=0;n<40;++n){player.move(start+QPoint(n%10*3,n/10*3));QCOMPARE(player.controls_->pos(),player.centralWidget()->mapToGlobal(QPoint(0,player.controlsTop_)));QCOMPARE(player.info_->pos(),player.pane_->surface()->mapToGlobal(QPoint(12,12)));QTest::qWait(10);}QCOMPARE(player.pane_->surface()->size(),viewport);QCOMPARE(redraw.size(),0);QCOMPARE(view.size(),0);QCOMPARE(reload.size(),0);QTRY_VERIFY(player.outputSnapshot().presentedVideoFrames>before.presentedVideoFrames+12);QCOMPARE(player.outputSnapshot().timelineGeneration,before.timelineGeneration);QCOMPARE(player.outputSnapshot().droppedVideoFrames,before.droppedVideoFrames);QCOMPARE(player.outputSnapshot().audioUnderruns,before.audioUnderruns);
        player.toggleFullscreen();QTRY_VERIFY(player.isFullScreen());QTest::qWait(200);player.updateInfo();const auto full=player.pane_->surface()->size();const auto generation=player.snapshot().timelineGeneration;
        for(int n=0;n<5;++n){QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(120,2)));player.updateChrome();QVERIFY(player.fullscreenTitle_->isVisible());QVERIFY(player.info_->geometry().top()>player.fullscreenTitle_->geometry().bottom());QVERIFY(player.audioInfo_->geometry().top()>player.fullscreenTitle_->geometry().bottom());QCOMPARE(player.pane_->surface()->size(),full);QCursor::setPos(player.audioInfo_->mapToGlobal(QPoint(player.audioInfo_->width()-30,100)));player.updateChrome();QVERIFY(!player.playlistPanel_->isVisible());player.titleIdle_.invalidate();player.updateChrome();QVERIFY(!player.fullscreenTitle_->isVisible());QCOMPARE(player.info_->pos().y(),player.pane_->surface()->mapToGlobal(QPoint(12,12)).y());}
        QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(120,2)));player.updateChrome();auto *mode=player.info_->findChild<QComboBox *>("playerInfoAudioMode");QCursor::setPos(mode->mapToGlobal(mode->rect().center()));player.updateChrome();QCOMPARE(QApplication::widgetAt(QCursor::pos()),mode);QTest::mouseClick(mode,Qt::LeftButton);QTRY_VERIFY(mode->view()->isVisible());QTest::keyClick(mode,Qt::Key_Home);QTest::keyClick(mode,Qt::Key_Enter);QTRY_VERIFY(!player.audioInfo_->isVisible());mode->setCurrentIndex(1);QTRY_VERIFY(player.audioInfo_->isVisible());player.updateChrome();QCOMPARE(player.pane_->surface()->size(),full);QCOMPARE(player.snapshot().timelineGeneration,generation);player.info_->grab().save("build/overlay-1.0.8/tab-panel.png");player.audioInfo_->grab().save("build/overlay-1.0.8/audio-panel.png");
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        if(!player.direct_)QTRY_VERIFY_WITH_TIMEOUT(!player.pending_ && player.lastFrame_==frameAtPosition100ns(player.position(),player.clip_.totalFrames,player.clip_.fpsNumerator,player.clip_.fpsDenominator),5000);
        QTest::qWait(100);const auto paused=player.outputSnapshot();auto *renderer=player.direct_?player.clock_.get():player.output_.get();const auto pixels=renderer->captureFloat();for(int n=0;n<10;++n){QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(120,n%2?2:300)));player.titleIdle_.invalidate();player.updateChrome();}QTest::qWait(100);QCOMPARE(player.outputSnapshot().presentedVideoFrames,paused.presentedVideoFrames);QCOMPARE(renderer->captureFloat(),pixels);QVERIFY(!pixels.isNull());
    }
    void screenshotPathsAndSettings(){
        QTemporaryDir directory;PlayerWindow player;player.show();QCOMPARE(player.screenshotDirectory(),QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto destination=directory.filePath(QStringLiteral("默认截图 路径"));
        QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);auto *path=dialog->findChild<QLineEdit *>("playerScreenshotPath");QVERIFY(path);path->setText(destination);dialog->findChild<QPushButton *>("playerApplySettings")->click();dialog->reject();});player.showSettings();QCOMPARE(player.screenshotDirectory(),destination);
        QImage image(64,48,QImage::Format_RGBA64);image.fill(QColor("#327acc"));player.imageMode_=true;player.source_="path-test.png";player.pane_->setImage(image);player.captureImage(true);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,10000);QCOMPARE(QDir(destination).entryList({"*-source.png"},QDir::Files).size(),1);
        QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(20,20),player.pane_->surface()->mapToGlobal(QPoint(20,20)));QApplication::sendEvent(player.pane_->surface(),&event);QPointer<QMenu> menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QMenu *specified=nullptr;for(auto *action:menu->actions())if(action->text()==QStringLiteral("截图到指定路径…"))specified=action->menu();QVERIFY(specified);QCOMPARE(specified->actions().size(),2);
        const auto selected=directory.filePath(QStringLiteral("单次截图 路径"));QVERIFY(QDir().mkpath(selected));const bool nativeDialogs=qApp->testAttribute(Qt::AA_DontUseNativeDialogs);qApp->setAttribute(Qt::AA_DontUseNativeDialogs,true);const auto restore=qScopeGuard([&]{qApp->setAttribute(Qt::AA_DontUseNativeDialogs,nativeDialogs);});
        QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QFileDialog *>();QVERIFY(dialog);dialog->setDirectory(selected);QMetaObject::invokeMethod(dialog,"accept");});specified->actions().first()->trigger();if(menu)menu->close();QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,10000);QCOMPARE(QDir(selected).entryList({"*-source.png"},QDir::Files).size(),1);QCOMPARE(player.screenshotDirectory(),destination);
    }
    void algorithmBadgesReflectCurrentStages(){
        PlayerWindow player;player.show();const auto builtin=QDir(PresetStore::directory()).filePath("builtin/");const QStringList presets{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy"},labels{"A4KCNN+","A4KCNN","A4K+","A4K","Jinc","D3D11"};
        for(int i=0;i<6;++i){player.preset_=builtin+presets[i];player.qualityStage_=i;player.direct_=i>=4;player.message_->clear();player.updateAlgorithmBadges();QCOMPARE(player.algorithmBadge_->text(),labels[i]);QVERIFY(!player.algorithmBadge_->isHidden());QVERIFY(player.interpolationBadge_->isHidden());QVERIFY(player.message_->isHidden());}
        const QStringList interpolation{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"},names{"RIFE+","RIFE","MVT+","MVT"};for(bool native:{false,true})for(int i=0;i<4;++i){player.preset_=builtin+interpolation[i];if(native)player.preset_.replace(".vpy","-D3D11.vpy");player.direct_=false;player.updateAlgorithmBadges();QCOMPARE(player.algorithmBadge_->text(),native?QString("D3D11"):QString("Jinc"));QCOMPARE(player.interpolationBadge_->text(),names[i]);QVERIFY(!player.interpolationBadge_->isHidden());}
        player.imageMode_=true;player.updateAlgorithmBadges();QVERIFY(player.algorithmBadge_->isHidden());QVERIFY(player.interpolationBadge_->isHidden());
    }
    void screenshotPng16RoundTrip(){
        QTemporaryDir directory;const auto path=directory.filePath("precision.png");QImage image(64,40,QImage::Format_RGBA64);image.setColorSpace(QColorSpace(QColorSpace::SRgb));
        for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgba64 *>(image.scanLine(y));for(int x=0;x<image.width();++x)row[x]=QRgba64::fromRgba64(quint16(x*977+y),quint16(y*1511+x),quint16(65535-x*941-y),quint16(32000+x*101+y));}
        QVERIFY2(writeScreenshotPng(path,image,{{"hdr",false}}).isEmpty(),"PNG write failed");QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();QCOMPARE(uchar(bytes[24]),uchar(16));QCOMPARE(uchar(bytes[25]),uchar(6));
        const auto decoded=QImage(path).convertToFormat(QImage::Format_RGBA64);QCOMPARE(decoded.size(),image.size());for(int y=0;y<image.height();++y)QVERIFY(std::memcmp(decoded.constScanLine(y),image.constScanLine(y),image.width()*8)==0);QCOMPARE(decoded.colorSpace(),image.colorSpace());
        QVERIFY(!QFileInfo::exists(path+".rgba32f"));
    }
    void hdrScreenshotKeepsFloatSamples(){
        QTemporaryDir directory;const auto path=directory.filePath("hdr.png");QImage image(4,2,QImage::Format_RGBA32FPx4);image.setColorSpace(QColorSpace(QColorSpace::SRgbLinear));
        for(int y=0;y<2;++y){auto *row=reinterpret_cast<float *>(image.scanLine(y));for(int x=0;x<4;++x){row[x*4]=x==0?-.25f:2.5375f;row[x*4+1]=x==2?14.0f:2.5375f;row[x*4+2]=x==3?-.125f:2.5375f;row[x*4+3]=1;}}
        const auto before=image.copy();QVERIFY(writeScreenshotPng(path,image,{{"hdr",true},{"encoding","scRGB-linear"},{"linearUnitNits",80}}).isEmpty());QFile raw(path+".rgba32f");QVERIFY(raw.open(QIODevice::ReadOnly));const auto bytes=raw.readAll();QCOMPARE(bytes.size(),image.width()*image.height()*16);
        for(int y=0;y<2;++y){QVERIFY(std::memcmp(bytes.constData()+y*image.width()*16,image.constScanLine(y),image.width()*16)==0);QVERIFY(std::memcmp(before.constScanLine(y),image.constScanLine(y),image.width()*16)==0);}
        QFile json(path+".json");QVERIFY(json.open(QIODevice::ReadOnly));const auto metadata=QJsonDocument::fromJson(json.readAll()).object();QCOMPARE(metadata.value("pngPrimaries").toInt(),1);QCOMPARE(metadata.value("pngTransfer").toInt(),13);QCOMPARE(metadata.value("floatEncoding").toString(),QString("scRGB-linear"));QCOMPARE(metadata.value("floatRowBytes").toInt(),64);
        const auto preview=QImage(path).convertToFormat(QImage::Format_RGBA64);QCOMPARE(preview.colorSpace(),QColorSpace(QColorSpace::SRgb));QVERIFY(reinterpret_cast<const QRgba64 *>(preview.constScanLine(0))[1].red()>55000);const auto decoded=QImage(directory.filePath("hdr.hdr.png")).convertToFormat(QImage::Format_RGBA64);QVERIFY(!decoded.isNull());const double p=std::pow(203.0/10000.0,2610.0/16384.0),pq=std::pow((3424.0/4096.0+2413.0/128.0*p)/(1+2392.0/128.0*p),2523.0/32.0);const auto white=reinterpret_cast<const QRgba64 *>(decoded.constScanLine(0))[1];QVERIFY(std::abs(int(white.red())-qRound(pq*65535))<=1);QVERIFY(std::abs(int(white.green())-qRound(pq*65535))<=1);QVERIFY(std::abs(int(white.blue())-qRound(pq*65535))<=1);
    }
    void sourceScreenshotPreserves16BitGradient_data(){QTest::addColumn<bool>("hdr");QTest::newRow("SDR")<<false;QTest::newRow("PQ")<<true;}
    void sourceScreenshotPreserves16BitGradient(){
        QFETCH(bool,hdr);PlayerWindow player;player.show();player.source_=QUuid::createUuid().toString(QUuid::WithoutBraces)+".mkv";player.direct_=false;
        auto &frame=player.displayedFrame_;frame.width=320;frame.height=32;frame.format=ThreeFpExternalPixelFormat::GbrP16;frame.colorPrimaries=hdr?9:1;frame.colorTransfer=hdr?16:13;frame.colorMatrix=0;frame.colorRange=2;
        for(int plane=0;plane<3;++plane){frame.strides[plane]=frame.width*2+16;frame.planes[plane].resize(frame.strides[plane]*frame.height);for(int y=0;y<frame.height;++y){auto *row=reinterpret_cast<quint16 *>(frame.planes[plane].data()+y*frame.strides[plane]);for(int x=0;x<frame.width;++x)row[x]=quint16(10000+x);}}
        player.captureImage(true);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto files=directory.entryList({QFileInfo(player.source_).completeBaseName()+"*-VS.png"},QDir::Files);QCOMPARE(files.size(),1);const auto path=directory.filePath(files.first());const auto cleanup=qScopeGuard([&]{QFile::remove(path);QFile::remove(path+".json");QFile::remove(path+".rgba32f");QFile::remove(path.left(path.size()-4)+".hdr.png");QFile::remove(path.left(path.size()-4)+".hdr.png.json");});
        const auto image=QImage(hdr?path.left(path.size()-4)+".hdr.png":path).convertToFormat(QImage::Format_RGBA64);QCOMPARE(image.size(),QSize(320,32));const auto *row=reinterpret_cast<const QRgba64 *>(image.constScanLine(0));for(int x=0;x<320;++x){QVERIFY(std::abs(int(row[x].red())-(10000+x))<=1);QVERIFY(std::abs(int(row[x].green())-(10000+x))<=1);QVERIFY(std::abs(int(row[x].blue())-(10000+x))<=1);}
        QCOMPARE(QFileInfo::exists(path+".rgba32f"),hdr);
    }
    void sourceScreenshotYuvRange_data(){
        QTest::addColumn<int>("matrix");QTest::addColumn<int>("range");QTest::addColumn<int>("transfer");
        QTest::newRow("709-full")<<1<<2<<1;QTest::newRow("709-limited")<<1<<1<<1;QTest::newRow("unspecified")<<2<<1<<2;QTest::newRow("PQ")<<9<<1<<16;QTest::newRow("HLG")<<9<<1<<18;
    }
    void sourceScreenshotYuvRange(){
        QFETCH(int,matrix);QFETCH(int,range);QFETCH(int,transfer);PlayerWindow player;player.source_=QUuid::createUuid().toString(QUuid::WithoutBraces)+".mkv";player.direct_=false;
        auto &frame=player.displayedFrame_;frame.width=320;frame.height=32;frame.format=ThreeFpExternalPixelFormat::Yuv444P16;frame.colorMatrix=matrix;frame.colorRange=range;frame.colorTransfer=transfer;frame.colorPrimaries=matrix==9?9:1;
        for(int plane=0;plane<3;++plane){frame.strides[plane]=frame.width*2+16;frame.planes[plane].resize(frame.strides[plane]*frame.height);for(int y=0;y<frame.height;++y){auto *row=reinterpret_cast<quint16 *>(frame.planes[plane].data()+y*frame.strides[plane]);for(int x=0;x<frame.width;++x)row[x]=plane==0?quint16(10000+x):32768;}}
        player.captureImage(true);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto files=directory.entryList({QFileInfo(player.source_).completeBaseName()+"*-VS.png"},QDir::Files);QVERIFY2(files.size()==1,qPrintable(player.message_->text()));const auto path=directory.filePath(files.first());const auto cleanup=qScopeGuard([&]{QFile::remove(path);QFile::remove(path+".json");QFile::remove(path+".rgba32f");QFile::remove(path.left(path.size()-4)+".hdr.png");QFile::remove(path.left(path.size()-4)+".hdr.png.json");});
        const auto image=QImage(transfer==16 || transfer==18?path.left(path.size()-4)+".hdr.png":path).convertToFormat(QImage::Format_RGBA64);const auto *row=reinterpret_cast<const QRgba64 *>(image.constScanLine(0));for(int x=0;x<320;++x){const auto expected=range==2?10000+x:qRound((10000+x-4096)*65535.0/56064.0);QVERIFY(std::abs(int(row[x].red())-expected)<=2);QVERIFY(std::abs(int(row[x].green())-expected)<=2);QVERIFY(std::abs(int(row[x].blue())-expected)<=2);}
        QCOMPARE(QFileInfo::exists(path+".rgba32f"),transfer==16 || transfer==18);if(transfer==16 || transfer==18){const QImage preview(path);QCOMPARE(preview.colorSpace(),QColorSpace(QColorSpace::SRgb));QCOMPARE(preview.size(),image.size());QFile png(path);QVERIFY(png.open(QIODevice::ReadOnly));QCOMPARE(uchar(png.readAll()[24]),uchar(16));QFile json(path+".json");QVERIFY(json.open(QIODevice::ReadOnly));const auto metadata=QJsonDocument::fromJson(json.readAll()).object();QVERIFY(metadata.value("sourceHdr").toBool());QVERIFY(!metadata.value("hdr").toBool());QCOMPARE(metadata.value("pngTransfer").toInt(),13);}
    }
    void imageScreenshotsKeep16BitPixels(){
        PlayerWindow player;player.show();player.source_=QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";player.imageMode_=true;
        QImage image(640,480,QImage::Format_RGBA64);image.setColorSpace(QColorSpace(QColorSpace::SRgb));for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgba64 *>(image.scanLine(y));for(int x=0;x<image.width();++x)row[x]=QRgba64::fromRgba64(10001,20002,30003,65535);}player.pane_->setImage(image);QTest::qWait(100);
        const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto prefix=QFileInfo(player.source_).completeBaseName();const auto cleanup=qScopeGuard([&]{for(const auto &name:directory.entryList({prefix+"*"},QDir::Files))QFile::remove(directory.filePath(name));});
        for(bool source:{true,false}){player.captureImage(source);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const auto files=directory.entryList({prefix+(source?"*-source.png":"*-display.png")},QDir::Files);QVERIFY2(files.size()==1,qPrintable(player.message_->text()));const auto result=QImage(directory.filePath(files.first())).convertToFormat(QImage::Format_RGBA64);if(source){QCOMPARE(result.size(),image.size());for(int y=0;y<image.height();++y)QVERIFY(std::memcmp(result.constScanLine(y),image.constScanLine(y),image.width()*8)==0);}else{QCOMPARE(result.size(),player.pane_->surface()->size()*player.pane_->surface()->devicePixelRatioF());const auto pixel=reinterpret_cast<const QRgba64 *>(result.constScanLine(result.height()/2))[result.width()/2];QVERIFY(std::abs(int(pixel.red())-10001)<=8);QVERIFY(std::abs(int(pixel.green())-20002)<=8);QVERIFY(std::abs(int(pixel.blue())-30003)<=8);}}
    }
    void nativeSourceScreenshot(){
        QTemporaryDir dir;const auto source=dir.filePath("native-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",source));PlayerWindow player;player.show();QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>1,10000);QVERIFY(player.direct_);player.togglePlayback();
        player.captureImage(true);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto prefix=QFileInfo(source).completeBaseName();const auto cleanup=qScopeGuard([&]{for(const auto &name:directory.entryList({prefix+"*"},QDir::Files))QFile::remove(directory.filePath(name));});const auto files=directory.entryList({prefix+"*-source.png"},QDir::Files);QVERIFY2(files.size()==1,qPrintable(player.message_->text()));QFile file(directory.filePath(files.first()));QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(uchar(file.readAll()[24]),uchar(16));const QImage image(file.fileName());QCOMPARE(image.size(),QSize(int(player.snapshot().videoWidth),int(player.snapshot().videoHeight)));
    }
    void floatingImageScreenshotsKeepExtendedRange(){
        PlayerWindow player;player.show();player.source_=QUuid::createUuid().toString(QUuid::WithoutBraces)+".exr";player.imageMode_=true;QImage image(32,16,QImage::Format_RGBA32FPx4);image.setColorSpace(QColorSpace(QColorSpace::SRgbLinear));
        for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<float *>(image.scanLine(y));for(int x=0;x<image.width();++x){row[x*4]=2.5f;row[x*4+1]=-.125f;row[x*4+2]=6.25f;row[x*4+3]=1;}}player.pane_->setImage(image);QTest::qWait(100);
        const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto prefix=QFileInfo(player.source_).completeBaseName();const auto cleanup=qScopeGuard([&]{for(const auto &name:directory.entryList({prefix+"*"},QDir::Files))QFile::remove(directory.filePath(name));});
        for(bool source:{true,false}){player.captureImage(source);QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const auto files=directory.entryList({prefix+(source?"*-source.png":"*-display.png")},QDir::Files);QVERIFY2(files.size()==1,qPrintable(player.message_->text()));const auto path=directory.filePath(files.first());QFile json(path+".json"),raw(path+".rgba32f");QVERIFY(json.open(QIODevice::ReadOnly));QVERIFY(raw.open(QIODevice::ReadOnly));const auto metadata=QJsonDocument::fromJson(json.readAll()).object();QCOMPARE(metadata.value("floatEncoding").toString(),QString("linear-RGB"));const auto bytes=raw.readAll();const int w=metadata.value("width").toInt(),h=metadata.value("height").toInt();QCOMPARE(bytes.size(),w*h*16);const auto *center=reinterpret_cast<const float *>(bytes.constData())+(h/2*w+w/2)*4;QVERIFY(std::abs(center[0]-2.5f)<.005);QVERIFY(std::abs(center[1]+.125f)<.005);QVERIFY(std::abs(center[2]-6.25f)<.005);if(source)for(int y=0;y<h;++y)QVERIFY(std::memcmp(bytes.constData()+y*w*16,image.constScanLine(y),w*16)==0);}
    }
    void hdrRenderedScreenshotReadback(){
        QWidget surface;surface.setAttribute(Qt::WA_NativeWindow);surface.resize(320,240);surface.show();ThreeFpApi api;ThreeFpPlayer renderer(api,&surface);auto settings=VsrColorDefaultSettings();settings.engine=1;settings.output=2;settings.icc=0;settings.peakDetect=0;settings.displayPeak=1000;QVERIFY(renderer.setColorSettings(settings));
        VapourSynthFrame frame;frame.width=64;frame.height=48;frame.format=ThreeFpExternalPixelFormat::Yuv444P16;frame.colorMatrix=9;frame.colorTransfer=16;frame.colorPrimaries=9;frame.colorRange=2;frame.totalFrames=10;frame.duration100ns=400000;
        for(int plane=0;plane<3;++plane){frame.strides[plane]=frame.width*2;frame.planes[plane].resize(frame.strides[plane]*frame.height);auto *pixels=reinterpret_cast<quint16 *>(frame.planes[plane].data());std::fill(pixels,pixels+frame.width*frame.height,plane==0?quint16(49270):quint16(32768));}
        QVERIFY(renderer.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(renderer.snapshot().presentedVideoFrames>0,10000);QCOMPARE(renderer.colorStatus().outputHdr,1u);QCOMPARE(renderer.colorStatus().outputBits,16u);const auto captured=renderer.captureFloat();QVERIFY(!captured.isNull());QCOMPARE(captured.format(),QImage::Format_RGBA32FPx4);QCOMPARE(captured.text("readbackBitDepth"),QString("16"));
        ThreeFpPixelProbe pixel{};QVERIFY(renderer.samplePixel(160,120,pixel));const auto *row=reinterpret_cast<const float *>(captured.constScanLine(120));QVERIFY(std::isfinite(row[640]));QVERIFY(row[640]>1 && row[640]<20);QVERIFY(std::abs(row[640]-row[641])<.1);QVERIFY(std::abs(row[640]-row[642])<.1);QCOMPARE(row[643],1.0f);QCOMPARE(row[640],pixel.red);QCOMPARE(row[641],pixel.green);QCOMPARE(row[642],pixel.blue);
        QTemporaryDir dir;const auto path=dir.filePath("rendered-hdr.png");QVERIFY(writeScreenshotPng(path,captured,{{"hdr",true},{"encoding","scRGB-linear"},{"linearUnitNits",80}}).isEmpty());QFile raw(path+".rgba32f");QVERIFY(raw.open(QIODevice::ReadOnly));const auto data=raw.readAll();for(int y=0;y<captured.height();++y)QVERIFY(std::memcmp(data.constData()+y*captured.width()*16,captured.constScanLine(y),captured.width()*16)==0);
    }
    void separateThemeTransparencyControls(){
        PlayerWindow player;player.show();player.activateWindow();const auto bottom=player.settings_->value("theme/bottomTransparency",50),playlist=player.settings_->value("theme/playlistTransparency",50);
        bool changed=false;QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);const auto close=qScopeGuard([dialog]{dialog->reject();});
            auto *a=dialog->findChild<QSpinBox *>("playerBottomTransparency"),*b=dialog->findChild<QSpinBox *>("playerPlaylistTransparency");QVERIFY(a && b);QCOMPARE(a->minimum(),0);QCOMPARE(a->maximum(),100);QCOMPARE(b->minimum(),0);QCOMPARE(b->maximum(),100);a->setValue(25);b->setValue(65);dialog->findChild<QListWidget *>()->setCurrentRow(1);dialog->grab().save("build/chrome-audio-1.0.7/theme-settings.png");dialog->findChild<QPushButton *>("playerApplySettings")->click();changed=true;
        });player.showSettings();QVERIFY(changed);QCOMPARE(player.settings_->value("theme/bottomTransparency").toInt(),25);QCOMPARE(player.settings_->value("theme/playlistTransparency").toInt(),65);
        player.settings_->setValue("theme/bottomTransparency",bottom);player.settings_->setValue("theme/playlistTransparency",playlist);player.applyAppearance();player.dockPlaylist();
    }
    void transparentChromeAndFullscreenReveal(){
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("theme/bottomTransparency",70);settings.setValue("theme/playlistTransparency",35);settings.setValue("render/stableViewport",true);settings.sync();
        QTemporaryDir dir;QImage image(1600,900,QImage::Format_RGB32);image.fill(QColor(60,180,140));const auto path=dir.filePath("background.png");QVERIFY(image.save(path));
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QVERIFY(player.openFile(path));QTRY_VERIFY(player.ready_);player.updateChrome();
        const auto viewport=player.pane_->surface()->size();QVERIFY(player.controls_->isWindow());QVERIFY(player.controls_->testAttribute(Qt::WA_TranslucentBackground));
        const auto alpha=qAlpha(player.controls_->grab().toImage().pixel(2,2));QVERIFY(alpha>=75 && alpha<=78);
        QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(player.centralWidget()->width()-8,100)));QTRY_VERIFY(player.playlistPanel_->isVisible());const auto playlistAlpha=qAlpha(player.playlistPanel_->grab().toImage().pixel(2,2));QVERIFY(playlistAlpha>=164 && playlistAlpha<=167);QCOMPARE(player.pane_->surface()->size(),viewport);
        QTest::qWait(200);player.screen()->grabWindow(0).save("build/chrome-audio-1.0.7/transparent-player.png");player.setPlaylistPinned(true);QTRY_VERIFY(player.playlistPanel_->isVisible());QVERIFY(!player.playlistPanel_->isWindow());QCOMPARE(player.playlistPanel_->parentWidget(),player.videoSplit_);QVERIFY(player.pane_->surface()->width()<player.centralWidget()->width());
        player.videoSplit_->setSizes({700,400});QTest::qWait(100);QVERIFY(player.playlistPanel_->width()>350);player.setPlaylistPinned(false);
        player.toggleFullscreen();QTRY_VERIFY(player.isFullScreen());QTest::qWait(150);const auto full=player.pane_->surface()->size();
        QCursor::setPos(player.centralWidget()->mapToGlobal(player.centralWidget()->rect().center()));QTRY_VERIFY_WITH_TIMEOUT(!player.controls_->isVisible(),1600);
        QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(120,player.controlsTop_+2)));QTRY_VERIFY(player.controls_->isVisible());QVERIFY(player.controlsTop_ < player.centralWidget()->height()-24);QCOMPARE(player.pane_->surface()->size(),full);
        QTRY_VERIFY_WITH_TIMEOUT(!player.controls_->isVisible(),1600);
        QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(120,2)));QTRY_VERIFY(player.fullscreenTitle_->isVisible());QCOMPARE(player.fullscreenTitleText_->text(),player.windowTitle());QCOMPARE(player.pane_->surface()->size(),full);
        player.setPlaylistPinned(true);player.updateChrome();QVERIFY(player.playlistPanel_->geometry().top()>=player.fullscreenTitle_->geometry().bottom());player.setPlaylistPinned(false);
        player.findChild<QPushButton *>("playerExitFullscreen")->click();QTRY_VERIFY(!player.isFullScreen());QVERIFY(!player.fullscreenTitle_->isVisible());
        const auto video=dir.filePath("green.mkv");QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","color=c=0x3cb48c:s=320x180:r=24:d=3","-c:v","libx264","-y",video});QVERIFY(encoder.waitForFinished(15000));QCOMPARE(encoder.exitCode(),0);player.loadPreset({});QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);if(player.playing_)player.togglePlayback();
        player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::qWait(200);QCursor::setPos(player.centralWidget()->mapToGlobal(QPoint(player.centralWidget()->width()-4,100)));QTRY_VERIFY(player.playlistPanel_->isVisible());QTest::qWait(200);
        QPixmap capture;QColor background;
        for(int attempt=0;attempt<10;++attempt){player.raise();player.activateWindow();QTest::qWait(100);player.updateChrome();QTest::qWait(50);const auto top=player.centralWidget()->mapToGlobal(QPoint());capture=player.screen()->grabWindow(0,top.x(),top.y(),player.centralWidget()->width(),player.centralWidget()->height());background=capture.toImage().pixelColor(player.centralWidget()->rect().center()*capture.devicePixelRatio());if(background.green()>150)break;}
        if(background.green()<=150)QSKIP("Desktop foreground is occupied; widget alpha and fullscreen assertions passed, live composition requires foreground.");
        const auto pixels=capture.toImage();const auto rgb=[&](QPoint point){return pixels.pixelColor(point*capture.devicePixelRatio());};capture.save("build/chrome-audio-1.0.7/transparent-native-video.png");
        const auto blend=[&](QPoint point,int alpha){const auto color=rgb(point);for(int c=0;c<3;++c){const int source=c==0?background.red():c==1?background.green():background.blue();const int observed=c==0?color.red():c==1?color.green():color.blue();QVERIFY(std::abs(observed-(source*(255-alpha)+(c==0?32:c==1?33:36)*alpha)/255)<10);}};
        blend(QPoint(3,player.controlsTop_+3),76);blend(QPoint(player.centralWidget()->width()-player.playlistPanel_->width()+3,player.centralWidget()->height()/2),165);
    }
    void audioInputOutputMetersAndTab(){
        QTemporaryDir dir;const auto file=dir.filePath("meter.wav");QProcess encoder;QStringList waves;for(int c=0;c<8;++c)waves<<QString("%1*sin(2*PI*200*t)").arg((c+1)*.08,0,'f',2);
        encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","aevalsrc="+waves.join('|')+":s=48000:d=12:c=7.1","-c:a","pcm_f32le","-y",file});QVERIFY(encoder.waitForFinished(20000));QCOMPARE(encoder.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/volume",100);settings.setValue("player/muted",false);settings.setValue("player/speed",1.);settings.setValue("player/preset",QString());settings.setValue("decode/audio","3FP");settings.setValue("audio/eq/enabled",false);settings.setValue("audio/wave",100);settings.setValue("info/audioDetailed",true);settings.sync();
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.audioMode_ && player.snapshot().decodedAudioFrames>0 && player.playing_,10000);
        ThreeFpAudioLevels input,output;const auto levels=[&]{return player.clock_->audioLevels(input,true) && player.clock_->audioLevels(output,false) && input.values[0]>.07f && output.values[0]>.01f;};QTRY_VERIFY(levels());QCOMPARE(input.channels,8u);QVERIFY(output.channels==2 || output.channels==8);
        for(int c=0;c<8;++c)QVERIFY(std::abs(input.values[c]-(c+1)*.08f)<.002f);
        const auto baseline=output.values[0];player.volume_->setValue(25);QTRY_VERIFY_WITH_TIMEOUT(levels() && output.values[0]<baseline*.3f,3000);for(int c=0;c<8;++c)QVERIFY(std::abs(input.values[c]-(c+1)*.08f)<.002f);
        QTest::keyClick(&player,Qt::Key_Tab);QTRY_VERIFY(player.info_->isVisible());QTRY_VERIFY(player.audioInfo_->isVisible());QVERIFY(player.audioInfo_->isWindow());QVERIFY(!(player.audioInfo_->windowFlags()&Qt::WindowStaysOnTopHint));QVERIFY(player.info_->text().startsWith("源文件："));
        auto *area=player.info_->findChild<QScrollArea *>("playerInfoTextArea");auto *slider=player.info_->findChild<QSlider *>("playerInfoOpacity");QVERIFY(area && slider);QVERIFY(area->geometry().bottom()<slider->geometry().top());
        const auto original=player.info_->size();for(int n=0;n<50;++n)player.updateInfo();QCOMPARE(player.info_->size(),original);QVERIFY(player.info_->geometry().right()<player.audioInfo_->geometry().left());player.audioInfo_->grab().save("build/chrome-audio-1.0.7/audio-detail.png");player.info_->grab().save("build/chrome-audio-1.0.7/tab-files.png");
        auto *mode=player.info_->findChild<QComboBox *>("playerInfoAudioMode");mode->setCurrentIndex(0);QTRY_VERIFY(!player.audioInfo_->isVisible());mode->setCurrentIndex(1);QTRY_VERIFY(player.audioInfo_->isVisible());
        QDialog dialog(&player);dialog.setWindowModality(Qt::WindowModal);dialog.show();dialog.activateWindow();QTRY_VERIFY(!player.info_->isVisible() && !player.audioInfo_->isVisible());dialog.close();player.activateWindow();QTRY_VERIFY(player.info_->isVisible() && player.audioInfo_->isVisible());
        QTest::keyClick(&player,Qt::Key_Tab);QVERIFY(!player.info_->isVisible());QVERIFY(!player.audioInfo_->isVisible());QTest::qWait(200);QVERIFY(!player.audioInfo_->isVisible());
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTRY_VERIFY(player.clock_->audioLevels(input,true) && input.values[0]==0);
    }
    void softwareCodecPipeline_data(){QTest::addColumn<QString>("encoder");QTest::newRow("HEVC")<<QString("libx265");QTest::newRow("AV1")<<QString("libaom-av1");}
    void softwareCodecPipeline(){
        QFETCH(QString,encoder);QTemporaryDir dir;const auto source=dir.filePath("software.mkv");QProcess encode;
        QStringList args{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=4","-f","lavfi","-i","sine=sample_rate=48000:duration=3.5","-c:v",encoder,"-pix_fmt","yuv444p10le","-threads","2"};
        if(encoder=="libx265")args<<"-preset"<<"ultrafast"<<"-x265-params"<<"pools=2:frame-threads=2:log-level=error";else args<<"-cpu-used"<<"8";
        args<<"-c:a"<<"flac"<<"-y"<<source;encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),args);QVERIFY(encode.waitForFinished(30000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        QWidget surface;surface.resize(640,360);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer player(api,&surface);QVERIFY(player.setDecodeMode(1));player.setMuted(true);
        QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,15000);const auto info=QJsonDocument::fromJson(player.mediaInfo().toUtf8()).object();QCOMPARE(info.value("videoDecoderThreads").toInt(),int(std::min(encoder=="libx265"?16u:32u,std::max(1u,std::thread::hardware_concurrency()))));QCOMPARE(info.value("softwareVideoDecodeAsync").toBool(),encoder=="libaom-av1");
        QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().position100ns>5000000,15000);QVERIFY(player.pause());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(150);const auto paused=player.snapshot();QTest::qWait(150);QCOMPARE(player.snapshot().presentedVideoFrames,paused.presentedVideoFrames);
        for(const qint64 target:{20000000LL,3000000LL,38000000LL}){const auto before=player.snapshot();QVERIFY(player.seek(target));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>before.timelineGeneration && player.snapshot().presentedVideoFrames>before.presentedVideoFrames && qAbs(player.snapshot().position100ns-target)<500000,5000);QVERIFY(!player.capture().isNull());}
        QVERIFY(player.seekFrame(72));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().frameIndex,72,5000);QVERIFY(player.play());QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ended,7000);QCOMPARE(player.snapshot().frameIndex,95);
        player.stop();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Idle);QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,10000);QVERIFY(player.play());QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ended,7000);const auto ended=player.snapshot();QCOMPARE(ended.frameIndex,95);QVERIFY(ended.decodedVideoFrames>=96);QCOMPARE(ended.droppedVideoFrames,quint64(0));QCOMPARE(ended.coalescedVideoFrames,quint64(0));QCOMPARE(ended.queuedVideoFrames,0u);QVERIFY(ended.audioPosition100ns>39000000);QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().videoOutputBitDepth,10u,1000);
        QVERIFY(player.play());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);QVERIFY(player.seek(38000000));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ended,7000);QCOMPARE(player.snapshot().frameIndex,95);QVERIFY(player.lastError().isEmpty());
    }
    void hardware8kQueueLookAhead_data(){QTest::addColumn<int>("algorithm");QTest::newRow("D3D11")<<519;QTest::newRow("Jinc")<<1284;}
    void hardware8kQueueLookAhead(){
        QFETCH(int,algorithm);const auto source=qEnvironmentVariable("VSR_8K_QUEUE_SOURCE");if(source.isEmpty())QSKIP("Set VSR_8K_QUEUE_SOURCE to an 8K hardware-decodable video");
        QWidget surface;const double dpi=surface.devicePixelRatioF();surface.resize(qRound(1182/dpi),qRound(665/dpi));surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer player(api,&surface);
        QVERIFY(player.setDecodeMode(2));QVERIFY(player.setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(algorithm),static_cast<ThreeFpScalingAlgorithm>(algorithm & 255)));QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,15000);QCOMPARE(player.snapshot().decodeMode,2u);QCOMPARE(player.snapshot().videoWidth,7680u);QCOMPARE(player.snapshot().videoHeight,4320u);
        QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>100,10000);const auto before=player.snapshot();unsigned peak=0;for(int i=0;i<40;++i){QTest::qWait(250);const auto s=player.snapshot();QCOMPARE(s.state,ThreeFpState::Playing);QCOMPARE(s.decodeMode,2u);peak=qMax(peak,s.queuedVideoFrames);}
        const auto after=player.snapshot();qInfo()<<"8K queue peak"<<peak<<"accepted"<<after.presentedVideoFrames-before.presentedVideoFrames<<"dropped"<<after.droppedVideoFrames-before.droppedVideoFrames;QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().videoOutputBitDepth,10u,1000);QCOMPARE(after.videoScalingMode,algorithm==519?1u:0u);QVERIFY(peak>=4);QVERIFY(peak<=8);QVERIFY(after.position100ns-before.position100ns>90000000);QCOMPARE(after.droppedVideoFrames,before.droppedVideoFrames);QCOMPARE(after.coalescedVideoFrames,before.coalescedVideoFrames);QCOMPARE(after.audioUnderruns,before.audioUnderruns);QVERIFY(after.swapChainPresents-before.swapChainPresents>=450);
        QVERIFY(player.pause());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(100);const auto paused=player.snapshot();QTest::qWait(200);QCOMPARE(player.snapshot().presentedVideoFrames,paused.presentedVideoFrames);QVERIFY(player.seek(80000000));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>paused.timelineGeneration && qAbs(player.snapshot().position100ns-80000000)<1000000,5000);QCOMPARE(player.snapshot().queuedVideoFrames,0u);QVERIFY(!player.capture().isNull());QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().position100ns>90000000,5000);
    }
    void bdMenuSubtitleSelection_data(){QTest::addColumn<int>("track");for(int i=0;i<5;++i)QTest::newRow(qPrintable(QString::number(i)))<<i;}
    void bdMenuSubtitleSelection(){
        QFETCH(int,track);const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.setValue("playback/remember",false);settings.setValue("subtitle/visible",true);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openBluRay(root,true));QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->presentedFrames()>0,15000);player.discMenuNavigation_=false;QTest::qWait(1500);
        HWND hit=nullptr;EnumChildWindows(reinterpret_cast<HWND>(player.discSurface_->winId()),[](HWND child,LPARAM result)->BOOL{if(GetPropW(child,L"VSR.BD.InputOwner"))*reinterpret_cast<HWND *>(result)=child;return TRUE;},reinterpret_cast<LPARAM>(&hit));QVERIFY(hit);RECT r{};GetClientRect(hit,&r);const auto video=QSize(1920,1080).scaled(QSize(r.right,r.bottom),Qt::KeepAspectRatio);
        const auto click=[&](int x,int y){const auto at=MAKELPARAM((r.right-video.width())/2+qRound(double(x)*video.width()/1920),(r.bottom-video.height())/2+qRound(double(y)*video.height()/1080));PostMessageW(hit,WM_MOUSEMOVE,0,at);PostMessageW(hit,WM_LBUTTONDOWN,MK_LBUTTON,at);PostMessageW(hit,WM_LBUTTONUP,0,at);QTest::qWait(500);};click(1208,765);click(1110,810+track*38);player.discMenuNavigation_=true;click(1160,127);
        QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->active() && player.positionRestored_,15000);int actualPid=-1;for(const auto &entry:player.media_.value("streams").toArray())if(entry.toObject().value("index").toInt()==player.primarySubtitle_)actualPid=entry.toObject().value("streamId").toInt();QCOMPARE(actualPid,0x1200+track);qInfo()<<"BD menu subtitle"<<track<<"PID"<<actualPid<<"selected index"<<player.primarySubtitle_;
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);if(player.playing_)player.togglePlayback();player.seekTime(30000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_,5000);
        QTemporaryDir referenceDir;const auto sup=referenceDir.filePath("reference.sup");QProcess extract;extract.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-i",QDir(QFileInfo(player.mediaInput_).absolutePath()).filePath("STREAM/00000.m2ts"),"-map",QString("0:%1").arg(2+track),"-c:s","copy","-t","8","-y",sup});QVERIFY(extract.waitForFinished(15000));QCOMPARE(extract.exitCode(),0);PlayerSubtitles reference;QSignalSpy expectedFrames(&reference,&PlayerSubtitles::imageReady),actualFrames(player.subtitles_.get(),&PlayerSubtitles::imageReady),failures(&reference,&PlayerSubtitles::errorOccurred);reference.load(0,sup,-1,"hdmv_pgs_subtitle",{});reference.render(30000000,QSize(640,360),QSize(1920,1080),true);QTRY_VERIFY_WITH_TIMEOUT(!expectedFrames.isEmpty(),10000);QVERIFY(failures.isEmpty());const auto expected=qvariant_cast<QImage>(expectedFrames.last().first());player.subtitles_->render(30000000,QSize(640,360),QSize(1920,1080),true);QTRY_VERIFY_WITH_TIMEOUT(std::any_of(actualFrames.cbegin(),actualFrames.cend(),[&](const auto &frame){return qvariant_cast<QImage>(frame.first())==expected;}),10000);expected.save(QString("build/bd-menu-subtitle-%1.png").arg(track));

    }
    void bdSubtitleLanguages() {
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");BlurayTitle title;for(const auto &entry:BlurayCatalog::scan(root).titles)if(entry.warning.isEmpty() && entry.ticks>title.ticks)title=entry;QString error;const auto path=BlurayCatalog::prepare(title,&error);QVERIFY(!path.isEmpty());
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("decode/mode",1);settings.setValue("player/preset",QString());settings.setValue("subtitle/visible",true);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.snapshot().presentedVideoFrames>0,15000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);player.seekTime(30000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_,5000);
        QSignalSpy frames(player.subtitles_.get(),&PlayerSubtitles::imageReady),failures(player.subtitles_.get(),&PlayerSubtitles::errorOccurred);QSet<QByteArray> hashes;int count=0;
        for(const auto &entry:player.media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()!="subtitle")continue;const int index=stream.value("index").toInt();frames.clear();player.selectSubtitle(0,index);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),10000);QVERIFY2(failures.isEmpty(),qPrintable(player.message_->text()));const auto image=qvariant_cast<QImage>(frames.last().first());QVERIFY(!image.isNull());bool opaque=false;for(int y=0;y<image.height() && !opaque;++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))){opaque=true;break;}QVERIFY(opaque);image.save(QString("build/bd-subtitle-track-%1.png").arg(index));const auto hash=QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(image.constBits()),image.sizeInBytes()),QCryptographicHash::Sha256);qInfo()<<"BD subtitle"<<stream<<hash.toHex();hashes.insert(hash);++count;}
        QCOMPARE(count,5);QCOMPARE(hashes.size(),5);
    }
    void bluRayModeDialogWidth() {
        PlayerWindow player;player.show();bool checked=false;QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerBluRayMode");QVERIFY(dialog);QVERIFY(dialog->isVisible());QVERIFY(dialog->width()>=560);dialog->grab().save("build/bd-mode-width.png");checked=true;dialog->reject();});player.chooseBluRayMode("unused");QVERIFY(checked);
    }
    void mouseSeekRendering_data(){QTest::addColumn<uint>("mode");QTest::addColumn<bool>("disc");QTest::addColumn<bool>("resume");for(bool disc:{false,true})for(uint mode:{1u,2u})for(bool resume:{false,true})QTest::newRow(qPrintable(QString("%1-%2-%3").arg(disc?"BD":"video",mode==1?"SW":"HW",resume?"playing":"paused")))<<mode<<disc<<resume;}
    void mouseSeekRendering(){
        QFETCH(uint,mode);QFETCH(bool,disc);QFETCH(bool,resume);QTemporaryDir dir;QString path;
        if(disc){const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");BlurayTitle title;for(const auto &entry:BlurayCatalog::scan(root).titles)if(entry.warning.isEmpty() && entry.ticks>title.ticks)title=entry;QString error;path=BlurayCatalog::prepare(title,&error);QVERIFY(!path.isEmpty());}
        else{path=dir.filePath("seek.mkv");QProcess encode;encode.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=1280x720:rate=24:duration=8","-c:v","libx264","-preset","ultrafast","-g","192","-threads","2","-y",path});QVERIFY(encode.waitForFinished(15000));QCOMPARE(encode.exitCode(),0);}
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("decode/mode",mode);settings.setValue("player/preset",QString());settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);auto *slider=player.timeline_;
        if(resume){player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);}
        const qint64 tolerance=resume?1500000:500000;
        for(const double fraction:{.75,.15,.9,.3}){
            const auto before=player.snapshot();QElapsedTimer elapsed;elapsed.start();QTest::mouseClick(slider,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(slider->width()*fraction),slider->height()/2));const int value=slider->value();const qint64 target=before.duration100ns*value/100000;bool rebound=false;QTimer watch;connect(&watch,&QTimer::timeout,&player,[&]{if(player.seekUiTarget_>=0 && qAbs(slider->value()-value)>10)rebound=true;});watch.start(1);
            QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().presentedVideoFrames>before.presentedVideoFrames && qAbs(player.position()-target)<tolerance,3000);watch.stop();QVERIFY(!rebound);qInfo()<<"Mouse click"<<disc<<mode<<resume<<target/1e7<<"ms"<<elapsed.elapsed()<<"PTS"<<player.snapshot().framePts;QVERIFY(elapsed.elapsed()<1000);QTRY_COMPARE(player.snapshot().state,resume?ThreeFpState::Playing:ThreeFpState::Paused);
        }
        const auto dragPresented=player.outputSnapshot().presentedVideoFrames;
        QTest::mousePress(slider,Qt::LeftButton,Qt::NoModifier,QPoint(slider->width()/10,slider->height()/2));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>dragPresented && qAbs(player.position()-player.snapshot().duration100ns*slider->value()/100000)<500000,1000);QVERIFY(slider->isSliderDown());
        for(const double fraction:{.2,.8,.25,.85,.3,.9}){const QPoint point(qRound(slider->width()*fraction),slider->height()/2);QMouseEvent move(QEvent::MouseMove,point,slider->mapToGlobal(point),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(slider,&move);QTest::qWait(25);QVERIFY(qAbs(slider->value()-int(fraction*100000))<1000);}
        const auto target=player.snapshot().duration100ns*slider->value()/100000;QElapsedTimer release;release.start();QTest::mouseRelease(slider,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(slider->width()*.9),slider->height()/2));QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.queuedSeek_<0 && qAbs(player.position()-target)<tolerance,3000);qInfo()<<"Mouse drag release"<<disc<<mode<<resume<<release.elapsed();QVERIFY(release.elapsed()<1000);QTRY_COMPARE(player.snapshot().state,resume?ThreeFpState::Playing:ThreeFpState::Paused);
    }
    void bdMenuEpisodeSelection_data(){QTest::addColumn<int>("episode");for(int i=0;i<6;++i)QTest::newRow(qPrintable(QString::number(i+8)))<<i;}
    void bdMenuEpisodeSelection(){
        QFETCH(int,episode);
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");PlayerWindow player;player.show();QVERIFY(player.openBluRay(root,true));QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->presentedFrames()>0,15000);QTest::qWait(1500);
        HWND hit=nullptr;EnumChildWindows(reinterpret_cast<HWND>(player.discSurface_->winId()),[](HWND child,LPARAM result)->BOOL{if(GetPropW(child,L"VSR.BD.InputOwner"))*reinterpret_cast<HWND *>(result)=child;return TRUE;},reinterpret_cast<LPARAM>(&hit));QVERIFY(hit);RECT r{};GetClientRect(hit,&r);const auto video=QSize(1920,1080).scaled(QSize(r.right,r.bottom),Qt::KeepAspectRatio);const int x=(r.right-video.width())/2+qRound(1150.*video.width()/1920),y=(r.bottom-video.height())/2+qRound((198+episode*62.)*video.height()/1080);const auto at=MAKELPARAM(x,y);PostMessageW(hit,WM_MOUSEMOVE,0,at);PostMessageW(hit,WM_LBUTTONDOWN,MK_LBUTTON,at);PostMessageW(hit,WM_LBUTTONUP,0,at);
        QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->active(),15000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);const qint64 expected=qRound64(episode*1421.42*10000000);QTRY_VERIFY_WITH_TIMEOUT(player.position()>=expected && player.position()<expected+40000000,15000);qInfo()<<"BD menu selected"<<episode+8<<"actual seconds"<<player.position()/1e7<<"source"<<player.mediaInput_;QVERIFY(player.mediaInput_.endsWith(".ffconcat"));player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);const auto before=player.snapshot().presentedVideoFrames;player.seekTime(expected+60000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().presentedVideoFrames>before && qAbs(player.position()-expected-60000000)<500000,3000);
    }
    void recursiveImageFolderAndWrap() {
        QTemporaryDir dir;QVERIFY(QDir().mkpath(dir.filePath("child/deeper")));QImage image(32,24,QImage::Format_RGB32);image.fill(Qt::red);
        QVERIFY(image.save(dir.filePath("a.png")));QVERIFY(image.save(dir.filePath("child/b.png")));QVERIFY(image.save(dir.filePath("child/deeper/c.png")));
        PlayerWindow player;player.show();QVERIFY(player.openFolder(dir.path()));QTRY_VERIFY(player.ready_);QCOMPARE(player.files_.size(),3);QCOMPARE(player.playlistDirectory_,dir.path());
        auto *folder=player.playlist_->topLevelItem(0);QVERIFY(folder->data(0,Qt::UserRole+1).toBool());folder->setExpanded(true);QTRY_COMPARE(folder->childCount(),2);folder->child(0)->setExpanded(true);QTRY_COMPARE(folder->child(0)->childCount(),1);
        player.nextFile(1);QTRY_VERIFY(player.ready_);QCOMPARE(player.source_,dir.filePath("child/b.png"));QCOMPARE(player.playlistDirectory_,dir.path());player.nextFile(1);QTRY_VERIFY(player.ready_);player.nextFile(1);QTRY_VERIFY(player.ready_);QCOMPARE(player.source_,dir.filePath("a.png"));QVERIFY(player.findChild<QLabel *>("playerImageWrapNotice")->isVisible());QCOMPARE(player.files_.size(),3);
    }
    void bottomControlsDoNotOpenPlaylist() {
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());player.setPlaylistPinned(false);
        const auto bottom=player.controls_->mapToGlobal(QPoint(player.controls_->width()-8,player.controls_->height()/2));QCursor::setPos(bottom);QTest::qWait(150);player.updateChrome();QVERIFY(!player.playlistPanel_->isVisible());
        player.setPlaylistPinned(true);QVERIFY(player.playlistPanel_->parentWidget()==player.videoSplit_);player.setPlaylistPinned(false);player.updateChrome();QVERIFY(!player.playlistPanel_->isVisible());
    }
    void externalPlainLyricsTakePriority() {
        const auto root=qEnvironmentVariable("VSR_LYRIC_KIT");if(root.isEmpty())QSKIP("Set VSR_LYRIC_KIT");QTemporaryDir dir;const auto audio=dir.filePath("song.opus");QVERIFY(QFile::copy(QDir(root).filePath("A_opus_LYRICS_only.opus"),audio));
        QFile sidecar(dir.filePath("song.lrc"));QVERIFY(sidecar.open(QIODevice::WriteOnly));sidecar.write("\xef\xbb\xbf" "External plain lyrics\nSecond line");sidecar.close();const auto result=PlayerAudioMetadata::read(audio);QCOMPARE(result.lyricSource,QString("song.lrc"));QCOMPARE(result.lyrics.size(),1);QCOMPARE(result.lyrics.first().text,QString("External plain lyrics\nSecond line"));
    }
    void fullBluRaySeekPerformance() {
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");const auto scan=BlurayCatalog::scan(source);BlurayTitle longest;for(const auto &title:scan.titles)if(title.warning.isEmpty() && title.ticks>longest.ticks)longest=title;QVERIFY(longest.ticks>7200*45000LL);QString error;const auto playlist=BlurayCatalog::prepare(longest,&error);QVERIFY2(!playlist.isEmpty(),qPrintable(error));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/preset",QString());settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(playlist));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);QVERIFY(player.mediaInput_.endsWith(".ffconcat"));
        for(const double fraction:{.8,.2,.65,.05,.95}){const qint64 target=qRound64(longest.ticks/45000.*10000000*fraction);const auto presents=player.snapshot().presentedVideoFrames;QElapsedTimer seek;seek.start();player.seekTime(target);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>presents && qAbs(player.position()-target)<10000000,5000);qInfo()<<"BD seek seconds"<<target/10000000.<<"ms"<<seek.elapsed();QVERIFY2(seek.elapsed()<2500,"BD seek exceeded 2.5 seconds");}
    }
    void discMenuProgrammeRestoresProcessing() {
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/preset",QString());settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openBluRay(source,true));QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->presentedFrames()>0,15000);QTest::qWait(1500);qInfo()<<"menu programme"<<player.discMenu_->programme();player.discMenu_->navigate(0);
        QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->active(),15000);QVERIFY(player.source_.endsWith(".mpls"));QVERIFY(player.mediaInput_.endsWith(".ffconcat"));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);player.infoVisible_=true;player.updateInfo();QVERIFY(player.info_->text().contains("视频"));QVERIFY(!player.info_->text().contains("菜单合成"));player.showContextMenu(player.pane_->surface()->mapToGlobal(QPoint(40,40)));auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);bool processing=false;for(const auto *action:menu->actions())processing|=action->text()==QString("图像处理");QVERIFY(processing);menu->close();
    }
    void bluRayPresetProducesFilteredFrames() {
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");const auto scan=BlurayCatalog::scan(source);BlurayTitle shortTitle;for(const auto &title:scan.titles)if(title.warning.isEmpty() && title.ticks>0 && (shortTitle.ticks==0 || title.ticks<shortTitle.ticks))shortTitle=title;QString error;const auto playlist=BlurayCatalog::prepare(shortTitle,&error);QVERIFY2(!playlist.isEmpty(),qPrintable(error));QTemporaryDir dir;const auto preset=dir.filePath("bd-filter.vpy");QFile file(preset);QVERIFY(file.open(QIODevice::WriteOnly));file.write("import vapoursynth as vs\nclip = vs.core.ffms2.Source(source=_vsr_source)\nclip = vs.core.resize.Bilinear(clip, width=160, height=90)\nclip.set_output()\n");file.close();
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/preset",preset);settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(playlist));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0 && !player.direct_,15000);QCOMPARE(player.clip_.width,160);QCOMPARE(player.clip_.height,90);QCOMPARE(player.preset_,preset);player.infoVisible_=true;player.updateInfo();QVERIFY(player.info_->text().contains("160"));
        QTRY_VERIFY_WITH_TIMEOUT(player.playing_,5000);const auto presents=player.outputSnapshot().presentedVideoFrames;const qint64 target=player.snapshot().duration100ns/2;player.seekTime(target);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.seekUiTarget_<0 && player.outputSnapshot().presentedVideoFrames>presents && qAbs(player.position()-target)<2000000,10000);QTRY_VERIFY_WITH_TIMEOUT(player.playing_,5000);QCOMPARE(player.outputSnapshot().videoWidth,160);
    }
    void bdMenuFromEmptyWindow(){
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");PlayerWindow player;player.show();QVERIFY(!player.pane_->surface()->isVisible());QVERIFY(player.openBluRay(source,true));QTRY_VERIFY2_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->active(),qPrintable(player.message_->text()),15000);QVERIFY(player.pane_->surface()->isVisible());QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->presentedFrames()>0,15000);
        QVERIFY(!player.findChild<QWidget *>("playerDiscNavigation"));QPointer<QWidget> oldSurface=player.discSurface_;player.findChild<QPushButton *>("playerOpenDiscMenu")->click();QVERIFY(player.discSurface_!=oldSurface);QVERIFY(player.discSurface_->isVisible());QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->presentedFrames()>0,15000);QTRY_VERIFY_WITH_TIMEOUT(oldSurface.isNull(),5000);
        player.raise();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QCursor::setPos(player.discSurface_->mapToGlobal(QPoint(100,100)));QTest::qWait(1800);CURSORINFO cursor{};cursor.cbSize=sizeof(cursor);QVERIFY(GetCursorInfo(&cursor));QCOMPARE(cursor.hCursor,LoadCursorW(nullptr,IDC_ARROW));QVERIFY(cursor.flags & CURSOR_SHOWING);auto hit=WindowFromPoint(cursor.ptScreenPos);const bool interactive=GetPropW(hit,L"VSR.BD.InputOwner")==reinterpret_cast<HANDLE>(player.discSurface_);if(interactive){mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);}else{qInfo()<<"Desktop unavailable: native messages test menu coordinates; pointer visibility needs foreground verification";EnumChildWindows(reinterpret_cast<HWND>(player.discSurface_->winId()),[](HWND child,LPARAM result)->BOOL{if(GetPropW(child,L"VSR.BD.InputOwner"))*reinterpret_cast<HWND *>(result)=child;return TRUE;},reinterpret_cast<LPARAM>(&hit));QVERIFY(GetPropW(hit,L"VSR.BD.InputOwner"));PostMessageW(hit,WM_RBUTTONUP,0,0);}QTRY_VERIFY(qApp->activePopupWidget());QVERIFY(qApp->activePopupWidget()->geometry().contains(QCursor::pos()));qApp->activePopupWidget()->close();
        player.raise();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::qWait(1000);const auto size=QSize(1920,1080).scaled(player.discSurface_->size(),Qt::KeepAspectRatio);const auto origin=(player.discSurface_->size()-size)/2;const auto point=player.discSurface_->mapToGlobal(QPoint(origin.width()+qRound(1155.*size.width()/1920),origin.height()+qRound(127.*size.height()/1080)));const auto topLeft=player.discSurface_->mapToGlobal(QPoint());player.screen()->grabWindow(0,topLeft.x(),topLeft.y(),player.discSurface_->width(),player.discSurface_->height()).save("build/bd-menu-empty-window.png");qInfo()<<"BD mouse"<<point<<player.discSurface_->size()<<player.devicePixelRatioF();QCursor::setPos(point);QTest::qWait(250);if(interactive){mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);}else{GetCursorPos(&cursor.ptScreenPos);ScreenToClient(hit,&cursor.ptScreenPos);const auto at=MAKELPARAM(cursor.ptScreenPos.x,cursor.ptScreenPos.y);PostMessageW(hit,WM_MOUSEMOVE,0,at);PostMessageW(hit,WM_LBUTTONDOWN,MK_LBUTTON,at);PostMessageW(hit,WM_LBUTTONUP,0,at);}QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->active(),15000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);
    }
    void bdSwitchTimings_data(){QTest::addColumn<uint>("mode");QTest::newRow("HW")<<2u;QTest::newRow("SW")<<1u;}
    void bdSwitchTimings(){
        QFETCH(uint,mode);
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("decode/mode",mode);settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openBluRay(source));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);QStringList episodes,shorts,audio;for(const auto &path:player.files_){const auto group=player.blurayGroups_.value(path);if(group=="episode")episodes<<path;else if(group=="audio")audio<<path;else if(group=="extra" && BlurayCatalog::metadata(path).value("duration100ns").toDouble()<1800000000.)shorts<<path;}QVERIFY(episodes.size()>1 && shorts.size()>1 && !audio.isEmpty());
        for(const auto &path:QStringList{episodes[1],episodes[0],audio[0],shorts[0],shorts[1],episodes[1],audio[0]}){QTreeWidgetItem *item=nullptr;for(auto *candidate:player.playlist_->findItems({},Qt::MatchContains|Qt::MatchRecursive))if(candidate->data(0,Qt::UserRole).toString()==path){item=candidate;break;}QVERIFY(item);QElapsedTimer time;time.start();player.playlist_->itemDoubleClicked(item,0);QCOMPARE(player.source_,path);const auto response=time.elapsed();if(player.discMenu_ && player.discMenu_->active())QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->presentedFrames()>0,15000);else if(player.audioMode_)QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().decodedAudioFrames>0 && player.snapshot().state==ThreeFpState::Playing,15000);else QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2 && player.snapshot().state==ThreeFpState::Playing,15000);QVERIFY2(response<500,"GUI waited for old playback teardown");qInfo()<<"BD switch"<<player.blurayGroups_.value(path)<<QFileInfo(path).fileName()<<"response ms"<<response<<"first frame/audio ms"<<time.elapsed();}
        for(const auto &path:QStringList{shorts[0],shorts[1],shorts[0],shorts[1]}){QElapsedTimer time;time.start();QVERIFY2(player.openFile(path),qPrintable(player.message_->text()));QVERIFY(time.elapsed()<500);}QTRY_COMPARE_WITH_TIMEOUT(player.source_,shorts[1],15000);QTRY_VERIFY_WITH_TIMEOUT(player.pendingMediaOpen_.isEmpty() && player.snapshot().presentedVideoFrames>0,15000);QTest::qWait(1500);QCOMPARE(player.findChildren<QWidget *>("playerDiscSurface").size(),0);
    }
    void stableViewportForAllPopups(){
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("render/stableViewport",true);settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.sync();QTemporaryDir dir;const auto video=dir.filePath("viewport.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",video));
        PlayerWindow player;player.show();player.activateWindow();QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);player.layoutChrome();QTest::qWait(100);const auto size=player.pane_->surface()->size();const auto windowSize=player.size();
        player.setPlaylistPinned(true);QTest::qWait(100);QVERIFY(player.width()>windowSize.width());QCOMPARE(player.playlistPanel_->parentWidget(),player.videoSplit_);player.setPlaylistPinned(false);QTest::qWait(100);QCOMPARE(player.pane_->surface()->size(),size);
        player.toggleFullscreen();QTest::qWait(150);player.layoutChrome();const auto full=player.pane_->surface()->size();QVERIFY(full!=size);player.controls_->hide();QTest::qWait(100);QCOMPARE(player.pane_->surface()->size(),full);
        QCursor::setPos(player.pane_->mapToGlobal(QPoint(30,30)));player.showContextMenu(player.pane_->mapToGlobal(QPoint(100,100)));player.updateChrome();QVERIFY(!player.controls_->isVisible());QCOMPARE(player.pane_->surface()->size(),full);player.findChild<QMenu *>("playerContextMenu")->close();
        QDialog dialog(&player);dialog.show();QTest::qWait(50);QCOMPARE(player.pane_->surface()->size(),full);dialog.close();player.toggleFullscreen();QTest::qWait(100);QCOMPARE(player.pane_->surface()->size(),size);player.resize(player.width()+50,player.height()+20);QTest::qWait(150);QVERIFY(player.pane_->surface()->size()!=size);
    }
    void bdSubtitlesShortTitlesAndMenu(){
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE to test original BD");const auto scan=BlurayCatalog::scan(source);QVERIFY(!scan.titles.isEmpty());
        QString error;const auto episode=BlurayCatalog::prepare(scan.titles.first(),&error);QVERIFY2(!episode.isEmpty(),qPrintable(error));QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.setValue("playback/remember",false);settings.setValue("player/speed",1.);settings.sync();PlayerWindow player;player.show();QTemporaryDir snapshots;int captureCount=0;const auto capture=[&]{const auto path=snapshots.filePath(QString::number(++captureCount)+".png");if(!player.discMenu_->snapshot(path))return QImage();for(int i=0;i<80;++i){QImage image(path);if(!image.isNull())return image;QTest::qWait(25);}return QImage();};QSignalSpy failures(player.subtitles_.get(),&PlayerSubtitles::errorOccurred);QElapsedTimer elapsed;elapsed.start();QVERIFY(player.openFile(episode));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);qInfo()<<"BD first video ms"<<elapsed.elapsed();
        QSignalSpy images(player.subtitles_.get(),&PlayerSubtitles::imageReady);int subtitle=-1;for(const auto &entry:player.media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()=="subtitle"){subtitle=stream.value("index").toInt();break;}}QVERIFY(subtitle>=0);player.togglePlayback();player.selectSubtitle(0,subtitle);player.seekTime(30000000);QTest::qWait(1500);QCOMPARE(failures.size(),0);QVERIFY(!images.isEmpty());bool opaque=false;for(const auto &entry:images){const auto image=qvariant_cast<QImage>(entry.at(0));for(int y=0;y<image.height() && !opaque;++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))){opaque=true;break;}}QVERIFY2(opaque,"BD PGS did not produce visible subtitle pixels");
        int shortTitles=0;for(const auto &title:scan.titles)if(title.ticks<180*45000 && title.warning.isEmpty()){const auto playlist=BlurayCatalog::prepare(title,&error);QVERIFY(player.openFile(playlist));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);if(player.playing_)player.togglePlayback();QTest::qWait(100);const auto at=player.pane_->surface()->mapToGlobal(QPoint());const auto image=player.screen()->grabWindow(0,at.x(),at.y(),player.pane_->surface()->width(),player.pane_->surface()->height()).toImage();QVERIFY(!image.isNull());image.save("build/bd-short-"+QString::number(++shortTitles)+".png");}QCOMPARE(shortTitles,3);
        const auto root=QFileInfo(scan.titles.first().disc).absolutePath();QVERIFY(player.openBluRay(root,true));QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->active(),15000);QTRY_COMPARE_WITH_TIMEOUT(player.discMenu_->state(),3,15000);QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->presentedFrames()>0,10000);QTest::qWait(1500);QVERIFY(!player.findChild<QWidget *>("playerDiscNavigation"));const auto before=capture();QVERIFY(!before.isNull());before.save("build/bd-menu-before.png");QVERIFY(!before.copy(0,0,before.width(),before.height()/2).isGrayscale());player.discMenu_->navigate(4);QTest::qWait(300);player.discMenu_->navigate(3);player.discMenu_->navigate(0);QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->active(),15000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);
        QVERIFY(player.openDiscMenu(root));QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->presentedFrames()>0,15000);player.toggleFullscreen();player.raise();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::qWait(300);player.controls_->hide();const auto menuSize=player.pane_->surface()->size();const auto point=player.discSurface_->mapToGlobal(QPoint(100,100));SetCursorPos(point.x(),point.y());HWND hit=nullptr;EnumChildWindows(reinterpret_cast<HWND>(player.discSurface_->winId()),[](HWND child,LPARAM result)->BOOL{if(GetPropW(child,L"VSR.BD.InputOwner"))*reinterpret_cast<HWND *>(result)=child;return TRUE;},reinterpret_cast<LPARAM>(&hit));QVERIFY(hit);PostMessageW(hit,WM_RBUTTONUP,0,MAKELPARAM(100,100));QTRY_VERIFY_WITH_TIMEOUT(qApp->activePopupWidget(),3000);QVERIFY(!player.controls_->isVisible());QCOMPARE(player.pane_->surface()->size(),menuSize);qApp->activePopupWidget()->close();PostMessageW(hit,WM_KEYDOWN,VK_TAB,0);QTRY_VERIFY(player.infoVisible_);PostMessageW(hit,WM_KEYDOWN,VK_TAB,0);QTRY_VERIFY(!player.infoVisible_);player.toggleFullscreen();
        for(const auto &clip:QStringList{"00010","00011"})QVERIFY(BlurayCatalog::clipPlaybackIssue(QDir(scan.titles.first().disc).filePath("STREAM/"+clip+".m2ts")).contains("application_type=5"));
    }
    void emptyDecoderAndDirectBluRay(){PlayerWindow player;player.show();player.updateInfo();auto *decoder=player.findChild<QPushButton *>("playerDecoder");QVERIFY(decoder);QCOMPARE(decoder->text(),QString("3FP-HW"));decoder->click();player.updateInfo();QCOMPARE(decoder->text(),QString("3FP-SW"));decoder->click();const auto source=qEnvironmentVariable("VSR_BD_PLAYER_SOURCE");if(source.isEmpty())return;QVERIFY(player.openBluRay(source));QTRY_VERIFY_WITH_TIMEOUT(!player.files_.isEmpty(),15000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>5,20000);QVERIFY(player.playlistPinned_);QVERIFY(!player.media_.value("chapters").toArray().isEmpty());QVERIFY(!player.findChild<QWidget *>("blurayRemux"));QVERIFY(player.playlist_->topLevelItemCount()>0);}
    void softwareUploadAndBilinearMatchReference() {
        const auto reference=qEnvironmentVariable("VSR_REFERENCE_DLL");
        if(reference.isEmpty())QSKIP("Set VSR_REFERENCE_DLL to the unoptimized native DLL");
        const auto original=qgetenv("VSR_3FP_DLL");
        const auto restore=qScopeGuard([&]{qputenv("VSR_3FP_DLL",original);});
        QVector<ThreeFpPixelProbe> expected;
        double maximumError=0;
        for(int pass=0;pass<2;++pass){
            qputenv("VSR_3FP_DLL",pass==0?reference.toUtf8():original);
            QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
            ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer player(api,&surface);
            QVERIFY(player.setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(519),static_cast<ThreeFpScalingAlgorithm>(7)));
            int sampleIndex=0;
            for(int mode=0;mode<8;++mode){
                const bool subsampled=mode%4<2;
                const int bits=mode%4==0?8:mode%4==3?16:10;
                const bool pq=mode>=4;
                VapourSynthFrame frame;frame.width=320;frame.height=240;frame.totalFrames=1;frame.duration100ns=400000;
                frame.format=subsampled?(bits==8?ThreeFpExternalPixelFormat::Yuv420P8:ThreeFpExternalPixelFormat::Yuv420P10):
                    (bits==16?ThreeFpExternalPixelFormat::Yuv444P16:ThreeFpExternalPixelFormat::Yuv444P10);
                frame.colorRange=2;frame.colorMatrix=pq?9:1;frame.colorPrimaries=pq?9:1;frame.colorTransfer=pq?16:1;frame.chromaLocation=1;
                const unsigned maximum=(1u<<bits)-1;
                for(int plane=0;plane<3;++plane){
                    const int width=plane && subsampled?frame.width/2:frame.width;
                    const int height=plane && subsampled?frame.height/2:frame.height;
                    frame.strides[plane]=width*(bits==8?1:2)+16;
                    frame.planes[plane].resize(frame.strides[plane]*height);
                    for(int y=0;y<height;++y)for(int x=0;x<width;++x){
                        const unsigned value=plane?maximum/2+((x*7+y*3+plane*19)%101-50)*int(maximum/512):
                            ((x*11+y*17)%257)*maximum/256;
                        auto *row=frame.planes[plane].data()+y*frame.strides[plane];
                        if(bits==8)row[x]=char(value);else reinterpret_cast<quint16*>(row)[x]=quint16(value);
                    }
                }
                const auto pixels=frame.planes;QVERIFY2(player.submitFrame(frame),qPrintable(player.lastError()));
                QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>0,5000);
                for(int y:{13,59,107})for(int x:{17,79,143}){
                    ThreeFpPixelProbe pixel{};QVERIFY(player.samplePixel(x,y,pixel));
                    if(pass==0)expected<<pixel;
                    else {const auto &baseline=expected[sampleIndex];QCOMPARE(pixel.outputBitDepth,baseline.outputBitDepth);
                        for(double error:{std::abs(pixel.red-baseline.red),std::abs(pixel.green-baseline.green),std::abs(pixel.blue-baseline.blue)}){
                            maximumError=qMax(maximumError,error);QVERIFY(error<=1.0/1023+1e-6);
                        }
                    }
                    ++sampleIndex;
                }
                QCOMPARE(frame.planes,pixels);
            }
        }
        qInfo()<<"Native reference maximum RGB error"<<maximumError;
    }
    void bluRayNativePlaybackAndChapters() {
        const auto source=qEnvironmentVariable("VSR_BD_PLAYER_SOURCE");if(source.isEmpty())QSKIP("Set VSR_BD_PLAYER_SOURCE to a complete local BD folder");
        qInfo()<<"BD scan"<<source;const auto scan=BlurayCatalog::scan(source);QVector<BlurayTitle> titles;for(const auto &title:scan.titles)if(title.suggested && title.warning.isEmpty())titles<<title;QVERIFY(!titles.isEmpty());
        QString error;QStringList files;for(int i=0;i<qMin(2,titles.size());++i){const auto file=BlurayCatalog::prepare(titles[i],&error);QVERIFY2(!file.isEmpty(),qPrintable(error));files<<file;}
        qInfo()<<"BD prepared"<<files;PlayerWindow player;player.resize(960,540);player.show();player.settings_->setValue("basic/autoplay",true);player.settings_->setValue("playback/remember",false);player.settings_->setValue("player/renderer","VS");player.settings_->setValue("decode/audio","3FP");player.files_=files;qInfo()<<"BD opening";
        QVERIFY2(player.openFile(files.first()),qPrintable(player.clock_->lastError()));QVERIFY(player.mediaInput_.endsWith(".ffconcat"));QTest::qWait(3000);qInfo()<<"BD status"<<int(player.snapshot().state)<<"decoded"<<player.snapshot().decodedVideoFrames<<"presented"<<player.snapshot().presentedVideoFrames<<"duration"<<player.snapshot().duration100ns<<player.message_->text();QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>5,20000);QVERIFY(player.direct_);QVERIFY(!player.media_.value("chapters").toArray().isEmpty());QCOMPARE(player.files_.size(),files.size());
        const auto generation=player.snapshot().timelineGeneration;player.seekTime(600000000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>generation,15000);QTRY_VERIFY_WITH_TIMEOUT(qAbs(player.position()-600000000)<50000000,15000);
        if(files.size()>1){QVERIFY(player.openFile(files[1]));QTRY_VERIFY_WITH_TIMEOUT(player.ready_ && player.position()<100000000,20000);QTest::qWait(1000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().decodedVideoFrames>5 && player.snapshot().presentedVideoFrames>5,20000);QVERIFY(!player.media_.value("chapters").toArray().isEmpty());QCOMPARE(player.fileIndex_,1);QCOMPARE(player.source_,files[1]);QVERIFY(player.mediaInput_.endsWith(".ffconcat"));}qInfo()<<"BD frames"<<player.snapshot().presentedVideoFrames<<"chapters"<<player.media_.value("chapters").toArray().size();
    }
    void frameEditTakesPriorityAndPrefetchKeepsGpuBusy() {
        PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());
        player.ready_=true;player.clip_.fpsNumerator=48;player.clip_.fpsDenominator=1;player.clip_.totalFrames=1000;
        player.frame_->setFocus();player.frame_->selectAll();QTest::keyClicks(player.frame_,"37");QVERIFY(player.frameEditPending_);QTest::keyClick(player.frame_,Qt::Key_Return);
        QVERIFY(!player.isFullScreen());QVERIFY(!player.frameEditPending_);QCOMPARE(player.manualFrame_,37);
        player.pane_->setFocus();QTest::keyClick(player.pane_,Qt::Key_Return);QVERIFY(!player.isFullScreen());
        player.frame_->setFocus();player.frame_->selectAll();QTest::keyClicks(player.frame_,"51");player.pane_->setFocus();QTRY_VERIFY(!player.frameEditPending_);QCOMPARE(player.manualFrame_,51);
        player.usage_.gpu=80;player.usage_.cpu=90;player.usage_.ram=20;player.usage_.vram=30;player.usage_.totalMemoryMiB=32768;
        player.settings_->setValue("performance/gpu",50);player.settings_->setValue("performance/cpu",50);QVERIFY(player.prefetchCount()>0);
        for(int stage=0;stage<8;++stage){player.setInterpolation(stage);QCOMPARE(player.interpolationStage(),stage%4);QCOMPARE(player.interpolationRenderer(),stage/4);QVERIFY(QFileInfo::exists(player.preset_));}
        player.setInterpolation(1,true,1);QVERIFY(player.advanceInterpolation());QCOMPARE(player.interpolationStage(),2);QCOMPARE(player.interpolationRenderer(),1);
    }
    void informationPanelResizesWithoutWrapping() {
        QWidget host;host.resize(1200,800);host.show();host.activateWindow();QTRY_VERIFY(host.isActiveWindow());PlayerInfoPanel panel(&host);panel.showText("当前帧率：95.90 fps · 已提交：100\n目标帧率：95.90 · 预解码：8 帧\n补帧：RIFE\n缩放滤镜：Jinc\n色彩引擎：3FP");panel.show();QTest::qWait(20);QVERIFY(host.isActiveWindow());QVERIFY(panel.isWindow());QVERIFY(panel.width()<=host.width()-24);
        QVERIFY(!panel.wordWrap());const auto minimum=panel.minimumSize();const auto pointSize=panel.font().pointSizeF();const auto initial=panel.size();for(int i=0;i<1000;++i)panel.showText(panel.text());QCOMPARE(panel.size(),initial);QCOMPARE(panel.minimumSize(),minimum);
        panel.resize(minimum*2);QVERIFY(panel.font().pointSizeF()>pointSize);for(auto *label:panel.findChildren<QLabel *>())if(label->text()=="透明度"){QCOMPARE(label->font().pointSizeF(),panel.font().pointSizeF());QVERIFY(label->width()>=QFontMetrics(label->font()).horizontalAdvance(label->text()));}panel.resize(1,1);QCOMPARE(panel.size(),minimum);
        auto *opacity=panel.findChild<QSlider *>("playerInfoOpacity");QVERIFY(opacity);QCOMPARE(opacity->value(),50);opacity->setValue(40);QCOMPARE(panel.findChild<QLabel *>("playerInfoOpacityPercent")->text(),QString("40%"));QVERIFY(panel.findChild<QSizeGrip *>("playerInfoResize"));const auto image=panel.grab().toImage();QVERIFY(qAlpha(image.pixel(3,3))>=150 && qAlpha(image.pixel(3,3))<=155);QCOMPARE(qAlpha(image.pixel(3,image.height()-10)),qAlpha(image.pixel(3,3)));QDialog settings(&host);settings.setWindowModality(Qt::WindowModal);settings.show();settings.activateWindow();QTRY_VERIFY(!panel.isVisible());settings.close();host.activateWindow();QTRY_VERIFY(panel.isVisible());panel.grab().save("build/player-tab-transparency.png");
    }
    void nativeAudioClockDoesNotDecodeVideo() {
        QTemporaryDir directory;const auto path=directory.filePath("clock.mkv");QProcess encoder;
        encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-y","-f","lavfi","-i","testsrc2=size=320x180:rate=48:duration=3","-f","lavfi","-i","sine=duration=3","-c:v","ffv1","-c:a","pcm_s16le",path});QVERIFY(encoder.waitForFinished(20000));QCOMPARE(encoder.exitCode(),0);
        ThreeFpApi api;QWidget surface;surface.resize(320,180);surface.show();ThreeFpPlayer clock(api,&surface);QVERIFY(clock.setClockOnly(true));QVERIFY(clock.setDecodeMode(1));QVERIFY(clock.openFile(path));QTRY_COMPARE(clock.snapshot().state,ThreeFpState::Ready);QCOMPARE(clock.snapshot().videoWidth,320u);
        QVERIFY(clock.play());QTRY_VERIFY(clock.snapshot().position100ns>4000000);QCOMPARE(clock.snapshot().decodedVideoFrames,quint64(0));QTRY_VERIFY(clock.snapshot().decodedAudioFrames>0);
        QVERIFY(clock.pause());QTRY_COMPARE(clock.snapshot().state,ThreeFpState::Paused);const auto generation=clock.snapshot().timelineGeneration;QVERIFY(clock.seek(15000000));QTRY_VERIFY(clock.snapshot().timelineGeneration>generation);QCOMPARE(clock.snapshot().position100ns,qint64(15000000));QCOMPARE(clock.snapshot().decodedVideoFrames,quint64(0));
        clock.stop();QTRY_COMPARE(clock.snapshot().state,ThreeFpState::Idle);QVERIFY(clock.setClockOnly(false));QVERIFY(clock.openFile(path));QTRY_COMPARE(clock.snapshot().state,ThreeFpState::Ready);QVERIFY(clock.play());QTRY_VERIFY(clock.snapshot().decodedVideoFrames>0);
    }
    void interpolationPerformance() {
        const auto source=qEnvironmentVariable("VSR_PERF_SOURCE");if(source.isEmpty())QSKIP("Set VSR_PERF_SOURCE for interpolation throughput measurement.");
        const int stage=qEnvironmentVariableIntValue("VSR_PERF_STAGE"),seconds=qMax(5,qEnvironmentVariableIntValue("VSR_PERF_SECONDS"));
        QSettings(configPath(),QSettings::IniFormat).setValue("player/muted",true);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("playback/remember",false);settings.setValue("basic/autoplay",true);settings.setValue("subtitle/visible",false);settings.sync();
        PlayerWindow player;player.show();player.setInterpolation(stage);
        const double dpi=player.pane_->surface()->devicePixelRatioF();player.pane_->surface()->setFixedSize(qRound(2560/dpi),qRound(1440/dpi));
        QFile preset(player.preset_);QByteArray original;
        const int gpuThreads=qEnvironmentVariableIntValue("VSR_PERF_GPU_THREADS");
if(qEnvironmentVariableIntValue("VSR_PERF_INPUT_DIVISOR")==2 || gpuThreads>0){QVERIFY(preset.open(QIODevice::ReadOnly));original=preset.readAll();preset.close();auto script=QString::fromUtf8(original);if(qEnvironmentVariableIntValue("VSR_PERF_INPUT_DIVISOR")==2){const auto end=script.indexOf('\n',script.indexOf("src = core.ffms2.Source("));QVERIFY(end>0);script.insert(end+1,"src = core.std.SelectEvery(src, cycle=2, offsets=0)\n");}if(gpuThreads>0)script.replace("threads=4,","threads="+QString::number(gpuThreads)+",");QVERIFY(preset.open(QIODevice::WriteOnly|QIODevice::Truncate));preset.write(script.toUtf8());preset.close();}
        const auto restore=qScopeGuard([&]{if(!original.isEmpty()&&preset.open(QIODevice::WriteOnly|QIODevice::Truncate)){preset.write(original);preset.close();}});
        QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.lastFrame_>=0,60000);QTRY_VERIFY_WITH_TIMEOUT(player.playing_,10000);
        const auto wait=[](int milliseconds){QEventLoop loop;QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();};
        player.seekTime(1200000000);wait(5000);QVERIFY(player.interpolationStage()>=0);QVERIFY(!player.direct_);
        QFile csv(qEnvironmentVariable("VSR_PERF_CSV"));QVERIFY(csv.open(QIODevice::WriteOnly|QIODevice::Truncate));csv.write("seconds,position,submitted,skipped,accepted,dropped,coalesced,prefetch,cpu,gpu,memoryMiB,frame,lagMs\n");
        QElapsedTimer timer;timer.start();for(int i=0;i<=seconds;++i){if(i)wait(qMax(0,i*1000-int(timer.elapsed())));player.usage_=player.resources_.sample();const auto output=player.outputSnapshot();csv.write(QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13\n").arg(timer.elapsed()/1000.,0,'f',3).arg(player.position()/1e7,0,'f',3).arg(player.submittedFrames_).arg(player.skippedFrames_).arg(output.presentedVideoFrames).arg(output.droppedVideoFrames).arg(output.coalescedVideoFrames).arg(player.prefetchCount()).arg(player.usage_.processCpu,0,'f',1).arg(player.usage_.gpu,0,'f',1).arg(player.usage_.memoryMiB).arg(player.lastFrame_).arg((player.position()/1e7-player.lastFrame_*double(player.clip_.fpsDenominator)/player.clip_.fpsNumerator)*1000,0,'f',2).toUtf8());csv.flush();}
        qInfo()<<"interpolation-performance"<<stage<<player.clip_.width<<player.clip_.height<<double(player.clip_.fpsNumerator)/player.clip_.fpsDenominator<<"prefetch"<<player.prefetchCount();
        QVERIFY(player.playing_);QCOMPARE(player.interpolationStage(),stage%4);QCOMPARE(player.interpolationRenderer(),stage/4);
        QCOMPARE(player.snapshot().decodedVideoFrames,quint64(0));
    }
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
    void nativeRecoveryPerformance() {
        const auto source=qEnvironmentVariable("VSR_RECOVERY_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_RECOVERY_SOURCE for real software/AAC recovery measurements.");
        const unsigned mode=qEnvironmentVariableIntValue("VSR_RECOVERY_MODE")==2?2:1;
        const int seconds=qMax(10,qEnvironmentVariableIntValue("VSR_RECOVERY_SECONDS"));
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));
        QWidget surface;surface.setWindowFlags(Qt::FramelessWindowHint);surface.setGeometry(QApplication::primaryScreen()->geometry());surface.showFullScreen();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpPlayer player(api,&surface);QVERIFY(player.setDecodeMode(mode));
        QSignalSpy errors(&player,&ThreeFpPlayer::errorOccurred);
        const auto algorithm=qEnvironmentVariable("VSR_RECOVERY_ALGORITHM")=="Jinc"?ThreeFpScalingAlgorithm::Jinc2:ThreeFpScalingAlgorithm::D3D11Native;
        QVERIFY(player.setScalingAlgorithms(algorithm,algorithm));
        QElapsedTimer cold;cold.start();QVERIFY(player.openFile(source));
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,20000);
        QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,30000);
        const auto firstFrameMs=cold.elapsed();
        QTest::qWait(mode==2?10000:2000);
        QFile csv(qEnvironmentVariable("VSR_RECOVERY_CSV"));QVERIFY(csv.open(QIODevice::WriteOnly|QIODevice::Truncate));
        csv.write("wall_s,position_s,frame_pts,decoded,accepted,dropped,coalesced,presents,audio_decoded,audio_underruns,cpu_process,gpu,memoryMiB,first_frame_ms,video_clock_error_s,queued,upload_ms,device_wait_ms,present_wait_ms\n");
        PlayerResources resources;resources.sample();const auto first=player.snapshot();QElapsedTimer timer;timer.start();
        for(int i=0;i<=seconds;++i){
            if(i)QTest::qWait(qMax(0,i*1000-int(timer.elapsed())));
            const auto s=player.snapshot();const double wall=timer.elapsed()/1000.;const auto u=resources.sample();
            QVERIFY2(errors.isEmpty(),qPrintable(player.lastError()));QCOMPARE(s.state,ThreeFpState::Playing);
            const double error=s.frameTimeBaseDenominator?s.framePts*double(s.frameTimeBaseNumerator)/s.frameTimeBaseDenominator-s.position100ns/1e7:0;
            csv.write(QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19\n").arg(wall,0,'f',3).arg(s.position100ns/1e7,0,'f',3).arg(s.framePts).arg(s.decodedVideoFrames).arg(s.presentedVideoFrames).arg(s.droppedVideoFrames).arg(s.coalescedVideoFrames).arg(s.swapChainPresents).arg(s.decodedAudioFrames).arg(s.audioUnderruns).arg(u.processCpu).arg(u.gpu).arg(u.memoryMiB).arg(firstFrameMs).arg(error,0,'f',4).arg(s.queuedVideoFrames).arg(s.videoUpload100ns/10000.).arg(s.deviceLockWait100ns/10000.).arg(s.presentWait100ns/10000.).toUtf8());csv.flush();
            QVERIFY2(s.state==ThreeFpState::Playing,qPrintable(player.lastError()));
        }
        const auto last=player.snapshot();
        qInfo()<<"recovery"<<source<<mode<<"decoded"<<last.decodedVideoFrames-first.decodedVideoFrames<<"accepted"<<last.presentedVideoFrames-first.presentedVideoFrames<<"drops"<<last.droppedVideoFrames-first.droppedVideoFrames<<"position-seconds"<<(last.position100ns-first.position100ns)/1e7<<"first-frame-ms"<<firstFrameMs;
        QVERIFY(last.position100ns>first.position100ns+10'000'000);
        QVERIFY(last.presentedVideoFrames>first.presentedVideoFrames+10);
        if(mode==2 && algorithm==ThreeFpScalingAlgorithm::D3D11Native) {
            QCOMPARE(last.droppedVideoFrames-first.droppedVideoFrames,quint64(0));
            QCOMPARE(last.coalescedVideoFrames-first.coalescedVideoFrames,quint64(0));
            QVERIFY(last.presentedVideoFrames-first.presentedVideoFrames>quint64(seconds*47.4));
            QVERIFY((last.position100ns-first.position100ns)/1e7>seconds*.98);
            QVERIFY((last.framePts-first.framePts)*double(last.frameTimeBaseNumerator)/last.frameTimeBaseDenominator>seconds*.98);
        }
        qInfo()<<"opening-drops"<<first.droppedVideoFrames<<"output"<<surface.size()*surface.devicePixelRatioF()<<"screen-refresh"<<QApplication::primaryScreen()->refreshRate();
    }
    void softwareAv1SeekRecovery() {
        const auto source=qEnvironmentVariable("VSR_RECOVERY_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_RECOVERY_SOURCE to the AV1 12-bit regression video.");
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));
        QWidget surface;surface.resize(960,540);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpPlayer player(api,&surface);QVERIFY(player.setDecodeMode(1));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));
        QSignalSpy errors(&player,&ThreeFpPlayer::errorOccurred);
        QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,20000);
        const auto info=QJsonDocument::fromJson(player.mediaInfo().toUtf8()).object();
        QCOMPARE(info.value("videoDecoderThreads").toInt(),int(std::min(32u,2*std::max(1u,std::thread::hardware_concurrency()))));
        QVERIFY(info.value("softwareVideoDecodeAsync").toBool());
        bool format=false;for(const auto &entry:info.value("streams").toArray()) {
            const auto stream=entry.toObject();if(stream.value("type").toString()!="video")continue;
            QCOMPARE(stream.value("decoderFrameDelay").toInt(),info.value("videoDecoderThreads").toInt());
            format=stream.value("decoderPixelFormat").toString()=="yuv444p12le";
        }
        QVERIFY(format);QVERIFY(player.play());
        for(const qint64 target:{0LL,600000000LL,100000000LL,840000000LL}) {
            const auto generation=player.snapshot().timelineGeneration;QElapsedTimer timer;timer.start();
            QVERIFY(player.seek(target));
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>generation && player.snapshot().presentedVideoFrames>0,15000);
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().position100ns>target+10000000,15000);
            qInfo()<<"AV1 seek"<<target/1e7<<"resume-ms"<<timer.elapsed();
            const auto first=player.snapshot();QTest::qWait(3000);const auto last=player.snapshot();
            QVERIFY2(errors.isEmpty(),qPrintable(player.lastError()));
            QCOMPARE(last.state,ThreeFpState::Playing);QVERIFY(last.presentedVideoFrames>first.presentedVideoFrames+30);
            QVERIFY(player.pause());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
            const auto stopping=player.snapshot();QTest::qWait(200);const auto paused=player.snapshot();
            QVERIFY(paused.presentedVideoFrames<=stopping.presentedVideoFrames+1);
            QTest::qWait(200);QCOMPARE(player.snapshot().presentedVideoFrames,paused.presentedVideoFrames);
            QVERIFY(player.play());
        }
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ended,20000);
        QVERIFY2(errors.isEmpty(),qPrintable(player.lastError()));
        player.stop();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Idle);
        QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,20000);
        QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>20,10000);
        QVERIFY2(errors.isEmpty(),qPrintable(player.lastError()));
    }
    void softwareAv1Gui() {
        const auto source=qEnvironmentVariable("VSR_RECOVERY_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_RECOVERY_SOURCE to the AV1 12-bit regression video.");
        QSettings settings(configPath(),QSettings::IniFormat);
        settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);
        const bool subtitles=qEnvironmentVariable("VSR_RECOVERY_SUBTITLES")!="0";
        settings.setValue("decode/mode",1);settings.setValue("player/speed",1.);settings.setValue("subtitle/visible",subtitles);
        settings.sync();PlayerWindow player;player.show();player.toggleFullscreen();
        player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
        QElapsedTimer opening;opening.start();QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && player.outputSnapshot().presentedVideoFrames>0,20000);
        const auto firstMs=opening.elapsed();QVERIFY(player.direct_);QCOMPARE(player.outputSnapshot().decodeMode,1u);
        const int seconds=qEnvironmentVariableIntValue("VSR_RECOVERY_SECONDS")>0?qEnvironmentVariableIntValue("VSR_RECOVERY_SECONDS"):30;
        QFile csv(qEnvironmentVariable("VSR_RECOVERY_CSV"));const bool record=csv.open(QIODevice::WriteOnly|QIODevice::Truncate);
        if(record)csv.write("wall_s,position_s,decoded,accepted,dropped,coalesced,audio_underruns,first_frame_ms,video_lag_ms,upload_ms,device_wait_ms\n");
        const auto first=player.outputSnapshot();QElapsedTimer elapsed;elapsed.start();
        for(int i=0;i<=seconds;++i){
            if(i)QTest::qWait(qMax(0,i*1000-int(elapsed.elapsed())));const auto s=player.outputSnapshot();
            if(record){csv.write(QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11\n").arg(elapsed.elapsed()/1000.,0,'f',3).arg(s.position100ns/1e7,0,'f',3).arg(s.decodedVideoFrames).arg(s.presentedVideoFrames).arg(s.droppedVideoFrames).arg(s.coalescedVideoFrames).arg(s.audioUnderruns).arg(firstMs).arg(s.frameTimeBaseDenominator?(s.position100ns-s.framePts*double(s.frameTimeBaseNumerator)*1e7/s.frameTimeBaseDenominator)/10000:0,0,'f',2).arg(s.videoUpload100ns/10000.,0,'f',2).arg(s.deviceLockWait100ns/10000.,0,'f',2).toUtf8());csv.flush();}
        }
        const auto last=player.outputSnapshot();
        QCOMPARE(last.state,ThreeFpState::Playing);QCOMPARE(last.decodeMode,1u);QVERIFY(last.videoWidth>0);
        QVERIFY(last.presentedVideoFrames>first.presentedVideoFrames+10);
        qInfo()<<"AV1 full GUI"<<seconds<<"seconds"<<"accepted"<<last.presentedVideoFrames-first.presentedVideoFrames
            <<"dropped"<<last.droppedVideoFrames-first.droppedVideoFrames<<"audio-underruns"<<last.audioUnderruns-first.audioUnderruns
            <<"first-ms"<<firstMs<<"output-bits"<<last.videoOutputBitDepth;
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(200);
        const auto shot=player.clock_->capture();QVERIFY(!shot.isNull());QSet<QRgb> colors;
        for(int y=20;y<shot.height()-20;y+=40)for(int x=20;x<shot.width()-20;x+=40)colors.insert(shot.pixel(x,y));
        QVERIFY(colors.size()>30);
    }
    void croppedSubtitlePixels() {
        const auto source=qEnvironmentVariable("VSR_RECOVERY_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_RECOVERY_SOURCE for native subtitle placement checks.");
        ThreeFpApi api;QVERIFY(api.available());QWidget surface;surface.resize(640,360);surface.show();
        QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpPlayer player(api,&surface);QVERIFY(player.setDecodeMode(1));
        QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,15000);
        QVERIFY(player.play());QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);
        QVERIFY(player.pause());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(200);
        QImage caption(surface.size()*surface.devicePixelRatioF(),QImage::Format_ARGB32_Premultiplied);caption.fill(Qt::transparent);
        const QRect area(110,80,250,90);QPainter painter(&caption);painter.fillRect(area,QColor(255,32,64,190));painter.end();
        QVERIFY(player.setSubtitle(caption));QTest::qWait(200);const auto full=player.capture();QVERIFY(!full.isNull());
        caption.setText("vsrSubtitleRect","110,80,250,90");QVERIFY(player.setSubtitle(caption));QTest::qWait(200);
        QCOMPARE(player.capture(),full);
        caption.fill(Qt::transparent);caption.setText("vsrSubtitleRect","0,0,0,0");QVERIFY(player.setSubtitle(caption));QTest::qWait(200);
        QVERIFY(player.capture()!=full);
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
        PlayerWindow player;player.show();player.toggleFullscreen();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
        auto *surface=player.pane_->surface();const double dpi=surface->devicePixelRatioF();surface->setFixedSize(qRound(3840/dpi),qRound(2160/dpi));
        QVERIFY(QTest::qWaitForWindowExposed(&player));
        bool visibleSubtitle=false;
        connect(player.subtitles_.get(),&PlayerSubtitles::imageReady,&player,[&](const QImage &image){
            if(visibleSubtitle || image.isNull())return;
            for(int y=0;y<image.height();y+=4){const auto *row=reinterpret_cast<const QRgb *>(image.constScanLine(y));for(int x=0;x<image.width();x+=4)if(qAlpha(row[x])){visibleSubtitle=true;return;}}
        });
        QElapsedTimer cold;cold.start();QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().swapChainPresents>0 && player.outputSnapshot().presentedVideoFrames>0,20000);const auto firstFrameMs=cold.elapsed();
        player.seekTime(1200000000);QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().timelineGeneration>0 && player.position()>=1200000000,20000);
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,5000);
        const auto synchronized=[&]{const auto s=player.outputSnapshot();return s.frameTimeBaseDenominator>0 && qAbs(s.framePts*double(s.frameTimeBaseNumerator)*1e7/s.frameTimeBaseDenominator-s.position100ns)<500000;};
        QFile prerollCsv(qEnvironmentVariable("VSR_PERF_CSV")+".preroll.csv");
        QVERIFY(prerollCsv.open(QIODevice::WriteOnly|QIODevice::Truncate));
        prerollCsv.write("wall_s,position_s,frame_pts,accepted,dropped,audio_underruns,video_clock_error_s\n");
        QElapsedTimer preroll,stable;preroll.start();
        while(preroll.elapsed()<30000 && (!stable.isValid() || stable.elapsed()<5000)){
            QTest::qWait(100);
            const auto s=player.outputSnapshot();
            const double error=s.frameTimeBaseDenominator?s.framePts*double(s.frameTimeBaseNumerator)/s.frameTimeBaseDenominator-s.position100ns/1e7:0;
            prerollCsv.write(QString("%1,%2,%3,%4,%5,%6,%7\n").arg(preroll.elapsed()/1000.,0,'f',3).arg(s.position100ns/1e7,0,'f',4).arg(s.framePts).arg(s.presentedVideoFrames).arg(s.droppedVideoFrames).arg(s.audioUnderruns).arg(error,0,'f',4).toUtf8());prerollCsv.flush();
            if(!synchronized())stable.invalidate();else if(!stable.isValid())stable.start();
        }
        QVERIFY2(stable.isValid() && stable.elapsed()>=5000,"Video/audio clocks did not remain synchronized for five seconds.");
        qInfo()<<"synchronized-preroll-ms"<<preroll.elapsed();
        QFile csv(qEnvironmentVariable("VSR_PERF_CSV"));QVERIFY(csv.open(QIODevice::WriteOnly|QIODevice::Truncate));
        csv.write("wall_s,position_s,decoded,accepted,dropped,coalesced,presents,audio_underruns,width,height,decode_mode,scaling_mode,subtitle_visible,first_frame_ms,seek_generation,frame_pts\n");
        const auto first=player.outputSnapshot();QElapsedTimer timer;timer.start();
        for(int sample=0;sample<=seconds;++sample){
            if(sample)QTest::qWait(qMax(0,sample*1000-int(timer.elapsed())));
            RECT rect{};QVERIFY(GetClientRect(reinterpret_cast<HWND>(surface->winId()),&rect));QCOMPARE(rect.right-rect.left,3840L);QCOMPARE(rect.bottom-rect.top,2160L);
            const auto s=player.outputSnapshot();QVERIFY(player.direct_);QCOMPARE(s.state,ThreeFpState::Playing);QCOMPARE(s.decodeMode,2u);QCOMPARE(s.videoScalingMode,0u);
            QVERIFY(s.frameTimeBaseDenominator>0);
            QVERIFY(qAbs(s.framePts*double(s.frameTimeBaseNumerator)*1e7/s.frameTimeBaseDenominator-s.position100ns)<1000000);
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
        const auto at=player.pane_->surface()->mapToGlobal(QPoint());const auto image=player.screen()->grabWindow(0,at.x(),at.y(),player.pane_->surface()->width(),player.pane_->surface()->height()).toImage();QVERIFY(!image.isNull());QSet<QRgb> colors;for(int y=40;y<image.height()-40;y+=40)for(int x=40;x<image.width()-40;x+=40)colors.insert(image.pixel(x,y));QVERIFY(colors.size()>30);
        QVERIFY(image.save(qEnvironmentVariable("VSR_PERF_CSV")+".png"));
    }
    void hardwareNativeAndShaderCapture() {
        const auto source=qEnvironmentVariable("VSR_HARDWARE_CAPTURE_SOURCE");if(source.isEmpty())QSKIP("Set VSR_HARDWARE_CAPTURE_SOURCE for decoder-surface capture checks.");
        ThreeFpApi api;QWidget surface;surface.resize(960,540);surface.show();ThreeFpPlayer native(api,&surface);QVERIFY(native.setDecodeMode(2));
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));QVERIFY(native.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(native.snapshot().state,ThreeFpState::Ready,20000);
        const auto generation=native.snapshot().timelineGeneration;QVERIFY(native.seek(1200000000));QTRY_VERIFY_WITH_TIMEOUT(native.snapshot().timelineGeneration>generation && native.snapshot().swapChainPresents>0,10000);QTest::qWait(500);
        const auto frame=native.snapshot().frameIndex;
        const auto capture=[&]{const auto image=native.capture();QSet<QRgb> colors;for(int y=20;y<image.height()-20;y+=20)for(int x=20;x<image.width()-20;x+=20)colors.insert(image.pixel(x,y));return colors.size()>30?image:QImage{};};
        const auto original=capture();QVERIFY(!original.isNull());QCOMPARE(native.snapshot().videoScalingMode,0u);
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::Jinc2,ThreeFpScalingAlgorithm::Jinc2));QTest::qWait(300);QVERIFY(!capture().isNull());QCOMPARE(native.snapshot().videoScalingMode,0u);
        QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));native.setView(2,.1f,.1f);QTest::qWait(300);const auto zoomed=capture();QVERIFY(!zoomed.isNull());QVERIFY(zoomed!=original);
        native.setView(1,0,0);QTest::qWait(300);QVERIFY(!capture().isNull());QCOMPARE(native.snapshot().videoScalingMode,0u);QCOMPARE(native.snapshot().frameIndex,frame);
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

    void interpolationLowestWarning() {
        PlayerWindow player;player.show();player.pane_->setSurfaceActive(true);player.timer_->stop();player.setInterpolation(3,true);player.ready_=player.playing_=true;player.qualitySettling_.start();QTest::qWait(2050);player.updateProfile();QTest::qWait(5050);player.submittedFrames_=100;player.skippedFrames_=20;player.updateProfile();QCOMPARE(player.interpolationStage(),3);QVERIFY(player.findChild<QLabel *>("playerInterpolationWarning")->isVisible());
        player.qualityTimer_.invalidate();player.updateProfile();QTest::qWait(5050);player.submittedFrames_+=100;player.updateProfile();QVERIFY(!player.findChild<QLabel *>("playerInterpolationWarning")->isVisible());
        QVERIFY(player.clock_->setScalingAlgorithms(ThreeFpScalingAlgorithm::Lanczos4,ThreeFpScalingAlgorithm::Lanczos4));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("player/interpolationStart",2);settings.sync();player.source_.clear();player.imageMode_=false;player.setInterpolation(0,true);QTemporaryDir dir;QImage image(16,16,QImage::Format_RGB32);image.fill(Qt::red);const auto path=dir.filePath("source.png");QVERIFY(image.save(path));QVERIFY(player.openFile(path));QCOMPARE(player.interpolationStage(),2);
    }

    void imageToolsViewAndDialogs() {
        QTemporaryDir directory;QImage image(120,80,QImage::Format_RGB32);for(int y=0;y<80;++y)for(int x=0;x<120;++x)image.setPixelColor(x,y,y<40?(x<60?Qt::red:Qt::green):(x<60?Qt::blue:Qt::yellow));const auto source=directory.filePath("工具测试.png");QVERIFY(image.save(source));
        PlayerWindow player;player.show();auto *pane=player.findChild<PreviewPane *>();auto *tools=player.findChild<PlayerImageTools *>();QVERIFY(tools);QVERIFY(!tools->isVisible());QVERIFY(player.openFile(source));QTRY_VERIFY(!pane->image().isNull());QVERIFY(tools->isVisible());
        const QStringList names{"edit","rotateLeft","rotateRight","mirror","recycle","wallpaper","convert"};int previous=-1;
        for(const auto &name:names){auto *button=tools->findChild<QToolButton *>("imageTool_"+name);QVERIFY(button);QVERIFY(!button->icon().isNull());QVERIFY(!button->toolTip().isEmpty());QCOMPARE(button->toolButtonStyle(),Qt::ToolButtonIconOnly);QVERIFY(button->x()>previous);previous=button->x();QVERIFY(button->isEnabled());}
        const auto *pixels=pane->image().constBits();tools->findChild<QToolButton *>("imageTool_rotateRight")->click();QCOMPARE(pane->imageDisplaySize(),QSize(80,120));QCOMPARE(pane->image().constBits(),pixels);
        const auto corner=[pane](double x,double y){const auto fitted=pane->imageDisplaySize().scaled(pane->surface()->size(),Qt::KeepAspectRatio);const auto shot=pane->surface()->grab().toImage();const auto dpr=shot.devicePixelRatio();return shot.pixelColor(qRound(((pane->surface()->width()-fitted.width())/2+fitted.width()*x)*dpr),qRound(((pane->surface()->height()-fitted.height())/2+fitted.height()*y)*dpr));};
        QCOMPARE(corner(.25,.25),QColor(Qt::blue));QCOMPARE(corner(.75,.25),QColor(Qt::red));QCOMPARE(corner(.25,.75),QColor(Qt::yellow));QCOMPARE(corner(.75,.75),QColor(Qt::green));
        pane->adoptView(10000,1,1);QCOMPARE(pane->surface()->grab().toImage().pixelColor(pane->surface()->width()*pane->devicePixelRatioF()/2,pane->surface()->height()*pane->devicePixelRatioF()/2),QColor(Qt::blue));pane->adoptView(1,0,0);
        tools->findChild<QToolButton *>("imageTool_rotateLeft")->click();QVERIFY(pane->imageTransform().isIdentity());tools->findChild<QToolButton *>("imageTool_mirror")->click();QCOMPARE(pane->imageDisplaySize(),image.size());QCOMPARE(pane->image().constBits(),pixels);
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
    void timelineSingleJump_data(){QTest::addColumn<bool>("oneCpu");QTest::newRow("SW")<<false;QTest::newRow("SW-one-CPU")<<true;}
    void timelineSingleJump(){
        QFETCH(bool,oneCpu);DWORD_PTR affinity=0,system=0;QVERIFY(GetProcessAffinityMask(GetCurrentProcess(),&affinity,&system));if(oneCpu)QVERIFY(SetProcessAffinityMask(GetCurrentProcess(),affinity & (~affinity+1)));const auto restoreAffinity=qScopeGuard([&]{SetProcessAffinityMask(GetCurrentProcess(),affinity);});
        QTemporaryDir dir;const auto source=dir.filePath("jump.mkv");QProcess encode;encode.start("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe",{"-v","error","-f","lavfi","-i","testsrc2=size=1280x720:rate=24:duration=8","-c:v","libx264","-preset","ultrafast","-g","192","-y",source});QVERIFY(encode.waitForFinished(15000));QCOMPARE(encode.exitCode(),0);QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("decode/mode",1);settings.setValue("playback/remember",false);settings.sync();PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,10000);
        QTest::qWait(300);auto *slider=player.timeline_;auto generation=player.snapshot().timelineGeneration;const auto before=player.outputSnapshot().presentedVideoFrames;slider->setSliderDown(true);slider->setValue(75000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().presentedVideoFrames>before && std::abs(player.position()-60000000)<500000,10000);qInfo()<<"preview generation"<<generation<<player.snapshot().timelineGeneration<<"target"<<player.timelineSeekTarget_<<slider->sliderPosition()<<player.snapshot().duration100ns;slider->setSliderDown(false);QTest::qWait(200);QCOMPARE(player.snapshot().timelineGeneration,generation+1);
        generation=player.snapshot().timelineGeneration;const auto presented=player.outputSnapshot().presentedVideoFrames;QElapsedTimer elapsed;elapsed.start();QTest::mouseClick(slider,Qt::LeftButton,Qt::NoModifier,QPoint(slider->width()/2,slider->height()/2));QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().presentedVideoFrames>presented && std::abs(player.position()-40000000)<2000000,10000);QCOMPARE(player.snapshot().timelineGeneration,generation+1);qInfo()<<"SW timeline click jump ms"<<elapsed.elapsed();
    }
    void timelinePerformance() {
        const auto source=qEnvironmentVariable("VSR_SCRUB_SOURCE");if(source.isEmpty())QSKIP("Set VSR_SCRUB_SOURCE for real 2160p scrubbing measurements.");
        const int stage=qEnvironmentVariableIntValue("VSR_SCRUB_STAGE");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("playback/remember",false);settings.sync();
        PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath(stage==5?"builtin/Anime-5-D3D11.vpy":"builtin/Anime-4-Jinc.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>0,20000);
        player.seekTime(1200000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_,20000);QTest::qWait(500);
        const auto first=player.outputSnapshot();QElapsedTimer elapsed;elapsed.start();QTimer samples;samples.setTimerType(Qt::PreciseTimer);samples.setInterval(16);player.timeline_->setSliderDown(true);
        connect(&samples,&QTimer::timeout,&player,[&]{const qint64 target=1200000000+elapsed.elapsed()*10000;player.timeline_->setValue(int(target*100000/player.snapshot().duration100ns));});samples.start();QEventLoop loop;QTimer::singleShot(10000,&loop,&QEventLoop::quit);loop.exec();samples.stop();
        const auto last=player.outputSnapshot();qInfo()<<"timeline-performance"<<stage<<player.clip_.width<<player.clip_.height<<"shown"<<last.presentedVideoFrames-first.presentedVideoFrames<<"per-second"<<(last.presentedVideoFrames-first.presentedVideoFrames)*1000./elapsed.elapsed();
        player.timeline_->setSliderDown(false);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_,20000);QVERIFY(player.direct_);QVERIFY(!player.playing_);
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
    void externalSubtitleVariants() {
        QFile config(configPath());const bool existed=config.exists();QByteArray saved;if(config.open(QIODevice::ReadOnly))saved=config.readAll();config.close();const auto restore=qScopeGuard([&]{if(!existed){QFile::remove(configPath());return;}QFile file(configPath());if(file.open(QIODevice::WriteOnly))file.write(saved);});
        QTemporaryDir dir;const auto source=dir.filePath("xxx.mkv"),sc=dir.filePath("xxx.sc-jp.ass"),tc=dir.filePath("xxx.tc-jp.ass");
        QProcess encode;encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=2","-c:v","ffv1","-y",source});QVERIFY(encode.waitForFinished(15000));QCOMPARE(encode.exitCode(),0);
        for(const auto &path:{sc,tc}){QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write("[Script Info]\nScriptType: v4.00+\nPlayResX: 320\nPlayResY: 180\n[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\nStyle: Default,Arial,24,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,1,0,2,10,10,10,1\n[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\nDialogue: 0,0:00:00.00,0:00:02.00,Default,,0,0,0,,");file.write(path==sc?"SC and JP caption\n":"TC and JP caption\n");}
        QCOMPARE(playerMatchingSubtitles(source),QStringList({sc,tc}));
        QCOMPARE(playerMatchingSubtitles(dir.filePath("ReleasePrefixxxx.mkv")),QStringList({sc,tc}));
        QVERIFY(QFile::copy(sc,dir.filePath("Movie.Title.sc-jp.ass")));QCOMPARE(playerMatchingSubtitles(dir.filePath("Release.Movie.Title.1080p.mkv")),QStringList({dir.filePath("Movie.Title.sc-jp.ass")}));
        QVERIFY(QFile::copy(sc,dir.filePath("S01E01.sc-jp.ass")));QVERIFY(QFile::copy(tc,dir.filePath("S01E02.tc-jp.ass")));QCOMPARE(playerMatchingSubtitles(dir.filePath("Show-S01E01-1080p.mkv")),QStringList({dir.filePath("S01E01.sc-jp.ass")}));
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("decode/mode",1);settings.setValue("player/preset",QString());settings.setValue("playback/remember",false);settings.setValue("subtitle/visible",true);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(player.externalSubtitle_,sc,10000);QCOMPARE(player.primarySubtitle_,-2);for(const auto &entry:player.media_.value("streams").toArray())QVERIFY(entry.toObject().value("type").toString()!="subtitle");
        QSignalSpy frames(player.subtitles_.get(),&PlayerSubtitles::imageReady),errors(player.subtitles_.get(),&PlayerSubtitles::errorOccurred);player.subtitles_->render(5000000,QSize(640,360),QSize(320,180),true);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),5000);QVERIFY(errors.isEmpty());const auto first=qvariant_cast<QImage>(frames.last().first());QVERIFY(!first.isNull());
        player.showContextMenu(player.mapToGlobal(QPoint(80,80)));auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);QMenu *captions=nullptr;for(auto *action:menu->actions())if(action->text()==QStringLiteral("字幕设置"))captions=action->menu();QVERIFY(captions);QAction *traditional=nullptr;auto *primary=captions->actions().first()->menu();for(auto *action:primary->actions())if(action->text()==QFileInfo(tc).fileName())traditional=action;QVERIFY(traditional);traditional->trigger();QCOMPARE(player.externalSubtitle_,tc);menu->close();frames.clear();player.subtitles_->render(5000000,QSize(640,360),QSize(320,180),true);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),5000);QVERIFY(errors.isEmpty());QVERIFY(qvariant_cast<QImage>(frames.last().first())!=first);
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
        const auto menu=[&](){QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(20,20),pane->surface()->mapToGlobal(QPoint(20,20)));QApplication::sendEvent(pane->surface(),&context);return player.findChild<QMenu *>("playerContextMenu");};
        auto *root=menu();QVERIFY(root);QMenu *audio=nullptr,*subtitles=nullptr;int ai=-1,si=-1;for(int i=0;i<root->actions().size();++i){auto *a=root->actions()[i];if(a->text()==QStringLiteral("音频设置")){audio=a->menu();ai=i;}if(a->text()==QStringLiteral("字幕设置")){subtitles=a->menu();si=i;}}QVERIFY(audio && subtitles);QCOMPARE(si,ai+1);
        auto *tracks=audio->actions().first()->menu();QVERIFY(tracks);QVERIFY(tracks->actions().size()>=4);tracks->actions()[1]->trigger();QTRY_VERIFY(player.snapshot().isExternalAudio==0 && player.snapshot().selectedAudioStream==2);
        audio->actions().last()->trigger();root->close();QTRY_VERIFY(player.findChild<QDialog *>("playerEqualizer"));auto *eq=player.findChild<QDialog *>("playerEqualizer");QVERIFY(!eq->isModal());QCOMPARE(eq->findChildren<QSlider *>().size(),12);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);auto *toggle=eq->findChild<QCheckBox *>();toggle->setChecked(true);eq->findChild<QSlider *>("equalizerBand4")->setValue(6);const auto before=player.position();QTRY_VERIFY_WITH_TIMEOUT(player.position()>before,3000);QCOMPARE(player.snapshot().state,ThreeFpState::Playing);QCOMPARE(player.windowTitle(),title);eq->grab().save("build/audio-equalizer.png");eq->close();
        root=menu();audio=nullptr;for(auto *a:root->actions())if(a->text()==QStringLiteral("音频设置"))audio=a->menu();QVERIFY(audio);auto *sync=audio->actions()[1]->menu();sync->actions()[1]->trigger();root->close();QTest::qWait(100);root=menu();for(auto *a:root->actions())if(a->text()==QStringLiteral("音频设置"))audio=a->menu();QVERIFY(audio->actions()[1]->menu()->actions().last()->text().contains("0.1"));root->close();
        QMimeData captions;captions.setUrls({QUrl::fromLocalFile(subtitle)});const auto captionTop=pane->surface()->mapTo(&player,QPoint(60,pane->surface()->height()/5));QDragEnterEvent captionEnter(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionEnter);QDragMoveEvent captionMove(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionMove);QVERIFY(player.findChild<QLabel *>("playerDropHint")->isVisible());QVERIFY(player.findChild<QLabel *>("playerDropHint")->text().contains(QStringLiteral("次字幕")));QDropEvent captionDrop(captionTop,Qt::CopyAction,&captions,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&player,&captionDrop);QCOMPARE(player.windowTitle(),title);root=menu();for(auto *a:root->actions())if(a->text()==QStringLiteral("字幕设置"))subtitles=a->menu();const auto secondary=subtitles->actions()[1]->menu()->actions();QVERIFY(std::any_of(secondary.cbegin(),secondary.cend(),[&](const auto *a){return a->isChecked() && a->text().contains(QFileInfo(subtitle).fileName());}));root->close();
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
        const QPointF cursor(pane->surface()->rect().center());QWheelEvent wheel(cursor,pane->surface()->mapToGlobal(cursor.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        const auto zoom=pane->zoom();QApplication::sendEvent(pane->surface(),&wheel);QVERIFY(pane->zoom()>zoom);
        QVERIFY(!player.findChild<QSlider *>("playerTimeline")->isEnabled());
        QTemporaryDir directory;QImage small(320,180,QImage::Format_RGBA8888);small.fill(Qt::red);const auto next=directory.filePath("small.png");QVERIFY(small.save(next));QVERIFY(player.openFile(next));QTRY_COMPARE(pane->image().size(),small.size());
        const auto smallZoom=pane->zoom();QApplication::sendEvent(pane->surface(),&wheel);QVERIFY(pane->zoom()>smallZoom);
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
        QTRY_VERIFY2_WITH_TIMEOUT(player.outputSnapshot().videoWidth==320u,qPrintable(player.message_->text()),15000);
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
        const auto preset=dir.filePath("capture.vpy");QVERIFY(PresetStore::write(preset,"import vapoursynth as vs\nvs.core.ffms2.Source(source=_vsr_source).set_output()\n"));PlayerWindow player;player.show();player.loadPreset(preset);QVERIFY(player.openFile(source));QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);player.togglePlayback();QTest::qWait(500);
        auto *pane=player.findChild<PreviewPane *>();QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(30,30),pane->surface()->mapToGlobal(QPoint(30,30)));QApplication::sendEvent(pane->surface(),&event);auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);
        QMenu *tracks=nullptr,*capture=nullptr;for(auto *action:menu->actions()){if(action->text()==QStringLiteral("字幕设置"))tracks=action->menu();if(action->text()==QStringLiteral("图像截取"))capture=action->menu();}QVERIFY(tracks && capture);auto *primary=tracks->actions().first()->menu();QVERIFY(primary);const auto choices=primary->actions();QVERIFY(std::any_of(choices.cbegin(),choices.cend(),[&](const auto *a){return a->isChecked() && a->text().contains(QFileInfo(path).fileName());}));
        capture->actions().first()->trigger();QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);QVERIFY2(!player.message_->text().contains(QStringLiteral("截图失败")),qPrintable(player.message_->text()));capture->actions().last()->trigger();menu->close();QTRY_VERIFY_WITH_TIMEOUT(!player.screenshotPending_,20000);const QDir images(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));const auto name=QFileInfo(source).completeBaseName();QCOMPARE(images.entryList({name+"*.png"},QDir::Files).size(),2);
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
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu && menu->isVisible());QCOMPARE(menu->actions()[0]->text(),QStringLiteral("打开文件…"));QCOMPARE(menu->actions()[1]->text(),QStringLiteral("打开文件夹…"));bool hasLink=false;for(const auto *action:menu->actions())hasLink|=action->text()==QStringLiteral("打开链接…");QVERIFY(hasLink);menu->close();
        QVERIFY(player.openFolder(dir.path()));auto *list=player.findChild<QTreeWidget *>("playerPlaylist");QVERIFY(list);QCOMPARE(list->topLevelItemCount(),2);auto *folder=list->topLevelItem(0);QVERIFY(folder->data(0,Qt::UserRole+1).toBool());folder->setExpanded(true);QTRY_COMPARE(folder->childCount(),1);QCOMPARE(folder->child(0)->text(0),QString("two.mkv"));
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
        const int width=player.width();pin->trigger();menu->close();QTRY_VERIFY(panel->isVisible());QVERIFY(player.width()>width);QCOMPARE(panel->parentWidget(),player.videoSplit_);
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
            QCOMPARE(engine->currentData().toInt(),2);QVERIFY(tone->isEnabled());engine->setCurrentIndex(engine->findData(0));QVERIFY(!tone->isEnabled());engine->setCurrentIndex(engine->findData(1));QVERIFY(tone->isEnabled());icc->setCurrentIndex(0);
            auto *buttons=dialog->findChild<QDialogButtonBox *>();QCOMPARE(buttons->button(QDialogButtonBox::Ok)->text(),QStringLiteral("确定"));QCOMPARE(buttons->button(QDialogButtonBox::Cancel)->text(),QStringLiteral("取消"));QCOMPARE(buttons->button(QDialogButtonBox::Apply)->text(),QStringLiteral("应用"));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            QTest::qWait(500);qInfo()<<"color engine"<<player.output_->colorStatus().engine<<"fallback"<<player.output_->colorStatus().fallback;
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
        bool edited=false;QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");if(!dialog)return;auto *categories=dialog->findChild<QListWidget *>();if(!categories || categories->count()!=9){dialog->reject();return;}
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
    void lyricCarriageKit() {
        const auto root=qEnvironmentVariable("VSR_LYRIC_KIT");if(root.isEmpty())QSKIP("Set VSR_LYRIC_KIT to the extracted A-P kit");
        const QDir directory(root);int count=0;
        for(const auto &file:directory.entryInfoList(QDir::Files,QDir::Name))if(QRegularExpression("^[A-P]_").match(file.fileName()).hasMatch() && file.suffix()!="lrc"){
            const auto result=PlayerAudioMetadata::read(file.absoluteFilePath());QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY2(result.audioOnly,qPrintable(file.fileName()));QCOMPARE(result.lyrics.size(),12);QCOMPARE(result.lyrics.first().start,5000000LL);QCOMPARE(result.lyrics.last().start,390000000LL);QVERIFY(result.lyrics.first().text.contains("第01行"));QVERIFY(result.lyrics.last().text.contains("第12行"));++count;
            qInfo()<<file.fileName()<<result.lyricSource<<result.lyrics.size();
        }QCOMPARE(count,16);
        const auto lines=PlayerAudioMetadata::parseLyrics(QString(QChar(0xfeff))+"[offset:100]\n[00:00.50][00:02.00]First\n[00:04.000]Last",50000000);QCOMPARE(lines.size(),3);QCOMPARE(lines.first().start,6000000LL);QCOMPARE(lines.last().end,50000000LL);
    }
    void audioCoverAndLiveLyrics() {
        const auto root=qEnvironmentVariable("VSR_LYRIC_KIT");if(root.isEmpty())QSKIP("Set VSR_LYRIC_KIT to the extracted A-P kit");
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("player/preset",QString());settings.setValue("subtitle/visible",true);settings.sync();
        PlayerWindow player;player.show();int count=0;
        for(const auto &file:QDir(root).entryInfoList(QDir::Files,QDir::Name))if(QRegularExpression("^[A-P]_").match(file.fileName()).hasMatch() && file.suffix()!="lrc"){
            QVERIFY(player.openFile(file.absoluteFilePath()));QTRY_COMPARE_WITH_TIMEOUT(player.source_,file.absoluteFilePath(),15000);QTRY_VERIFY_WITH_TIMEOUT(player.pendingMediaOpen_.isEmpty(),15000);QTRY_COMPARE_WITH_TIMEOUT(player.property("lyricCount").toInt(),12,15000);qInfo()<<"Lyric kit"<<file.fileName()<<player.property("lyricSource").toString();QVERIFY(player.property("audioOnly").toBool());player.seekTime(60000000);auto *lyrics=player.findChild<QLabel *>("playerLyrics");QTRY_VERIFY_WITH_TIMEOUT(lyrics->text().contains("第02行"),5000);QVERIFY(lyrics->isVisible());QVERIFY(lyrics->testAttribute(Qt::WA_NativeWindow));QVERIFY(lyrics->geometry().bottom()<player.pane_->surface()->mapFromGlobal(player.controls_->mapToGlobal(QPoint())).y());QVERIFY(!player.media_.value("streams").toArray().isEmpty());QVERIFY(player.externalAudio_.isEmpty());++count;
        }QCOMPARE(count,16);player.grab().save("build/audio-lyrics-1.0.6.png");
        const QString aeg="C:/Users/ARXChem/Music/Kugou/Aegleseeker.flac";if(!QFileInfo::exists(aeg))return;
        QVERIFY(player.openFile(aeg));QTRY_VERIFY_WITH_TIMEOUT(!player.audioMetadata_.cover.isNull(),15000);QVERIFY(player.property("audioOnly").toBool());QVERIFY(player.externalAudio_.isEmpty());
        player.togglePlayback();QTRY_VERIFY(player.position()>5000000);QTRY_VERIFY(player.snapshot().decodedAudioFrames>0);QCOMPARE(player.snapshot().decodedVideoFrames,quint64(0));player.togglePlayback();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());QTest::keyClick(&player,Qt::Key_Tab);auto *panel=static_cast<PlayerInfoPanel *>(player.findChild<QLabel *>("playerInfoPanel"));QTRY_VERIFY(panel->text().contains("无动态视频"));QVERIFY(panel->text().contains("静态图片"));panel->grab().save("build/audio-cover-tab-1.0.6.png");player.grab().save("build/audio-cover-1.0.6.png");
    }
    void fullBluRayPlaylist() {
        const auto source=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE to the complete disc");
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openBluRay(source));auto *list=player.findChild<QTreeWidget *>("playerPlaylist");
        QTreeWidgetItem *audio=nullptr;QTRY_VERIFY_WITH_TIMEOUT(([&]{for(int i=0;i<list->topLevelItemCount();++i)if(list->topLevelItem(i)->text(0)=="音频")audio=list->topLevelItem(i);return audio!=nullptr;})(),15000);
        QCOMPARE(audio->childCount(),29);QVERIFY(!audio->isExpanded());QVERIFY(list->topLevelItemCount()>6);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().duration100ns>0 && player.snapshot().state!=ThreeFpState::Opening,20000);player.togglePlayback();QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>5,20000);player.togglePlayback();audio->setExpanded(true);player.grab().save("build/bd-full-playlist-1.0.6.png");
        const auto file=audio->child(0)->data(0,Qt::UserRole).toString(),nextAudioFile=audio->child(1)->data(0,Qt::UserRole).toString();qInfo()<<"BD audio"<<file<<QFileInfo(file).size();QVERIFY2(player.openFile(file),qPrintable(player.clock_->lastError()));QTRY_VERIFY_WITH_TIMEOUT(player.property("audioOnly").toBool() && player.snapshot().duration100ns>0,15000);player.nextFile(-1);QCOMPARE(QFileInfo(player.source_).suffix(),QString("mpls"));player.nextFile(1);QCOMPARE(player.source_,file);player.nextFile(1);QCOMPARE(player.source_,nextAudioFile);
    }
    void mksSubtitlesAndChapters() {
        const auto root=qEnvironmentVariable("VSR_LYRIC_KIT");if(root.isEmpty())QSKIP("Set VSR_LYRIC_KIT to the extracted kit");QTemporaryDir temp;
        const auto chapters=temp.filePath("chapters.xml"),mks=temp.filePath("captions.mks"),video=temp.filePath("video.mp4");QFile file(chapters);QVERIFY(file.open(QIODevice::WriteOnly));file.write("<?xml version=\"1.0\"?><Chapters><EditionEntry><ChapterAtom><ChapterTimeStart>00:00:00.000</ChapterTimeStart><ChapterDisplay><ChapterString>First</ChapterString></ChapterDisplay></ChapterAtom><ChapterAtom><ChapterTimeStart>00:00:04.000</ChapterTimeStart><ChapterDisplay><ChapterString>Second</ChapterString></ChapterDisplay></ChapterAtom></EditionEntry></Chapters>");file.close();
        QProcess merge;merge.start(BlurayCatalog::mkvmerge(),{"-o",mks,"--chapters",chapters,"--no-audio","--no-video",QDir(root).filePath("H_mka_tags_plus_ass.mka")});QVERIFY(merge.waitForFinished(10000));QCOMPARE(merge.exitCode(),0);
        QProcess ffmpeg;ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","color=black:size=640x360:rate=24:duration=8","-c:v","libx264","-preset","ultrafast","-y",video});QVERIFY(ffmpeg.waitForFinished(10000));QCOMPARE(ffmpeg.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/preset",QString());settings.setValue("subtitle/visible",true);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(video));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);QVERIFY(player.openFile(mks));QTRY_COMPARE_WITH_TIMEOUT(player.externalChapters_.size(),2,5000);QCOMPARE(player.externalSubtitleTracks_[0].size(),1);QCOMPARE(player.primarySubtitle_,-2);QCOMPARE(player.timeline_->property("chapters").toJsonArray().at(1).toObject().value("start100ns").toDouble(),40000000.);
        player.togglePlayback();player.showContextMenu(player.mapToGlobal(QPoint(30,30)));auto *menu=player.findChild<QMenu *>("playerContextMenu");QAction *second=nullptr;for(auto *action:menu->findChildren<QAction *>())if(action->text().startsWith("Second ·"))second=action;QVERIFY(second);second->trigger();menu->close();QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.position()-40000000)<50000,5000);
        QTest::qWait(500);const auto at=player.pane_->surface()->mapToGlobal(QPoint());const auto image=player.screen()->grabWindow(0,at.x(),at.y(),player.pane_->surface()->width(),player.pane_->surface()->height()).toImage();QVERIFY(!image.isNull());int bright=0;for(int y=image.height()/2;y<image.height();++y)for(int x=0;x<image.width();++x)if(qRed(image.pixel(x,y))>100)++bright;QVERIFY2(bright>20,"External MKS ASS was not rendered");
    }
    void nativeD3D11Output() {
        const QString source="D:/Animation Enhance/GBC 108048/Girls.Band.Cry.02.AV1.FLAC.1080p48F.mkv";if(!QFileInfo::exists(source))QSKIP("Local AV1 file unavailable");QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("render/upscale",7);settings.setValue("render/downscale",7);settings.sync();PlayerWindow player;player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Realistic.vpy"));QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>10,20000);qInfo()<<"D3D11 native scaling mode"<<player.outputSnapshot().videoScalingMode;QCOMPARE(player.outputSnapshot().videoScalingMode,0u);player.togglePlayback();
        ThreeFpApi api;QWidget surface;surface.resize(960,540);surface.show();ThreeFpPlayer native(api,&surface);native.setDecodeMode(2);QVERIFY(native.setScalingAlgorithms(ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native));QVERIFY(native.openFile(source));QTRY_COMPARE_WITH_TIMEOUT(native.snapshot().state,ThreeFpState::Ready,15000);QVERIFY(native.seek(600000000));QTest::qWait(700);const auto image=native.capture();QVERIFY(!image.isNull());QSet<QRgb> colors;for(int y=30;y<image.height()-30;y+=30)for(int x=30;x<image.width()-30;x+=30)colors.insert(image.pixel(x,y));QVERIFY(colors.size()>8);QCOMPARE(native.snapshot().videoScalingMode,0u);
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
