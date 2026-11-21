#include "color/ScreenshotColor.h"
#include "cap/DesktopCapture.h"
#include "cap/CaptureFallback.h"
#include "image/ImageJxl.h"
#include "image/ImageHdrSurface.h"
#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QImageReader>
#include <QScreen>
#include <limits>
using namespace vsr::screenshot;

class TestScreenshotColor : public QObject {
    Q_OBJECT
    static QImage scRgb(int width=1) {
        QImage image(width,1,QImage::Format_RGBA32FPx4);image.fill(Qt::white);image.setColorSpace(QColorSpace::SRgbLinear);image.setText("capHdr","scRGB-FP16");return image;
    }
private slots:
    void displayClassification() {
        DisplayEvidence d;QCOMPARE(d.classification(),DisplayClass::Unknown);QVERIFY(d.needsFloatCapture());
        d.outputBits=10;QCOMPARE(d.classification(),DisplayClass::Unknown);
        d.modeKnown=true;QCOMPARE(d.classification(),DisplayClass::LegacySdr);QVERIFY(!d.needsFloatCapture());
        d.advancedActive=true;QCOMPARE(d.classification(),DisplayClass::AdvancedSdr);QVERIFY(d.needsFloatCapture());
        d.hdrActive=true;QCOMPARE(d.classification(),DisplayClass::Hdr);
        d.hdrActive=false;d.wideActive=true;QCOMPARE(d.classification(),DisplayClass::AdvancedSdr);
        d.wideActive=false;d.modeKnown=false;QCOMPARE(d.classification(),DisplayClass::Unknown);QVERIFY(d.needsFloatCapture());
    }
    void unknownAndPrecision() {
        QImage image(1,1,QImage::Format_RGBA8888);image.fill(Qt::red);
        const auto p=exportPlan(image);QCOMPARE(p.source.primaries,Primaries::Unknown);QCOMPARE(p.source.effectiveBits,0);QCOMPARE(p.source.sourceBits,8);QCOMPARE(p.containerBits,16);QVERIFY(!p.degradation.isEmpty());
        const auto converted=exportPixels(image,p);QCOMPARE(converted.depth(),64);QCOMPARE(converted.pixelColor(0,0),image.pixelColor(0,0));
        auto hdr=scRgb();const auto preview=previewPlan(hdr);QCOMPARE(preview.source.sdrWhiteNits,0.);QCOMPARE(preview.whiteNits,80.);QVERIFY(!preview.degradation.isEmpty());
        auto f=evidence(hdr);f.backend="WGC-FP16";f.sourceIdentity="display:1";f.frameIdentity="frozen:1";attach(hdr,f);QCOMPARE(evidence(hdr).backend,f.backend);QCOMPARE(evidence(hdr).sourceBits,16);QCOMPARE(evidence(hdr).effectiveBits,0);
    }
    void pqAbsoluteAndAlpha() {
        auto image=scRgb(3);auto *v=reinterpret_cast<float*>(image.bits());
        const double nits[]{80,1000,4000},expected[]{.4858567654,.7518270962,.9025723933};
        for(int i=0;i<3;++i){for(int c=0;c<3;++c)v[i*4+c]=float(nits[i]/80.);v[i*4+3]=.25f;}
        const auto p=exportPlan(image);QCOMPARE(p.cicpPrimaries,9);QCOMPARE(p.cicpTransfer,16);QCOMPARE(p.rgbMatrix,0);QCOMPARE(p.yuvMatrix,9);
        const auto pixels=exportPixels(image,p);QCOMPARE(pixels.colorSpace(),p.target);
        for(int i=0;i<3;++i){const auto c=pixels.pixelColor(i,0).rgba64();QVERIFY(std::abs(c.red()/65535.-expected[i])<.5/65535.+1e-7);QCOMPARE(c.alpha(),quint16(16384));}
        image.setText("capSdrWhiteNits","480");QCOMPARE(exportPixels(image,exportPlan(image)).pixelColor(1,0),pixels.pixelColor(1,0));
    }
    void signedGamutAndWideSdr() {
        auto image=scRgb();auto *v=reinterpret_cast<float*>(image.bits());v[0]=-.1f;v[1]=1;v[2]=.3f;v[3]=.75f;
        const auto rgb=toRec2020(v[0],v[1],v[2]);QVERIFY(rgb[0]>0);QVERIFY(rgb[0]<toRec2020(0,1,.3)[0]);
        const auto pq=exportPixels(image,exportPlan(image));QVERIFY(std::abs(pq.pixelColor(0,0).redF()-encodePqNits(rgb[0]*80.))<1./65535.);
        auto f=evidence(image);f.advancedSdr=true;f.sdrWhiteNits=80;attach(image,f);
        const auto p=exportPlan(image);QVERIFY(p.scRgbToWideSdr);QVERIFY(!p.scRgbToPq);QCOMPARE(p.cicpPrimaries,9);QCOMPARE(p.cicpTransfer,13);
        const auto wide=exportPixels(image,p);QVERIFY(std::abs(wide.pixelColor(0,0).redF()-srgb(rgb[0]))<1./65535.);QCOMPARE(wide.pixelColor(0,0).rgba64().alpha(),quint16(49151));
    }
    void desktopHighlightRatios() {
        for(double white:{80.,203.,480.}) {
            auto image=scRgb();image.setText("capSdrWhiteNits",QString::number(white));auto *v=reinterpret_cast<float*>(image.bits());
            v[0]=float(4*white/80);v[1]=float(2*white/80);v[2]=float(white/80);v[3]=1;
            const auto preview=cap::preview(image).pixelColor(0,0);QCOMPARE(preview.redF(),1.f);QVERIFY(std::abs(preview.greenF()-srgb(.5))<2./65535.);QVERIFY(std::abs(preview.blueF()-srgb(.25))<2./65535.);
        }
    }
    void photographicAndDesktopRemainDistinct(){
        auto image=scRgb();auto facts=evidence(image);facts.desktop=false;attach(image,facts);
        QCOMPARE(previewPlan(image).preview,Preview::Photographic);
        const double l=80./203.,mapped=l*(2.51*l+.03)/(l*(2.43*l+.59)+.14);
        QVERIFY(std::abs(cap::preview(image).pixelColor(0,0).redF()-srgb(mapped))<2./65535.);
        const auto archive=exportPixels(image,exportPlan(image));QVERIFY(archive.text("capSdrWhiteNits").isEmpty());QCOMPARE(previewPlan(archive).preview,Preview::Photographic);
        facts.desktop=true;attach(image,facts);QCOMPARE(previewPlan(image).preview,Preview::DesktopWhite);QCOMPARE(cap::preview(image).pixelColor(0,0).redF(),1.f);
    }
    void adjacentLevelsAndSpecialValues() {
        auto image=scRgb(4);auto *v=reinterpret_cast<float*>(image.bits());
        const float values[]{1,1,1,1,1.0009765625f,1.0009765625f,1.0009765625f,1,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),-.2f,.5f,1.f,1};
        memcpy(v,values,sizeof(values));const auto out=cap::pngPixels(image);QVERIFY(out.pixelColor(1,0).redF()>out.pixelColor(0,0).redF());QCOMPARE(out.pixelColor(2,0).alpha(),0);
        const auto preview=cap::preview(image);QCOMPARE(preview.pixelColor(2,0).rgba64().alpha(),quint16(0));
        for(int x=0;x<4;++x)for(double c:{out.pixelColor(x,0).redF(),preview.pixelColor(x,0).redF()})QVERIFY(std::isfinite(c));
    }
    void customIccAndStandardDeclarations() {
        QTemporaryDir dir;QImage image(2,2,QImage::Format_RGBA64);image.fill(QColor(23,51,89,137));
        const QColorSpace custom(QColorSpace::Primaries::DciP3D65,QColorSpace::TransferFunction::Gamma,2.2f);image.setColorSpace(custom);
        auto plan=exportPlan(image);QCOMPARE(plan.cicpPrimaries,0);QVERIFY(cap::savePng(image,dir.filePath("icc.png")));
        const auto decoded=QImageReader(dir.filePath("icc.png")).read();QVERIFY(decoded.colorSpace().isValid());QCOMPARE(decoded.colorSpace().iccProfile(),custom.iccProfile());QCOMPARE(decoded.pixelColor(0,0),image.pixelColor(0,0));
        QVERIFY(vsr::writeJxl(dir.filePath("icc.jxl"),image).isEmpty());const auto jxl=vsr::readJxl(dir.filePath("icc.jxl"));QCOMPARE(jxl.colorSpace().iccProfile(),custom.iccProfile());QCOMPARE(jxl.pixelColor(0,0),image.pixelColor(0,0));
        for(const QColorSpace space:{QColorSpace(QColorSpace::SRgb),QColorSpace(QColorSpace::DisplayP3),QColorSpace(QColorSpace::Bt2100Pq),QColorSpace(QColorSpace::SRgbLinear)}) {
            image.setColorSpace(space);const auto path=dir.filePath("standard.png");QVERIFY(cap::savePng(image,path));QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();QVERIFY(bytes.contains("cICP"));
            QCOMPARE(QImageReader(path).read().colorSpace(),space);
        }
    }
    void frozenOwnership() {
        auto image=scRgb();FrozenFrame frozen(image);reinterpret_cast<float*>(image.bits())[0]=12.5f;QCOMPARE(reinterpret_cast<const float*>(frozen.pixels().constBits())[0],1.f);
        auto copy=frozen.pixels();reinterpret_cast<float*>(copy.bits())[0]=7;QCOMPARE(reinterpret_cast<const float*>(frozen.pixels().constBits())[0],1.f);
        float external[]{1,2,3,1};QImage borrowed(reinterpret_cast<uchar*>(external),1,1,16,QImage::Format_RGBA32FPx4);FrozenFrame owned(borrowed);external[0]=99;QCOMPARE(reinterpret_cast<const float*>(owned.pixels().constBits())[0],1.f);
    }
    void transientRecovery() {
        DisplayEvidence display;display.modeKnown=true;display.wideActive=true;
        int calls=0;auto wgc=[&](QString *error){++calls;if(calls==1){*error="access lost";return QImage();}return scRgb();};
        auto dxgi=[](QString *error){*error="format unavailable";return QImage();};
        auto legacy=[](QString *){QImage i(2,2,QImage::Format_RGBA8888);i.fill(Qt::red);return i;};
        auto first=cap::captureWithFallback(display,wgc,dxgi,legacy);QVERIFY(!first.floating);QVERIFY(first.degradation.contains("access lost"));
        auto second=cap::captureWithFallback(display,wgc,dxgi,legacy);QVERIFY(second.floating);QCOMPARE(second.backend,QString("WGC-FP16"));QVERIFY(second.degradation.isEmpty());QCOMPARE(calls,2);
        display.wideActive=false;cap::captureWithFallback(display,wgc,dxgi,legacy);QCOMPARE(calls,2);
    }
    void fp16CompositorPixels() {
        for(double white:{80.,203.,480.}){
            QImage image(32,16,QImage::Format_RGBA32FPx4);image.setColorSpace(QColorSpace::SRgbLinear);image.fill(QColor::fromRgbF(2.,.5,.25,1));
            QImage overlay(image.size(),QImage::Format_RGBA8888);overlay.fill(Qt::transparent);for(int y=0;y<16;++y)for(int x=16;x<32;++x)overlay.setPixelColor(x,y,QColor(255,255,255,128));
            QString error;const auto out=vsr::ImageHdrSurface::compositionProbe(image,overlay,white,true,&error);QVERIFY2(!out.isNull(),qPrintable(error));
            const auto *left=reinterpret_cast<const float*>(out.constScanLine(8))+8*4;QVERIFY(std::abs(left[0]-2)<.002);QVERIFY(std::abs(left[1]-.5)<.002);
            const auto *right=reinterpret_cast<const float*>(out.constScanLine(8))+24*4;const double alpha=128./255.,expected=2*(1-alpha)+(white/80)*alpha;QVERIFY(std::abs(right[0]-expected)<.005);
            const auto evidencePath=qEnvironmentVariable("VSR_COLOR_EVIDENCE");if(!evidencePath.isEmpty()){
                auto image=out;image.setText("capHdr","scRGB-FP16");image.setText("capSdrWhiteNits",QString::number(white));
                QVERIFY(cap::savePng(image,evidencePath+QString("/gpu-composite-%1.png").arg(white)));
                qInfo()<<"WARP FP16 composition"<<"paperWhiteNits"<<white<<"backgroundR"<<left[0]<<"overlayR"<<right[0]<<"expectedR"<<expected;
            }
        }
    }
    void shadersCompile() {QString error;QVERIFY2(vsr::ImageHdrSurface::validateShaders(&error),qPrintable(error));}
    void nativeHdrEligibility(){
        if(!qEnvironmentVariableIsSet("VSR_NATIVE_HDR_PROBE"))QSKIP("Set VSR_NATIVE_HDR_PROBE for current display presentation eligibility");
        QWidget parent;parent.resize(160,96);vsr::ImageHdrSurface surface(&parent);surface.setGeometry(parent.rect());
        if(!surface.setHdrEnabled(true))QSKIP(qPrintable(surface.status()));
        auto image=scRgb(160);image=image.scaled(160,96);surface.present(image,parent.rect(),{},0,203,true);QVERIFY2(surface.hdrActive(),qPrintable(surface.status()));qInfo()<<surface.status();
    }
    void nativeDpiGeometry(){
        if(!qEnvironmentVariableIsSet("VSR_NATIVE_HDR_PROBE") || QGuiApplication::platformName()!="windows")QSKIP("Set VSR_NATIVE_HDR_PROBE on Windows for hidden native geometry validation");
        auto *screen=QGuiApplication::primaryScreen();const auto display=cap::displayEvidence(screen);
        QWidget parent;parent.setWindowFlags(Qt::Tool|Qt::FramelessWindowHint);parent.setGeometry(screen->geometry());
        QVERIFY(cap::placeCaptureWindow(&parent,display.nativeBounds));vsr::ImageHdrSurface surface(&parent);surface.setGeometry(parent.rect());
        QVERIFY(cap::placeCaptureWindow(&surface,QRect(QPoint(),display.nativeBounds.size())));
        QCOMPARE(surface.presentationPixelSize(),display.nativeBounds.size());QVERIFY(!parent.isVisible());
        qInfo()<<"hidden capture geometry"<<"DPR"<<parent.devicePixelRatioF()<<"logical"<<parent.size()<<"native"<<surface.presentationPixelSize();
    }
    void independentFixtures() {
        const auto path=qEnvironmentVariable("VSR_COLOR_EVIDENCE");if(path.isEmpty())QSKIP("Set VSR_COLOR_EVIDENCE to retain independent decoder fixtures");QVERIFY(QDir().mkpath(path));
        QImage integer(256,32,QImage::Format_RGBA64);integer.setColorSpace(QColorSpace::SRgb);
        for(int y=0;y<32;++y){auto *row=reinterpret_cast<QRgba64*>(integer.scanLine(y));for(int x=0;x<256;++x)row[x]=QRgba64::fromRgba64(quint16(x*257+y),quint16(y*1700+x),quint16(65535-x*173-y),quint16(1000+x*211));}
        auto hdr=scRgb(256);auto *v=reinterpret_cast<float*>(hdr.bits());for(int x=0;x<256;++x){v[x*4]=float(1+x/1024.);v[x*4+1]=float(.5+x/2048.);v[x*4+2]=float(.25+x/4096.);v[x*4+3]=1;}
        for(const auto &entry:{std::pair{QString("integer"),integer},std::pair{QString("pq"),cap::pngPixels(hdr)}}){
            QVERIFY(cap::savePng(entry.second,path+"/"+entry.first+".png"));QVERIFY(vsr::writeJxl(path+"/"+entry.first+".jxl",entry.second).isEmpty());QFile raw(path+"/"+entry.first+".rgba64le");QVERIFY(raw.open(QIODevice::WriteOnly));for(int y=0;y<entry.second.height();++y)QCOMPARE(raw.write(reinterpret_cast<const char*>(entry.second.constScanLine(y)),entry.second.width()*8),entry.second.width()*8);
        }
    }
};
QTEST_MAIN(TestScreenshotColor)
#include "TestScreenshotColor.moc"
