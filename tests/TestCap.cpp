#include "cap/DesktopCapture.h"
#include <QApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <QImageReader>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDir>
#include <QColorSpace>
#include <QTest>
#include "image/HdrPreview.h"

class TestCap : public QObject {
    Q_OBJECT
private slots:
    void hdrPngKeepsPrecisionAndLuminance() {
        QTemporaryDir dir;
        QImage raw(3,1,QImage::Format_RGBA32FPx4);
        auto *p=reinterpret_cast<float*>(raw.bits());
        const float values[]={1,1,1,1,12.5f,12.5f,12.5f,1,.125f,.125f,.125f,1};
        memcpy(p,values,sizeof(values));raw.setColorSpace(QColorSpace::SRgbLinear);raw.setText("capHdr","scRGB-FP16");
        const auto path=dir.filePath("hdr.png");QVERIFY(cap::savePng(raw,path));
        const auto png=QImageReader(path).read();QCOMPARE(png.depth(),64);
        // ST 2084: 80 nits -> 0.4858568; 1000 nits -> 0.7518271.
        QVERIFY(qAbs(png.pixelColor(0,0).redF()-.4858568)<.00003);
        QVERIFY(qAbs(png.pixelColor(1,0).redF()-.7518271)<.00003);
        QVERIFY(png.pixelColor(1,0).redF()>png.pixelColor(0,0).redF());
        QVERIFY(png.colorSpace().isValid());
        QVERIFY(!QFile::exists(path+".scrgb-f32"));
        QVERIFY(!QFile::exists(path+".json"));
        QCOMPARE(png.colorSpace(),QColorSpace(QColorSpace::Bt2100Pq));
        const auto preview=cap::preview(raw);QCOMPARE(preview.depth(),64);
        const auto display=vsr::imageForSdrDisplay(png);
        for(int x=0;x<3;++x)QVERIFY(qAbs(preview.pixelColor(x,0).redF()-display.pixelColor(x,0).redF())<.0001);
    }
    void sdrCompatiblePng() {
        QTemporaryDir dir;QImage raw(8,8,QImage::Format_RGBA8888);raw.fill(QColor(23,47,91));
        raw.setText("capHdr","SDR");const auto path=dir.filePath("sdr.png");
        QVERIFY(cap::savePng(raw,path));const auto image=QImageReader(path).read();
        QCOMPARE(image.depth(),64);
        QCOMPARE(image.pixelColor(0,0),raw.pixelColor(0,0));
        QVERIFY(!QFile::exists(path+".scrgb-f32"));
    }
    void hdrSdrCompanionUsesIntegerPixels() {
        QTemporaryDir dir;QImage raw(3,2,QImage::Format_RGBA32FPx4);
        raw.fill(Qt::white);raw.setColorSpace(QColorSpace::SRgbLinear);raw.setText("capHdr","scRGB-FP16");
        const auto preview=cap::preview(raw);QCOMPARE(preview.depth(),64);QVERIFY(preview.text("capHdr").isEmpty());
        const auto path=dir.filePath("capture_SDR.png");QVERIFY(cap::savePng(preview,path));
        const auto saved=QImageReader(path).read();QCOMPARE(saved.depth(),64);
        QCOMPARE(saved.pixelColor(2,1),preview.pixelColor(2,1));
        QCOMPARE(saved.colorSpace(),QColorSpace(QColorSpace::SRgb));
        QVERIFY(!QFile::exists(path+".scrgb-f32"));QVERIFY(!QFile::exists(path+".json"));
    }
    void pngActuallyCompresses() {
        QTemporaryDir dir;QImage image(512,256,QImage::Format_RGBA64);image.fill(QColor(23,47,91));image.setColorSpace(QColorSpace::SRgb);
        const auto path=dir.filePath("compressed.png");QVERIFY(cap::savePng(image,path));
        QVERIFY(QFileInfo(path).size()<image.sizeInBytes()/50);
        QCOMPARE(QImageReader(path).read(),image);
    }
    void realHdrFixture() {
        const auto source=qEnvironmentVariable("VSR_CAP_HDR_FIXTURE");if(source.isEmpty())QSKIP("Set VSR_CAP_HDR_FIXTURE for captured HDR file validation.");
        QImage reference(source);QVERIFY(!reference.isNull());
        QFile floating(source+".scrgb-f32");QVERIFY(floating.open(QIODevice::ReadOnly));const auto bytes=floating.readAll();
        QImage raw(reference.size(),QImage::Format_RGBA32FPx4);QCOMPARE(bytes.size(),raw.sizeInBytes());memcpy(raw.bits(),bytes.constData(),size_t(bytes.size()));
        raw.setColorSpace(QColorSpace::SRgbLinear);raw.setText("capHdr","scRGB-FP16");
        const auto output=QDir::current().filePath("build/audit-hdr-20261017");QVERIFY(QDir().mkpath(output));
        QElapsedTimer timer;timer.start();const auto hdrPath=output+"/capture.png";QVERIFY(cap::savePng(raw,hdrPath));const auto hdrMs=timer.elapsed();
        const auto mapped=cap::preview(raw);const auto previewMs=timer.elapsed()-hdrMs;
        const auto sdrPath=output+"/capture_SDR.png";QVERIFY(cap::savePng(mapped,sdrPath));const auto totalMs=timer.elapsed();
        const auto hdr=QImageReader(hdrPath).read();QCOMPARE(hdr,reference);QCOMPARE(hdr.colorSpace(),QColorSpace(QColorSpace::Bt2100Pq));
        const auto display=vsr::imageForSdrDisplay(hdr);double maxError=0;quint64 totalError=0,samples=0;
        for(int y=0;y<hdr.height();++y)for(int x=0;x<hdr.width();x+=13){const auto a=mapped.pixelColor(x,y).rgba64(),b=display.pixelColor(x,y).rgba64();
            for(const int error:{qAbs(int(a.red())-b.red()),qAbs(int(a.green())-b.green()),qAbs(int(a.blue())-b.blue())}){maxError=qMax(maxError,double(error));totalError+=error;++samples;}}
        qInfo()<<"HDR-fixture"<<hdr.size()<<"hdr-bytes"<<QFileInfo(hdrPath).size()<<"sdr-bytes"<<QFileInfo(sdrPath).size()<<"hdr-ms"<<hdrMs<<"preview-ms"<<previewMs<<"total-ms"<<totalMs<<"SDR-player-max-u16-error"<<maxError<<"mean"<<double(totalError)/samples;
        QVERIFY(maxError<128);QVERIFY(QFileInfo(hdrPath).size()<QFileInfo(source).size()/2);
        QVERIFY(!QFileInfo::exists(hdrPath+".json"));QVERIFY(!QFileInfo::exists(hdrPath+".scrgb-f32"));
    }
    void desktopCapture() {
        if(!qEnvironmentVariableIsSet("VSR_CAP_DEVICE_TEST"))QSKIP("Set VSR_CAP_DEVICE_TEST for a live display capture.");
        bool capturedHdr=false;
        for(auto *screen:QApplication::screens()) {
            QString error;const auto image=cap::captureScreen(screen,&error);
            QVERIFY2(!image.isNull(),qPrintable(error));
            qInfo()<<"desktop-capture"<<screen->name()<<image.size()<<image.depth()<<image.text("capHdr");
            capturedHdr|=image.text("capHdr")=="scRGB-FP16" && image.depth()==128;
        }
        if(qEnvironmentVariable("VSR_CAP_DEVICE_TEST")=="HDR") {
            QVERIFY2(capturedHdr,"No active HDR output was available to desktop capture in this session.");
        }
    }
};
QTEST_MAIN(TestCap)
#include "TestCap.moc"
