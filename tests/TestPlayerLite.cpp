#include "player/PlayerWindow.h"
#include "player/PlayerAudioMetadata.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "player/PlayerDiscMenu.h"
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
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <windows.h>
using namespace vsr;
class TestPlayer final : public QObject {
    Q_OBJECT
    QByteArray previous_;bool existed_=false;
    QString config() const {return QDir(QCoreApplication::applicationDirPath()).filePath("player.ini");}
private slots:
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
    void allowedModes(){
        PlayerWindow player;player.show();QCOMPARE(player.qualityStage(),5);
        player.loadPreset("Anime-4-Jinc.vpy");QCOMPARE(player.qualityStage(),4);
        player.loadPreset("Interpolation-0-RIFE.vpy");QCOMPARE(player.qualityStage(),4);
        player.loadPreset("Anime-0-CNN-Enhanced.vpy");QCOMPARE(player.qualityStage(),4);
        player.loadPreset("Anime-5-D3D11.vpy");QCOMPARE(player.qualityStage(),5);
        QVERIFY(!player.server_->available());QVERIFY(!GetModuleHandleW(L"VapourSynth.dll"));QVERIFY(!GetModuleHandleW(L"libvlc.dll"));
    }
    void settingsRejectFullBackends(){
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("player/renderer","madVR");settings.setValue("decode/video","LAV");settings.setValue("decode/audio","LAV");settings.setValue("color/engine",1);settings.sync();
        PlayerWindow player;player.show();QCOMPARE(player.settings_->value("decode/video").toString(),QString("3FP"));QCOMPARE(player.settings_->value("decode/audio").toString(),QString("3FP"));QCOMPARE(player.settings_->value("color/engine").toInt(),0);
        bool checked=false;QTimer::singleShot(100,&player,[&]{auto* dialog=player.findChild<QDialog*>("playerSettings");QVERIFY(dialog);auto* video=dialog->findChild<QComboBox*>("playerVideoDecoder");auto* audio=dialog->findChild<QComboBox*>("playerAudioDecoder");QVERIFY(video && audio);QCOMPARE(video->count(),1);QCOMPARE(audio->count(),1);checked=true;dialog->reject();});player.showSettings();QVERIFY(checked);
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
