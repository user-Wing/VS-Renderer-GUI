#include "player/PlayerWindow.h"
#include "render/ShaderPackage.h"
#include <QDateTime>
#include <QColorSpace>
#include "ui/PageTheme.h"
#include "player/PlayerImageTools.h"
#include <QToolButton>
#include "image/ImageJxl.h"
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
    void pageThemeVisible(){QTemporaryDir directory;QImage background(16,16,QImage::Format_RGB32);background.fill(Qt::green);const auto path=directory.filePath("green.png"),ini=directory.filePath("themes.ini");QVERIFY(background.save(path));QSettings settings(ini,QSettings::IniFormat);settings.setValue("VSP-Main/image",path);settings.setValue("VSP-Main/transparency",0);settings.setValue("VSP-Main/brightness",100);settings.sync();QLabel label("Text");label.setStyleSheet("background:#101010;color:white;");label.resize(240,100);applyPageTheme(&label,ini,"VSP-Main");label.show();QTest::qWait(50);QCOMPARE(label.grab().toImage().pixelColor(4,4),QColor(Qt::green));}

    void ffmpegImageExportDepthAndResize(){
        QTemporaryDir dir;ImageOutput output;output.source=dir.filePath("source.png");output.image=QImage(32,24,QImage::Format_RGBA64);output.image.fill(QColor(80,140,210,180));QVERIFY(output.image.save(output.source));output.size=QSize(48,36);
        for(const auto &algorithm:QStringList{"hlsl:jinc","lanczos3"})for(int bits:{8,16}){output.resizeAlgorithm=algorithm;output.format="png";output.bitDepth=QString::number(bits);output.destination=dir.filePath(QString("%1-%2.png").arg(algorithm.startsWith("hlsl")?"hlsl":"ffmpeg").arg(bits));const auto error=PlayerImageTools::writeOutput(output);QVERIFY2(error.isEmpty(),qPrintable(error));QFile file(output.destination);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(int(uchar(file.read(25)[24])),bits);QCOMPARE(QImage(output.destination).size(),output.size);}
        output.format="jxl";output.bitDepth="12";output.destination=dir.filePath("twelve.jxl");QCOMPARE(PlayerImageTools::writeOutput(output),QString());QString error;const auto decoded=readJxl(output.destination,&error);QVERIFY2(!decoded.isNull(),qPrintable(error));QCOMPARE(decoded.text("sourceBitDepth"),"12");
        PlayerWindow player;player.show();QVERIFY(player.openFile(output.source));QTRY_VERIFY(!player.pane_->image().isNull());auto *tools=player.findChild<PlayerImageTools*>();tools->findChild<QToolButton*>("imageTool_convert")->click();auto *mode=player.findChild<QComboBox*>("imageConvertMode");QVERIFY(mode);QCOMPARE(mode->count(),1);QVERIFY(!mode->isEnabled());QVERIFY(!player.findChild<QComboBox*>("imageConvertShader"));mode->window()->close();
    }
    void hlslImageResampling(){
        ThreeFpApi api;QVERIFY(api.available());QImage image(17,11,QImage::Format_RGBA32FPx4);image.setColorSpace(QColorSpace::SRgbLinear);for(int y=0;y<image.height();++y){auto *p=reinterpret_cast<float*>(image.scanLine(y));for(int x=0;x<image.width();++x){p[x*4]=2.5f;p[x*4+1]=.25f;p[x*4+2]=.75f;p[x*4+3]=.5f;}}
        for(unsigned algorithm=0;algorithm<5;++algorithm){const auto result=api.resampleImage(image,QSize(31,19),algorithm);QVERIFY(!result.isNull());QCOMPARE(result.size(),QSize(31,19));const auto straight=result.convertToFormat(QImage::Format_RGBA32FPx4);const auto *pixel=reinterpret_cast<const float*>(straight.constScanLine(9))+15*4;QVERIFY(std::abs(pixel[0]-2.5f)<.002f);QVERIFY(std::abs(pixel[3]-.5f)<.002f);QCOMPARE(result.colorSpace(),image.colorSpace());}
    }
    void secondaryAndDownscalePixels(){
        QTemporaryDir directory;const auto file=directory.filePath("scalers.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24","-t","2","-c:v","ffv1",file});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        PlayerWindow player;player.show();QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,15000);player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);player.loadPreset("Anime-6-1X-Jinc.vpy");QTest::qWait(100);
        player.settings_->setValue("render/secondUpscale",0);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto nearest=player.clock_->capture();
        player.settings_->setValue("render/secondUpscale",4);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto jinc=player.clock_->capture();QVERIFY(!nearest.isNull());QVERIFY(!jinc.isNull());QVERIFY(nearest!=jinc);
        player.pane_->surface()->setFixedSize(128,72);QTest::qWait(100);player.settings_->setValue("render/downscale",0);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto downNearest=player.clock_->capture();
        player.settings_->setValue("render/downscale",4);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto downJinc=player.clock_->capture();QVERIFY(!downNearest.isNull());QVERIFY(!downJinc.isNull());QVERIFY(downNearest!=downJinc);
    }

    void minimalModeKeepsPlaybackPosition(){
        QTemporaryDir directory;const auto file=directory.filePath("minimal-position.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24","-f","lavfi","-i","sine=frequency=440:sample_rate=48000","-t","15","-c:v","ffv1","-c:a","flac",file});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        PlayerWindow player;player.settings_->setValue("playback/remember",false);player.resize(1100,650);player.show();QVERIFY(player.openFile(file));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,15000);
        player.seekTime(50000000);QTRY_VERIFY_WITH_TIMEOUT(player.position()>=49000000,5000);if(player.playing_)player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        QTest::keyClick(&player,Qt::Key_Period);QVERIFY(player.minimalMode_);QTest::qWait(500);QVERIFY(player.position()>=49000000);QVERIFY(!player.playing_);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
        QTest::keyClick(&player,Qt::Key_Period);QVERIFY(!player.minimalMode_);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,5000);QVERIFY(player.position()>=49000000);QVERIFY(!player.playing_);QVERIFY(player.snapshot().state!=ThreeFpState::Playing);
        player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);QTest::keyClick(&player,Qt::Key_Period);QVERIFY(player.minimalMode_);QTRY_COMPARE(player.snapshot().state,ThreeFpState::Playing);const auto at=player.position();QVERIFY(at>=49000000);QTest::qWait(350);QVERIFY(player.position()>at);
        const auto audioAt=player.position();QTest::keyClick(&player,Qt::Key_Period);QVERIFY(!player.minimalMode_);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0 && player.snapshot().state==ThreeFpState::Playing,5000);QVERIFY(player.position()>=audioAt-1000000);
    }
    void minimalModeDoesNotDecodeVideo(){
        QTemporaryDir directory;const auto path=directory.filePath("audio-mode.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24","-f","lavfi","-i","sine=frequency=440:sample_rate=48000","-t","4","-c:v","ffv1","-c:a","flac",path});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        PlayerWindow player;player.show();QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().decodedVideoFrames>2,15000);player.setMinimalMode(true);
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().decodedAudioFrames>0 && player.snapshot().state==ThreeFpState::Playing,15000);const auto before=player.snapshot();QTest::qWait(400);const auto after=player.snapshot();
        QCOMPARE(after.decodedVideoFrames,before.decodedVideoFrames);QVERIFY(after.audioPosition100ns>before.audioPosition100ns);QVERIFY(player.minimalMode_);QVERIFY(!player.videoBadge_->isVisible());
        player.setMinimalMode(false);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().decodedVideoFrames>2,15000);
    }


    void threeStageScalingMenu(){
        PlayerWindow player;player.showContextMenu(QPoint(50,50));
        auto *menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        QVERIFY(menu->findChild<QMenu*>("playerUpsampling"));
        QVERIFY(menu->findChild<QMenu*>("playerSecondUpsampling"));
        QVERIFY(menu->findChild<QMenu*>("playerDownsampling"));
        auto *second=menu->findChild<QAction*>("playerSecondScale7");QVERIFY(second);second->trigger();
        QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),7);
        auto *down=menu->findChild<QAction*>("playerDownscale3");QVERIFY(down);down->trigger();
        QCOMPARE(player.scalingAlgorithm(true),3);menu->close();
    }
    void boostedVolume(){
        PlayerWindow player;QCOMPARE(player.volume_->maximum(),200);
        player.volume_->setValue(175);QCOMPARE(player.volume_->value(),175);
        QCOMPARE(player.settings_->value("player/volume").toInt(),175);
    }

    void nativeAudioBoost(){
        QTemporaryDir directory;const auto file=directory.filePath("gain.wav");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","aevalsrc=0.1*sin(2*PI*440*t):s=48000:d=8","-ac","2","-c:a","pcm_f32le",file});QVERIFY(ff.waitForFinished(10000));QCOMPARE(ff.exitCode(),0);
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("basic/autoplay",true);settings.setValue("player/muted",false);settings.setValue("player/volume",100);settings.sync();PlayerWindow player;player.show();QVERIFY(player.openFile(file));
        ThreeFpAudioLevels output;QTRY_VERIFY_WITH_TIMEOUT(player.clock_->audioLevels(output,false) && output.values[0]>.02f,5000);const auto baseline=output.values[0];player.volume_->setValue(200);QTRY_VERIFY_WITH_TIMEOUT(player.clock_->audioLevels(output,false) && output.values[0]>baseline*1.8f,5000);QVERIFY(output.values[0]<baseline*2.2f);qInfo()<<"PCM gain"<<baseline<<output.values[0];
    }

    void customShaderMenu(){
        PlayerWindow player;player.showContextMenu(player.mapToGlobal(QPoint(60,60)));
        auto *menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        QMenu *custom=nullptr;
        for(auto *action:menu->actions()){
            QVERIFY(action->text()!=QStringLiteral("缩放模式"));
            auto label=action->text();label.remove(' ');
            if(label==QStringLiteral("自定义GLSL/HLSL着色器"))custom=action->menu();
        }
        QVERIFY(custom);
        bool hasImport=false,hasBrowse=false;
        for(auto *action:custom->actions()){hasImport|=action->text().contains(QStringLiteral("导入着色器"));hasBrowse|=action->text().contains(QStringLiteral("存储路径"));}
        QVERIFY(hasImport);QVERIFY(hasBrowse);menu->close();
    }
    void customShaderPixelsAndFailureRecovery(){
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const auto media=directory.filePath("solid.mkv");QProcess ffmpeg;
        ffmpeg.start("ffmpeg",{"-v","error","-f","lavfi","-i","color=c=0xff8040:size=320x180:rate=24","-t","6","-c:v","ffv1",media});
        QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        const auto hlsl=directory.filePath("invert.hlsl");
        QFile file(hlsl);QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Texture2D<float4> image:register(t0);SamplerState sampler0:register(s0);\nfloat4 main(float2 uv:TEXCOORD0):SV_Target{return float4(1.0-image.SampleLevel(sampler0,uv,0).rgb,1);}\n");file.close();
        const auto package=ShaderPackage::prepare(hlsl,directory.filePath("cache"));
        QVERIFY2(!package.packagePath.isEmpty(),qPrintable(package.error));
        const auto cacheTime=QFileInfo(package.packagePath).lastModified();
        const auto cached=ShaderPackage::prepare(hlsl,directory.filePath("cache"));
        QCOMPARE(cached.packagePath,package.packagePath);QCOMPARE(QFileInfo(cached.packagePath).lastModified(),cacheTime);
        QWidget surface;surface.resize(640,360);surface.show();ThreeFpApi api;ThreeFpPlayer player(api,&surface);
        QVERIFY(player.setDecodeMode(1));player.setMuted(true);QVERIFY(player.openFile(media));
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Ready,15000);QVERIFY(player.play());
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>2,10000);QVERIFY(player.pause());
        QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
        const auto center=surface.size()*surface.devicePixelRatioF();
        ThreeFpPixelProbe original{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,original));
        QVERIFY2(player.setCustomShader(package.packagePath),qPrintable(player.lastError()));
        ThreeFpPixelProbe inverted{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,inverted));
        QVERIFY(qAbs(inverted.red-(1-original.red))<.04f);QVERIFY(qAbs(inverted.green-(1-original.green))<.04f);QVERIFY(qAbs(inverted.blue-(1-original.blue))<.04f);
        QVERIFY(!player.setCustomShader(directory.filePath("missing.vsrshader")));
        ThreeFpPixelProbe retained{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,retained));
        QVERIFY(qAbs(retained.red-inverted.red)<.01f);QVERIFY(qAbs(retained.green-inverted.green)<.01f);
        const auto invalidGlsl=directory.filePath("invalid-size.glsl");QFile invalidShader(invalidGlsl);QVERIFY(invalidShader.open(QIODevice::WriteOnly));
        invalidShader.write("//!HOOK MAIN\n//!WIDTH 0\nvec4 hook(){return vec4(1.0);}\n");invalidShader.close();
        const auto invalidPackage=ShaderPackage::prepare(invalidGlsl,directory.filePath("cache"));
        QVERIFY2(!invalidPackage.packagePath.isEmpty(),qPrintable(invalidPackage.error));
        QVERIFY(!player.setCustomShader(invalidPackage.packagePath));
        ThreeFpPixelProbe afterInvalid{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,afterInvalid));
        QVERIFY(qAbs(afterInvalid.red-inverted.red)<.01f);QVERIFY(qAbs(afterInvalid.green-inverted.green)<.01f);QVERIFY(qAbs(afterInvalid.blue-inverted.blue)<.01f);
        QVERIFY(player.setCustomShader({}));
        ThreeFpPixelProbe restored{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,restored));
        QVERIFY(qAbs(restored.red-original.red)<.01f);QVERIFY(qAbs(restored.green-original.green)<.01f);
        const auto glsl=directory.filePath("multi.glsl");QFile shader(glsl);QVERIFY(shader.open(QIODevice::WriteOnly));
        shader.write("//!DESC Invert\n//!HOOK MAIN\n//!BIND HOOKED\n//!SAVE inverse\nvec4 hook(){return vec4(1.0-HOOKED_tex(HOOKED_pos).rgb,1.0);}\n//!DESC Half\n//!HOOK MAIN\n//!BIND inverse\nvec4 hook(){return vec4(inverse_tex(inverse_pos).rgb*0.5,1.0);}\n");shader.close();
        const auto converted=ShaderPackage::prepare(glsl,directory.filePath("cache"));
        QVERIFY2(!converted.packagePath.isEmpty(),qPrintable(converted.error));QVERIFY(player.setCustomShader(converted.packagePath));
        ThreeFpPixelProbe multi{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,multi));
        QVERIFY(qAbs(multi.red-(1-original.red)*.5f)<.04f);QVERIFY(qAbs(multi.green-(1-original.green)*.5f)<.04f);QVERIFY(qAbs(multi.blue-(1-original.blue)*.5f)<.04f);
        QVERIFY(shader.open(QIODevice::WriteOnly|QIODevice::Truncate));shader.write("//!HOOK CHROMA\n//!BIND HOOKED\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}\n");shader.close();
        const auto unsupported=ShaderPackage::prepare(glsl,directory.filePath("cache"));
        QVERIFY(unsupported.packagePath.isEmpty());QVERIFY(!unsupported.error.isEmpty());
        QVERIFY(shader.open(QIODevice::WriteOnly|QIODevice::Truncate));shader.write("//!HOOK PREKERNEL\n//!SAVE PREKERNEL\nvec4 hook(){return vec4(1.0,0.0,0.0,1.0);}\n");shader.close();
        const auto prekernel=ShaderPackage::prepare(glsl,directory.filePath("cache"));
        QVERIFY2(!prekernel.packagePath.isEmpty(),qPrintable(prekernel.error));QVERIFY(player.setCustomShader(prekernel.packagePath));
        ThreeFpPixelProbe red{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,red));
        QVERIFY(red.red>.98f);QVERIFY(red.green<.02f);QVERIFY(red.blue<.02f);
        QVERIFY(shader.open(QIODevice::WriteOnly|QIODevice::Truncate));
        shader.write("//!HOOK MAIN\n//!SAVE narrow\n//!COMPONENTS 1\nvec4 hook(){return vec4(0.2,0.7,0.8,0.4);}\n//!HOOK MAIN\n//!BIND narrow\nvec4 hook(){vec4 value=narrow_tex(narrow_pos);return vec4(value.r,value.g,value.a,1.0);}\n");shader.close();
        const auto components=ShaderPackage::prepare(glsl,directory.filePath("cache"));
        QVERIFY2(!components.packagePath.isEmpty(),qPrintable(components.error));QVERIFY(player.setCustomShader(components.packagePath));
        ThreeFpPixelProbe narrow{};player.redraw();QVERIFY(player.samplePixel(center.width()/2,center.height()/2,narrow));
        QVERIFY(qAbs(narrow.red-.2f)<.02f);QVERIFY(narrow.green<.02f);QVERIFY(narrow.blue>.98f);
        QVERIFY(player.setCustomShader({}));
    }
    void customShaderImportPersistenceAndHdrRecovery(){
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const auto sdr=directory.filePath("sdr.mkv"),hdr=directory.filePath("hdr.mp4"),encodedHdr=directory.filePath("hdr-encoded.mkv"),shaderFolder=directory.filePath("stored-shaders");
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg",{"-v","error","-f","lavfi","-i","color=c=0xff8040:size=320x180:rate=24","-t","8","-c:v","ffv1","-pix_fmt","yuv420p",sdr});
        QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        ffmpeg.start("ffmpeg",{"-v","error","-f","lavfi","-i","color=c=0xff8040:size=320x180:rate=24","-t","8","-c:v","libx265","-preset","ultrafast","-pix_fmt","yuv420p10le",
            "-x265-params","log-level=error:colorprim=bt2020:transfer=smpte2084:colormatrix=bt2020nc",
            "-color_primaries","bt2020","-color_trc","smpte2084","-colorspace","bt2020nc",encodedHdr});
        QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        // Write container colour tags from the encoded stream as well as its HEVC VUI.
        ffmpeg.start("ffmpeg",{"-v","error","-i",encodedHdr,"-c","copy","-color_primaries","bt2020",
            "-color_trc","smpte2084","-colorspace","bt2020nc","-movflags","+write_colr",hdr});
        QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        const auto hdrProbe=PlayerAudioMetadata::probe(hdr);const auto hdrStreams=hdrProbe.value("streams").toArray();
        QVERIFY2(hdrStreams.size()==1,"ffprobe must expose the generated HDR video stream before playback");
        const auto hdrStream=hdrStreams.first().toObject();qInfo().noquote()<<"HDR fixture ffprobe"<<QJsonDocument(hdrStream).toJson(QJsonDocument::Compact);
        QCOMPARE(hdrStream.value("color_transfer").toString(),QString("smpte2084"));
        QCOMPARE(hdrStream.value("color_primaries").toString(),QString("bt2020"));
        const auto imported=directory.filePath("invert.glsl");
        const QByteArray source="//!HOOK MAIN\n//!BIND HOOKED\nvec4 hook(){return vec4(1.0-HOOKED_tex(HOOKED_pos).rgb,1.0);}\n";
        QFile shader(imported);QVERIFY(shader.open(QIODevice::WriteOnly));QCOMPARE(shader.write(source),qint64(source.size()));shader.close();
        QSettings settings(config(),QSettings::IniFormat);settings.setValue("decode/mode",1);settings.setValue("player/muted",true);settings.setValue("render/shaderDirectory",shaderFolder);settings.sync();
        QString stored,packagePath;QDateTime cacheTime;ThreeFpPixelProbe original{},filtered{};
        {
            PlayerWindow player;player.resize(1100,600);player.show();player.loadPreset("Anime-5-D3D11.vpy");QVERIFY(player.openFile(sdr));
            QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.snapshot().state==ThreeFpState::Playing && player.outputSnapshot().presentedVideoFrames>2,15000);
            player.togglePlayback();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
            const auto center=player.pane_->surface()->size()*player.pane_->surface()->devicePixelRatioF();
            player.clock_->redraw();QVERIFY(player.clock_->samplePixel(center.width()/2,center.height()/2,original));
            QVERIFY2(player.importCustomShader(imported),qPrintable(player.message_->text()));
            QTRY_VERIFY_WITH_TIMEOUT((player.clock_->redraw(),player.clock_->samplePixel(center.width()/2,center.height()/2,filtered) && qAbs(filtered.red-(1-original.red))<.04f && qAbs(filtered.green-(1-original.green))<.04f && qAbs(filtered.blue-(1-original.blue))<.04f),5000);
            stored=player.settings_->value("render/customShader").toString();packagePath=player.customShaderPackage_;player.settings_->sync();
            QVERIFY(!stored.isEmpty());QVERIFY(stored!=imported);QCOMPARE(QFileInfo(stored).absolutePath(),QFileInfo(shaderFolder).absoluteFilePath());
            QFile copy(stored);QVERIFY(copy.open(QIODevice::ReadOnly));QCOMPARE(copy.readAll(),source);
            QFile cache(packagePath);QVERIFY(cache.open(QIODevice::ReadOnly));QCOMPARE(cache.read(8),QByteArray("VSRHLSL1"));
            QCOMPARE(QFileInfo(packagePath).absolutePath(),QDir(shaderFolder).filePath("cache"));cacheTime=QFileInfo(packagePath).lastModified();
        }
        QVERIFY(QFile::remove(imported));
        {
            PlayerWindow player;player.resize(1100,600);player.show();
            QCOMPARE(player.settings_->value("render/customShader").toString(),stored);QCOMPARE(player.customShaderPackage_,packagePath);QCOMPARE(QFileInfo(packagePath).lastModified(),cacheTime);
            QVERIFY(player.openFile(sdr));
            QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.snapshot().state==ThreeFpState::Playing && player.outputSnapshot().presentedVideoFrames>2,15000);
            const auto center=player.pane_->surface()->size()*player.pane_->surface()->devicePixelRatioF();ThreeFpPixelProbe restarted{};
            QTRY_VERIFY_WITH_TIMEOUT((player.clock_->redraw(),player.clock_->samplePixel(center.width()/2,center.height()/2,restarted) && qAbs(restarted.red-filtered.red)<.02f && qAbs(restarted.green-filtered.green)<.02f && qAbs(restarted.blue-filtered.blue)<.02f),5000);
            QVERIFY(player.openFile(hdr));
            QTimer hdrMetadataLog;hdrMetadataLog.setInterval(10);
            connect(&hdrMetadataLog,&QTimer::timeout,&player,[&]{
                if(!player.positionRestored_)return;
                hdrMetadataLog.stop();const auto snap=player.snapshot();
                qInfo().noquote()<<"HDR first metadata: state"<<int(snap.state)<<"isHdrSource"<<snap.isHdrSource
                    <<"selectedVideoStream"<<snap.selectedVideoStream<<"frames"<<snap.presentedVideoFrames
                    <<"position"<<snap.position100ns<<"hlslFailed"<<player.hlslFailed_
                    <<"native mediaInfo"<<player.clock_->mediaInfo()
                    <<"frontend media"<<QJsonDocument(player.media_).toJson(QJsonDocument::Compact);
            });hdrMetadataLog.start();
            QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().isHdrSource && player.snapshot().state==ThreeFpState::Playing && player.outputSnapshot().presentedVideoFrames>2,
                qPrintable(QString("state=%1 isHdrSource=%2 frames=%3 position=%4 hlslFailed=%5 hlslFallback=[%6] message=[%7] lastError=[%8] selectedAudioStream=%9 nativeMediaInfo=[%10]")
                    .arg(int(player.snapshot().state)).arg(player.snapshot().isHdrSource)
                    .arg(qulonglong(player.outputSnapshot().presentedVideoFrames)).arg(qint64(player.position()))
                    .arg(player.hlslFailed_).arg(player.hlslFallback_).arg(player.message_->text())
                    .arg(player.clock_->lastError()).arg(player.clock_->snapshot().selectedAudioStream).arg(player.clock_->mediaInfo())),20000);
            const auto hdrFrames=player.outputSnapshot().presentedVideoFrames;const auto hdrPosition=player.position();
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing && player.outputSnapshot().presentedVideoFrames>hdrFrames+2 && player.position()>hdrPosition,5000);
            QCOMPARE(player.settings_->value("render/customShader").toString(),stored);QVERIFY(!player.hlslFallback_.isEmpty());QVERIFY(!player.server_->available());
            QVERIFY(player.openFile(sdr));
            QTRY_VERIFY_WITH_TIMEOUT(!player.snapshot().isHdrSource && player.snapshot().state==ThreeFpState::Playing && player.outputSnapshot().presentedVideoFrames>2,15000);
            ThreeFpPixelProbe recovered{};
            QTRY_VERIFY2_WITH_TIMEOUT((player.clock_->redraw(),player.clock_->samplePixel(center.width()/2,center.height()/2,recovered) && qAbs(recovered.red-filtered.red)<.02f && qAbs(recovered.green-filtered.green)<.02f && qAbs(recovered.blue-filtered.blue)<.02f),
                qPrintable(QString("recovered=(%1,%2,%3) filtered=(%4,%5,%6) hlslFailed=%7 fallback=[%8] message=[%9] lastError=[%10] nativeMediaInfo=[%11]")
                    .arg(recovered.red).arg(recovered.green).arg(recovered.blue).arg(filtered.red).arg(filtered.green).arg(filtered.blue)
                    .arg(player.hlslFailed_).arg(player.hlslFallback_).arg(player.message_->text()).arg(player.clock_->lastError()).arg(player.clock_->mediaInfo())),5000);
            QVERIFY(!player.hlslFailed_);QCOMPARE(player.settings_->value("render/customShader").toString(),stored);
        }
    }
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
        auto *percent=player.findChild<QLineEdit *>("playerVolumePercent");auto *button=player.findChild<QPushButton *>("playerEqualizerButton");QVERIFY(percent && button);QCOMPARE(percent->text(),QString("63%"));const auto muted=player.mute_->icon().pixmap(24,24).toImage();QVERIFY(!muted.isNull());player.mute_->click();QVERIFY(!player.mute_->isChecked());QVERIFY(player.mute_->icon().pixmap(24,24).toImage()!=muted);
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
        auto* action=menu->findChild<QAction*>(QString("playerAnimeStage%1").arg(stage));if(stage==4 || stage==5)action=menu->findChild<QAction*>(QString("playerPrimaryScale%1").arg(stage==4?4:7));QVERIFY(action);QVERIFY(action->isEnabled());action->trigger();menu->close();
        QCOMPARE(player.qualityStage(),stage);const int algorithms[]{10,9,11,12,4,7,13,14};QCOMPARE(player.scalingAlgorithm(),algorithms[stage]);
        const auto filtered=player.clock_->capture();QVERIFY(!filtered.isNull());QCOMPARE(filtered.size(),original.size());QCOMPARE(player.outputSnapshot().framePts,pts);
        QVERIFY2(player.clock_->lastError().isEmpty(),qPrintable(player.clock_->lastError()));
        if(stage!=5)QVERIFY2(filtered!=original,"The selected filter must change rendered pixels");
        const QStringList files{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy"};
        if(stage==0 || stage==2){player.loadPreset(files[stage==6?7:stage+1]);const auto other=player.clock_->capture();QVERIFY(!other.isNull());QVERIFY2(filtered!=other,"Enhanced/1X renderer variants must produce distinct output");player.loadPreset(files[stage]);}
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
                for(int i=0;i<list->count();++i){QCOMPARE(bool(list->item(i)->flags() & Qt::ItemIsUserCheckable),list->item(i)->data(Qt::UserRole).toInt()!=5);if(list->item(i)->data(Qt::UserRole).toInt()==2)list->setCurrentItem(list->item(i));}
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
