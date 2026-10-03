#include "image/ImageEditorCanvas.h"
#include <QApplication>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>
#include <atomic>
#include <cstring>

using namespace vsr;
namespace {
QImage floating(QSize size,float r,float g,float b,float a=1) {
    QImage result(size,QImage::Format_RGBA32FPx4);for(int y=0;y<size.height();++y){auto *row=reinterpret_cast<float *>(result.scanLine(y));for(int x=0;x<size.width();++x){row[x*4]=r;row[x*4+1]=g;row[x*4+2]=b;row[x*4+3]=a;}}return result;
}
float channel(const QImage &image,int x,int y,int c) {return reinterpret_cast<const float *>(image.constScanLine(y))[x*4+c];}
quint16 coverage(ImageDocument &document,QPoint p) {const auto image=document.selectionRegion(QRect(p,QSize(1,1)));return reinterpret_cast<const quint16 *>(image.constBits())[0];}
}
class TestImageEditorAdvanced final : public QObject {
    Q_OBJECT
private slots:
    void viewportPanningReusesNativePreview() {
        ImageDocument document({100000,100000},ImagePrecision::UInt16);std::atomic<int> reads=0;
        const auto id=document.addLazyLayer("Huge",document.size(),{},[&](const QRect &region){++reads;QImage pixels(region.size(),QImage::Format_RGBA64);pixels.fill(QColor(120,80,30));return pixels;});
        ImageEditorTools tools(&document);ImageEditorCanvas canvas(&tools);canvas.resize(700,500);canvas.show();canvas.actualSize();QSignalSpy ready(&canvas,&ImageEditorCanvas::previewReady);QTRY_VERIFY_WITH_TIMEOUT(!canvas.previewBusy(),30000);QVERIFY(reads.load()>0);const int before=reads.load(),renders=ready.count();const auto zoom=canvas.zoom();const auto position=canvas.screenPoint({50000,50000});
        auto wheel=[&](Qt::KeyboardModifiers modifiers){const QPointF at(canvas.width()/2.,canvas.height()/2.);QWheelEvent event(at,canvas.mapToGlobal(at.toPoint()),{},QPoint(0,120),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);QApplication::sendEvent(&canvas,&event);};
        wheel(Qt::NoModifier);QCOMPARE(canvas.zoom(),zoom);QCOMPARE(canvas.screenPoint({50000,50000}),position+QPointF(0,40));QVERIFY(!canvas.previewBusy());
        wheel(Qt::ShiftModifier);QCOMPARE(canvas.screenPoint({50000,50000}),position+QPointF(40,40));QCOMPARE(canvas.zoom(),zoom);QVERIFY(!canvas.previewBusy());
        wheel(Qt::ControlModifier);QCOMPARE(canvas.zoom(),zoom*1.2);QVERIFY(!canvas.previewBusy());QTest::qWait(10);QCOMPARE(reads.load(),before);QCOMPARE(ready.count(),renders);
        auto info=document.layers().last();info.opacity=.5;document.updateLayer(info);QTRY_VERIFY_WITH_TIMEOUT(!canvas.previewBusy(),30000);QVERIFY(ready.count()>renders);QVERIFY(reads.load()>before);QCOMPARE(document.precision(),ImagePrecision::UInt16);QCOMPARE(tools.layer(),id);
        const auto revision=document.revision();const int updated=ready.count();canvas.invalidateDocumentPreview();QTRY_VERIFY_WITH_TIMEOUT(!canvas.previewBusy(),30000);QVERIFY(ready.count()>updated);QCOMPARE(document.revision(),revision);
    }
    void enlargedPreviewUsesSmoothDisplayAndLockedFeedback() {
        ImageDocument document({64,64},ImagePrecision::UInt16);QImage source(64,64,QImage::Format_RGBA64);source.fill(Qt::black);for(int y=0;y<64;++y)for(int x=0;x<y;++x)reinterpret_cast<QRgba64 *>(source.scanLine(y))[x]=QRgba64::fromRgba64(65535,65535,65535,65535);
        const auto id=document.addLayer("Diagonal",source);ImageEditorTools tools(&document);ImageEditorCanvas canvas(&tools);canvas.resize(500,400);canvas.show();canvas.actualSize();canvas.zoomBy(3);QTRY_VERIFY_WITH_TIMEOUT(!canvas.previewBusy(),30000);
        const auto image=canvas.grab().toImage();const double dpr=image.devicePixelRatio();QImage reference(image.size(),QImage::Format_ARGB32_Premultiplied);reference.setDevicePixelRatio(dpr);reference.fill(Qt::black);
        QPainter painter(&reference);painter.setRenderHint(QPainter::SmoothPixmapTransform);painter.drawImage(QRectF(canvas.screenPoint({0,0}),QSizeF(document.size())*canvas.zoom()),ImageHdr::preview(source));painter.end();
        const auto comparePixel=[&](QPoint point){const auto actual=image.pixelColor(point),expected=reference.pixelColor(point);return std::abs(actual.red()-expected.red())<=2&&std::abs(actual.green()-expected.green())<=2&&std::abs(actual.blue()-expected.blue())<=2;};
        int intermediate=0;for(int y=12;y<48;++y){const auto at=canvas.screenPoint({double(y),double(y)});for(int x=-4;x<=4;++x){const QPoint point(qRound(at.x()*dpr)+x,qRound(at.y()*dpr));QVERIFY2(comparePixel(point),qPrintable(QString("Smooth reference differs at %1,%2").arg(point.x()).arg(point.y())));const auto color=image.pixelColor(point);if(color.red()>5&&color.red()<250)++intermediate;}}
        for(const auto &at:{QPointF(10,30),QPointF(30,10),QPointF(20,40),QPointF(40,20)}){const auto point=canvas.screenPoint(at)*dpr;QVERIFY(comparePixel(point.toPoint()));}
        QVERIFY(intermediate>20);QCOMPARE(document.readRegion(id,{0,0,64,64}),source);
        auto info=document.layers().last();info.locked=true;document.updateLayer(info);canvas.setTool(ImageEditorTool::Move);QSignalSpy errors(&canvas,&ImageEditorCanvas::previewError);const auto point=canvas.screenPoint({32,32}).toPoint();QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,point);QTest::mouseMove(&canvas,point+QPoint(20,15));QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,point+QPoint(20,15));QCOMPARE(errors.count(),1);QCOMPARE(document.layers().last().offset,QPoint());QCOMPARE(document.readRegion(id,{0,0,64,64}),source);
    }
    void defaultSdrPreviewPreservesExpectedPixels() {
        QImage source(3,1,QImage::Format_RGBA64);reinterpret_cast<QRgba64 *>(source.scanLine(0))[0]=QRgba64::fromRgba64(1001,2003,55005,65535);reinterpret_cast<QRgba64 *>(source.scanLine(0))[1]=QRgba64::fromRgba64(32768,30001,25003,32768);reinterpret_cast<QRgba64 *>(source.scanLine(0))[2]=QRgba64::fromRgba64(65535,0,65535,0);source.setColorSpace(QColorSpace::SRgb);
        QCOMPARE(ImageHdr::preview(source),source.convertToFormat(QImage::Format_RGBA8888));QCOMPARE(reinterpret_cast<const QRgba64 *>(source.constScanLine(0))[0].red(),quint16(1001));
        auto p3=source;p3.setColorSpace(QColorSpace::DisplayP3);QCOMPARE(ImageHdr::preview(p3),p3.convertedToColorSpace(QColorSpace::SRgb,QImage::Format_RGBA8888));
        const auto hdr=floating({1,1},-.25f,12.5f,.5f);const auto shown=ImageHdr::preview(hdr);QCOMPARE(shown.pixelColor(0,0).green(),255);QCOMPARE(channel(hdr,0,0,0),-.25f);QCOMPARE(channel(hdr,0,0,1),12.5f);
    }
    void pickVisiblePixelLayers() {
        ImageDocument document({32,24},ImagePrecision::Float32);
        const auto background=document.addLayer("Background",floating({32,24},-.25f,12.5f,.5f));
        const auto group=document.addLayer("Group");auto groupInfo=document.layers().last();groupInfo.group=true;document.updateLayer(groupInfo);
        auto pixels=floating({4,4},12.5f,-.25f,.5f);reinterpret_cast<float *>(pixels.scanLine(1))[1*4+3]=0;
        const auto child=document.addLayer("Child",pixels,{8,6});auto childInfo=document.layers().last();childInfo.parentId=group;document.updateLayer(childInfo);
        ImageEditorTools tools(&document);tools.setLayer(background);QSignalSpy changed(&tools,&ImageEditorTools::layerChanged);
        QCOMPARE(tools.pickLayer({8,6}),child);QCOMPARE(tools.pickLayer({9,7}),background);QCOMPARE(tools.pickLayer({2,2}),background);QVERIFY(tools.pickLayer({-1,0}).isNull());
        tools.setLayer(child);QCOMPARE(changed.count(),1);tools.setLayer(child);tools.setLayer(QUuid::createUuid());tools.setLayer({});QCOMPARE(changed.count(),1);QCOMPARE(tools.layer(),child);
        QVERIFY(tools.autoPickLayer({9,7}));QCOMPARE(tools.layer(),background);QCOMPARE(changed.count(),2);QVERIFY(!tools.autoPickLayer({32,24}));QCOMPARE(tools.layer(),background);QCOMPARE(changed.count(),2);
        childInfo.locked=true;document.updateLayer(childInfo);QCOMPARE(tools.pickLayer({8,6}),background);childInfo.locked=false;document.updateLayer(childInfo);
        groupInfo.locked=true;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({8,6}),background);groupInfo.locked=false;groupInfo.visible=false;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({8,6}),background);
        groupInfo.visible=true;groupInfo.opacity=0;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({8,6}),background);groupInfo.opacity=1;document.updateLayer(groupInfo);
        QImage mask(1,1,QImage::Format_Grayscale16);mask.fill(0);document.writeMask(child,{0,0},mask);QCOMPARE(tools.pickLayer({8,6}),background);childInfo.maskEnabled=false;document.updateLayer(childInfo);QCOMPARE(tools.pickLayer({8,6}),child);
        document.writeMask(group,{8,6},mask);QCOMPARE(tools.pickLayer({8,6}),background);groupInfo.maskEnabled=false;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({8,6}),child);
        QCOMPARE(channel(document.readRegion(child,{0,0,1,1}),0,0,0),12.5f);QCOMPARE(channel(document.readRegion(background,{0,0,1,1}),0,0,0),-.25f);
    }
    void pickClippingAndLazyBounds() {
        ImageDocument document({40,30},ImagePrecision::UInt16);QImage backgroundPixels(40,30,QImage::Format_RGBA64);backgroundPixels.fill(Qt::white);const auto background=document.addLayer("Background",backgroundPixels);
        ImageEditorTools tools(&document);int reads=0;const auto group=document.addLayer("Lazy group");auto groupInfo=document.layers().last();groupInfo.group=true;document.updateLayer(groupInfo);
        const auto lazy=document.addLazyLayer("Lazy",{4,4},{12,10},[&](const QRect &region){++reads;QImage pixels(region.size(),QImage::Format_RGBA64);pixels.fill(Qt::transparent);const auto visible=region.intersected(QRect(0,0,4,4));for(int y=visible.top();y<visible.y()+visible.height();++y)for(int x=visible.left();x<visible.x()+visible.width();++x)reinterpret_cast<QRgba64 *>(pixels.scanLine(y-region.y()))[x-region.x()]=QRgba64::fromRgba64(65535,0,0,65535);return pixels;});auto lazyInfo=document.layers().last();lazyInfo.parentId=group;document.updateLayer(lazyInfo);
        QCOMPARE(tools.pickLayer({1,1}),background);QCOMPARE(reads,0);groupInfo.visible=false;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({12,10}),background);QCOMPARE(reads,0);
        groupInfo.visible=true;document.updateLayer(groupInfo);QCOMPARE(tools.pickLayer({12,10}),lazy);QVERIFY(reads>0);
        const auto base=document.addLayer("Clip base");auto baseInfo=document.layers().last();baseInfo.group=true;document.updateLayer(baseInfo);
        QImage basePixels(4,4,QImage::Format_RGBA64);basePixels.fill(Qt::blue);reinterpret_cast<QRgba64 *>(basePixels.scanLine(1))[1]=QRgba64::fromRgba64(0,0,65535,0);
        document.addLayer("Base pixels",basePixels,{24,10});auto baseChild=document.layers().last();baseChild.parentId=base;document.updateLayer(baseChild);
        QImage topPixels(4,4,QImage::Format_RGBA64);topPixels.fill(Qt::green);const auto clipped=document.addLayer("Clipped",topPixels,{24,10});auto clipInfo=document.layers().last();clipInfo.clipping=true;document.updateLayer(clipInfo);
        QCOMPARE(document.composite({24,10,1,1}).pixelColor(0,0),QColor(Qt::green));
        QCOMPARE(tools.pickLayer({24,10}),clipped);
        QCOMPARE(document.composite({25,11,1,1}).pixelColor(0,0),QColor(Qt::white));
        QCOMPARE(tools.pickLayer({25,11}),background);
        baseInfo.visible=false;document.updateLayer(baseInfo);
        QCOMPARE(document.composite({24,10,1,1}).pixelColor(0,0),QColor(Qt::white));
        QCOMPARE(tools.pickLayer({24,10}),background);
        baseInfo.visible=true;baseInfo.locked=true;document.updateLayer(baseInfo);QCOMPARE(tools.pickLayer({24,10}),clipped);clipInfo.locked=true;document.updateLayer(clipInfo);QCOMPARE(tools.pickLayer({24,10}),background);
        QImage mask(1,1,QImage::Format_Grayscale16);mask.fill(0);document.writeMask(base,{24,10},mask);clipInfo.locked=false;document.updateLayer(clipInfo);QCOMPARE(tools.pickLayer({24,10}),background);
    }
    void connectedSelectionAndEdgeSegmentation() {
        ImageDocument document({30,20},ImagePrecision::UInt16);QImage source(30,20,QImage::Format_RGBA64);source.fill(Qt::white);
        for(int y=4;y<16;++y)for(int x=4;x<13;++x)reinterpret_cast<QRgba64 *>(source.scanLine(y))[x]=QRgba64::fromRgba64(1001,2003,55005,65535);
        for(int y=4;y<16;++y)for(int x=20;x<25;++x)reinterpret_cast<QRgba64 *>(source.scanLine(y))[x]=QRgba64::fromRgba64(1001,2003,55005,65535);
        const auto layer=document.addLayer("Pixels",source);document.history()->clear();ImageEditorTools tools(&document);tools.setTolerance(.001);
        QVERIFY(tools.selectColor({6,6},ImageSelectionMode::Replace));QCOMPARE(coverage(document,{6,6}),quint16(65535));QCOMPARE(coverage(document,{21,6}),quint16(0));QCOMPARE(coverage(document,{2,2}),quint16(0));
        QVERIFY(tools.selectColor({21,6},ImageSelectionMode::Add));QCOMPARE(coverage(document,{21,6}),quint16(65535));document.history()->undo();QCOMPARE(coverage(document,{21,6}),quint16(0));
        QVERIFY(tools.selectObject({1,1,16,18},ImageSelectionMode::Replace));QCOMPARE(coverage(document,{6,6}),quint16(65535));QCOMPARE(coverage(document,{2,2}),quint16(0));QCOMPARE(coverage(document,{21,6}),quint16(0));
        QVERIFY(tools.featherSelection(2));QVERIFY(coverage(document,{4,6})>0);QVERIFY(coverage(document,{4,6})<65535);document.history()->undo();QCOMPARE(coverage(document,{4,6}),quint16(65535));
        document.clearSelection();QVERIFY(tools.invertSelection());QCOMPARE(coverage(document,{6,6}),quint16(0));
        document.clearSelection();QPainterPath fillArea;fillArea.addRect(5,5,4,4);QVERIFY(tools.selectPath(fillArea,ImageSelectionMode::Replace));const auto selectionId=document.selectionId();tools.setBrushColor(Qt::red);const int previousHistory=document.history()->count();QVERIFY(tools.floodFill({6,6}));QCOMPARE(document.selectionId(),selectionId);QCOMPARE(document.history()->count(),previousHistory+1);QCOMPARE(document.readRegion(layer,{6,6,1,1}).pixelColor(0,0),QColor(Qt::red));QCOMPARE(document.readRegion(layer,{10,10,1,1}).pixelColor(0,0).rgba64(),QRgba64::fromRgba64(1001,2003,55005,65535));document.history()->undo();QCOMPARE(document.readRegion(layer,{6,6,1,1}).pixelColor(0,0).rgba64(),QRgba64::fromRgba64(1001,2003,55005,65535));
        ImageDocument wide({600,2},ImagePrecision::UInt16);ImageEditorTools wideTools(&wide);QVERIFY(wideTools.invertSelection());QCOMPARE(coverage(wide,{500,0}),quint16(0));
        ImageDocument huge({100000,100000},ImagePrecision::Float32);ImageEditorTools hugeTools(&huge);QSignalSpy errors(&hugeTools,&ImageEditorTools::errorOccurred);QVERIFY(!hugeTools.selectColor({1,1},ImageSelectionMode::Replace));QCOMPARE(errors.count(),1);QVERIFY(!huge.hasSelection());QVERIFY(hugeTools.selectColor({1,1},ImageSelectionMode::Replace,{0,0,4,4}));QCOMPARE(coverage(huge,{1,1}),quint16(65535));
    }
    void nativeEffectsCloningAndHistory() {
        ImageDocument document({64,64},ImagePrecision::Float32);const auto source=floating({64,64},-.25f,12.5f,.50001f);const auto id=document.addLayer("HDR",source);document.history()->clear();ImageEditorTools tools(&document);tools.setBrushRadius(2);tools.setBrushOpacity(1);
        QVERIFY(tools.beginStroke({20.5,20.5},ImageEditorTool::Blur));tools.endStroke();auto pixel=document.readRegion(id,{20,20,1,1});QVERIFY(std::abs(channel(pixel,0,0,1)-12.5f)<1e-5f);QCOMPARE(channel(pixel,0,0,0),-.25f);
        QVERIFY(tools.beginStroke({20.5,20.5},ImageEditorTool::Dodge));tools.endStroke();pixel=document.readRegion(id,{20,20,1,1});QCOMPARE(channel(pixel,0,0,1),15.625f);QCOMPARE(channel(pixel,0,0,3),1.f);
        QVERIFY(tools.beginStroke({20.5,20.5},ImageEditorTool::HistoryBrush));tools.endStroke();pixel=document.readRegion(id,{20,20,1,1});QCOMPARE(channel(pixel,0,0,1),12.5f);QCOMPARE(channel(pixel,0,0,0),-.25f);
        document.writeRegion(id,{40,40},floating({5,5},.1f,.2f,.3f));tools.setCloneSource({10.5,10.5});QVERIFY(tools.beginStroke({42.5,42.5},ImageEditorTool::Clone));tools.endStroke();pixel=document.readRegion(id,{42,42,1,1});QCOMPARE(channel(pixel,0,0,1),12.5f);QCOMPARE(channel(pixel,0,0,0),-.25f);
        document.history()->undo();pixel=document.readRegion(id,{42,42,1,1});QCOMPARE(channel(pixel,0,0,1),.2f);
        auto info=document.layers().last();info.locked=true;document.updateLayer(info);const int history=document.history()->count();QVERIFY(!tools.beginStroke({20.5,20.5},ImageEditorTool::Burn));QCOMPARE(document.history()->count(),history);
        ImageDocument grouped({100,100},ImagePrecision::Float32);const auto group=grouped.addLayer("Group");auto groupInfo=grouped.layers().last();groupInfo.group=true;grouped.updateLayer(groupInfo);const auto child=grouped.addLayer("Child",floating({2,2},-.25f,12.5f,.5f),{10,10});auto childInfo=grouped.layers().last();childInfo.parentId=group;grouped.updateLayer(childInfo);grouped.history()->clear();ImageEditorTools groupTools(&grouped);groupTools.setLayer(group);QVERIFY(groupTools.beginMove({10,10}));groupTools.continueMove({20,15});groupTools.endMove();QCOMPARE(grouped.layers().last().offset,QPoint(20,15));QCOMPARE(channel(grouped.composite({20,15,1,1}),0,0,1),12.5f);QCOMPARE(grouped.history()->count(),1);grouped.history()->undo();QCOMPARE(grouped.layers().last().offset,QPoint(10,10));childInfo.locked=true;grouped.updateLayer(childInfo);QVERIFY(!groupTools.beginMove({10,10}));QVERIFY(!groupTools.beginStroke({10,10},ImageEditorTool::Brush));QVERIFY(!child.isNull());
    }
    void nativeHealingShapesAndGradient() {
        ImageDocument document({40,40},ImagePrecision::Float32);const auto id=document.addLayer("Pixels",floating({40,40},-.25f,12.5f,.50001f));ImageEditorTools tools(&document);tools.setBrushRadius(2);document.writeRegion(id,{20,20},floating({1,1},30,50,20));
        QVERIFY(tools.beginStroke({20.5,20.5},ImageEditorTool::Heal));tools.endStroke();auto pixel=document.readRegion(id,{20,20,1,1});QCOMPARE(channel(pixel,0,0,0),-.25f);QCOMPARE(channel(pixel,0,0,1),12.5f);QCOMPARE(channel(pixel,0,0,3),1.f);
        QPainterPath region;region.addRect(0,0,4,4);tools.setBrushColor(Qt::red);QVERIFY(tools.fillPath(region));pixel=document.readRegion(id,{2,2,1,1});QCOMPARE(channel(pixel,0,0,0),1.f);QCOMPARE(channel(document.readRegion(id,{10,10,1,1}),0,0,1),12.5f);document.history()->undo();QCOMPARE(channel(document.readRegion(id,{2,2,1,1}),0,0,1),12.5f);
        QVERIFY(tools.selectPath(region,ImageSelectionMode::Replace));tools.setGradientEndColor(Qt::blue);QVERIFY(tools.fillGradient({0,0},{4,0}));pixel=document.readRegion(id,{2,2,1,1});QVERIFY(channel(pixel,0,0,0)>.2f);QVERIFY(channel(pixel,0,0,2)>.2f);QCOMPARE(channel(document.readRegion(id,{10,10,1,1}),0,0,1),12.5f);
        document.clearSelection();tools.setText("A",QFont("Arial",12));const int history=document.history()->count();QVERIFY(tools.createText({5,25}));QCOMPARE(document.history()->count(),history+1);
        ImageDocument edge({30,20},ImagePrecision::UInt8);QImage pixels(30,20,QImage::Format_RGBA8888);pixels.fill(Qt::black);for(int y=0;y<20;++y)for(int x=15;x<30;++x)pixels.setPixelColor(x,y,Qt::white);edge.addLayer("Edge",pixels);ImageEditorTools edgeTools(&edge);const auto snapped=edgeTools.magneticPoint({12,10});QVERIFY(snapped.x()>=14);QVERIFY(snapped.x()<=16);
    }
    void maskEditingPreservesPixelsAndHistory() {
        ImageDocument document({600,400},ImagePrecision::Float32);const auto original=floating({10,10},-.25f,12.5f,.50001f);const auto id=document.addLayer("Masked",original,{250,250});document.history()->clear();ImageEditorTools tools(&document);tools.setEditingMask(true);tools.setBrushRadius(1);tools.setBrushOpacity(.5);tools.setBrushColor(Qt::black);
        QVERIFY(tools.beginStroke({255.5,255.5},ImageEditorTool::Brush));tools.endStroke();auto mask=document.maskRegion(id,{5,5,1,1});QCOMPARE(reinterpret_cast<const quint16 *>(mask.constBits())[0],quint16(32768));const auto unchanged=document.readRegion(id,{0,0,10,10});QVERIFY(std::memcmp(unchanged.constScanLine(0),original.constScanLine(0),10*16)==0);
        document.history()->undo();mask=document.maskRegion(id,{5,5,1,1});QCOMPARE(reinterpret_cast<const quint16 *>(mask.constBits())[0],quint16(65535));document.history()->redo();
        tools.setBrushOpacity(1);QPainterPath path;path.addRect(254,254,3,3);QVERIFY(tools.fillPath(path));mask=document.maskRegion(id,{5,5,1,1});QCOMPARE(reinterpret_cast<const quint16 *>(mask.constBits())[0],quint16(0));QVERIFY(!tools.beginStroke({255.5,255.5},ImageEditorTool::Clone));
        QVERIFY(tools.beginStroke({255.5,255.5},ImageEditorTool::Eraser));tools.endStroke();mask=document.maskRegion(id,{5,5,1,1});QCOMPARE(reinterpret_cast<const quint16 *>(mask.constBits())[0],quint16(65535));
    }
    void advancedCanvasGestures() {
        ImageDocument document({100,80},ImagePrecision::UInt16);QImage pixels(100,80,QImage::Format_RGBA64);pixels.fill(Qt::white);document.addLayer("Layer",pixels);document.history()->clear();ImageEditorTools tools(&document);ImageEditorCanvas canvas(&tools);canvas.resize(640,480);canvas.show();canvas.fitToWindow();QTest::qWait(20);
        const auto drag=[&](QPointF a,QPointF b){QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint(a).toPoint());QTest::mouseMove(&canvas,canvas.screenPoint(b).toPoint());QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint(b).toPoint());};
        canvas.setTool(ImageEditorTool::Ruler);QSignalSpy measured(&canvas,&ImageEditorCanvas::rulerMeasured);drag({10,10},{40,50});QCOMPARE(measured.count(),1);QVERIFY(std::abs(measured.first().first().toDouble()-50)<1);
        canvas.setTool(ImageEditorTool::Slice);QSignalSpy slices(&canvas,&ImageEditorCanvas::sliceCreated);drag({10,10},{40,30});QCOMPARE(slices.count(),1);QVERIFY(slices.first().first().toRect().width()>=29);
        canvas.setTool(ImageEditorTool::Path);tools.setBrushColor(Qt::red);tools.setBrushRadius(1);QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint({10,10}).toPoint());QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint({40,10}).toPoint());QTest::keyClick(&canvas,Qt::Key_Return);QVERIFY(!canvas.vectorPath().isEmpty());QVERIFY(document.composite({20,10,1,1}).pixelColor(0,0).red()>200);QVERIFY(document.composite({20,10,1,1}).pixelColor(0,0).green()<200);
        canvas.setTool(ImageEditorTool::Crop);drag({10,10},{50,40});drag({30,25},{40,30});QTest::keyClick(&canvas,Qt::Key_Return);QVERIFY(document.size().width()>=39 && document.size().width()<=41);QVERIFY(document.size().height()>=29 && document.size().height()<=31);document.history()->undo();QCOMPARE(document.size(),QSize(100,80));
    }
};
QTEST_MAIN(TestImageEditorAdvanced)
#include "TestImageEditorAdvanced.moc"
