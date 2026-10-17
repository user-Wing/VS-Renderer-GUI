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
#include <windows.h>
using namespace vsr;
class TestPlayer final : public QObject {
    Q_OBJECT
    QByteArray previous_;bool existed_=false;
    QString config() const {return QDir(QCoreApplication::applicationDirPath()).filePath("player.ini");}
private slots:
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
