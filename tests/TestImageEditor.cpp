#include "image/ImageEditorWindow.h"
#include "image/ImageEditorCanvas.h"
#include "player/PlayerImageTools.h"
#include "ui/PreviewPane.h"
#include <QTreeWidget>
#include <QFile>
#include <QLineF>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QWheelEvent>
#include <QPointer>
#include <cstring>

using namespace vsr;
class TestImageEditor final : public QObject {
    Q_OBJECT
private slots:
    void viewerWheelZoomsImagesAndRetainsShiftPan() {
        PreviewPane pane("Preview","");pane.resize(700,500);pane.show();QImage image(1400,1000,QImage::Format_RGB32);image.fill(Qt::white);pane.setImage(image);pane.adoptView(3,0,0);
        auto *surface=pane.surface();const auto point=surface->rect().center();auto wheel=[&](Qt::KeyboardModifiers modifiers){QWheelEvent event(point,surface->mapToGlobal(point),{},QPoint(0,120),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);QApplication::sendEvent(surface,&event);};
        wheel(Qt::NoModifier);QCOMPARE(pane.zoom(),3.75f);const auto y=pane.pan().y();wheel(Qt::ShiftModifier);QVERIFY(pane.pan().x()>0);QCOMPARE(pane.pan().y(),y);wheel(Qt::ControlModifier);QVERIFY(pane.zoom()>3.75f);
        pane.setImage({});pane.setVideoSize({1920,1080});pane.adoptView(1,0,0);wheel(Qt::NoModifier);QVERIFY(pane.zoom()>1);
    }
    void detailedEditorReusesOneWindow() {
        QTemporaryDir directory;QVERIFY(directory.isValid());QImage image(240,180,QImage::Format_RGBA8888);image.fill(Qt::red);const auto first=directory.filePath("one.png"),second=directory.filePath("two.png");QVERIFY(image.save(first));QVERIFY(image.save(second));
        PreviewPane pane("Preview","");pane.setImage(image);PlayerImageTools toolbar(&pane);toolbar.setSource(first);toolbar.setReady(true);auto *button=toolbar.findChild<QToolButton *>("imageTool_edit");QVERIFY(button);button->click();QPointer<ImageEditorWindow> editor;for(auto *widget:qApp->topLevelWidgets())if(auto *candidate=qobject_cast<ImageEditorWindow *>(widget))editor=candidate;QVERIFY(editor);QCOMPARE(editor->documentCount(),1);
        toolbar.setSource(second);toolbar.setReady(true);button->click();QCOMPARE(editor->documentCount(),2);int count=0;for(auto *widget:qApp->topLevelWidgets())if(qobject_cast<ImageEditorWindow *>(widget))++count;QCOMPARE(count,1);toolbar.setSource(first);toolbar.setReady(true);button->click();QCOMPARE(editor->documentCount(),2);QCOMPARE(editor->document()->metadata("sourcePath"),first);editor->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QVERIFY(editor.isNull());
    }
    void precisionStrokeSelectionAndUndo() {
        ImageDocument document({600,400},ImagePrecision::UInt16);QImage original(600,400,QImage::Format_RGBA64);
        original.fill(QColor::fromRgba64(1001,2003,3005,65535));const auto id=document.addLayer("Source",original);document.history()->clear();
        ImageEditorTools tools(&document);tools.setLayer(id);tools.setBrushRadius(4);tools.setBrushColor(Qt::red);
        QPainterPath rectangle;rectangle.addRect(QRectF(254,190,4,8));QVERIFY(tools.selectPath(rectangle,ImageSelectionMode::Replace));
        QVERIFY(tools.beginStroke({250.5,194.5},false));tools.continueStroke({263.5,194.5});tools.endStroke();QCOMPARE(document.history()->count(),2);
        const auto edited=document.readRegion(id,{250,194,14,1});QCOMPARE(edited.pixelColor(5,0),QColor(Qt::red));
        QCOMPARE(edited.pixelColor(0,0).rgba64(),QRgba64::fromRgba64(1001,2003,3005,65535));QCOMPARE(edited.pixelColor(13,0).rgba64(),QRgba64::fromRgba64(1001,2003,3005,65535));
        document.history()->undo();const auto restored=document.readRegion(id,{250,194,14,1});QVERIFY(std::memcmp(restored.constBits(),original.constScanLine(194)+250*8,14*8)==0);
        document.history()->redo();QCOMPARE(document.readRegion(id,{255,194,1,1}).pixelColor(0,0),QColor(Qt::red));
        tools.beginStroke({255.5,194.5},true);tools.endStroke(true);QCOMPARE(document.readRegion(id,{255,194,1,1}).pixelColor(0,0),QColor(Qt::red));QCOMPARE(document.history()->count(),2);
    }
    void floatEraserPreservesRgbAndMoveCoordinates() {
        ImageDocument document({512,512},ImagePrecision::Float32);QImage source(10,10,QImage::Format_RGBA32FPx4);
        for(int y=0;y<10;++y){auto *row=reinterpret_cast<float *>(source.scanLine(y));for(int x=0;x<10;++x){row[x*4]=-.25f;row[x*4+1]=12.5f;row[x*4+2]=.50001f;row[x*4+3]=1;}}
        const auto id=document.addLayer("Float",source,{250,250});document.history()->clear();ImageEditorTools tools(&document);tools.setLayer(id);tools.setBrushRadius(1);tools.setBrushOpacity(.5);
        tools.beginStroke({255.5,255.5},true);tools.endStroke();auto pixels=document.readRegion(id,{5,5,1,1});const auto *value=reinterpret_cast<const float *>(pixels.constBits());
        QCOMPARE(value[0],-.25f);QCOMPARE(value[1],12.5f);QCOMPARE(value[2],.50001f);QCOMPARE(value[3],.5f);
        document.history()->undo();tools.beginMove({250,250});tools.continueMove({240,260});tools.endMove();QCOMPARE(document.layers().last().offset,QPoint(240,260));
        QCOMPARE(document.composite({245,265,1,1}).pixelColor(0,0).alphaF(),1.f);document.history()->undo();QCOMPARE(document.layers().last().offset,QPoint(250,250));
        tools.beginMove({250,250});tools.continueMove({300,300});tools.endMove(true);QCOMPARE(document.layers().last().offset,QPoint(250,250));
    }
    void selectionCombinationsAndGiantLimit() {
        ImageDocument document({512,512},ImagePrecision::UInt8);ImageEditorTools tools(&document);
        QPainterPath a,b;a.addRect(QRectF(250,250,12,12));b.addEllipse(QRectF(254,254,12,12));
        QVERIFY(tools.selectPath(a,ImageSelectionMode::Replace));QVERIFY(tools.selectPath(b,ImageSelectionMode::Subtract));
        QCOMPARE(reinterpret_cast<const quint16 *>(document.selectionRegion({260,260,1,1}).constBits())[0],quint16(0));
        QCOMPARE(reinterpret_cast<const quint16 *>(document.selectionRegion({251,251,1,1}).constBits())[0],quint16(65535));
        QVERIFY(tools.selectPath(b,ImageSelectionMode::Add));QCOMPARE(reinterpret_cast<const quint16 *>(document.selectionRegion({260,260,1,1}).constBits())[0],quint16(65535));
        QVERIFY(tools.selectPath(a,ImageSelectionMode::Intersect));QCOMPARE(reinterpret_cast<const quint16 *>(document.selectionRegion({264,260,1,1}).constBits())[0],quint16(0));
        ImageDocument huge({100000,100000},ImagePrecision::UInt16);ImageEditorTools giant(&huge);QSignalSpy errors(&giant,&ImageEditorTools::errorOccurred);QPainterPath all;all.addRect(QRectF(0,0,100000,100000));QVERIFY(!giant.selectPath(all,ImageSelectionMode::Replace));QCOMPARE(errors.count(),1);QVERIFY(!huge.hasSelection());QCOMPARE(huge.history()->count(),0);
    }
    void pngPrecisionAndLimits() {
        QTemporaryDir directory;QVERIFY(directory.isValid());ImageDocument document({3,2},ImagePrecision::UInt16);QImage image(3,2,QImage::Format_RGBA64);image.fill(QColor::fromRgba64(1001,2003,3005,65535));document.addLayer("16bit",image);document.setColorSpace(QColorSpace::SRgb);
        const auto path=directory.filePath("edited.png");QVERIFY2(ImageEditorTools::savePng(&document,path).isEmpty(),"PNG export failed");QImage decoded(path);QCOMPARE(decoded.pixelColor(0,0).rgba64(),QRgba64::fromRgba64(1001,2003,3005,65535));QCOMPARE(decoded.colorSpace(),QColorSpace(QColorSpace::SRgb));
        ImageDocument floating({3,2},ImagePrecision::Float32);QVERIFY(!ImageEditorTools::savePng(&floating,directory.filePath("float.png")).isEmpty());QVERIFY(!QFile::exists(directory.filePath("float.png")));
        ImageDocument huge({100000,100000},ImagePrecision::UInt16);QVERIFY(!ImageEditorTools::savePng(&huge,directory.filePath("huge.png")).isEmpty());QVERIFY(!QFile::exists(directory.filePath("huge.png")));
    }
    void canvasMouseAndWindow() {
        QImage image(400,300,QImage::Format_RGBA64);image.fill(Qt::white);ImageEditorWindow editor(image,"example.png");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTest::qWait(50);
        auto *canvas=editor.canvas();canvas->fitToWindow();QCOMPARE(editor.document()->precision(),ImagePrecision::UInt16);QCOMPARE(editor.document()->history()->count(),0);
        QVERIFY(editor.findChild<QTreeWidget *>("imageEditorLayers"));canvas->setTool(ImageEditorTool::Rectangle);
        const auto first=canvas->screenPoint({100,100}).toPoint(),last=canvas->screenPoint({180,180}).toPoint();
        QTest::mousePress(canvas,Qt::LeftButton,Qt::NoModifier,first);QTest::mouseMove(canvas,last);QTest::mouseRelease(canvas,Qt::LeftButton,Qt::NoModifier,last);QVERIFY(editor.document()->hasSelection());
        auto layer=editor.document()->layers().first();layer.locked=false;editor.document()->updateLayer(layer);canvas->setTool(ImageEditorTool::Brush);editor.tools()->setBrushColor(Qt::red);const auto point=canvas->screenPoint({120.5,120.5}).toPoint();QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,point);QCOMPARE(editor.document()->composite({120,120,1,1}).pixelColor(0,0),QColor(Qt::red));
        canvas->setTool(ImageEditorTool::Eyedropper);QSignalSpy picked(canvas,&ImageEditorCanvas::colorPicked);QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,point);QCOMPARE(picked.count(),1);QCOMPARE(editor.tools()->brushColor(),QColor(Qt::red));
        const auto zoom=canvas->zoom();QWheelEvent wheel(point,canvas->mapToGlobal(point),{},QPoint(0,120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(canvas,&wheel);QVERIFY(canvas->zoom()>zoom);QVERIFY(QLineF(canvas->documentPoint(point),QPointF(120.5,120.5)).length()<2);
        canvas->setTool(ImageEditorTool::Polygon);for(const auto &p:QList<QPointF>{{20,20},{80,20},{50,80}})QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->screenPoint(p).toPoint());QTest::keyClick(canvas,Qt::Key_Return);QVERIFY(editor.document()->hasSelection());QCOMPARE(reinterpret_cast<const quint16 *>(editor.document()->selectionRegion({50,40,1,1}).constBits())[0],quint16(65535));
        QTest::qWait(20);QVERIFY(editor.grab().save("build/image-editor-smoke.png"));
        editor.document()->history()->setClean();
    }
    void untouchedWindowAndHugeCanvas() {
        QImage pixels(3,2,QImage::Format_RGBA32FPx4);pixels.fill(Qt::white);auto *pixel=reinterpret_cast<float *>(pixels.scanLine(0));pixel[0]=-.25f;pixel[1]=12.5f;
        ImageEditorWindow editor(pixels,"float.png");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTest::qWait(20);QCOMPARE(editor.document()->precision(),ImagePrecision::Float32);QVERIFY(editor.document()->history()->isClean());
        const auto read=editor.document()->readRegion(editor.tools()->layer(),{0,0,3,2});QVERIFY(std::memcmp(read.constScanLine(0),pixels.constScanLine(0),3*16)==0);QVERIFY(editor.close());
        ImageDocument huge({100000,100000},ImagePrecision::UInt16);huge.setCacheBudget(0);huge.addLayer("Sparse");ImageEditorTools tools(&huge);ImageEditorCanvas canvas(&tools);canvas.resize(640,480);canvas.show();canvas.fitToWindow();QTest::qWait(20);
        QVERIFY(canvas.zoom()<.01);QCOMPARE(huge.residentBytes(),0);QVERIFY(huge.storageError().isEmpty());QVERIFY(!canvas.grab().isNull());
    }
    void selectionOutlineAfterUndoAndNewBranch() {
        ImageDocument document({400,300},ImagePrecision::UInt16);QImage white(400,300,QImage::Format_RGBA64);white.fill(Qt::white);document.addLayer("White",white);document.history()->clear();
        ImageEditorTools tools(&document);ImageEditorCanvas canvas(&tools);canvas.resize(640,480);canvas.show();canvas.fitToWindow();QTest::qWait(20);
        const auto select=[&](QPointF first,QPointF last){canvas.setTool(ImageEditorTool::Rectangle);QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint(first).toPoint());QTest::mouseMove(&canvas,canvas.screenPoint(last).toPoint());QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint(last).toPoint());canvas.setTool(ImageEditorTool::Hand);QTest::qWait(20);};
        const auto outlineAt=[&](QPointF point){const auto screenshot=canvas.grab().toImage();const auto at=(canvas.screenPoint(point)*screenshot.devicePixelRatio()).toPoint();for(int y=at.y()-2;y<=at.y()+2;++y)for(int x=at.x()-2;x<=at.x()+2;++x)if(screenshot.pixelColor(x,y).red()<128)return true;return false;};
        select({40,40},{120,100});QTRY_VERIFY(!canvas.previewBusy());const auto selectionA=document.selectionRegion({0,0,400,300});QVERIFY(outlineAt({80,40}));QVERIFY(!outlineAt({260,140}));
        select({220,140},{300,200});QTRY_VERIFY(!canvas.previewBusy());QVERIFY(!outlineAt({80,40}));QVERIFY(outlineAt({260,140}));
        document.history()->undo();QTRY_VERIFY(!canvas.previewBusy());QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(outlineAt({80,40}));QVERIFY(!outlineAt({260,140}));
        canvas.setTool(ImageEditorTool::Move);const auto first=canvas.screenPoint({150,80}).toPoint(),last=canvas.screenPoint({155,85}).toPoint();QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,first);QTest::mouseMove(&canvas,last);QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,last);canvas.setTool(ImageEditorTool::Hand);QTest::qWait(20);
        QTRY_VERIFY(!canvas.previewBusy());QVERIFY(!document.history()->canRedo());QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(outlineAt({80,40}));QVERIFY(!outlineAt({260,140}));
        document.history()->undo();QTRY_VERIFY(!canvas.previewBusy());QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(outlineAt({80,40}));document.history()->redo();QTRY_VERIFY(!canvas.previewBusy());QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(outlineAt({80,40}));QVERIFY(!outlineAt({260,140}));
    }
    void selectionOutlineAfterHistoryLimit() {
        ImageDocument document({400,300},ImagePrecision::UInt16);QImage white(400,300,QImage::Format_RGBA64);white.fill(Qt::white);document.addLayer("White",white);document.history()->clear();
        ImageEditorTools tools(&document);ImageEditorCanvas canvas(&tools);canvas.resize(640,480);canvas.show();canvas.fitToWindow();QTest::qWait(20);canvas.setTool(ImageEditorTool::Rectangle);
        const auto first=canvas.screenPoint({40,40}).toPoint(),last=canvas.screenPoint({120,100}).toPoint();QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,first);QTest::mouseMove(&canvas,last);QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,last);canvas.setTool(ImageEditorTool::Hand);
        const auto selectionA=document.selectionRegion({0,0,400,300});const auto selectionId=document.selectionId();QVERIFY(selectionId!=0);
        auto layer=document.layers().last();for(int i=1;i<=101;++i){layer.offset={i%2,0};document.updateLayer(layer,"Move layer");}
        QCOMPARE(document.history()->count(),100);QCOMPARE(document.selectionId(),selectionId);QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QTest::qWait(20);
        const auto hasOutline=[&]{const auto screenshot=canvas.grab().toImage();const auto at=(canvas.screenPoint({80,40})*screenshot.devicePixelRatio()).toPoint();for(int y=at.y()-2;y<=at.y()+2;++y)for(int x=at.x()-2;x<=at.x()+2;++x)if(screenshot.pixelColor(x,y).red()<128)return true;return false;};
        QVERIFY(hasOutline());document.history()->undo();QTest::qWait(20);QCOMPARE(document.selectionId(),selectionId);QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(hasOutline());
        document.history()->redo();QTest::qWait(20);QCOMPARE(document.selectionId(),selectionId);QCOMPARE(document.selectionRegion({0,0,400,300}),selectionA);QVERIFY(hasOutline());
    }
    void playerDetailedEditorEntry() {
        PreviewPane pane("Image","IMG");QImage image(30,20,QImage::Format_RGBA8888);image.fill(Qt::blue);pane.setImage(image);
        PlayerImageTools toolbar(&pane);toolbar.setSource("example.png");toolbar.setReady(true);toolbar.show();auto *button=toolbar.findChild<QToolButton *>("imageTool_edit");QVERIFY(button);QVERIFY(button->isEnabled());QTest::mouseClick(button,Qt::LeftButton);
        auto *editor=toolbar.findChild<ImageEditorWindow *>("imageEditorWindow");QVERIFY(editor);QCOMPARE(editor->document()->size(),QSize(30,20));QCOMPARE(editor->document()->composite({0,0,1,1}).pixelColor(0,0),QColor(Qt::blue));editor->close();
    }
};
QTEST_MAIN(TestImageEditor)
#include "TestImageEditor.moc"
