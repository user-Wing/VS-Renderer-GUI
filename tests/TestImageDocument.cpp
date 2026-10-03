#include "image/ImageDocument.h"
#include <QTest>
#include <cstring>
using namespace vsr;
class TestImageDocument final : public QObject {
    Q_OBJECT
private slots:
    void cacheAndDiscardedHistory(){
        ImageDocument doc({600,600},ImagePrecision::UInt8);doc.setCacheBudget(256*256*4);QImage source(600,600,QImage::Format_ARGB32);source.fill(QColor(13,27,51,143));const auto layer=doc.addLayer("Cache",source);doc.history()->clear();
        QVERIFY(doc.residentBytes()<=256*256*4);QCOMPARE(doc.readRegion(layer,{510,510,8,8}).pixelColor(0,0),QColor(13,27,51,143));
        QImage patch(8,8,QImage::Format_RGBA8888);patch.fill(Qt::green);doc.writeRegion(layer,{510,510},patch);doc.history()->undo();QCOMPARE(doc.readRegion(layer,{510,510,8,8}).pixelColor(0,0),QColor(13,27,51,143));
        patch.fill(Qt::blue);doc.writeRegion(layer,{510,510},patch);QVERIFY(!doc.history()->canRedo());QCOMPARE(doc.readRegion(layer,{510,510,8,8}).pixelColor(0,0),QColor(Qt::blue));QVERIFY(doc.residentBytes()<=256*256*4);
        doc.removeLayer(layer);doc.history()->clear();QCOMPARE(doc.residentBytes(),0);QCOMPARE(doc.storedBytes(),0);QVERIFY(doc.storageError().isEmpty());
    }
    void sparsePrecisionAndSwap(){
        ImageDocument doc({100000,100000},ImagePrecision::UInt16);doc.setColorSpace(QColorSpace::SRgb);doc.setMetadata("CICP","9/16/9");doc.setCacheBudget(0);
        QImage image(17,13,QImage::Format_RGBA64);
        for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<quint16 *>(image.scanLine(y));for(int x=0;x<image.width();++x){row[x*4]=1000+x;row[x*4+1]=2000+y;row[x*4+2]=50001;row[x*4+3]=x?32769:0;}}
        const auto layer=doc.addLayer("16-bit");doc.writeRegion(layer,{250,250},image);
        QCOMPARE(doc.residentBytes(),0);QVERIFY(doc.storedBytes()>0);QVERIFY2(doc.storageError().isEmpty(),qPrintable(doc.storageError()));
        const auto read=doc.readRegion(layer,{250,250,17,13});QCOMPARE(read.format(),QImage::Format_RGBA64);QCOMPARE(read.colorSpace(),QColorSpace(QColorSpace::SRgb));QCOMPARE(doc.metadata("CICP"),QString("9/16/9"));
        for(int y=0;y<image.height();++y)QVERIFY(std::memcmp(read.constScanLine(y),image.constScanLine(y),image.width()*8)==0);
        QCOMPARE(doc.residentBytes(),0);QCOMPARE(doc.readRegion(layer,{-2,-2,2,2}).pixelColor(0,0).alpha(),0);
        QImage patch(2,2,QImage::Format_RGBA64);patch.fill(Qt::red);doc.writeRegion(layer,{-1,-1},patch);QCOMPARE(doc.readRegion(layer,{-1,-1,2,2}).pixelColor(0,0),QColor(Qt::red));
    }
    void floatRangeAndHistory(){
        ImageDocument doc({600,600},ImagePrecision::Float32);doc.setCacheBudget(0);const auto layer=doc.addLayer("HDR");doc.history()->clear();
        QImage image(4,4,QImage::Format_RGBA32FPx4);for(int y=0;y<4;++y){auto *row=reinterpret_cast<float *>(image.scanLine(y));for(int x=0;x<4;++x){row[x*4]=-0.25f;row[x*4+1]=12.5f;row[x*4+2]=1.00001f;row[x*4+3]=1;}}
        const auto before=doc.revision();doc.beginEdit("One stroke");doc.writeRegion(layer,{254,254},image);doc.writeRegion(layer,{300,300},image);QVERIFY(doc.revision()>before);doc.commitEdit();QCOMPARE(doc.history()->count(),1);
        auto read=doc.readRegion(layer,{254,254,4,4});for(int y=0;y<4;++y)QVERIFY(std::memcmp(read.constScanLine(y),image.constScanLine(y),4*16)==0);
        auto composed=doc.composite({254,254,4,4});const auto *values=reinterpret_cast<const float *>(composed.constScanLine(0));QCOMPARE(values[0],-.25f);QCOMPARE(values[1],12.5f);QCOMPARE(values[2],1.00001f);
        doc.history()->undo();QCOMPARE(doc.readRegion(layer,{254,254,1,1}).pixelColor(0,0).alpha(),0);doc.history()->redo();QCOMPARE(reinterpret_cast<const float *>(doc.readRegion(layer,{300,300,1,1}).constBits())[1],12.5f);
        doc.beginEdit("Cancelled stroke");QImage black(4,4,QImage::Format_RGBA32FPx4);black.fill(Qt::black);doc.writeRegion(layer,{254,254},black);doc.cancelEdit();QCOMPARE(doc.history()->count(),1);QCOMPARE(reinterpret_cast<const float *>(doc.readRegion(layer,{254,254,1,1}).constBits())[1],12.5f);
    }
    void layersMasksSelectionAndPreview(){
        ImageDocument doc({512,512},ImagePrecision::Float32);QImage base(2,2,QImage::Format_RGBA32FPx4);base.fill(Qt::red);const auto bottom=doc.addLayer("Bottom",base,{255,255});
        QImage top(2,2,QImage::Format_RGBA32FPx4);top.fill(Qt::blue);const auto upper=doc.addLayer("Top",top,{255,255});auto info=doc.layers().last();info.opacity=.5f;doc.updateLayer(info);
        auto result=doc.composite({255,255,2,2});const auto *p=reinterpret_cast<const float *>(result.constBits());QCOMPARE(p[0],.5f);QCOMPARE(p[2],.5f);QCOMPARE(p[3],1.f);
        QImage mask(1,1,QImage::Format_Grayscale16);mask.fill(Qt::black);doc.writeMask(upper,{},mask);result=doc.composite({255,255,2,2});p=reinterpret_cast<const float *>(result.constBits());QCOMPARE(p[0],1.f);QCOMPARE(p[2],0.f);QCOMPARE(reinterpret_cast<const quint16 *>(doc.maskRegion(upper,{1,1,1,1}).constBits())[0],quint16(65535));
        QVERIFY(!doc.hasSelection());QCOMPARE(doc.selectionId(),quint64(0));QCOMPARE(reinterpret_cast<const quint16 *>(doc.selectionRegion({0,0,1,1}).constBits())[0],quint16(65535));mask.fill(Qt::white);doc.writeSelection({255,255},mask);QVERIFY(doc.hasSelection());const auto selection=doc.selectionId();QVERIFY(selection>0);QCOMPARE(reinterpret_cast<const quint16 *>(doc.selectionRegion({0,0,1,1}).constBits())[0],quint16(0));doc.history()->undo();QVERIFY(!doc.hasSelection());QCOMPARE(doc.selectionId(),quint64(0));doc.history()->redo();QVERIFY(doc.hasSelection());QCOMPARE(doc.selectionId(),selection);doc.clearSelection();QVERIFY(!doc.hasSelection());QCOMPARE(doc.selectionId(),quint64(0));
        info.blend=ImageBlendMode::Multiply;doc.updateLayer(info);result=doc.composite({256,256,1,1});p=reinterpret_cast<const float *>(result.constBits());QCOMPARE(p[0],.5f);QCOMPARE(p[2],0.f);
        doc.moveLayer(bottom,1);QCOMPARE(doc.layers().last().id,bottom);doc.history()->undo();QCOMPARE(doc.layers().last().id,upper);doc.removeLayer(upper);QCOMPARE(doc.layers().size(),1);doc.history()->undo();QCOMPARE(doc.layers().size(),2);
        const auto preview=doc.compositePreview({0,0,512,512},{512,512});QCOMPARE(preview,doc.composite({0,0,512,512}));
        ImageDocument blendDoc({1,1},ImagePrecision::Float32);QImage blendPixel(1,1,QImage::Format_RGBA32FPx4);blendPixel.fill(Qt::black);const auto backdrop=blendDoc.addLayer("backdrop",blendPixel);blendPixel.fill(Qt::white);const auto foreground=blendDoc.addLayer("foreground",blendPixel);auto blendInfo=blendDoc.layers().last();blendInfo.blend=ImageBlendMode::ColorDodge;blendDoc.updateLayer(blendInfo);QCOMPARE(blendDoc.composite({0,0,1,1}).pixelColor(0,0),QColor(Qt::black));
        blendPixel.fill(Qt::white);blendDoc.writeRegion(backdrop,{},blendPixel);blendPixel.fill(Qt::black);blendDoc.writeRegion(foreground,{},blendPixel);blendInfo.blend=ImageBlendMode::ColorBurn;blendDoc.updateLayer(blendInfo);QCOMPARE(blendDoc.composite({0,0,1,1}).pixelColor(0,0),QColor(Qt::white));
        auto *channel=reinterpret_cast<float *>(blendPixel.bits());channel[0]=channel[1]=channel[2]=.1f;channel[3]=1;blendDoc.writeRegion(backdrop,{},blendPixel);blendPixel.fill(Qt::white);blendDoc.writeRegion(foreground,{},blendPixel);blendInfo.blend=ImageBlendMode::SoftLight;blendDoc.updateLayer(blendInfo);const auto softened=blendDoc.composite({0,0,1,1});QVERIFY(std::abs(reinterpret_cast<const float *>(softened.constBits())[0]-.296f)<.00001f);
        ImageDocument huge({100000,100000},ImagePrecision::Float32);huge.setCacheBudget(0);const auto id=huge.addLayer("Sparse");huge.writeRegion(id,{25000,25000},base);const auto small=huge.compositePreview({0,0,100000,100000},{2,2});QCOMPARE(small.size(),QSize(2,2));QCOMPARE(small.pixelColor(0,0),QColor(Qt::red));QCOMPARE(small.pixelColor(1,1).alpha(),0);QCOMPARE(huge.residentBytes(),0);
    }
};
QTEST_MAIN(TestImageDocument)
#include "TestImageDocument.moc"
