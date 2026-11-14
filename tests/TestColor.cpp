#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include <QTest>
#include <QWidget>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include <QColorSpace>
#include <cmath>
#include <array>
#include <algorithm>
#include <QElapsedTimer>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/hdr_dynamic_vivid_metadata.h>
#include <libavutil/version.h>
}
using namespace vsr;

namespace {
using Rgb = std::array<double,3>;
bool readVsFrame(VapourSynthFrameServer &server,const QString &script,VapourSynthFrame &frame,QString &error,int index=0) {
    const auto wait=[](const auto &ready){QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<20000)QTest::qWait(10);return ready();};
    if(!wait([&]{return !server.initializing();}) || !server.available()){error=server.errorString();return false;}
    QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
    server.loadScript(script,"color-bridge-identity.vpy");
    if(!wait([&]{return !loaded.isEmpty()||!errors.isEmpty();}) || !errors.isEmpty()){error=errors.isEmpty()?"VS script load timed out":errors.first().first().toString();return false;}
    server.requestFrame(index);
    if(!wait([&]{return !frames.isEmpty()||!errors.isEmpty();}) || !errors.isEmpty()){error=errors.isEmpty()?"VS frame request timed out":errors.first().first().toString();return false;}
    frame=qvariant_cast<VapourSynthFrame>(frames.first().first());return true;
}
double halfFloat(quint16 h) {
    const int exponent=(h>>10)&31;const double fraction=(h&1023)/1024.0;
    const double magnitude=exponent?std::ldexp(1+fraction,exponent-15):std::ldexp(fraction,-14);
    return h&32768?-magnitude:magnitude;
}
double sdrDecode(double v,int transfer) {
    v=std::max(v,0.0);
    if(transfer==8)return v;
    if(transfer==4)return std::pow(v,2.2);
    if(transfer==5)return std::pow(v,2.8);
    if(transfer==13)return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);
    return v<.081?v/4.5:std::pow((v+.099)/1.099,1/.45);
}
double srgbEncode(double v) {
    v=std::max(v,0.0);return v<=.0031308?12.92*v:1.055*std::pow(v,1/2.4)-.055;
}
Rgb referenceColor(bool rgb,int matrix,int transfer,int primaries,const Rgb &input) {
    Rgb coded=input;
    if(!rgb) {
        const double y=(input[0]*255-16)/219,u=(input[1]*255-128)/224,v=(input[2]*255-128)/224;
        if(matrix==8)coded={y+v-u,y+u,y-v-u};
        else if(matrix==10) {
            // BT.2020 CL restores R'/B' piecewise, then solves G in linear light.
            const double r=y+v*(v<=0?1.7184:.9936),b=y+u*(u<=0?1.9404:1.5816);
            const auto linear=[](double x){constexpr double a=1.09929682680944,b=.018053968510807;return x<4.5*b?x/4.5:std::pow((x+a-1)/a,1/.45);};
            const auto encode=[](double x){constexpr double a=1.09929682680944,b=.018053968510807;return x<b?4.5*x:a*std::pow(x,.45)-(a-1);};
            coded={r,encode((linear(y)-.2627*linear(r)-.0593*linear(b))/.678),b};
        } else {
            double kr=.299,kb=.114;
            if(matrix==1){kr=.2126;kb=.0722;}
            if(matrix==4){kr=.30;kb=.11;}
            if(matrix==7){kr=.212;kb=.087;}
            if(matrix==9){kr=.2627;kb=.0593;}
            coded={y+2*(1-kr)*v,y-2*kb*(1-kb)/(1-kr-kb)*u-2*kr*(1-kr)/(1-kr-kb)*v,y+2*(1-kb)*u};
        }
    }
    // Preserve the existing Rec.709 SDR display contract; all explicit other
    // transfer functions and gamut conversions target an sRGB SDR surface.
    if(transfer==1 && primaries==1)return coded;
    Rgb light;for(int i=0;i<3;++i)light[i]=sdrDecode(coded[i],transfer);
    if(primaries==9)light={1.660491*light[0]-.587641*light[1]-.072850*light[2],-.124550*light[0]+1.132900*light[1]-.008349*light[2],-.018151*light[0]-.100579*light[1]+1.118730*light[2]};
    if(primaries==12)light={1.224810*light[0]-.224970*light[1]-.000025*light[2],-.042043*light[0]+1.042084*light[1]+.000018*light[2],-.019642*light[0]-.078649*light[1]+1.098527*light[2]};
    for(auto &v:light)v=std::clamp(srgbEncode(v),0.0,1.0);
    return light;
}
}

