#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include <QTest>
#include <QWidget>
#include <QTemporaryDir>
#include <QFile>
#include <QSignalSpy>
#include <QColorSpace>
#include <cmath>
using namespace vsr;

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
        auto frame=yuv(9,1,9,128,100,155);QVERIFY(p.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(p.colorStatus().activeEngine==1,20000);QVERIFY(p.samplePixel(160,120,ncl));
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
        QCOMPARE(p.colorStatus().activeEngine,0u);QVERIFY(p.snapshot().swapChainPresents>0);
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
        ThreeFpPixelProbe sample{};QVERIFY(p.samplePixel(160,120,sample));qInfo()<<"RGB source"<<sample.red<<sample.green<<sample.blue;
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
