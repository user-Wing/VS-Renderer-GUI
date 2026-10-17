#include "cap/DesktopCapture.h"
#include "image/ImageJxl.h"
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
    void jxl16RoundTrip_data() {
        QTest::addColumn<int>("space");
        QTest::newRow("sRGB")<<int(QColorSpace::SRgb);QTest::newRow("PQ")<<int(QColorSpace::Bt2100Pq);QTest::newRow("HLG")<<int(QColorSpace::Bt2100Hlg);
    }
    void jxl16RoundTrip() {
        QFETCH(int,space);QTemporaryDir directory;const auto path=directory.filePath("precision.jxl");
        QImage image(129,49,QImage::Format_RGBA64);image.setColorSpace(QColorSpace(QColorSpace::NamedColorSpace(space)));image.setText("capSdrWhiteNits","480");
        for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgba64*>(image.scanLine(y));for(int x=0;x<image.width();++x)row[x]=QRgba64::fromRgba64(quint16(x*431+y),quint16(y*1133+x),quint16(65535-x*397-y),quint16(x?32000+x*101+y:0));}
        auto error=vsr::writeJxl(path,image);QVERIFY2(error.isEmpty(),qPrintable(error));
        const auto decoded=vsr::readJxl(path,&error);QVERIFY2(!decoded.isNull(),qPrintable(error));QCOMPARE(decoded.size(),image.size());QCOMPARE(decoded.colorSpace(),image.colorSpace());QCOMPARE(decoded.text("sourceBitDepth"),QString("16"));QCOMPARE(decoded.text("capSdrWhiteNits"),QString("480"));
        for(int y=0;y<image.height();++y)QVERIFY(std::memcmp(decoded.constScanLine(y),image.constScanLine(y),image.width()*8)==0);
        QCOMPARE(QDir(directory.path()).entryList(QDir::Files).size(),1);
        QFile file(path);QVERIFY(file.open(QIODevice::ReadWrite));QVERIFY(file.resize(file.size()/2));file.close();QVERIFY(vsr::readJxl(path,&error).isNull());QVERIFY(!error.isEmpty());
    }
    void jxlHdrFixture() {
        const auto root=qEnvironmentVariable("VSR_CAP_WHITE_FIXTURE");if(root.isEmpty())QSKIP("Set VSR_CAP_WHITE_FIXTURE for the supplied HDR screenshot");
        QImageReader reader(QDir(root).filePath(QStringLiteral("过曝.png")));const auto source=reader.read();QVERIFY2(!source.isNull(),qPrintable(reader.errorString()));
        auto image=source.convertToFormat(QImage::Format_RGBA64);image.setColorSpace(QColorSpace::Bt2100Pq);image.setText("capSdrWhiteNits","480");
        const auto output=QStringLiteral("build/audit-jxl-20261017");QVERIFY(QDir().mkpath(output));const auto path=QDir(output).filePath("capture_HDR.jxl");
        QElapsedTimer timer;timer.start();auto error=vsr::writeJxl(path,image);QVERIFY2(error.isEmpty(),qPrintable(error));const auto encodeMs=timer.elapsed();timer.restart();const auto decoded=vsr::readJxl(path,&error);QVERIFY2(!decoded.isNull(),qPrintable(error));
        qInfo()<<"JXL HDR"<<"encode-ms"<<encodeMs<<"decode-ms"<<timer.elapsed()<<"bytes"<<QFileInfo(path).size()<<"original-PNG-bytes"<<QFileInfo(reader.fileName()).size();
        for(int y=0;y<image.height();++y)QVERIFY(std::memcmp(decoded.constScanLine(y),image.constScanLine(y),image.width()*8)==0);
        const auto before=vsr::imageForSdrDisplay(image),after=vsr::imageForSdrDisplay(decoded);QCOMPARE(after,before);
        error=vsr::writeJxl(QDir(output).filePath("capture_SDR.jxl"),before);QVERIFY2(error.isEmpty(),qPrintable(error));
    }
    void desktopWhiteRestoresSdr_data() {
        QTest::addColumn<double>("whiteNits");
        QTest::newRow("80-nit")<<80.0;QTest::newRow("203-nit")<<203.0;QTest::newRow("480-nit")<<480.0;
    }
    void desktopWhiteRestoresSdr() {
        QFETCH(double,whiteNits);
        QImage raw(257,2,QImage::Format_RGBA32FPx4);raw.setColorSpace(QColorSpace::SRgbLinear);raw.setText("capHdr","scRGB-FP16");raw.setText("capSdrWhiteNits",QString::number(whiteNits));
        const auto linear=[](double v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};
        for(int y=0;y<raw.height();++y)for(int x=0;x<raw.width();++x){auto *p=reinterpret_cast<float*>(raw.scanLine(y))+x*4;for(int c=0;c<3;++c)p[c]=float(linear(double((x+c*71)%257)/256)*whiteNits/80);p[3]=.5f;}
        const auto restored=cap::preview(raw);QCOMPARE(restored.colorSpace(),QColorSpace(QColorSpace::SRgb));
        for(int y=0;y<raw.height();++y)for(int x=0;x<raw.width();++x){const auto *p=reinterpret_cast<const quint16*>(restored.constScanLine(y))+x*4;for(int c=0;c<3;++c)QVERIFY(qAbs(int(p[c])-qRound(double((x+c*71)%257)/256*65535))<=1);QCOMPARE(p[3],quint16(32768));}
        QTemporaryDir dir;const auto path=dir.filePath("capture.png");QVERIFY(cap::savePng(raw,path));
        const auto hdr=QImageReader(path).read();QCOMPARE(hdr.text("capSdrWhiteNits"),QString::number(whiteNits));
        const auto pngPreview=cap::preview(hdr);QCOMPARE(pngPreview.colorSpace(),restored.colorSpace());
        for(int x=1;x<257;++x){const auto a=pngPreview.pixelColor(x,0).rgba64(),b=restored.pixelColor(x,0).rgba64();QVERIFY(qAbs(int(a.red())-b.red())<128);QVERIFY(qAbs(int(a.green())-b.green())<128);QVERIFY(qAbs(int(a.blue())-b.blue())<128);}
        auto crop=raw.copy(QRect(50,0,32,2));QCOMPARE(crop.text("capSdrWhiteNits"),QString::number(whiteNits));QCOMPARE(cap::preview(crop),restored.copy(QRect(50,0,32,2)));
    }
    void desktopWhiteFallbackAndHighlights() {
        QImage raw(3,1,QImage::Format_RGBA32FPx4);raw.setColorSpace(QColorSpace::SRgbLinear);raw.setText("capHdr","scRGB-FP16");
        const float values[]={-1,0,2,1,6,6,6,1,12.5f,12.5f,12.5f,1};memcpy(raw.bits(),values,sizeof(values));
        for(const QString value:{QString(),QStringLiteral("nan"),QStringLiteral("-1"),QStringLiteral("0")}){raw.setText("capSdrWhiteNits",value);QCOMPARE(cap::sdrWhiteNits(raw),80.0);}
        raw.setText("capSdrWhiteNits","480");const auto result=cap::preview(raw);
        QCOMPARE(result.pixelColor(0,0).red(),0);QCOMPARE(result.pixelColor(0,0).green(),0);QVERIFY(result.pixelColor(0,0).blue()>150 && result.pixelColor(0,0).blue()<160);
        QCOMPARE(result.pixelColor(1,0),QColor(Qt::white));QCOMPARE(result.pixelColor(2,0),QColor(Qt::white));
        QVERIFY(result.text("capSdrWhiteNits").isEmpty());
    }
    void desktopHdrFixture() {
        const auto directory=qEnvironmentVariable("VSR_CAP_WHITE_FIXTURE");if(directory.isEmpty())QSKIP("Set VSR_CAP_WHITE_FIXTURE for desktop white-level validation.");
        QImage hdr(QDir(directory).filePath(QStringLiteral("过曝.png"))),reference(QDir(directory).filePath(QStringLiteral("正常的图片.png")));
        QVERIFY(!hdr.isNull());QVERIFY(!reference.isNull());QCOMPARE(hdr.colorSpace(),QColorSpace(QColorSpace::Bt2100Pq));
        // 480 nits is inferred from these historical samples, never a production default.
        hdr.setText("capSdrWhiteNits","480");QElapsedTimer timer;timer.start();const auto restored=cap::preview(hdr);const auto elapsed=timer.elapsed();
        const auto output=QDir::current().filePath("build/audit-sdr-white-20261017");QVERIFY(QDir().mkpath(output));QVERIFY(cap::savePng(restored,output+"/capture_SDR.png"));QVERIFY(cap::savePng(hdr,output+"/capture_HDR.png"));
        const auto a=restored.scaled(192,102,Qt::IgnoreAspectRatio,Qt::FastTransformation),b=reference.scaled(192,102,Qt::IgnoreAspectRatio,Qt::FastTransformation);
        double total=0,maxError=0;int count=0;
        for(const QPoint point:{QPoint(180,20),QPoint(175,60),QPoint(190,80),QPoint(30,5),QPoint(30,90),QPoint(5,85),QPoint(190,5)}){
            const auto actual=a.pixelColor(point).rgba64(),expected=b.pixelColor(point).rgba64();
            qInfo()<<point<<a.pixelColor(point)<<b.pixelColor(point);
            for(const int error:{qAbs(int(actual.red())-expected.red()),qAbs(int(actual.green())-expected.green()),qAbs(int(actual.blue())-expected.blue())}){const double codes=error/257.0;total+=codes;maxError=qMax(maxError,codes);++count;}
        }
        qInfo()<<"desktop-SDR fixture"<<restored.size()<<"mapping-ms"<<elapsed<<"mean-RGB8-error"<<total/count<<"max-RGB8-error"<<maxError;
        QVERIFY(total/count<3);QVERIFY(maxError<12);QCOMPARE(restored.colorSpace(),QColorSpace(QColorSpace::SRgb));
    }
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