class TestColor final : public QObject {
    Q_OBJECT
    static VapourSynthFrame yuv(unsigned matrix,unsigned transfer,unsigned primaries,int y,int u,int v,int bits=8) {
        VapourSynthFrame f;f.width=64;f.height=48;f.totalFrames=10;f.duration100ns=400000;
        f.format=bits==8?ThreeFpExternalPixelFormat::Yuv444P8:ThreeFpExternalPixelFormat::Yuv444P16;
        f.colorMatrix=matrix;f.colorTransfer=transfer;f.colorPrimaries=primaries;f.colorRange=0;f.chromaLocation=0;
        const int values[]={y,u,v};for(int i=0;i<3;++i){f.strides[i]=f.width*(bits==8?1:2);f.planes[i].resize(f.strides[i]*f.height);if(bits==8)f.planes[i].fill(char(values[i]));else{auto *p=reinterpret_cast<quint16*>(f.planes[i].data());std::fill(p,p+f.width*f.height,quint16(values[i]));}}
        return f;
    }
    static VsrColorSettings advanced() {auto c=VsrColorDefaultSettings();c.engine=1;c.output=1;c.icc=0;c.peakDetect=0;return c;}
private slots:
    void futureImportRemainsAtScriptStart() {
        VapourSynthFrameServer server; VapourSynthFrame frame; QString error;
        const QString script = "from __future__ import annotations\nimport vapoursynth as vs\nvs.core.std.BlankClip(width=64,height=48,length=1).set_output()\n";
        QVERIFY2(readVsFrame(server, script, frame, error), qPrintable(error));
        QCOMPARE(frame.width, 64);
    }
    void legacyFrameRejectsMetadata_data() {
        QTest::addColumn<int>("field");
        const char* names[] = {"pts", "sar-num", "sar-den", "field-based", "stream-hdr10plus"};
        for (int i=0; i<5; ++i) QTest::newRow(names[i]) << i;
    }
    void legacyFrameRejectsMetadata() {
        QFETCH(int, field);
        const auto legacy = qgetenv("VSR_TEST_LEGACY_3FP_DLL");
        if (legacy.isEmpty()) QSKIP("Set VSR_TEST_LEGACY_3FP_DLL to a pre-v2 Native DLL");
        const auto previous = qgetenv("VSR_3FP_DLL");
        qputenv("VSR_3FP_DLL", legacy);
        const auto restore = qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_3FP_DLL");else qputenv("VSR_3FP_DLL",previous);});
        QWidget surface; surface.resize(64,48); surface.show();
        ThreeFpApi api; QVERIFY(api.available());
        ThreeFpConfiguration config{}; config.size=sizeof(config);config.version=api.apiVersion();
        config.outputWindow=reinterpret_cast<void*>(surface.winId());config.sdrPeakNits=100;config.sdrPaperWhiteNits=203;
        void* handle=nullptr; QCOMPARE(api.create(&config,&handle),ThreeFpResult::Success);
        const auto cleanup=qScopeGuard([&]{api.destroy(handle);});
        QByteArray plane(64*48,char(128)); ThreeFpExternalVideoFrame frame{};
        frame.size=sizeof(frame);frame.version=2;frame.width=64;frame.height=48;frame.totalFrames=1;frame.duration100ns=400000;
        frame.format=ThreeFpExternalPixelFormat::Yuv444P8;frame.colorRange=1;frame.colorMatrix=frame.colorPrimaries=frame.colorTransfer=1;
        for(int p=0;p<3;++p){frame.data[p]=plane.constData();frame.linesize[p]=64;}
        QCOMPARE(api.submitExternalVideoFrame(handle,&frame),ThreeFpResult::Success);
        if(field==0)frame.colorMetadata.presentationTime100ns=0;
        if(field==1)frame.colorMetadata.sarNumerator=4;
        if(field==2)frame.colorMetadata.sarDenominator=3;
        if(field==3)frame.colorMetadata.fieldBased=2;
        if(field==4)frame.colorMetadata.streamHdr10Plus=1;
        QCOMPARE(api.submitExternalVideoFrame(handle,&frame),ThreeFpResult::NotSupported);
    }
    void grayOutputRange_data() {
        QTest::addColumn<int>("range"); QTest::addColumn<int>("code");
        for(int range:{1,2})for(int code:{16,128,235})
            QTest::newRow(qPrintable(QString("range%1-code%2").arg(range).arg(code))) << range << code;
    }
    void grayOutputRange() {
        QFETCH(int,range);QFETCH(int,code);
        QWidget surface;surface.resize(192,144);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer player(api,&surface);
        VapourSynthFrame frame;frame.width=64;frame.height=48;frame.totalFrames=1;frame.duration100ns=400000;
        frame.format=ThreeFpExternalPixelFormat::Gray8;frame.colorMatrix=frame.colorTransfer=frame.colorPrimaries=1;frame.colorRange=range;
        frame.strides[0]=64;frame.planes[0]=QByteArray(64*48,char(code));
        QVERIFY(player.submitFrame(frame));QVERIFY(player.setOutputFormat("gray16le"));QVERIFY(player.submitFrame(frame));
        QTRY_VERIFY(player.snapshot().swapChainPresents>0);
        ThreeFpPixelProbe pixel{};QVERIFY(player.samplePixel(96*surface.devicePixelRatioF(),72*surface.devicePixelRatioF(),pixel));
        const double expected=range==1?std::clamp((code-16)/219.0,0.0,1.0):code/255.0;
        QVERIFY2(std::abs(pixel.red-expected)<1.5/255.0,qPrintable(QString("actual %1 expected %2").arg(pixel.red).arg(expected)));
    }
    void packedYuvRejectsUnsupportedMatrix() {
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const QString ffmpeg=QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");
        const auto encoded=directory.filePath("packed.mkv"),media=directory.filePath("tagged.mkv");
        QProcess encode;encode.start(ffmpeg,{"-hide_banner","-loglevel","error","-y","-f","lavfi","-i","color=c=orange:size=64x48:rate=24:duration=0.25","-pix_fmt","yuyv422","-c:v","rawvideo","-allow_raw_vfw","1","-colorspace","bt709",encoded});
        QVERIFY(encode.waitForFinished(20000));QCOMPARE(encode.exitCode(),0);
        QProcess remux;remux.start(ffmpeg,{"-hide_banner","-loglevel","error","-y","-i",encoded,"-c","copy","-allow_raw_vfw","1","-colorspace","chroma-derived-nc","-color_primaries","smpte432","-color_trc","bt709",media});
        QVERIFY(remux.waitForFinished(10000));QCOMPARE(remux.exitCode(),0);
        QWidget surface;surface.resize(192,144);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer player(api,&surface);QVERIFY(player.setDecodeMode(1));QVERIFY(player.openFile(media));
        QTRY_COMPARE(player.snapshot().state,ThreeFpState::Ready);QVERIFY(player.seekFrame(0));
        QTRY_VERIFY_WITH_TIMEOUT(player.lastError().contains("unsupported component transform"),5000);
        QCOMPARE(player.snapshot().swapChainPresents,quint64(0));
    }
    void nativeMissingChromaDefaultsToLeft() {
        QWidget surface; surface.resize(320,240); surface.show(); QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api; ThreeFpPlayer player(api, &surface);
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Bilinear, ThreeFpScalingAlgorithm::Bilinear));
        VapourSynthFrame frame; frame.width=64; frame.height=48; frame.totalFrames=1;
        frame.duration100ns=400000; frame.format=ThreeFpExternalPixelFormat::Yuv420P8;
        frame.colorMatrix=frame.colorPrimaries=frame.colorTransfer=1; frame.colorRange=1;
        frame.strides={64,32,32,0}; frame.planes[0]=QByteArray(64*48,char(128));
        frame.planes[1]=QByteArray(32*24,char(128)); frame.planes[2]=QByteArray(32*24,char(128));
        for(int y=0;y<24;++y)for(int x=0;x<32;++x)frame.planes[1][y*32+x]=char(x%2?170:80);
        QVERIFY(player.submitFrame(frame)); QTRY_VERIFY(player.snapshot().presentedVideoFrames>0);
        QList<ThreeFpPixelProbe> unknown;
        for(int x:{17,49,81,113,145}){ThreeFpPixelProbe p{};QVERIFY(player.samplePixel(x*surface.devicePixelRatioF(),120*surface.devicePixelRatioF(),p));unknown<<p;}
        frame.chromaLocation=1; QVERIFY(player.submitFrame(frame)); int i=0;
        for(int x:{17,49,81,113,145}){ThreeFpPixelProbe p{};QVERIFY(player.samplePixel(x*surface.devicePixelRatioF(),120*surface.devicePixelRatioF(),p));const auto a=unknown.at(i++);QCOMPARE(a.red,p.red);QCOMPARE(a.green,p.green);QCOMPARE(a.blue,p.blue);}
    }
    void vsFramePresentationProperties() {
        const QString script="import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.YUV444P16,color=[30000,32768,32768])\nc=vs.core.std.SetFrameProps(c,_Matrix=1,_Primaries=1,_Transfer=1,_ColorRange=1,_FieldBased=2,_SARNum=4,_SARDen=3,_AbsoluteTime=0.123)\nc.set_output()\n";
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;QVERIFY2(readVsFrame(server,script,frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.fieldBased,2u);QCOMPARE(frame.colorMetadata.sarNumerator,4);QCOMPARE(frame.colorMetadata.sarDenominator,3);QCOMPARE(frame.colorMetadata.presentationTime100ns,qint64(1230000));
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY2(p.submitFrame(frame),qPrintable(p.lastError()));QTRY_VERIFY(p.snapshot().swapChainPresents>0);QCOMPARE(p.snapshot().position100ns,qint64(1230000));
    }
    void vsVividSideData() {
        AVDynamicHDRVivid vivid{}; vivid.system_start_code = 1; vivid.num_windows = 1;
        vivid.params[0].maximum_maxrgb = {1, 10};
        QByteArray bytes;
        for (quint32 v : {1u, avutil_version(), quint32(AV_FRAME_DATA_DYNAMIC_HDR_VIVID), quint32(sizeof(vivid))})
            bytes.append(reinterpret_cast<const char*>(&v), 4);
        bytes.append(reinterpret_cast<const char*>(&vivid), sizeof(vivid));
        const auto script = QString("import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.YUV444P16,color=[40000,32768,32768])\nc=vs.core.std.SetFrameProps(c,_Matrix=9,_Primaries=9,_Transfer=16,_ColorRange=1,_VsrFrameSideData=bytes.fromhex('%1'))\nc.set_output()\n").arg(QString::fromLatin1(bytes.toHex()));
        VapourSynthFrameServer server; VapourSynthFrame frame; QString error;
        QVERIFY2(readVsFrame(server, script, frame, error), qPrintable(error)); QCOMPARE(frame.colorSideData, bytes);
        QWidget surface; surface.resize(160,120); surface.show(); QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api; ThreeFpPlayer player(api, &surface); QVERIFY(player.setColorSettings(advanced()));
        QVERIFY2(player.submitFrame(frame), qPrintable(player.lastError()));
        QTRY_COMPARE_WITH_TIMEOUT(player.colorStatus().activeEngine, 1u, 20000);
        QCOMPARE(player.colorStatus().vividDetected, 1u);
        auto invalid = frame; invalid.colorSideData[4] ^= 1;
        QVERIFY(!player.submitFrame(invalid));
    }
    void vsHdr10PlusSideData() {
        AVDynamicHDRPlus plus{};plus.itu_t_t35_country_code=0xb5;plus.application_version=1;plus.num_windows=1;plus.targeted_system_display_maximum_luminance={100,1};
        plus.params[0].maxscl[0]=plus.params[0].maxscl[1]=plus.params[0].maxscl[2]={1,10};plus.params[0].average_maxrgb={1,50};plus.params[0].tone_mapping_flag=1;plus.params[0].knee_point_x={1,4};plus.params[0].knee_point_y={1,2};plus.params[0].num_bezier_curve_anchors=1;plus.params[0].bezier_curve_anchors[0]={3,4};
        const auto payload=[&]{QByteArray data;const auto word=[&](quint32 value){data.append(reinterpret_cast<const char*>(&value),4);};word(1);word(avutil_version());word(AV_FRAME_DATA_DYNAMIC_HDR_PLUS);word(sizeof(plus));data.append(reinterpret_cast<const char*>(&plus),sizeof(plus));return data;};
        const auto script=[](const QByteArray &bytes){return QString("import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.YUV444P16,color=[40000,32768,32768])\nc=vs.core.std.SetFrameProps(c,_Matrix=9,_Primaries=9,_Transfer=16,_ColorRange=1,_VsrFrameSideData=bytes.fromhex('%1'))\nc.set_output()\n").arg(QString::fromLatin1(bytes.toHex()));};
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;auto bytes=payload();QVERIFY2(readVsFrame(server,script(bytes),frame,error),qPrintable(error));QCOMPARE(frame.colorSideData,bytes);
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY(p.setColorSettings(advanced()));QVERIFY2(p.submitFrame(frame),qPrintable(p.lastError()));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);QCOMPARE(p.colorStatus().hdr10plus,1u);QCOMPARE(p.colorStatus().tone,2u);QTRY_VERIFY(p.snapshot().presentedVideoFrames>0);
        ThreeFpPixelProbe first{};QVERIFY(p.samplePixel(surface.width()*surface.devicePixelRatioF()/2,surface.height()*surface.devicePixelRatioF()/2,first));
        plus.params[0].knee_point_y={1,10};plus.params[0].bezier_curve_anchors[0]={1,10};bytes=payload();QVERIFY2(readVsFrame(server,script(bytes),frame,error),qPrintable(error));QCOMPARE(frame.colorSideData,bytes);QVERIFY(p.submitFrame(frame));QCOMPARE(p.colorStatus().hdr10plus,1u);
        ThreeFpPixelProbe second{};QVERIFY(p.samplePixel(surface.width()*surface.devicePixelRatioF()/2,surface.height()*surface.devicePixelRatioF()/2,second));QVERIFY2(std::abs(first.red-second.red)>.001,"Changing HDR10+ knee/anchor must reach the renderer through VS, not merely survive as bytes");
        const QString stripped="import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.YUV444P16,color=[40000,32768,32768])\nc=vs.core.std.SetFrameProps(c,_Matrix=9,_Primaries=9,_Transfer=16,_ColorRange=1)\nc.set_output()\n";
        QVERIFY2(readVsFrame(server,stripped,frame,error),qPrintable(error));QVERIFY(frame.colorSideData.isEmpty());QVERIFY(p.submitFrame(frame));QCOMPARE(p.colorStatus().hdr10plus,0u);
        // Stream-level HDR10+ ownership permits sparse per-frame payloads.
        // The native metadata state must hold a previous curve over the gap;
        // the independent optional libplacebo engine is not part of this check.
        QWidget nativeSurface;nativeSurface.resize(160,120);nativeSurface.show();QVERIFY(QTest::qWaitForWindowExposed(&nativeSurface));ThreeFpPlayer native(api,&nativeSurface);
        auto sparse=script(bytes);sparse.replace("c.set_output()","c=vs.core.std.SetFrameProps(c,_VsrStreamHDR10Plus=1)\nc.set_output()");
        QVERIFY2(readVsFrame(server,sparse,frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.streamHdr10Plus,1u);QVERIFY(native.submitFrame(frame));QTRY_VERIFY(native.snapshot().swapChainPresents>0);
        auto gap=stripped;gap.replace("c.set_output()","c=vs.core.std.SetFrameProps(c,_VsrStreamHDR10Plus=1)\nc.set_output()");QVERIFY2(readVsFrame(server,gap,frame,error),qPrintable(error));QVERIFY(frame.colorSideData.isEmpty());QCOMPARE(frame.colorMetadata.streamHdr10Plus,1u);frame.frameIndex=1;frame.totalFrames=2;QVERIFY(native.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(native.snapshot().dynamicMetadataHeldFrames>0,5000);
    }
    void nativeVsSourceIdentity_data() {
        QTest::addColumn<QString>("pixelFormat");QTest::addColumn<int>("bits");QTest::addColumn<int>("transfer");
        QTest::newRow("420p10-sdr")<<QString("yuv420p10le")<<10<<1;
        QTest::newRow("av1-420p10-sdr")<<QString("yuv420p10le")<<10<<1;
        QTest::newRow("444p16-pq")<<QString("yuv444p16le")<<16<<16;
        QTest::newRow("444p16-hlg")<<QString("yuv444p16le")<<16<<18;
    }
    void nativeVsSourceIdentity() {
        QFETCH(QString,pixelFormat);QFETCH(int,bits);QFETCH(int,transfer);
        const bool subsampled=pixelFormat.startsWith("yuv420");const int maximum=(1<<bits)-1;
        QTemporaryDir directory;QVERIFY(directory.isValid());QFile raw(directory.filePath("identity.yuv"));QVERIFY(raw.open(QIODevice::WriteOnly));
        for(int frame=0;frame<6;++frame)for(int plane=0;plane<3;++plane){
            const int width=plane&&subsampled?32:64,height=plane&&subsampled?24:48;QByteArray bytes(width*height*2,Qt::Uninitialized);auto* samples=reinterpret_cast<quint16*>(bytes.data());
            for(int y=0;y<height;++y)for(int x=0;x<width;++x)samples[y*width+x]=quint16(maximum*(plane==0?.20+.055*frame+.18*x/width+.10*y/height:.5+.025*(plane==1?1:-1)*(x-width/2)/width));
            QCOMPARE(raw.write(bytes),qint64(bytes.size()));
        }
        raw.close();const auto encoded=directory.filePath("encoded.mkv"),media=directory.filePath("identity.mkv");const QString ffmpeg=QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");QVERIFY(QFileInfo::exists(ffmpeg));
        const QString transferName=transfer==16?"smpte2084":transfer==18?"arib-std-b67":"bt709";
        const bool av1 = QByteArray(QTest::currentDataTag()).startsWith("av1");
        QProcess encode;encode.start(ffmpeg,{"-hide_banner","-loglevel","error","-y","-f","rawvideo","-pixel_format",pixelFormat,"-video_size","64x48","-framerate","24","-i",raw.fileName(),"-frames:v","6","-c:v",av1?"libaom-av1":"ffv1",av1?"-cpu-used":"-level",av1?"8":"3","-color_primaries",transfer==1?"bt709":"bt2020","-color_trc",transferName,"-colorspace",transfer==1?"bt709":"bt2020nc","-color_range","tv","-chroma_sample_location","left",encoded});
        QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        // FFV1's encoder can omit transfer/primaries from its output parameters;
        // write the intended Matroska stream tags explicitly after encoding.
        QProcess remux;remux.start(ffmpeg,{"-hide_banner","-loglevel","error","-y","-i",encoded,"-c","copy","-color_primaries",transfer==1?"bt709":"bt2020","-color_trc",transferName,"-colorspace",transfer==1?"bt709":"bt2020nc","-color_range","tv","-chroma_sample_location","left",media});
        QVERIFY(remux.waitForFinished(20000));QVERIFY2(remux.exitCode()==0,remux.readAllStandardError().constData());
        QProcess probe;probe.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffprobe.exe"),{"-v","error","-show_entries","stream=pix_fmt,color_transfer,color_primaries,color_space,color_range","-of","json",media});QVERIFY(probe.waitForFinished(10000));QCOMPARE(probe.exitCode(),0);const auto streams=QJsonDocument::fromJson(probe.readAllStandardOutput()).object().value("streams").toArray();QCOMPARE(streams.size(),1);const auto stream=streams.first().toObject();
        QCOMPARE(stream.value("pix_fmt").toString(),pixelFormat);QCOMPARE(stream.value("color_transfer").toString(),transferName);QCOMPARE(stream.value("color_primaries").toString(),transfer==1?QString("bt709"):QString("bt2020"));QCOMPARE(stream.value("color_space").toString(),transfer==1?QString("bt709"):QString("bt2020nc"));QCOMPARE(stream.value("color_range").toString(),QString("tv"));
        QWidget directSurface,vsSurface;for(auto* surface:{&directSurface,&vsSurface}){surface->resize(320,240);surface->show();QVERIFY(QTest::qWaitForWindowExposed(surface));}
        ThreeFpApi api;QVERIFY(api.available());ThreeFpPlayer direct(api,&directSurface),throughVs(api,&vsSurface);QVERIFY(direct.setDecodeMode(1));QVERIFY(direct.setScalingAlgorithms(ThreeFpScalingAlgorithm::Bilinear,ThreeFpScalingAlgorithm::Bilinear));QVERIFY(throughVs.setScalingAlgorithms(ThreeFpScalingAlgorithm::Bilinear,ThreeFpScalingAlgorithm::Bilinear));
        QVERIFY2(direct.openFile(media),qPrintable(direct.lastError()));QTRY_COMPARE_WITH_TIMEOUT(direct.snapshot().state,ThreeFpState::Ready,15000);
        VapourSynthFrameServer server;QString error;QString source=media;source.replace("\\","/");source.replace("'","\\'");const auto script=QString("import vapoursynth as vs\nc=vs.core.vsrsource.Source(source='%1')\nc.set_output()\n").arg(source);
        for(int index:{3,0,5}) {
            QVERIFY(direct.seekFrame(index));QTRY_COMPARE_WITH_TIMEOUT(direct.snapshot().frameIndex,qint64(index),10000);QTRY_VERIFY(direct.snapshot().presentedVideoFrames>0);
            VapourSynthFrame frame;QVERIFY2(readVsFrame(server,script,frame,error,index),qPrintable(error));QCOMPARE(frame.colorTransfer,unsigned(transfer));QCOMPARE(frame.colorPrimaries,transfer==1?1u:9u);QCOMPARE(frame.colorMatrix,transfer==1?1u:9u);QCOMPARE(frame.colorRange,1u);QCOMPARE(frame.frameIndex,qint64(index));QVERIFY(frame.duration100ns>0);
            const auto original=frame.planes;QVERIFY2(throughVs.submitFrame(frame),qPrintable(throughVs.lastError()));QTRY_VERIFY(throughVs.snapshot().swapChainPresents>0);QCOMPARE(frame.planes,original);
            for(int y:{9,24,39})for(int x:{11,32,51}) {
                VsrSourcePixelProbe sourcePixel{};QVERIFY(direct.sampleSourcePixel(x,y,false,sourcePixel));QCOMPARE(sourcePixel.bitDepth,unsigned(bits));
                for(int plane=0;plane<3;++plane){const int px=plane&&subsampled?x/2:x,py=plane&&subsampled?y/2:y;const auto* row=reinterpret_cast<const quint16*>(frame.planes[plane].constData()+py*frame.strides[plane]);QCOMPARE(sourcePixel.values[plane],unsigned(row[px]));}
                ThreeFpPixelProbe a{},b{};QVERIFY(direct.samplePixel((x+.5)*directSurface.width()*directSurface.devicePixelRatioF()/64,(y+.5)*directSurface.height()*directSurface.devicePixelRatioF()/48,a));QVERIFY(throughVs.samplePixel((x+.5)*vsSurface.width()*vsSurface.devicePixelRatioF()/64,(y+.5)*vsSurface.height()*vsSurface.devicePixelRatioF()/48,b));QCOMPARE(a.colorMode,b.colorMode);QCOMPARE(a.outputBitDepth,b.outputBitDepth);
                const double tolerance=a.outputBitDepth==16?.002:1.1/(a.outputBitDepth==10?1023:255);
                QVERIFY2(std::abs(a.red-b.red)<=tolerance&&std::abs(a.green-b.green)<=tolerance&&std::abs(a.blue-b.blue)<=tolerance,qPrintable(QString("frame %1 source(%2,%3) direct(%4,%5,%6) VS(%7,%8,%9)").arg(index).arg(x).arg(y).arg(a.red).arg(a.green).arg(a.blue).arg(b.red).arg(b.green).arg(b.blue)));
            }
        }
    }
    void nativeVsSourceHdrMetadata() {
        QTemporaryDir directory;QVERIFY(directory.isValid());const auto media=directory.filePath("mastering.mkv");const QString ffmpeg=QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");QVERIFY(QFileInfo::exists(ffmpeg));
        QProcess encode;encode.start(ffmpeg,{"-hide_banner","-loglevel","error","-y","-f","lavfi","-i","color=c=gray:size=96x64:rate=24:duration=0.25","-pix_fmt","yuv420p10le","-c:v","libx265","-preset","ultrafast","-x265-params","pools=1:frame-threads=1:master-display=G(8500,39850)B(6550,2300)R(35400,14600)WP(15635,16450)L(12000000,50):max-cll=900,300","-color_primaries","bt2020","-color_trc","smpte2084","-colorspace","bt2020nc",media});
        QVERIFY(encode.waitForFinished(20000));QVERIFY2(encode.exitCode()==0,encode.readAllStandardError().constData());
        QString source=media;source.replace("\\","/");source.replace("'","\\'");const auto prefix=QString("import vapoursynth as vs\nc=vs.core.vsrsource.Source(source='%1')\n").arg(source);
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;QVERIFY2(readVsFrame(server,prefix+"c.set_output()\n",frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.flags,7u);QCOMPARE(frame.colorMetadata.maxLuminance,1200.0);QVERIFY(std::abs(frame.colorMetadata.minLuminance-.005)<.000001);QCOMPARE(frame.colorMetadata.maxCll,900u);QCOMPARE(frame.colorMetadata.maxFall,300u);QVERIFY(!frame.colorSideData.isEmpty());
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY(p.setColorSettings(advanced()));QVERIFY2(p.submitFrame(frame),qPrintable(p.lastError()));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);QCOMPARE(p.colorStatus().masteringPeak,1200.0f);QCOMPARE(p.colorStatus().maxCll,900.0f);
        const auto edited=prefix+"c=vs.core.std.SetFrameProps(c,ContentLightLevelMax=700)\nc.set_output()\n";
        QVERIFY2(readVsFrame(server,edited,frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.maxCll,700u);QVERIFY(!frame.colorSideData.isEmpty());QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().maxCll,700.0f);
        // A real conversion changes CICP; stale source HDR side data must not be
        // restored merely because VS copied private properties from its input.
        const auto converted=prefix+"c=vs.core.resize.Bicubic(c,format=vs.RGBS,matrix_in=9,transfer_in=16,transfer=13,primaries_in=9,primaries=1)\nc.set_output()\n";
        QVERIFY2(readVsFrame(server,converted,frame,error),qPrintable(error));QCOMPARE(frame.colorTransfer,13u);QCOMPARE(frame.colorPrimaries,1u);QCOMPARE(frame.colorMetadata.flags,0u);QVERIFY(frame.colorSideData.isEmpty());
        QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().sourceTransfer,13u);QCOMPARE(p.colorStatus().maxCll,0.0f);QCOMPARE(p.colorStatus().maxFall,0.0f);
    }
    void vsFloatExchange_data() {
        QTest::addColumn<QString>("format");QTest::addColumn<int>("transfer");
        for(const QString format:{QString("RGBH"),QString("RGBS"),QString("GRAYH"),QString("GRAYS")})
            for(int transfer:{8,13})QTest::newRow(qPrintable(format+QString::number(transfer)))<<format<<transfer;
    }
    void vsFloatExchange() {
        QFETCH(QString,format);QFETCH(int,transfer);
        const bool rgb=format.startsWith("RGB"),half=format.endsWith("H");
        const Rgb values=rgb?Rgb{.25,.5,.75}:Rgb{.25,.25,.25};
        const QString script=QString("import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.%1,color=%2)\nc=vs.core.std.SetFrameProps(c,_Matrix=0,_Primaries=1,_Transfer=%3,_ColorRange=0)\nc.set_output()\n").arg(format,rgb?"[.25,.5,.75]":"[.25]").arg(transfer);
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;QVERIFY2(readVsFrame(server,script,frame,error),qPrintable(error));
        const auto expectedFormat=rgb?(half?ThreeFpExternalPixelFormat::GbrPF16:ThreeFpExternalPixelFormat::GbrPF32):(half?ThreeFpExternalPixelFormat::GrayF16:ThreeFpExternalPixelFormat::GrayF32);
        QCOMPARE(frame.format,expectedFormat);QCOMPARE(frame.colorTransfer,unsigned(transfer));QCOMPARE(frame.colorRange,2u);
        for(int plane=0;plane<(rgb?3:1);++plane)for(int y=0;y<frame.height;++y)for(int x=0;x<frame.width;++x){
            const auto* row=frame.planes[plane].constData()+y*frame.strides[plane];
            const double actual=half?halfFloat(reinterpret_cast<const quint16*>(row)[x]):reinterpret_cast<const float*>(row)[x];
            QCOMPARE(actual,values[plane]);
        }
        const auto original=frame.planes;
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;QVERIFY(api.available());
        ThreeFpConfiguration config{};config.size=sizeof(config);config.version=api.apiVersion();config.outputWindow=reinterpret_cast<void*>(surface.winId());config.sdrPeakNits=100;config.sdrPaperWhiteNits=203;config.sdrScRgbMode=0;
        void* handle=nullptr;QCOMPARE(api.create(&config,&handle),ThreeFpResult::Success);const auto cleanup=qScopeGuard([&]{api.destroy(handle);});
        ThreeFpExternalVideoFrame native{};native.size=sizeof(native);native.version=2;native.width=frame.width;native.height=frame.height;native.format=frame.format;native.colorRange=frame.colorRange;native.colorMatrix=frame.colorMatrix;native.colorPrimaries=frame.colorPrimaries;native.colorTransfer=frame.colorTransfer;native.duration100ns=400000;native.totalFrames=1;
        const int order[]={rgb?1:0,rgb?2:1,rgb?0:2};for(int i=0;i<(rgb?3:1);++i){native.data[i]=frame.planes[order[i]].constData();native.linesize[i]=frame.strides[order[i]];}
        QCOMPARE(api.submitExternalVideoFrame(handle,&native),ThreeFpResult::Success);
        ThreeFpSnapshot presented{};presented.size=sizeof(presented);QTRY_VERIFY_WITH_TIMEOUT(api.snapshot(handle,&presented)==ThreeFpResult::Success&&presented.swapChainPresents>0,5000);
        ThreeFpPixelProbe pixel{};pixel.size=sizeof(pixel);pixel.x=surface.width()*surface.devicePixelRatioF()/2;pixel.y=surface.height()*surface.devicePixelRatioF()/2;QCOMPARE(api.readPixel(handle,&pixel),ThreeFpResult::Success);QCOMPARE(pixel.colorMode,0u);
        const Rgb sample={pixel.red,pixel.green,pixel.blue};for(int i=0;i<3;++i)QVERIFY2(std::abs(sample[i]-(transfer==8?srgbEncode(values[i]):values[i]))<2.0/(pixel.outputBitDepth==10?1023:255),qPrintable(QString("float plane %1 actual %2").arg(i).arg(sample[i])));
        QCOMPARE(frame.planes,original);
    }
    void vsFloatOutOfRangePreserved() {
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;
        const QString script="import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.RGBS)\nc=vs.core.std.Expr(c,expr=['-0.125','0.25','1.5'])\nc=vs.core.std.SetFrameProps(c,_Matrix=0,_Primaries=1,_Transfer=8,_ColorRange=0)\nc.set_output()\n";
        QVERIFY2(readVsFrame(server,script,frame,error),qPrintable(error));QCOMPARE(frame.format,ThreeFpExternalPixelFormat::GbrPF32);
        const float values[]={-.125f,.25f,1.5f};for(int plane=0;plane<3;++plane)for(int y=0;y<frame.height;++y)for(int x=0;x<frame.width;++x)QCOMPARE(reinterpret_cast<const float*>(frame.planes[plane].constData()+y*frame.strides[plane])[x],values[plane]);
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);const auto original=frame.planes;QVERIFY2(p.submitFrame(frame),qPrintable(p.lastError()));QTRY_VERIFY(p.snapshot().swapChainPresents>0);QCOMPARE(frame.planes,original);
    }
    void vsStaticMetadataAndClear() {
        VapourSynthFrameServer server;VapourSynthFrame frame;QString error;
        const QString source="import vapoursynth as vs\nc=vs.core.std.BlankClip(width=64,height=48,length=1,format=vs.YUV444P16,color=[30000,32768,32768])\nc=vs.core.std.SetFrameProps(c,_Matrix=9,_Primaries=9,_Transfer=16,_ColorRange=1,MasteringDisplayPrimariesX=[.708,.170,.131],MasteringDisplayPrimariesY=[.292,.797,.046],MasteringDisplayWhitePointX=.3127,MasteringDisplayWhitePointY=.3290,MasteringDisplayMinLuminance=.005,MasteringDisplayMaxLuminance=1200.0,ContentLightLevelMax=900,ContentLightLevelAverage=300)\n";
        QVERIFY2(readVsFrame(server,source+"c.set_output()\n",frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.flags,7u);QCOMPARE(frame.colorMetadata.maxLuminance,1200.0);QCOMPARE(frame.colorMetadata.minLuminance,.005);QCOMPARE(frame.colorMetadata.maxCll,900u);QCOMPARE(frame.colorMetadata.maxFall,300u);QCOMPARE(frame.colorMetadata.primaryX[0],.708);QCOMPARE(frame.colorMetadata.whiteY,.3290);
        QWidget surface;surface.resize(160,120);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY(p.setColorSettings(advanced()));QVERIFY2(p.submitFrame(frame),qPrintable(p.lastError()));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
        const auto status=p.colorStatus();QVERIFY(std::abs(status.masteringPeak-1200)<.1);QVERIFY(std::abs(status.masteringBlack-.005)<.0001);QCOMPARE(status.maxCll,900.0f);QCOMPARE(status.maxFall,300.0f);
        const QString stripped=source+"c=vs.core.std.RemoveFrameProps(c,props=['MasteringDisplayPrimariesX','MasteringDisplayPrimariesY','MasteringDisplayWhitePointX','MasteringDisplayWhitePointY','MasteringDisplayMinLuminance','MasteringDisplayMaxLuminance','ContentLightLevelMax','ContentLightLevelAverage'])\nc=vs.core.std.SetFrameProps(c,_Matrix=1,_Primaries=1,_Transfer=13)\nc.set_output()\n";
        QVERIFY2(readVsFrame(server,stripped,frame,error),qPrintable(error));QCOMPARE(frame.colorMetadata.flags,0u);QVERIFY(frame.colorSideData.isEmpty());QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().sourceTransfer,13u);QCOMPARE(p.colorStatus().maxCll,0.0f);QCOMPARE(p.colorStatus().maxFall,0.0f);QVERIFY(p.colorStatus().masteringPeak<200);
    }
    void nativeReferenceColors_data() {
        QTest::addColumn<bool>("rgb");QTest::addColumn<int>("matrix");QTest::addColumn<int>("transfer");QTest::addColumn<int>("primaries");QTest::addColumn<int>("bits");QTest::addColumn<int>("c0");QTest::addColumn<int>("c1");QTest::addColumn<int>("c2");
        QTest::newRow("linear-gray")<<true<<0<<8<<1<<8<<128<<128<<128;
        QTest::newRow("linear-rgb16")<<true<<0<<8<<1<<16<<30000<<14000<<7000;
        QTest::newRow("srgb-rgb")<<true<<0<<13<<1<<8<<180<<100<<60;
        QTest::newRow("gamma22")<<true<<0<<4<<1<<8<<180<<100<<60;
        QTest::newRow("gamma28")<<true<<0<<5<<1<<8<<180<<100<<60;
        QTest::newRow("display-p3")<<true<<0<<13<<12<<8<<180<<100<<60;
        QTest::newRow("rec2020-srgb")<<true<<0<<13<<9<<8<<150<<120<<100;
        for(int matrix:{1,4,5,6,7,8})QTest::newRow(qPrintable(QString("yuv-matrix-%1").arg(matrix)))<<false<<matrix<<1<<1<<8<<128<<110<<145;
        QTest::newRow("2020-ncl")<<false<<9<<1<<9<<8<<128<<100<<155;
        QTest::newRow("2020-cl")<<false<<10<<1<<9<<8<<128<<100<<155;
    }
    void nativeReferenceColors() {
        QFETCH(bool,rgb);QFETCH(int,matrix);QFETCH(int,transfer);QFETCH(int,primaries);QFETCH(int,bits);QFETCH(int,c0);QFETCH(int,c1);QFETCH(int,c2);
        QWidget surface;surface.resize(64,48);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));
        ThreeFpConfiguration config{};config.size=sizeof(config);config.version=api.apiVersion();config.outputWindow=reinterpret_cast<void*>(surface.winId());config.sdrPeakNits=100;config.sdrPaperWhiteNits=203;config.sdrScRgbMode=0;
        void *handle=nullptr;QCOMPARE(api.create(&config,&handle),ThreeFpResult::Success);const auto cleanup=qScopeGuard([&]{api.destroy(handle);});
        QCOMPARE(api.setScalingAlgorithms(handle,ThreeFpScalingAlgorithm::Bilinear,ThreeFpScalingAlgorithm::Bilinear),ThreeFpResult::Success);
        QByteArray planes[3];const int values[]={c0,c1,c2};
        for(int i=0;i<3;++i){planes[i].resize(64*48*(bits/8));if(bits==8)planes[i].fill(char(values[i]));else std::fill_n(reinterpret_cast<quint16*>(planes[i].data()),64*48,quint16(values[i]));}
        ThreeFpExternalVideoFrame frame{};frame.size=sizeof(frame);frame.version=1;frame.width=64;frame.height=48;frame.format=rgb?(bits==8?ThreeFpExternalPixelFormat::GbrP8:ThreeFpExternalPixelFormat::GbrP16):ThreeFpExternalPixelFormat::Yuv444P8;frame.colorRange=rgb?2:1;frame.colorMatrix=matrix;frame.colorTransfer=transfer;frame.colorPrimaries=primaries;frame.totalFrames=1;frame.duration100ns=400000;
        const int order[]={rgb?1:0,rgb?2:1,rgb?0:2};for(int i=0;i<3;++i){frame.data[i]=planes[order[i]].constData();frame.linesize[i]=64*(bits/8);}
        QCOMPARE(api.submitExternalVideoFrame(handle,&frame),ThreeFpResult::Success);
        ThreeFpSnapshot presented{};presented.size=sizeof(presented);
        QTRY_VERIFY_WITH_TIMEOUT(api.snapshot(handle,&presented)==ThreeFpResult::Success && presented.swapChainPresents>0,5000);
        ThreeFpPixelProbe actual{};actual.size=sizeof(actual);actual.version=1;
        actual.x=std::uint32_t(surface.width()*surface.devicePixelRatioF()/2);
        actual.y=std::uint32_t(surface.height()*surface.devicePixelRatioF()/2);
        QCOMPARE(api.readPixel(handle,&actual),ThreeFpResult::Success);QCOMPARE(actual.colorMode,0u);
        const double maximum=bits==8?255:65535;const Rgb expected=referenceColor(rgb,matrix,transfer,primaries,{c0/maximum,c1/maximum,c2/maximum});
        const Rgb sample={actual.red,actual.green,actual.blue};const double tolerance=2.0/(actual.outputBitDepth==10?1023:255);
        for(int i=0;i<3;++i)QVERIFY2(std::abs(sample[i]-expected[i])<=tolerance,qPrintable(QString("channel %1 actual %2 reference %3 tolerance %4").arg(i).arg(sample[i],0,'g',10).arg(expected[i],0,'g',10).arg(tolerance)));
    }
    void upstreamPassthroughSeamAndSnapshot(){
        QWidget surface;surface.resize(160,90);surface.show();ThreeFpApi api;QVERIFY(api.available());
        ThreeFpConfiguration config{};config.size=sizeof(config);config.version=api.apiVersion();config.outputWindow=reinterpret_cast<void*>(surface.winId());config.sdrPeakNits=100;config.sdrPaperWhiteNits=203;
        void *handle=nullptr;QCOMPARE(api.create(&config,&handle),ThreeFpResult::Success);QVERIFY(handle);const auto destroy=qScopeGuard([&]{api.destroy(handle);});
        ThreeFpSnapshot snapshot{};snapshot.size=sizeof(snapshot);QCOMPARE(api.snapshot(handle,&snapshot),ThreeFpResult::Success);QCOMPARE(snapshot.version,23u);QCOMPARE(snapshot.size,608u);QCOMPARE(snapshot.audioPassthroughActive,0u);
        QLibrary native(api.libraryPath());QVERIFY(native.load());using Mode=ThreeFpResult(*)(void*,unsigned);using State=ThreeFpResult(*)(void*,unsigned*,unsigned*,unsigned*,char*,unsigned,quint64*);
        const auto mode=reinterpret_cast<Mode>(native.resolve("FFF3FP_SetAudioPassthroughMode"));const auto state=reinterpret_cast<State>(native.resolve("FFF3FP_GetAudioPassthroughState"));QVERIFY(mode && state);
        QCOMPARE(mode(handle,3),ThreeFpResult::InvalidArgument);QCOMPARE(mode(handle,2),ThreeFpResult::Success);
        unsigned active=99,preference=99,codec=99;quint64 bursts=99;char name[128]{};QCOMPARE(state(handle,&active,&preference,&codec,name,sizeof(name),&bursts),ThreeFpResult::Success);QCOMPARE(active,0u);QCOMPARE(preference,2u);QCOMPARE(codec,0u);QCOMPARE(bursts,quint64(0));
        QCOMPARE(mode(handle,0),ThreeFpResult::Success);
    }
    void nativeUploadTelemetry() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer player(api,&surface);
        const auto frame=yuv(1,1,1,128,128,128);
        QVERIFY2(player.submitFrame(frame),qPrintable(player.lastError()));
        QTRY_VERIFY(player.snapshot().swapChainPresents>0);
        const auto first=player.snapshot();
        QCOMPARE(first.size,std::uint32_t(sizeof(ThreeFpSnapshot)));QCOMPARE(first.version,23u);
        QCOMPARE(first.uploadPath,2u);QCOMPARE(first.uploadCount,std::uint64_t(1));
        QCOMPARE(first.uploadCopyBytes,std::uint64_t(64*48*3));
        QVERIFY(first.videoUpload100ns>=first.uploadCopy100ns);
        player.redraw();QTRY_VERIFY(player.snapshot().swapChainPresents>first.swapChainPresents);
        QCOMPARE(player.snapshot().uploadCount,first.uploadCount);
        QVERIFY(player.submitFrame(frame));
        QCOMPARE(player.snapshot().uploadCount,first.uploadCount+1);
        QCOMPARE(player.snapshot().uploadCopyBytes,first.uploadCopyBytes*2);
        ThreeFpPixelProbe pixel{};QVERIFY(player.samplePixel(160,120,pixel));
        QVERIFY(pixel.red>.45f && pixel.red<.60f);QVERIFY(std::abs(pixel.red-pixel.green)<.01f);
    }

    void nativeDefaultAndPausedColorSwitch() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer p(api,&surface);
        auto frame=yuv(1,1,1,128,128,128);QVERIFY(p.submitFrame(frame));QTRY_VERIFY(p.snapshot().swapChainPresents>0);
        QCOMPARE(p.colorStatus().requestedEngine,0u);QCOMPARE(p.colorStatus().activeEngine,0u);
        const auto original=frame.planes;
        QVERIFY(p.setColorSettings(advanced()));
        QTRY_VERIFY_WITH_TIMEOUT(p.colorStatus().activeEngine==1,20000);
        QVERIFY2(p.colorStatus().activeEngine==1,p.colorStatus().fallback);
        QCOMPARE(frame.planes,original);QCOMPARE(p.colorStatus().sourceKind,2u);QCOMPARE(p.colorStatus().sourceMatrix,1u);
        ThreeFpPixelProbe sample{};QVERIFY(p.samplePixel(160,120,sample));qInfo()<<"709 gray"<<sample.red<<sample.green<<sample.blue;
        QVERIFY(std::abs(sample.red-sample.green)<.01);QVERIFY(sample.red>.45 && sample.red<.60);
        const auto count=p.colorStatus().renderedFrames;for(int i=0;i<5;++i){p.redraw();QTest::qWait(40);}
        QCOMPARE(p.colorStatus().renderedFrames,count);QVERIFY(p.colorStatus().cachedPresents>0);
        surface.resize(400,300);p.redraw();QTRY_VERIFY(p.colorStatus().renderedFrames>count);
        auto native=VsrColorDefaultSettings();QVERIFY(p.setColorSettings(native));QTRY_COMPARE(p.colorStatus().activeEngine,0u);
        QVERIFY(p.samplePixel(200,150,sample));QVERIFY(sample.red>.45 && sample.red<.60);
    }
    void automaticHdrToSdr() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;QVERIFY2(api.available(),qPrintable(api.errorString()));ThreeFpPlayer p(api,&surface);
        auto c=advanced();c.engine=2;QVERIFY(p.setColorSettings(c));
        auto frame=yuv(1,1,1,128,128,128);QVERIFY(p.submitFrame(frame));QTRY_VERIFY(p.snapshot().swapChainPresents>0);
        QCOMPARE(p.colorStatus().requestedEngine,2u);QCOMPARE(p.colorStatus().activeEngine,0u);
        ThreeFpPixelProbe sdr{};QVERIFY(p.samplePixel(160,120,sdr));QVERIFY(sdr.red>.45 && sdr.red<.60);
        for(const unsigned transfer:{16u,18u}) {
            frame=yuv(9,transfer,9,40000,32768,32768,16);QVERIFY(p.submitFrame(frame));
            QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
            QTRY_COMPARE(p.colorStatus().sourceTransfer,transfer);QCOMPARE(p.colorStatus().requestedEngine,2u);
            QTest::qWait(100);ThreeFpPixelProbe mapped{};QVERIFY(p.samplePixel(160,120,mapped));
            QVERIFY(std::isfinite(mapped.red));QVERIFY(mapped.red>0 && mapped.red<=1);QCOMPARE(p.colorStatus().outputHdr,0u);
            c.engine=1;QVERIFY(p.setColorSettings(c));QTest::qWait(100);ThreeFpPixelProbe custom{};QVERIFY(p.samplePixel(160,120,custom));
            QVERIFY(std::abs(mapped.red-custom.red)<.01f);
            c.engine=2;QVERIFY(p.setColorSettings(c));
        }
        c=VsrColorDefaultSettings();c.engine=2;c.output=1;QVERIFY(p.setColorSettings(c));
        frame=yuv(9,16,9,40000,32768,32768,16);QVERIFY(p.submitFrame(frame));
        QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);QCOMPARE(p.colorStatus().outputHdr,0u);
        ThreeFpPixelProbe defaults{};QVERIFY(p.samplePixel(160,120,defaults));
        QVERIFY(std::isfinite(defaults.red));QVERIFY(defaults.red>0 && defaults.red<=1);
        frame=yuv(1,1,1,128,128,128);QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().activeEngine,0u);
        QVERIFY(p.samplePixel(160,120,sdr));QVERIFY(sdr.red>.45 && sdr.red<.60);
    }
    void constantLuminanceAndHdr() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY(p.setColorSettings(advanced()));
        ThreeFpPixelProbe ncl{},cl{};
        auto frame=yuv(9,1,9,128,100,155);QVERIFY(p.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(p.colorStatus().activeEngine==1,20000);QTRY_VERIFY_WITH_TIMEOUT(p.samplePixel(160,120,ncl),5000);
        frame.colorMatrix=10;QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().sourceMatrix,10u);QTest::qWait(100);QVERIFY(p.samplePixel(160,120,cl));
        qInfo()<<"2020 NCL"<<ncl.red<<ncl.green<<ncl.blue<<"CL"<<cl.red<<cl.green<<cl.blue;
        QVERIFY(std::abs(cl.green-ncl.green)>.01 || std::abs(cl.red-ncl.red)>.01);
        QVERIFY(p.setColorSettings(VsrColorDefaultSettings()));QTRY_COMPARE(p.colorStatus().activeEngine,0u);
        QTest::qWait(100);ThreeFpPixelProbe nativeCl{};QVERIFY(p.samplePixel(160,120,nativeCl));
        qInfo()<<"native CL"<<nativeCl.red<<nativeCl.green<<nativeCl.blue;
        frame.colorMatrix=9;QVERIFY(p.submitFrame(frame));QTest::qWait(100);ThreeFpPixelProbe nativeNcl{};QVERIFY(p.samplePixel(160,120,nativeNcl));
        QVERIFY(std::abs(nativeCl.green-nativeNcl.green)>.01 || std::abs(nativeCl.red-nativeNcl.red)>.01);
        QVERIFY(p.setColorSettings(advanced()));
        frame=yuv(9,16,9,40000,32768,32768,16);QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().sourceTransfer,16u);
        QTest::qWait(100);QVERIFY(p.samplePixel(160,120,cl));QVERIFY(std::isfinite(cl.red));QVERIFY(cl.red>0 && cl.red<=1);
        QCOMPARE(p.colorStatus().outputHdr,0u);QCOMPARE(p.colorStatus().sourceBits,16u);
        frame.colorTransfer=18;QVERIFY(p.submitFrame(frame));QTRY_COMPARE(p.colorStatus().sourceTransfer,18u);QTest::qWait(100);QVERIFY(p.samplePixel(160,120,cl));QVERIFY(std::isfinite(cl.red));
    }
    void invalidProfileFallsBackAndRecovers() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer p(api,&surface);auto c=advanced();c.icc=2;qstrncpy(c.iccPath,"Z:/missing-profile.icc",sizeof(c.iccPath));QVERIFY(p.setColorSettings(c));
        auto frame=yuv(1,1,1,128,128,128);QVERIFY(p.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(p.colorStatus().fallback[0],20000);
        QCOMPARE(p.colorStatus().activeEngine,0u);QTRY_VERIFY_WITH_TIMEOUT(p.snapshot().swapChainPresents>0,5000);
        QVERIFY(p.setColorSettings(advanced()));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
        c=advanced();qstrncpy(c.lutPath,"Z:/missing-lut.cube",sizeof(c.lutPath));QVERIFY(p.setColorSettings(c));QTRY_COMPARE(p.colorStatus().activeEngine,0u);
        QVERIFY(p.submitFrame(frame));QTest::qWait(150);QCOMPARE(p.colorStatus().activeEngine,0u);
        QVERIFY(p.setColorSettings(advanced()));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
    }
    void customIccLutAndRgbOrder() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer p(api,&surface);QTemporaryDir dir;
        QFile profile(dir.filePath("显示器.icc"));QVERIFY(profile.open(QIODevice::WriteOnly));profile.write(QColorSpace(QColorSpace::SRgb).iccProfile());profile.close();
        auto c=advanced();c.icc=2;qstrncpy(c.iccPath,profile.fileName().toUtf8().constData(),sizeof(c.iccPath));QVERIFY(p.setColorSettings(c));
        VapourSynthFrame f;f.width=64;f.height=48;f.totalFrames=1;f.duration100ns=400000;f.format=ThreeFpExternalPixelFormat::GbrP8;
        f.colorMatrix=0;f.colorTransfer=13;f.colorPrimaries=1;f.colorRange=2;
        for(int i=0;i<3;++i){f.strides[i]=64;f.planes[i]=QByteArray(64*48,i==0?char(220):char(20));}
        QVERIFY(p.submitFrame(f));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);QCOMPARE(p.colorStatus().iccState,1u);
        ThreeFpPixelProbe sample{};QTRY_VERIFY_WITH_TIMEOUT(p.samplePixel(160,120,sample),5000);qInfo()<<"RGB source"<<sample.red<<sample.green<<sample.blue;
        QVERIFY(sample.red>sample.blue+.5f);QVERIFY(sample.red>sample.green+.5f);
        QVERIFY(std::abs(sample.red-220/255.0f)<.04f);QVERIFY(std::abs(sample.green-20/255.0f)<.04f);QVERIFY(std::abs(sample.blue-20/255.0f)<.04f);
        QVERIFY(p.setColorSettings(advanced()));QTRY_COMPARE(p.colorStatus().iccState,0u);QVERIFY(p.samplePixel(160,120,sample));qInfo()<<"RGB without ICC"<<sample.red<<sample.green<<sample.blue;
        QVERIFY(p.setColorSettings(VsrColorDefaultSettings()));QTRY_COMPARE(p.colorStatus().activeEngine,0u);QVERIFY(p.samplePixel(160,120,sample));qInfo()<<"RGB native"<<sample.red<<sample.green<<sample.blue;
        QFile cube(dir.filePath("identity.cube"));QVERIFY(cube.open(QIODevice::WriteOnly));cube.write("LUT_3D_SIZE 2\n0 0 0\n1 0 0\n0 1 0\n1 1 0\n0 0 1\n1 0 1\n0 1 1\n1 1 1\n");cube.close();
        qstrncpy(c.lutPath,cube.fileName().toUtf8().constData(),sizeof(c.lutPath));QVERIFY(p.setColorSettings(c));QTRY_COMPARE(p.colorStatus().lutActive,1u);
    }
    void decodedHardwareNoReadback() {
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));
        ThreeFpApi api;ThreeFpPlayer p(api,&surface);QVERIFY(p.setColorSettings(advanced()));
        QTemporaryDir dir;const auto path=dir.filePath("hardware.mkv");QVERIFY(QFile::copy(":/startup/warmup.mkv",path));
        QVERIFY(p.openFile(path));QTRY_COMPARE_WITH_TIMEOUT(p.snapshot().state,ThreeFpState::Ready,15000);
        QVERIFY(p.seekFrame(0));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
        QCOMPARE(p.colorStatus().sourceKind,1u);QCOMPARE(p.snapshot().hardwareTransfer100ns,quint64(0));
        QCOMPARE(p.snapshot().softwareConvert100ns,quint64(0));qInfo()<<"Decode mode"<<p.snapshot().decodeMode;
    }
    void realVsOutputPreserved() {
        VapourSynthFrameServer server;QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),20000);QVERIFY2(server.available(),qPrintable(server.errorString()));
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        server.loadScript("import vapoursynth as vs\nclip=vs.core.std.BlankClip(width=64,height=48,format=vs.YUV444P16,color=[30000,32768,32768],length=10)\nclip=vs.core.std.SetFrameProps(clip,_Matrix=9,_Primaries=9,_Transfer=16,_ColorRange=1)\nclip=vs.core.resize.Bicubic(clip,width=96,height=72)\nclip.set_output()\n","color-vs-test.vpy");
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty()||!errors.isEmpty(),20000);QVERIFY(errors.isEmpty());server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),10000);const auto frame=qvariant_cast<VapourSynthFrame>(frames.first().first());
        const auto bytes=frame.planes;QCOMPARE(frame.colorMatrix,9u);QCOMPARE(frame.colorTransfer,16u);
        QWidget surface;surface.resize(320,240);surface.show();QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpApi api;ThreeFpPlayer p(api,&surface);
        QVERIFY(p.setColorSettings(advanced()));QVERIFY(p.submitFrame(frame));QTRY_COMPARE_WITH_TIMEOUT(p.colorStatus().activeEngine,1u,20000);
        QCOMPARE(p.colorStatus().sourceKind,2u);QCOMPARE(p.colorStatus().sourceTransfer,16u);QCOMPARE(frame.planes,bytes);
    }
};
QTEST_MAIN(TestColor)
#include "TestColor.moc"
