#include "image/ImagePsd.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QtEndian>
#include <cstring>
using namespace vsr;
namespace {
void n(QByteArray &b,quint64 v,int count){for(int i=count-1;i>=0;--i)b+=char(v>>(i*8));}
void section(QByteArray &b,const QByteArray &payload){n(b,payload.size(),4);b+=payload;}
QByteArray zip(const QByteArray &bytes){return qCompress(bytes,6).mid(4);}
QByteArray prediction(QByteArray row,int depth){
    if(depth==16){auto *p=reinterpret_cast<uchar *>(row.data());const auto v=qFromBigEndian<quint16>(p+2)-qFromBigEndian<quint16>(p);qToBigEndian(quint16(v),p+2);}
    else if(depth==8)row[1]=char(uchar(row[1])-uchar(row[0]));
    else {const auto original=row;for(int c=0;c<4;++c){row[c*2]=original[c];row[c*2+1]=original[c+4];}for(int i=7;i>0;--i)row[i]=char(uchar(row[i])-uchar(row[i-1]));}
    return row;
}
QString fixture(QTemporaryDir &dir,int depth,int compression){
    const int bytes=depth/8;QList<QByteArray> channels;
    for(int c=0;c<4;++c){QByteArray row;for(int x=0;x<2;++x){if(depth==8)n(row,c==3?255:17+x*93+c*11,1);else if(depth==16)n(row,c==3?65535:101+x*10321+c*47,2);else{const float value=c==3?1.f:x?12.5f:-.25f;quint32 bits;std::memcpy(&bits,&value,4);n(row,bits,4);}}
        QByteArray data;n(data,compression,2);
        if(compression==0)data+=row;
        else if(compression==1){n(data,row.size()+1,2);n(data,row.size()-1,1);data+=row;}
        else data+=zip(compression==3?prediction(row,depth):row);
        channels<<data;
    }
    QByteArray layer;n(layer,0,4);n(layer,0,4);n(layer,1,4);n(layer,2,4);n(layer,4,2);
    for(int c:{0,1,2,-1}){n(layer,quint16(c),2);n(layer,channels[c==-1?3:c].size(),4);}layer+="8BIMnorm";n(layer,255,1);n(layer,0,1);n(layer,8,1);n(layer,0,1);
    QByteArray extra;n(extra,0,4);n(extra,0,4);extra+=QByteArray("\x03RGB",4);section(layer,extra);
    QByteArray info;n(info,1,2);info+=layer;for(const auto &ch:channels)info+=ch;if(info.size()%2)info+=char(0);
    QByteArray outer;if(depth==8){section(outer,info);n(outer,0,4);}else{n(outer,0,4);n(outer,0,4);outer+="8BIM";outer+=depth==16?"Lr16":"Lr32";section(outer,info);while(info.size()%4){outer+=char(0);info+=char(0);}}
    QByteArray file="8BPS";n(file,1,2);file+=QByteArray(6,0);n(file,4,2);n(file,1,4);n(file,2,4);n(file,depth,2);n(file,3,2);n(file,0,4);n(file,0,4);section(file,outer);n(file,0,2);file+=QByteArray(2*4*bytes,0);
    const auto path=dir.filePath(QString("fixture-%1-%2.psd").arg(depth).arg(compression));QFile out(path);if(!out.open(QIODevice::WriteOnly))return {};out.write(file);return path;
}
}
class TestImagePsd : public QObject {
    Q_OBJECT
private slots:
    void advancedBlendingPsdRoundTrip(){
        QTemporaryDir dir;for(auto precision:{ImagePrecision::UInt8,ImagePrecision::UInt16}){ImageDocument doc({2,1},precision);QImage pixels(2,1,doc.pixelFormat());pixels.fill(QColor(80,90,100));doc.addLayer("Backdrop",pixels);pixels.fill(QColor(120,150,210));const auto id=doc.addLayer("Advanced",pixels);auto info=doc.layers().last();info.fillOpacity=153.f/255;info.channels=5;for(int c=0;c<4;++c){info.blendIfSource[c]={float(3+c)/255,float(30+c)/255,float(180+c)/255,float(240+c)/255};info.blendIfBackdrop[c]={float(4+c)/255,float(40+c)/255,float(190+c)/255,float(250+c)/255};}doc.updateLayer(info);const auto expected=doc.composite({0,0,2,1});const auto path=dir.filePath(precision==ImagePrecision::UInt8?"advanced8.psd":"advanced16.psd");QVERIFY2(ImagePsd::save(&doc,path).isEmpty(),"PSD blending write failed");QString error;auto loaded=ImagePsd::load(path,&error);QVERIFY2(loaded,qPrintable(error));const auto actual=loaded->layers().last();QCOMPARE(actual.fillOpacity,info.fillOpacity);QCOMPARE(actual.channels,info.channels);QCOMPARE(actual.blendIfSource,info.blendIfSource);QCOMPARE(actual.blendIfBackdrop,info.blendIfBackdrop);QCOMPARE(loaded->readRegion(actual.id,{0,0,2,1}),doc.readRegion(id,{0,0,2,1}));QCOMPARE(loaded->composite({0,0,2,1}),expected);if(precision==ImagePrecision::UInt16){QFile::remove("build/image-editor-advanced.psd");QVERIFY(QFile::copy(path,"build/image-editor-advanced.psd"));}}
    }
    void independentCompressionFixtures(){
        QTemporaryDir dir;QVERIFY(dir.isValid());
        for(int depth:{8,16,32})for(int compression:{0,1,2,3}){
            QString error;auto doc=ImagePsd::load(fixture(dir,depth,compression),&error);QVERIFY2(doc,error.toUtf8());QCOMPARE(doc->layers().size(),1);
            const auto id=doc->layers().first().id;const auto pixels=doc->readRegion(id,QRect(0,0,2,1));QVERIFY(!pixels.isNull());
            if(depth==8){QCOMPARE(pixels.constBits()[0],uchar(17));QCOMPARE(pixels.constBits()[4],uchar(110));}
            else if(depth==16){const auto *p=reinterpret_cast<const quint16 *>(pixels.constBits());QCOMPARE(p[0],quint16(101));QCOMPARE(p[4],quint16(10422));}
            else{const auto *p=reinterpret_cast<const float *>(pixels.constBits());QCOMPARE(p[0],-.25f);QCOMPARE(p[4],12.5f);}
            const auto original=pixels;QImage change(1,1,doc->pixelFormat());change.fill(Qt::green);doc->writeRegion(id,{},change);doc->history()->undo();QCOMPARE(doc->readRegion(id,QRect(0,0,2,1)),original);
            const auto smooth=doc->compositePreview({0,0,2,1},{1,1},ImageSamplingQuality::Bilinear);QVERIFY(!smooth.isNull());if(depth==32)QVERIFY(std::abs(reinterpret_cast<const float *>(smooth.constBits())[0]-6.125f)<1e-5f);
        }
    }
    void groupsMasksAndPsdRoundTrip(){
        QTemporaryDir dir;ImageDocument doc({7,5},ImagePrecision::UInt16);doc.setColorSpace(QColorSpace::SRgb);
        QImage pixels(3,2,QImage::Format_RGBA64);pixels.fill(QColor(QRgba64::fromRgba64(12345,54321,7,65535)));pixels.setColorSpace(doc.colorSpace());const auto group=doc.addLayer("组");auto info=doc.layers().last();info.group=true;doc.updateLayer(info);
        const auto id=doc.addLayer("独立像素",pixels,{2,1});info=doc.layers().last();info.parentId=group;info.blend=ImageBlendMode::Screen;doc.updateLayer(info);
        QImage mask(3,2,QImage::Format_Grayscale16);mask.fill(Qt::white);reinterpret_cast<quint16 *>(mask.scanLine(0))[0]=123;doc.writeMask(id,{},mask);const auto expected=doc.composite({0,0,7,5});
        const auto path=dir.filePath("saved.psd");QVERIFY2(ImagePsd::save(&doc,path).isEmpty(),"PSD write failed");QString error;auto loaded=ImagePsd::load(path,&error);QVERIFY2(loaded,error.toUtf8());QCOMPARE(loaded->layers().size(),2);QVERIFY(loaded->layers().last().group);QCOMPARE(loaded->layers().first().parentId,loaded->layers().last().id);QCOMPARE(loaded->readRegion(loaded->layers().first().id,{0,0,3,2}),pixels);QCOMPARE(loaded->maskRegion(loaded->layers().first().id,{0,0,3,2}),mask);QCOMPARE(loaded->composite({0,0,7,5}),expected);
        auto layer=loaded->layers().last();layer.visible=false;loaded->updateLayer(layer);QCOMPARE(loaded->composite({0,0,7,5}).pixelColor(3,2).alpha(),0);loaded->history()->undo();QCOMPARE(loaded->composite({0,0,7,5}),expected);
        QFile::remove("build/image-editor-roundtrip.psd");QVERIFY(QFile::copy(path,"build/image-editor-roundtrip.psd"));
    }
    void canvasPrecisionAndLazyExtents(){
        ImageDocument doc({10,10},ImagePrecision::UInt16);int reads=0;const auto id=doc.addLazyLayer("lazy",{2,2},{1,1},[&](const QRect &r){++reads;QImage image(r.size(),QImage::Format_RGBA64);image.fill(QColor(QRgba64::fromRgba64(123,456,789,65535)));return image;});
        QCOMPARE(reads,0);QImage pixel(1,1,QImage::Format_RGBA64);pixel.fill(Qt::red);doc.writeRegion(id,{-2,-3},pixel);QVERIFY(doc.layerBounds(id).contains(-2,-3));bool included=false;for(auto r:doc.layerRegions(id))included|=r.contains(-2,-3);QVERIFY(included);
        doc.crop({2,2,6,6});QCOMPARE(doc.size(),QSize(6,6));QCOMPARE(doc.layers().first().offset,QPoint(-1,-1));doc.history()->undo();QCOMPARE(doc.size(),QSize(10,10));QCOMPARE(doc.layers().first().offset,QPoint(1,1));
        doc.convertPrecision(ImagePrecision::Float32);QCOMPARE(doc.precision(),ImagePrecision::Float32);doc.history()->undo();QCOMPARE(doc.precision(),ImagePrecision::UInt16);
        doc.convertPrecision(ImagePrecision::UInt8);doc.convertPrecision(ImagePrecision::UInt16);QCOMPARE(reinterpret_cast<const quint16 *>(doc.readRegion(id,{0,0,1,1}).constBits())[0],quint16(0));doc.history()->undo();doc.history()->undo();QCOMPARE(reinterpret_cast<const quint16 *>(doc.readRegion(id,{0,0,1,1}).constBits())[0],quint16(123));
    }
    void snapshotSurvivesSourceAndGroupMask(){
        auto doc=std::make_unique<ImageDocument>(QSize(10,10),ImagePrecision::Float32);doc->setCacheBudget(0);QImage pixels(10,10,QImage::Format_RGBA32FPx4);pixels.fill(QColor(Qt::red));auto id=doc->addLayer("backdrop",pixels);
        const auto group=doc->addLayer("group");auto info=doc->layers().last();info.group=true;info.passThrough=true;info.opacity=.5f;doc->updateLayer(info);
        pixels.fill(QColor(Qt::green));const auto child=doc->addLayer("multiply",pixels);info=doc->layers().last();info.parentId=group;info.blend=ImageBlendMode::Multiply;doc->updateLayer(info);QImage mask(10,10,QImage::Format_Grayscale16);mask.fill(Qt::white);reinterpret_cast<quint16 *>(mask.scanLine(0))[0]=0;doc->writeMask(group,{},mask);
        const auto expected=doc->composite({0,0,10,10});QCOMPARE(expected.pixelColor(0,0),QColor(Qt::red));const auto *p=reinterpret_cast<const float *>(expected.constScanLine(1));QCOMPARE(p[0],.5f);QCOMPARE(p[1],0.f);
        auto snapshot=doc->snapshot();doc.reset();QCOMPARE(snapshot->composite({0,0,10,10}),expected);QCOMPARE(snapshot->layerBounds(child),QRect(0,0,10,10));
        const auto duplicate=snapshot->duplicateLayer(group);QVERIFY(!duplicate.isNull());QCOMPARE(snapshot->layers().size(),5);snapshot->removeLayer(duplicate);QCOMPARE(snapshot->layers().size(),3);snapshot->history()->undo();QCOMPARE(snapshot->layers().size(),5);
        snapshot->history()->undo();QTemporaryDir dir;auto path=dir.filePath("group-mask.psd");snapshot->convertPrecision(ImagePrecision::UInt16);QVERIFY(ImagePsd::save(snapshot.get(),path).isEmpty());QString error;auto loaded=ImagePsd::load(path,&error);QVERIFY2(loaded,error.toUtf8());for(auto layer:loaded->layers())if(layer.group){QCOMPARE(loaded->maskRegion(layer.id,{0,0,10,10}),mask);QCOMPARE(loaded->maskPreview(layer.id,{0,0,10,10},{10,10}),mask);}
    }
    void truncatedAndUnsupported(){
        QTemporaryDir dir;auto path=fixture(dir,16,3);QFile f(path);QVERIFY(f.open(QIODevice::ReadWrite));f.resize(40);f.close();QString error;QVERIFY(!ImagePsd::load(path,&error));QVERIFY(!error.isEmpty());
        ImageDocument tooWide({300001,1},ImagePrecision::UInt8);QVERIFY(!ImagePsd::save(&tooWide,dir.filePath("oversized.psb")).isEmpty());QVERIFY(!QFile::exists(dir.filePath("oversized.psb")));
    }
    void realLargePsd(){
        const auto path=qEnvironmentVariable("VSR_PSD_SAMPLE");if(path.isEmpty())QSKIP("Set VSR_PSD_SAMPLE for the user's large PSD");
        QString error;QStringList warnings;auto doc=ImagePsd::load(path,&error,&warnings);QVERIFY2(doc,error.toUtf8());QCOMPARE(doc->size(),QSize(5950,8420));QCOMPARE(doc->precision(),ImagePrecision::UInt16);QCOMPARE(doc->layers().size(),9);
        QCOMPARE(doc->layers().first().name,QString("背景"));
        qInfo()<<"Layers:";for(auto info:doc->layers())qInfo()<<info.name<<info.visible<<info.offset<<info.extent;
        auto preview=doc->compositePreview(QRect(QPoint(),doc->size()),{350,495});QVERIFY(!preview.isNull());QVERIFY(doc->storageError().isEmpty());QVERIFY(preview.save("build/large-psd-preview.png"));
        const auto layer=doc->layers()[5];QVERIFY(!layer.group);const auto before=doc->readRegion(layer.id,{0,0,16,16});QVERIFY(!before.isNull());QImage pixel(1,1,doc->pixelFormat());pixel.fill(Qt::red);doc->writeRegion(layer.id,{},pixel);doc->history()->undo();QCOMPARE(doc->readRegion(layer.id,{0,0,16,16}),before);qInfo()<<"Rasterization warnings:"<<warnings;qInfo()<<"Resident tile bytes:"<<doc->residentBytes();
    }
};
QTEST_MAIN(TestImagePsd)
#include "TestImagePsd.moc"
