#include "player/PlayerWindow.h"
#include "player/PlayerAudioMetadata.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "player/PlayerDiscMenu.h"
#include "graph/PresetStore.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QSlider>
#include <QLabel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <QMenu>
#include <QListWidget>
#include "ui/PreviewPane.h"
#include <windows.h>
using namespace vsr;
class TestPlayer final : public QObject {
    Q_OBJECT
    QByteArray previous_;bool existed_=false;
    QString config() const {return QDir(QCoreApplication::applicationDirPath()).filePath("player.ini");}
private slots:
    void zoomOutClearsOutsideVideo_data(){QTest::addColumn<int>("stage");for(int stage=0;stage<8;++stage)QTest::newRow(qPrintable(QString::number(stage)))<<stage;}
    void zoomOutClearsOutsideVideo(){
        QFETCH(int,stage);QTemporaryDir directory;const auto file=directory.filePath("solid.mkv");QProcess process;
        process.start("ffmpeg",{"-v","error","-f","lavfi","-i","color=c=0xff8040:size=320x180:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(process.waitForFinished(10000));QCOMPARE(process.exitCode(),0);
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("decode/mode",1);settings.setValue("player/muted",true);settings.sync();
        const QStringList files{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy"};
        PlayerWindow player;player.resize(1100,760);player.show();player.loadPreset(files[stage]);QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(200);
        const auto size=player.pane_->surface()->size()*player.pane_->surface()->devicePixelRatioF();
        for(float zoom:{1.f,.5f,.4f,.25f,.4f,1.f,.25f,.5f}){
            player.pane_->adoptView(zoom,0,0);player.pane_->viewChanged(zoom,0,0);player.clock_->redraw();
            ThreeFpPixelProbe center{};QVERIFY(player.clock_->samplePixel(size.width()/2,size.height()/2,center));QVERIFY(center.red>0.5f);
            ThreeFpPixelProbe outside{};QVERIFY(player.clock_->samplePixel(size.width()/10,size.height()/2,outside));
            if(zoom<.8f)QVERIFY2(std::max({outside.red,outside.green,outside.blue})<.02f,qPrintable(QString("zoom=%1: outside video retained RGB %2,%3,%4").arg(zoom).arg(outside.red).arg(outside.green).arg(outside.blue)));
            else QVERIFY(outside.red>0.5f);
        }
    }
    void realVideoFrameStepping(){
        const auto source=qEnvironmentVariable("VSR_FRAME_STEP_SOURCE");if(source.isEmpty())QSKIP("Set VSR_FRAME_STEP_SOURCE to the reported long video");
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("player/muted",true);settings.sync();
        for(uint mode:{1u,2u}){settings.setValue("decode/mode",mode);settings.sync();PlayerWindow player;player.show();player.loadPreset("Anime-5-D3D11.vpy");QVERIFY(player.openFile(source));QCOMPARE(player.source_,QFileInfo(source).absoluteFilePath());QTRY_VERIFY2_WITH_TIMEOUT(player.ready_ && player.positionRestored_,qPrintable(QString("ready=%1 restored=%2 state=%3 error=%4").arg(player.ready_).arg(player.positionRestored_).arg(int(player.snapshot().state)).arg(player.clock_->lastError())),15000);QVERIFY(player.direct_);
            for(int stage:{4,5}){player.loadPreset(stage==4?"Anime-4-Jinc.vpy":"Anime-5-D3D11.vpy");QTRY_VERIFY_WITH_TIMEOUT(player.ready_,10000);player.seekTime(6000000000LL);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().position100ns>=6000000000LL,10000);
                for(const QString text:{QStringLiteral("›|"),QStringLiteral("|‹")}){const auto before=player.outputSnapshot();QPushButton *button=nullptr;for(auto *b:player.findChildren<QPushButton *>())if(b->text()==text)button=b;QVERIFY(button);button->click();QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().framePts!=before.framePts && player.outputSnapshot().frameIndex>=0,5000);QVERIFY(qAbs(qAbs(player.outputSnapshot().position100ns-before.position100ns)-208542)<20000);}
                const auto at=player.outputSnapshot().position100ns;player.togglePlayback();QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && player.outputSnapshot().position100ns>at+5000000,5000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
            }QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));
        }
    }
    void nativeDiscMenuControls(){
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");
        PlayerWindow player;player.resize(1100,760);player.show();QVERIFY(player.openBluRay(root,true));
        QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->active() && player.discMenu_->programme().value("menu").toBool() && player.discMenu_->presentedFrames()>0,30000);
        player.updateState();QVERIFY(!player.timeline_->isEnabled());QCOMPARE(player.timeline_->value(),0);
        const auto time=player.time_->text();QTest::qWait(1100);QCOMPARE(player.time_->text(),time);
        QVERIFY(player.timeline_->property("chapters").toJsonArray().isEmpty());
        const auto playEpisode=[&]{const auto surface=player.discSurface_->size(),video=QSize(960,540).scaled(surface,Qt::KeepAspectRatio);player.discMenu_->pointer(QPoint((surface.width()-video.width())/2+video.width()*575/960,(surface.height()-video.height())/2+video.height()*99/540),surface,true);};
        playEpisode();QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->programme().value("menu").toBool() && player.discMenu_->position()>10000000,15000);
        player.updateState();QVERIFY(player.timeline_->isEnabled());QVERIFY(!player.timeline_->property("chapters").toJsonArray().isEmpty());
        QTreeWidgetItem *graphics=nullptr;for(int i=0;i<player.playlist_->topLevelItemCount();++i){auto *item=player.playlist_->topLevelItem(i);if(player.blurayGroups_.value(item->data(0,Qt::UserRole).toString())=="menu"){graphics=item;break;}}
        QVERIFY(graphics);QVERIFY(graphics->flags().testFlag(Qt::ItemIsEnabled));
        player.playlist_->itemDoubleClicked(graphics,0);
        QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->programme().value("menu").toBool(),15000);
        playEpisode();QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->programme().value("menu").toBool() && player.discMenu_->position()>10000000,15000);
        player.showContextMenu(player.mapToGlobal(QPoint(80,80)));auto *menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        QAction *back=nullptr;for(auto *action:menu->actions()){QVERIFY(action->text()!=QStringLiteral("BD 原生节目菜单"));if(action->text()==QStringLiteral("BD 主菜单 / 弹出菜单"))back=action;}
        QVERIFY(back);back->trigger();menu->close();
        QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->programme().value("menu").toBool(),15000);
        player.updateState();QVERIFY(!player.timeline_->isEnabled());
        QTemporaryDir directory;const auto file=directory.filePath("normal.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",file));
        QVERIFY(player.openFile(file));QVERIFY(player.timeline_->isEnabled());QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);
    }
    void nativeDiscReopenWhilePlaying(){
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");
        PlayerWindow player;player.resize(1100,760);player.show();
        for(int cycle=0;cycle<12;++cycle){
            qInfo()<<"open menu cycle"<<cycle;QVERIFY(player.openBluRay(root,true));
            if(cycle%2)QVERIFY(player.openBluRay(root,true));
            QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_ && player.discMenu_->active() && player.discMenu_->programme().value("menu").toBool(),30000);
            const auto surface=player.discSurface_->size(),video=QSize(960,540).scaled(surface,Qt::KeepAspectRatio);
            player.discMenu_->pointer(QPoint((surface.width()-video.width())/2+video.width()*575/960,(surface.height()-video.height())/2+video.height()*99/540),surface,true);
            QTRY_VERIFY_WITH_TIMEOUT(!player.discMenu_->programme().value("menu").toBool() && player.discMenu_->position()>10000000,15000);
            qInfo()<<"programme playing"<<cycle;
        }
        QString continuous;for(const auto &file:player.files_)if(player.blurayLabels_.value(file).startsWith(QStringLiteral("整卷连续播放")))continuous=file;
        QVERIFY(!continuous.isEmpty());QVERIFY(player.openFile(continuous));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>3,30000);
        qInfo()<<"reopen from continuous playlist";QVERIFY(player.openBluRay(root,true));
        QTRY_VERIFY_WITH_TIMEOUT(player.discMenu_->active() && player.discMenu_->programme().value("menu").toBool(),30000);
    }
    void nativeDiscFirstPlay(){
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");
        QWidget surface;surface.resize(960,540);surface.show();PlayerDiscMenu menu;
        QVERIFY2(menu.open(root,reinterpret_cast<void*>(surface.winId())),qPrintable(menu.error()));
        QTRY_VERIFY2_WITH_TIMEOUT((menu.state(),menu.presentedFrames()>0),qPrintable(menu.error()),20000);
        qInfo()<<"authored FirstPlay"<<menu.programme()<<menu.position();
        QTRY_VERIFY2_WITH_TIMEOUT((menu.state(),menu.programme().value("menu").toBool()),qPrintable(QString("state=%1 position=%2 status=%3 error=%4").arg(menu.state()).arg(menu.position()).arg(QString::fromUtf8(QJsonDocument(menu.programme()).toJson(QJsonDocument::Compact))).arg(menu.error())),10000);
        const auto chapters=menu.programme().value("chapters").toArray();QVERIFY(!chapters.isEmpty());
        const int soak=qEnvironmentVariableIntValue("VSR_BD_SOAK_SECONDS");
        for(int second=0;second<soak;++second){QTest::qWait(1000);QCOMPARE(menu.state(),3);QVERIFY2(menu.error().isEmpty(),qPrintable(menu.error()));if(second%30==0)qInfo()<<"menu soak seconds"<<second<<"presents"<<menu.presentedFrames();}
        menu.pointer(QPoint(575,99),surface.size(),true);
        QTRY_VERIFY2_WITH_TIMEOUT((menu.state(),!menu.programme().value("menu").toBool()),qPrintable(menu.error()),10000);
        QTRY_VERIFY_WITH_TIMEOUT(menu.position()>10000000,10000);
        qint64 previous=-1;for(const auto &chapter:menu.programme().value("chapters").toArray()){
            const auto start=qint64(chapter.toDouble());QVERIFY(start>=previous && start<=menu.duration());previous=start;
        }
    }
    void nativeMenuRapidResume(){
        QTemporaryDir directory;const auto file=directory.filePath("sample.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",file));
        QWidget surface;surface.resize(640,360);surface.show();PlayerDiscMenu menu;
        QVERIFY(menu.open(file,reinterpret_cast<void*>(surface.winId())));QTRY_COMPARE_WITH_TIMEOUT(menu.state(),3,15000);
        menu.pause(true);menu.pause(false);QTest::qWait(100);QTRY_COMPARE_WITH_TIMEOUT(menu.state(),3,5000);
        const auto target=menu.duration()/2;QVERIFY(target>0);menu.seek(target);
        QTRY_VERIFY_WITH_TIMEOUT(menu.position()>=target,5000);QTRY_VERIFY(menu.presentedFrames()>0);
    }
    void nativeDiscSeekKeepsRequestedPosition(){
        const auto root=qEnvironmentVariable("VSR_FULL_BD_SOURCE");if(root.isEmpty())QSKIP("Set VSR_FULL_BD_SOURCE");
        QWidget surface;surface.resize(960,540);surface.show();PlayerDiscMenu menu;
        QVERIFY(menu.open(root,reinterpret_cast<void*>(surface.winId())));
        QTRY_VERIFY_WITH_TIMEOUT((menu.state(),menu.programme().value("menu").toBool()),20000);
        menu.pointer(QPoint(575,99),surface.size(),true);
        QTRY_VERIFY_WITH_TIMEOUT((menu.state(),!menu.programme().value("menu").toBool() && menu.position()>10000000),15000);
        QVERIFY(menu.duration()>36000000000LL);
        for(const qint64 target:{menu.duration()*3/4,menu.duration()/4}){
            const auto presents=menu.presentedFrames();menu.seek(target);QCOMPARE(menu.position(),target);
            QElapsedTimer elapsed;elapsed.start();
            while(elapsed.elapsed()<1500){QTest::qWait(10);QVERIFY2(qAbs(menu.position()-target)<30000000,"BD timeline bounced to the pre-seek clock");}
            QTRY_VERIFY_WITH_TIMEOUT(menu.presentedFrames()>presents,15000);
            QTRY_VERIFY_WITH_TIMEOUT(menu.position()>target+1000000,15000);
        }
    }
    void initTestCase(){QCoreApplication::setOrganizationName("VSRendererLiteTests");QCoreApplication::setApplicationName("VSPlayerLiteTests");QFile file(config());existed_=file.exists();if(file.open(QIODevice::ReadOnly))previous_=file.readAll();}
    void init(){QSettings settings(config(),QSettings::IniFormat);settings.clear();settings.setValue("playback/remember",false);settings.setValue("basic/autoplay",true);settings.sync();}
    void audioControlsPopup(){
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("player/volume",63);settings.setValue("player/muted",true);settings.sync();PlayerWindow player;player.show();player.activateWindow();QTRY_VERIFY(player.isActiveWindow());
        auto *percent=player.findChild<QLabel *>("playerVolumePercent");auto *button=player.findChild<QPushButton *>("playerEqualizerButton");QVERIFY(percent && button);QCOMPARE(percent->text(),QString("63%"));const auto muted=player.mute_->icon().pixmap(24,24).toImage();QVERIFY(!muted.isNull());player.mute_->click();QVERIFY(!player.mute_->isChecked());QVERIFY(player.mute_->icon().pixmap(24,24).toImage()!=muted);
        button->click();QTRY_VERIFY(player.equalizerPopup_ && player.equalizerPopup_->isVisible());QVERIFY(!qobject_cast<QDialog *>(player.equalizerPopup_.data()));QCOMPARE(player.equalizerPopup_->windowType(),Qt::Popup);player.equalizerPopup_->findChild<QSlider *>("equalizerMaster")->setValue(27);QCOMPARE(percent->text(),QString("27%"));button->click();QVERIFY(!player.equalizerPopup_ || !player.equalizerPopup_->isVisible());
    }
    void allowedModes(){
        PlayerWindow player;player.show();QCOMPARE(player.qualityStage(),5);
        player.loadPreset("Anime-4-Jinc.vpy");QCOMPARE(player.qualityStage(),4);
        player.loadPreset("Interpolation-0-RIFE.vpy");QCOMPARE(player.qualityStage(),4);
        player.loadPreset("Anime-0-CNN-Enhanced.vpy");QCOMPARE(player.qualityStage(),0);
        player.loadPreset("Anime-5-D3D11.vpy");QCOMPARE(player.qualityStage(),5);
        QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));QVERIFY(!GetModuleHandleW(L"libvlc.dll"));
    }
    void nativeAnimePresets_data(){QTest::addColumn<int>("stage");for(int stage=0;stage<8;++stage)QTest::newRow(qPrintable(QString::number(stage)))<<stage;}
    void nativeAnimePresets(){
        QFETCH(int,stage);QTemporaryDir directory;auto file=qEnvironmentVariable("VSR_ANIME4K_SOURCE");
        if(file.isEmpty()){file=directory.filePath("pattern.mkv");QProcess process;process.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(process.waitForFinished(10000));QCOMPARE(process.exitCode(),0);}
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("decode/mode",1);settings.setValue("player/muted",true);settings.sync();
        PlayerWindow player;player.resize(1100,760);player.show();QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QTest::qWait(200);const auto pts=player.outputSnapshot().framePts;
        const auto original=player.clock_->capture();QVERIFY(!original.isNull());
        player.showContextMenu(player.mapToGlobal(QPoint(80,80)));auto* menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        auto* action=menu->findChild<QAction*>(QString("playerAnimeStage%1").arg(stage));QVERIFY(action);QVERIFY(action->isEnabled());action->trigger();menu->close();
        QCOMPARE(player.qualityStage(),stage);const int algorithms[]{10,9,11,12,4,7,13,14};QCOMPARE(player.scalingAlgorithm(),algorithms[stage]);
        const auto filtered=player.clock_->capture();QVERIFY(!filtered.isNull());QCOMPARE(filtered.size(),original.size());QCOMPARE(player.outputSnapshot().framePts,pts);
        QVERIFY2(player.clock_->lastError().isEmpty(),qPrintable(player.clock_->lastError()));
        if(stage!=5)QVERIFY2(filtered!=original,"The selected filter must change rendered pixels");
        const QStringList files{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy"};
        if(stage==0 || stage==2 || stage==6){player.loadPreset(files[stage==6?7:stage+1]);const auto other=player.clock_->capture();QVERIFY(!other.isNull());QVERIFY2(filtered!=other,"Enhanced/1X renderer variants must produce distinct output");player.loadPreset(files[stage]);}
        const auto evidence=qEnvironmentVariable("VSR_ANIME4K_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);QVERIFY(filtered.save(QDir(evidence).filePath(QString("stage-%1.png").arg(stage))));}
        QVERIFY(player.direct_);QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));
        const auto before=player.outputSnapshot().position100ns;player.togglePlayback();QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().position100ns>before+2000000,5000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        player.seekTime(15000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().position100ns>=15000000,5000);QVERIFY(!player.clock_->capture().isNull());
        PlayerWindow reopened;reopened.show();QVERIFY(reopened.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(reopened.outputSnapshot().presentedVideoFrames>2,15000);QCOMPARE(reopened.qualityStage(),stage);QCOMPARE(reopened.scalingAlgorithm(),algorithms[stage]);
    }
    void anime4kFastMenuAndPixels_data(){QTest::addColumn<int>("decode");QTest::newRow("software")<<1;QTest::newRow("hardware")<<2;}
    void anime4kFastMenuAndPixels(){
        QFETCH(int,decode);QTemporaryDir directory;auto file=qEnvironmentVariable("VSR_ANIME4K_SOURCE");
        if(file.isEmpty()){file=directory.filePath("pattern.mkv");QProcess process;process.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(process.waitForFinished(10000));QCOMPARE(process.exitCode(),0);}
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("decode/mode",decode);settings.setValue("player/muted",true);settings.sync();
        PlayerWindow player;player.resize(1100,760);player.show();QVERIFY(player.openFile(file));
        QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
        const auto original=player.clock_->capture();QVERIFY(!original.isNull());
        player.showContextMenu(player.mapToGlobal(QPoint(80,80)));
        auto *menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        auto *action=menu->findChild<QAction*>("playerAnimeStage1");QVERIFY(action);QVERIFY(action->isEnabled());
        action->trigger();menu->close();QCOMPARE(player.scalingAlgorithm(),9);
        QVERIFY2(player.clock_->lastError().isEmpty(),qPrintable(player.clock_->lastError()));
        const auto anime=player.clock_->capture();QVERIFY(!anime.isNull());QCOMPARE(anime.size(),original.size());
        QVERIFY2(anime!=original,"Anime4K must change the rendered pixels");
        QVERIFY(player.direct_);QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));
        player.updateAlgorithmBadges();QCOMPARE(player.algorithmBadge_->text(),QString("A4KCNN"));
        player.infoVisible_=true;player.updateInfo();QVERIFY(player.info_->text().contains(player.qualityNames().at(1)));
        const auto before=player.outputSnapshot().position100ns;player.togglePlayback();QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().position100ns>before+5000000,5000);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        player.seekTime(15000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.outputSnapshot().position100ns>=15000000,5000);
        QVERIFY(!player.clock_->capture().isNull());QCOMPARE(player.scalingAlgorithm(),9);
        const auto evidence=qEnvironmentVariable("VSR_ANIME4K_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);QVERIFY(original.save(QDir(evidence).filePath("d3d11.png")));QVERIFY(anime.save(QDir(evidence).filePath("anime4k-fast.png")));}
        settings.sync();QCOMPARE(settings.value("player/animeStage").toInt(),1);
        player.loadPreset("Anime-4-Jinc.vpy");QCOMPARE(player.scalingAlgorithm(),4);
        player.loadPreset("Anime-5-D3D11.vpy");QCOMPARE(player.scalingAlgorithm(),7);
        settings.setValue("player/animeStage",1);settings.sync();PlayerWindow reopened;reopened.show();QVERIFY(reopened.openFile(file));
        QTRY_VERIFY_WITH_TIMEOUT(reopened.outputSnapshot().presentedVideoFrames>2,15000);QCOMPARE(reopened.scalingAlgorithm(),9);
    }
    void settingsRejectFullBackends(){
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("player/renderer","madVR");settings.setValue("decode/video","LAV");settings.setValue("decode/audio","LAV");settings.setValue("color/engine",1);settings.sync();
        PlayerWindow player;player.show();QCOMPARE(player.settings_->value("decode/video").toString(),QString("3FP"));QCOMPARE(player.settings_->value("decode/audio").toString(),QString("3FP"));QCOMPARE(player.settings_->value("color/engine").toInt(),0);
        bool checked=false;QTimer::singleShot(100,&player,[&]{auto* dialog=player.findChild<QDialog*>("playerSettings");QVERIFY(dialog);auto* video=dialog->findChild<QComboBox*>("playerVideoDecoder");auto* audio=dialog->findChild<QComboBox*>("playerAudioDecoder");QVERIFY(video && audio);QCOMPARE(video->count(),1);QCOMPARE(audio->count(),1);checked=true;dialog->reject();});player.showSettings();QVERIFY(checked);
    }
    void nativeAnimeSettingsAndMigration(){
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("render/anime4kFast",true);settings.setValue("player/animeStages",QStringList{"0","1","5"});settings.sync();
        {PlayerWindow player;player.show();QCOMPARE(player.qualityStage(),1);QVERIFY(!player.settings_->contains("render/anime4kFast"));
            QTimer::singleShot(100,&player,[&]{auto* dialog=player.findChild<QDialog*>("playerSettings");QVERIFY(dialog);auto* list=dialog->findChild<QListWidget*>("playerAnimeStages");QVERIFY(list);QCOMPARE(list->count(),8);QVERIFY(!list->isHidden());
                for(int i=0;i<list->count();++i){QVERIFY(!(list->item(i)->flags() & Qt::ItemIsUserCheckable));if(list->item(i)->data(Qt::UserRole).toInt()==2)list->setCurrentItem(list->item(i));}
                dialog->findChild<QPushButton*>("playerConfirmSettings")->click();});player.showSettings();QCOMPARE(player.settings_->value("player/animeStage").toInt(),2);QCOMPARE(player.settings_->value("player/animeStages").toStringList(),QStringList({"0","1","5"}));}
        PlayerWindow reopened;QCOMPARE(reopened.qualityStage(),2);
    }
    void ffprobeFromRuntimeOrPath(){
        QTemporaryDir directory;const auto file=directory.filePath("probe.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",file));
        const auto info=PlayerAudioMetadata::probe(file);QVERIFY(!info.value("streams").toArray().isEmpty());
    }
    void softwarePlaybackWithoutVs(){
        QTemporaryDir directory;const auto file=directory.filePath("sample.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",file));
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("decode/mode",1);settings.sync();
        PlayerWindow player;player.show();QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.outputSnapshot().presentedVideoFrames>2,15000);QVERIFY(player.direct_);QCOMPARE(player.qualityStage(),5);QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);QVERIFY(!player.clock_->capture().isNull());
        player.loadPreset("Anime-4-Jinc.vpy");QCOMPARE(player.qualityStage(),4);QVERIFY(player.direct_);QVERIFY(!player.server_->available());
    }
    void cleanupTestCase(){QFile file(config());if(existed_){QVERIFY(file.open(QIODevice::WriteOnly));file.write(previous_);}else file.remove();QSettings().clear();}
};
QTEST_MAIN(TestPlayer)
#include "TestPlayerLite.moc"
