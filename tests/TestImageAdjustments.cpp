#include "image/ImageAdjustments.h"
#include "image/ImageHdr.h"
#include "image/ImageHdrSurface.h"
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
#include <cstring>
#include <cmath>
using namespace vsr;
namespace {
QImage floatPixel(float r,float g,float b,float a=1){QImage image(1,1,QImage::Format_RGBA32FPx4);float p[4]={r,g,b,a};std::memcpy(image.bits(),p,16);image.setColorSpace(QColorSpace::SRgbLinear);return image;}
float channel(const QImage &image,int c){return reinterpret_cast<const float *>(image.constBits())[c];}
}
class TestImageAdjustments final : public QObject {
    Q_OBJECT
private slots:
    void nativeHdrCapabilityAndFallback(){
#ifdef Q_OS_WIN
        QString error;QVERIFY2(ImageHdrSurface::validateShaders(&error),qPrintable(error));
#endif
        QWidget parent;parent.resize(320,240);ImageHdrSurface surface(&parent);surface.setGeometry(parent.rect());QVERIFY(!surface.setHdrEnabled(false));QVERIFY(!surface.hdrActive());QVERIFY(surface.isHidden());const bool enabled=surface.setHdrEnabled(true);QCOMPARE(surface.hdrActive(),enabled);if(enabled){QVERIFY(surface.status().contains("scRGB"));surface.present(floatPixel(.25,4,-.25),{10,10,100,100},{},0,203);}else{QVERIFY(surface.status().startsWith("SDR fallback:"));QVERIFY(surface.isHidden());}surface.setHdrEnabled(false);QVERIFY(!surface.hdrActive());QVERIFY(surface.isHidden());
    }
    void floatAdjustmentsAndNativePrecision(){
        const auto image=floatPixel(-.25f,12.5f,1.125f,.4f);ImageAdjustmentSpec spec;
        spec.exposure=1;auto output=ImageAdjustments::process(image,spec);QCOMPARE(channel(output,0),-.5f);QCOMPARE(channel(output,1),25.f);QCOMPARE(channel(output,3),.4f);
        auto encoded=floatPixel(.5,.5,.5);encoded.setColorSpace(QColorSpace::SRgb);auto exposed=ImageAdjustments::process(encoded,spec);QVERIFY(std::abs(channel(exposed,0)-.685836f)<.0002f);
        spec.kind=ImageAdjustmentKind::Levels;output=ImageAdjustments::process(image,spec);QCOMPARE(output,image);
        spec.kind=ImageAdjustmentKind::Curves;spec.curves[0]={{0,0},{.5,.25},{1,1}};output=ImageAdjustments::process(image,spec);QCOMPARE(channel(output,0),-.125f);QCOMPARE(channel(output,1),18.25f);QCOMPARE(channel(output,3),.4f);
        spec.kind=ImageAdjustmentKind::HueSaturation;output=ImageAdjustments::process(image,spec);QCOMPARE(output,image);
        spec.kind=ImageAdjustmentKind::Invert;output=ImageAdjustments::process(image,spec);QCOMPARE(channel(output,0),1.25f);QCOMPARE(channel(output,1),-11.5f);QCOMPARE(channel(output,3),.4f);
        spec.kind=ImageAdjustmentKind::ChannelMixer;spec.channelMixer={{{{0,1,0}},{{0,0,1}},{{1,0,0}}}};output=ImageAdjustments::process(image,spec);QCOMPARE(channel(output,0),12.5f);QCOMPARE(channel(output,1),1.125f);QCOMPARE(channel(output,2),-.25f);
        QImage sixteen(1,1,QImage::Format_RGBA64);auto *p=reinterpret_cast<quint16 *>(sixteen.bits());p[0]=12001;p[1]=32003;p[2]=65431;p[3]=44447;spec={};output=ImageAdjustments::process(sixteen,spec);QCOMPARE(output,sixteen);
        spec.gamma=0;QString error;QVERIFY(!ImageAdjustments::validate(spec,&error));QVERIFY(!error.isEmpty());
        spec={};spec.curves[0]={{.5,.5},{.2,.2}};QVERIFY(!ImageAdjustments::validate(spec));
    }
    void selectedOffsetAdjustmentAndUndo(){
        ImageDocument doc({600,600},ImagePrecision::Float32);doc.setColorSpace(QColorSpace::SRgbLinear);const auto id=doc.addLayer("HDR",floatPixel(-.25f,4,1,.5f),{257,250});QImage selection(1,1,QImage::Format_Grayscale16);reinterpret_cast<quint16 *>(selection.bits())[0]=32768;doc.writeSelection({257,250},selection);doc.history()->clear();
        ImageAdjustmentSpec spec;spec.exposure=1;QString error;QVERIFY2(ImageAdjustments::apply(&doc,id,spec,"Exposure",&error),qPrintable(error));const auto output=doc.readRegion(id,{0,0,1,1});QVERIFY(std::abs(channel(output,0)+.375f)<.00001f);QVERIFY(std::abs(channel(output,1)-6)<.0001f);QCOMPARE(channel(output,3),.5f);QCOMPARE(doc.history()->count(),1);doc.history()->undo();QCOMPARE(doc.readRegion(id,{0,0,1,1}),floatPixel(-.25f,4,1,.5f));
        const auto group=doc.addLayer("Locked parent");auto parent=doc.layers().last();parent.group=true;parent.locked=true;doc.updateLayer(parent);auto child=doc.layers().first();child.parentId=group;doc.updateLayer(child);const auto unchanged=doc.readRegion(id,{0,0,1,1});const int historyCount=doc.history()->count();error.clear();QVERIFY(!ImageAdjustments::apply(&doc,id,spec,"Blocked exposure",&error));QVERIFY(!error.isEmpty());QCOMPARE(doc.readRegion(id,{0,0,1,1}),unchanged);QCOMPARE(doc.history()->count(),historyCount);
    }
    void profileConversionAndUndo(){
        ImageDocument doc({10,10},ImagePrecision::Float32);doc.setColorSpace(QColorSpace::SRgb);QImage image=floatPixel(.5f,.25f,.75f);image.setColorSpace(QColorSpace::SRgb);const auto id=doc.addLayer("sRGB",image);doc.history()->clear();QString error;
        QVERIFY2(ImageAdjustments::convertColorSpace(&doc,QColorSpace::SRgbLinear,&error),qPrintable(error));const auto output=doc.readRegion(id,{0,0,1,1});QVERIFY(std::abs(channel(output,0)-.214041f)<.0001f);QCOMPARE(doc.colorSpace(),QColorSpace(QColorSpace::SRgbLinear));doc.history()->undo();QCOMPARE(doc.colorSpace(),QColorSpace(QColorSpace::SRgb));QCOMPARE(doc.readRegion(id,{0,0,1,1}),image);
        ImageDocument hdr({1,1},ImagePrecision::Float32);hdr.setColorSpace(QColorSpace::SRgbLinear);const auto high=hdr.addLayer("HDR",floatPixel(-.25,4,.5));QVERIFY2(ImageAdjustments::convertColorSpace(&hdr,QColorSpace::SRgb,&error),qPrintable(error));const auto extended=hdr.readRegion(high,{0,0,1,1});QVERIFY(channel(extended,0)<0);QVERIFY(channel(extended,1)>1);hdr.history()->undo();QCOMPARE(hdr.colorSpace(),QColorSpace(QColorSpace::SRgbLinear));QCOMPARE(channel(hdr.readRegion(high,{0,0,1,1}),1),4.f);
    }
    void spatialFilterHasNoTileSeam(){
        QImage source(513,9,QImage::Format_RGBA32FPx4);source.setColorSpace(QColorSpace::SRgbLinear);for(int y=0;y<source.height();++y){auto *p=reinterpret_cast<float *>(source.scanLine(y));for(int x=0;x<source.width();++x){p[x*4]=x==255 || x==256?8.f:-.25f;p[x*4+1]=x/100.f;p[x*4+2]=.1f;p[x*4+3]=1;}}
        ImageAdjustmentSpec spec;spec.kind=ImageAdjustmentKind::GaussianBlur;spec.radius=3;const auto expected=ImageAdjustments::process(source,spec);ImageDocument doc(source.size(),ImagePrecision::Float32);doc.setColorSpace(QColorSpace::SRgbLinear);const auto id=doc.addLayer("Cross-tile",source);doc.history()->clear();QString error;QVERIFY2(ImageAdjustments::apply(&doc,id,spec,"Blur",&error),qPrintable(error));const auto actual=doc.readRegion(id,QRect(QPoint(),source.size()));for(int y=0;y<source.height();++y){const auto *a=reinterpret_cast<const float *>(actual.constScanLine(y)),*b=reinterpret_cast<const float *>(expected.constScanLine(y));for(int x=0;x<source.width()*4;++x)QVERIFY(std::abs(a[x]-b[x])<.00001f);}doc.history()->undo();QCOMPARE(doc.readRegion(id,QRect(QPoint(),source.size())),source);
        spec.kind=ImageAdjustmentKind::Median;spec.radius=1;QImage noise(3,3,QImage::Format_RGBA32FPx4);const auto neutral=floatPixel(.25,.25,.25);for(int y=0;y<3;++y)for(int x=0;x<3;++x)std::memcpy(noise.scanLine(y)+x*16,neutral.constBits(),16);const auto hot=floatPixel(12,12,12);std::memcpy(noise.scanLine(1)+16,hot.constBits(),16);QCOMPARE(channel(ImageAdjustments::process(noise,spec).copy({1,1,1,1}),0),.25f);
    }
    void hdrDisplayDoesNotChangeDocument(){
        const auto source=floatPixel(.2f,4,-.25f,.5f),saved=source;ImageHdrPreviewSettings settings;settings.toneMap=true;const auto mapped=ImageHdr::preview(source,settings);QCOMPARE(source,saved);QCOMPARE(mapped.format(),QImage::Format_RGBA8888);QVERIFY(mapped.pixelColor(0,0).green()>mapped.pixelColor(0,0).red());QCOMPARE(mapped.pixelColor(0,0).blue(),0);settings.channel=3;const auto alpha=ImageHdr::preview(source,settings);QCOMPARE(alpha.pixelColor(0,0),QColor(128,128,128,255));
        const auto histogram=ImageAdjustments::histogram(source);QCOMPARE(histogram[1][255],quint64(1));QCOMPARE(histogram[2][0],quint64(1));
        settings.channel=-1;auto untagged=source;untagged.setColorSpace({});QCOMPARE(ImageHdr::preview(untagged,settings),mapped);QVERIFY(!untagged.colorSpace().isValid());
        QImage integer(1,1,QImage::Format_RGBA8888);integer.fill(QColor(64,128,192,128));auto tagged=integer;tagged.setColorSpace(QColorSpace::SRgb);QCOMPARE(ImageHdr::preview(integer,settings),ImageHdr::preview(tagged,settings));
        auto pq=floatPixel(1,1,1);pq.setColorSpace(QColorSpace::Bt2100Pq);const auto linear=pq.colorTransformed(pq.colorSpace().transformationToColorSpace(QColorSpace::SRgbLinear),QImage::Format_RGBA32FPx4);QVERIFY(std::abs(channel(linear,0)-64)<.01f); // Native output scale is tied to Qt's PQ normalization.
    }
    void floatTiffPreservesHdrAndIcc(){
        QTemporaryDir directory;ImageDocument doc({3,2},ImagePrecision::Float32);doc.setColorSpace(QColorSpace::SRgbLinear);const auto source=floatPixel(-.25f,12.5f,1.125f,.4f);const auto id=doc.addLayer("HDR");for(int y=0;y<2;++y)for(int x=0;x<3;++x)doc.writeRegion(id,{x,y},source);QString error;const auto path=directory.filePath("hdr.tif");QVERIFY2(ImageHdr::exportFloatTiff(&doc,path,&error),qPrintable(error));QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();const auto *p=reinterpret_cast<const uchar *>(bytes.constData());QCOMPARE(qFromLittleEndian<quint16>(p),quint16(0x4949));QCOMPARE(qFromLittleEndian<quint16>(p+2),quint16(42));
        const auto ifd=qFromLittleEndian<quint32>(p+4);const auto count=qFromLittleEndian<quint16>(p+ifd);quint32 offset=0;bool hasIcc=false,hasFloat=false,hasAlpha=false;
        for(int i=0;i<count;++i){const auto *tag=p+ifd+2+i*12;const auto code=qFromLittleEndian<quint16>(tag);if(code==273)offset=qFromLittleEndian<quint32>(tag+8);if(code==34675){hasIcc=true;const auto length=qFromLittleEndian<quint32>(tag+4),profileOffset=qFromLittleEndian<quint32>(tag+8);QCOMPARE(bytes.mid(profileOffset,length),doc.colorSpace().iccProfile());}if(code==339){const auto formats=qFromLittleEndian<quint32>(tag+8);hasFloat=qFromLittleEndian<quint16>(p+formats)==3;}if(code==338)hasAlpha=qFromLittleEndian<quint16>(tag+8)==2;}
        QVERIFY(hasIcc && hasFloat && hasAlpha);QVERIFY(offset>0);QCOMPARE(bytes.size()-int(offset),3*2*16);float values[4];std::memcpy(values,p+offset,16);QCOMPARE(values[0],-.25f);QCOMPARE(values[1],12.5f);QCOMPARE(values[2],1.125f);QCOMPARE(values[3],.4f);
    }
};
QTEST_MAIN(TestImageAdjustments)
#include "TestImageAdjustments.moc"
