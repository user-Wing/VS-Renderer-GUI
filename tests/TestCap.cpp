#include "cap/DesktopCapture.h"
#include <QApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <QImageReader>
#include <QFile>
#include <QColorSpace>
#include <QTest>

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
        QFile file(path+".scrgb-f32");QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(),QByteArray(reinterpret_cast<const char*>(values),sizeof(values)));
        QVERIFY(QFile::exists(path+".json"));
        const auto preview=cap::preview(raw);QCOMPARE(preview.depth(),32);
    }
    void sdrCompatiblePng() {
        QTemporaryDir dir;QImage raw(8,8,QImage::Format_RGBA8888);raw.fill(QColor(23,47,91));
        raw.setText("capHdr","SDR");const auto path=dir.filePath("sdr.png");
        QVERIFY(cap::savePng(raw,path));const auto image=QImageReader(path).read();
        QCOMPARE(image.pixelColor(0,0),raw.pixelColor(0,0));
        QVERIFY(!QFile::exists(path+".scrgb-f32"));
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
