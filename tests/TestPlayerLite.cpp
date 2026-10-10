#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
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
