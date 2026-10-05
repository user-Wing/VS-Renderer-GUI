#include "image/ImageEditorWindow.h"
#include "image/ImageEditorCanvas.h"
#include "player/PlayerImage.h"
#include "player/PlayerPng.h"
#include <QImageReader>
#include <QElapsedTimer>
#include "image/ImagePsd.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QUndoView>
#include <QLabel>
#include <QLineF>
#include <QLineEdit>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>
#include <QTreeWidget>
#include <QToolButton>
#include <QMenu>

using namespace vsr;
namespace {
QImage pixels(QSize size,QColor color){QImage image(size,QImage::Format_RGBA64);image.fill(color);return image;}
void drag(ImageEditorCanvas &canvas,QPointF from,QPointF to,Qt::KeyboardModifiers modifiers=Qt::NoModifier){
    QTest::mousePress(&canvas,Qt::LeftButton,modifiers,canvas.screenPoint(from).toPoint());
    QTest::mouseMove(&canvas,canvas.screenPoint(to).toPoint());
    QTest::mouseRelease(&canvas,Qt::LeftButton,modifiers,canvas.screenPoint(to).toPoint());
}
ImageLayerInfo info(ImageDocument &document,const QUuid &id){for(const auto &l:document.layers())if(l.id==id)return l;return {};}
void hover(ImageEditorCanvas &canvas,QPointF position){
    // Deliver the same hover event deterministically without moving the user's desktop cursor.
    QMouseEvent event(QEvent::MouseMove,position,canvas.mapToGlobal(position.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(&canvas,&event);
}
}
class TestImageEditorInteraction final : public QObject {
    Q_OBJECT
private slots:
    void movePixelsFollowPointerBeforeCommit() {
        ImageDocument doc({300,220},ImagePrecision::UInt16);doc.addLayer("Background",pixels(doc.size(),Qt::white));const auto id=doc.addLayer("Object",pixels({40,20},Qt::red),{30,40});
        ImageEditorTools tools(&doc);tools.setLayer(id);ImageEditorCanvas canvas(&tools);canvas.resize(700,540);canvas.show();canvas.actualSize();canvas.setNativeHdrEnabled(false);canvas.setTool(ImageEditorTool::Move);QTRY_VERIFY(!canvas.previewBusy());
        const auto from=canvas.screenPoint({50,50}),to=canvas.screenPoint({110,50});QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,from.toPoint());
        QElapsedTimer timer;timer.start();hover(canvas,to);const auto rendered=canvas.grab().toImage();QVERIFY(timer.elapsed()<100);QCOMPARE(info(doc,id).offset,QPoint(30,40));
        const auto dpi=rendered.devicePixelRatioF();QCOMPARE(rendered.pixelColor((to*dpi).toPoint()).rgb(),QColor(Qt::red).rgb());QCOMPARE(rendered.pixelColor((from*dpi).toPoint()).rgb(),QColor(Qt::white).rgb());
        QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,to.toPoint());QCOMPARE(info(doc,id).offset,QPoint(90,40));
    }
    void groupedToolsAndDeleteRespectLocks() {
        ImageEditorWindow editor(pixels({100,80},Qt::white),"test.png");editor.show();
        bool grouped=false;for(auto *button:editor.findChildren<QToolButton *>())if(button->objectName().startsWith("imageEditorToolGroup_")&&button->menu()&&button->menu()->actions().size()>1){grouped=true;QCOMPARE(button->popupMode(),QToolButton::MenuButtonPopup);}QVERIFY(grouped);
        auto *add=editor.findChild<QAction *>("imageEditorAddLayer");auto *remove=editor.findChild<QAction *>("imageEditorDeleteLayer");QVERIFY(add&&remove);QCOMPARE(remove->shortcut(),QKeySequence(Qt::Key_Delete));
        auto *doc=editor.document();add->trigger();const auto count=doc->layers().size();remove->trigger();QCOMPARE(doc->layers().size(),count-1);doc->history()->undo();
        auto layer=doc->layers().last();layer.locked=true;doc->updateLayer(layer);editor.tools()->setLayer(layer.id);const auto lockedCount=doc->layers().size();remove->trigger();QCOMPARE(doc->layers().size(),lockedCount);
    }
    void largeLayerLazyPreviewAndEdit(){
        QImage image(48000,384,QImage::Format_ARGB32);image.fill(Qt::red);
        for(int y=0;y<image.height();++y){auto *row=reinterpret_cast<QRgb *>(image.scanLine(y));std::fill(row+24000,row+image.width(),qRgb(0,0,255));}
        ImageDocument document(image.size(),ImagePrecision::UInt16);const auto layer=document.addLayer("Large",image);QCOMPARE(document.residentBytes(),0);
        auto preview=document.compositePreview(QRect(QPoint(),image.size()),{480,16},ImageSamplingQuality::Bilinear);QVERIFY(!preview.isNull());
        QCOMPARE(preview.pixelColor(120,8).rgb(),QColor(Qt::red).rgb());QCOMPARE(preview.pixelColor(360,8).rgb(),QColor(Qt::blue).rgb());
        QVERIFY(preview.pixelColor(120,8).rgba64().alpha()>=65533);
        const QRect area(40000,100,32,32);QCOMPARE(document.readRegion(layer,area).pixelColor(16,16).rgba64(),QColor(Qt::blue).rgba64());
        document.writeRegion(layer,area.topLeft(),pixels(area.size(),Qt::green));
        QCOMPARE(document.compositePreview(area,area.size(),ImageSamplingQuality::Bilinear).pixelColor(16,16).rgba64(),QColor(Qt::green).rgba64());
        document.history()->undo();QCOMPARE(document.readRegion(layer,area).pixelColor(16,16).rgba64(),QColor(Qt::blue).rgba64());
    }
    void rgbaPngMatchesQt(){
        QTemporaryDir directory;std::atomic<quint64> generation{1};
        for(int pattern=0;pattern<5;++pattern){
            QImage source(257,129,QImage::Format_RGBA8888);
            for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x){auto *p=source.scanLine(y)+x*4;for(int c=0;c<4;++c)p[c]=uchar(pattern==0?0:pattern==1?255:pattern==2?x+c:pattern==3?y+c:(x*37+y*71+c*13)&255);}
            const auto path=directory.filePath(QString::number(pattern)+".png");QVERIFY(source.save(path));
            const auto decoded=decodeRgbaPng(path,generation,1);QVERIFY(!decoded.isNull());
            QCOMPARE(decoded,QImageReader(path).read().convertToFormat(QImage::Format_RGBA8888));
        }
        generation=2;QVERIFY(decodeRgbaPng(directory.filePath("0.png"),generation,1).isNull());
    }
    void parallelRgbaPngMatchesQt(){
        QTemporaryDir directory;std::atomic<quint64> generation{1};
        QImage source(20000,1800,QImage::Format_RGBA8888);
        for(int y=0;y<source.height();++y){auto *row=source.scanLine(y);for(int x=0;x<source.width();++x)for(int c=0;c<4;++c)row[x*4+c]=uchar((x*37+y*71+c*13+(y%17==0?x*y:0))&255);}
        const auto path=directory.filePath("parallel.png");QVERIFY(source.save(path));
        const auto decoded=decodeRgbaPng(path,generation,1);QVERIFY(!decoded.isNull());qInfo()<<decoded.text("pngStages");
        QCOMPARE(decoded,source);QCOMPARE(decoded,QImageReader(path).read().convertToFormat(QImage::Format_RGBA8888));
    }
    void largeImageEditorStartup(){
        const auto path=qEnvironmentVariable("VSR_TEST_IMAGE");if(path.isEmpty())QSKIP("Set VSR_TEST_IMAGE for the real large-image benchmark");
        PlayerImage decoder;QSignalSpy loaded(&decoder,&PlayerImage::loaded),failed(&decoder,&PlayerImage::failed);QElapsedTimer timer;timer.start();decoder.open(path);
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !failed.isEmpty(),180000);QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));
        const auto decodeMs=timer.elapsed();const auto image=qvariant_cast<QImage>(loaded.first().first());QVERIFY(!image.isNull());
        qInfo()<<image.text("pngStages");
        ImageEditorWindow editor(image,path);editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTRY_VERIFY_WITH_TIMEOUT(!editor.canvas()->previewBusy(),30000);
        qInfo("Image %dx%d decode=%lldms editor-ready=%lldms resident-tiles=%lld",image.width(),image.height(),decodeMs,timer.elapsed(),editor.document()->residentBytes());
        const auto limit=qEnvironmentVariableIntValue("VSR_TEST_IMAGE_LIMIT_MS");if(limit>0)QVERIFY2(timer.elapsed()<=limit,"Large image startup exceeded the requested time budget");
    }
    void selectionCopyCutAndIcons(){
        auto *doc=new ImageDocument({80,60},ImagePrecision::UInt16);const auto id=doc->addLayer("Source",pixels({80,60},Qt::red));doc->history()->clear();
        ImageEditorWindow editor(doc,"selection.psd");editor.setAttribute(Qt::WA_DeleteOnClose,false);
        QImage mask({7,5},QImage::Format_Grayscale16);mask.fill(65535);reinterpret_cast<quint16 *>(mask.scanLine(0))[0]=0;doc->writeSelection({20,10},mask);
        QCOMPARE(doc->selectionBounds(),QRect(20,10,7,5));QPoint origin;const auto selected=editor.tools()->selectedPixels(&origin);QCOMPARE(origin,QPoint(20,10));QCOMPARE(selected.size(),QSize(7,5));QCOMPARE(selected.format(),QImage::Format_RGBA64);QCOMPARE(selected.pixelColor(0,0).alpha(),0);
        editor.findChild<QAction *>("imageEditorCopy")->trigger();editor.findChild<QAction *>("imageEditorPaste")->trigger();const auto pasted=editor.tools()->layer();QCOMPARE(info(*doc,pasted).offset,origin);QCOMPARE(doc->readRegion(pasted,{0,0,7,5}),selected);QVERIFY(!doc->hasSelection());
        editor.tools()->setLayer(id);doc->writeSelection({20,10},mask);doc->history()->clear();QVERIFY(editor.tools()->layerFromSelection(true));QCOMPARE(doc->history()->count(),1);QCOMPARE(doc->readRegion(id,{21,11,1,1}).pixelColor(0,0).alpha(),0);doc->history()->undo();QCOMPARE(doc->readRegion(id,{21,11,1,1}).pixelColor(0,0),QColor(Qt::red));QVERIFY(doc->hasSelection());
        auto *rectangle=editor.findChild<QAction *>("imageEditorTool_"+QString::number(int(ImageEditorTool::Rectangle)));auto *ellipse=editor.findChild<QAction *>("imageEditorTool_"+QString::number(int(ImageEditorTool::Ellipse)));QVERIFY(rectangle&&ellipse);QVERIFY(rectangle->icon().pixmap(24,24).toImage()!=ellipse->icon().pixmap(24,24).toImage());
    }
    void boundsMoveScaleAndCancel(){
        ImageDocument doc({300,220},ImagePrecision::UInt16);const auto id=doc.addLayer("Object",pixels({40,20},Qt::red),{30,40});doc.history()->clear();
        ImageEditorTools tools(&doc);ImageEditorCanvas canvas(&tools);canvas.resize(700,540);canvas.show();canvas.actualSize();canvas.setTool(ImageEditorTool::Move);QTest::qWait(20);
        QCOMPARE(canvas.selectedLayerBounds(),QRectF(30,40,40,20));drag(canvas,{50,50},{63,57});QCOMPARE(info(doc,id).offset,QPoint(43,47));QCOMPARE(doc.history()->count(),1);doc.history()->undo();
        drag(canvas,{70,60},{110,76},Qt::ShiftModifier);QCOMPARE(canvas.selectedLayerBounds(),QRectF(30,40,80,40));QCOMPARE(doc.readRegion(id,{40,20,1,1}).pixelColor(0,0),QColor(Qt::red));QCOMPARE(doc.history()->count(),1);doc.history()->undo();
        const auto original=doc.readRegion(id,{0,0,40,20});canvas.startFreeTransform();QVERIFY(canvas.freeTransformActive());drag(canvas,{50,50},{62,57});QCOMPARE(info(doc,id).offset,QPoint(42,47));QCOMPARE(doc.history()->index(),0);QTest::keyClick(&canvas,Qt::Key_Escape);QVERIFY(!canvas.freeTransformActive());QCOMPARE(info(doc,id).offset,QPoint(30,40));QCOMPARE(doc.readRegion(id,{0,0,40,20}),original);
        canvas.startFreeTransform();drag(canvas,{70,60},{90,70});QTest::keyClick(&canvas,Qt::Key_Return);QVERIFY(!canvas.freeTransformActive());QCOMPARE(canvas.selectedLayerBounds(),QRectF(30,40,60,30));QCOMPARE(doc.history()->index(),1);doc.history()->undo();QCOMPARE(doc.readRegion(id,{0,0,40,20}),original);
    }
    void textLayerPickedAndMoved(){
        ImageDocument doc({300,200},ImagePrecision::UInt16);const auto base=doc.addLayer("Background",pixels(doc.size(),Qt::white));ImageEditorTools tools(&doc);tools.setText("Text",QFont("Arial",22));tools.setBrushColor(Qt::black);
        ImageEditorCanvas canvas(&tools);canvas.resize(700,500);canvas.show();canvas.actualSize();canvas.setTool(ImageEditorTool::Text);QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.screenPoint({40,80}).toPoint());
        QCOMPARE(doc.layers().size(),2);const auto text=tools.layer();QVERIFY(text!=base);QCOMPARE(canvas.tool(),ImageEditorTool::Move);const auto bounds=canvas.selectedLayerBounds();QVERIFY(!bounds.isEmpty());const auto initialOffset=info(doc,text).offset;const auto before=doc.readRegion(base,{0,0,300,200});drag(canvas,bounds.center(),bounds.center()+QPointF(14,9));QCOMPARE(info(doc,text).offset,initialOffset+QPoint(14,9));QCOMPARE(doc.readRegion(base,{0,0,300,200}),before);
        QSignalSpy position(&canvas,&ImageEditorCanvas::cursorPositionChanged);hover(canvas,canvas.screenPoint({75,99}));QVERIFY(!position.isEmpty());const auto value=position.last().first().toPointF();QVERIFY(QLineF(value,{75,99}).length()<1);
        const auto anchor=canvas.screenPoint({75,99});QTest::mousePress(&canvas,Qt::MiddleButton,Qt::NoModifier,anchor.toPoint());const auto moved=anchor+QPointF(24,18);hover(canvas,moved);QCOMPARE(position.last().first().toPointF(),canvas.documentPoint(moved));QTest::mouseRelease(&canvas,Qt::MiddleButton,Qt::NoModifier,moved.toPoint());
    }
    void tinyBoundsRemainMovable(){
        ImageDocument doc({100,80},ImagePrecision::UInt16);const auto id=doc.addLayer("Tiny",pixels({6,4},Qt::red),{30,20});ImageEditorTools tools(&doc);ImageEditorCanvas canvas(&tools);canvas.resize(500,400);canvas.show();canvas.actualSize();canvas.setTool(ImageEditorTool::Move);drag(canvas,{33,22},{45,29});QCOMPARE(info(doc,id).offset,QPoint(42,27));QCOMPARE(doc.layerBounds(id).size(),QSize(6,4));
    }
    void sharedControlsAndShortcuts(){
        auto *doc=new ImageDocument({400,300},ImagePrecision::UInt16);const auto bottom=doc->addLayer("Bottom",pixels(doc->size(),Qt::white));const auto top=doc->addLayer("Top",pixels({40,20},Qt::red),{40,50});doc->history()->clear();
        ImageEditorWindow editor(doc,"interaction.psd");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.resize(1366,900);editor.show();QTest::qWait(30);auto *tree=editor.findChild<QTreeWidget *>("imageEditorLayers");QVERIFY(tree);QCOMPARE(tree->topLevelItemCount(),2);
        auto *opacity=editor.findChild<QDoubleSpinBox *>("imageEditorLayerOpacity");QVERIFY(opacity);tree->setCurrentItem(tree->topLevelItem(0));QCOMPARE(editor.tools()->layer(),top);auto *selectedRow=tree->currentItem();opacity->setValue(60);QCOMPARE(tree->currentItem(),selectedRow);QCOMPARE(info(*doc,top).opacity,.6f);QCOMPARE(info(*doc,bottom).opacity,1.f);
        auto *canvas=editor.canvas();canvas->setFocus();QTest::keyClick(canvas,Qt::Key_T,Qt::ControlModifier);QVERIFY(canvas->freeTransformActive());QTest::keyClick(canvas,Qt::Key_Escape);QVERIFY(!canvas->freeTransformActive());
        const auto before=doc->composite({40,50,40,20});canvas->setFocus();QTest::keyClick(canvas,Qt::Key_E,Qt::ControlModifier);QCOMPARE(doc->layers().size(),1);QCOMPARE(editor.tools()->layer(),bottom);QCOMPARE(doc->composite({40,50,40,20}),before);doc->history()->undo();QCOMPARE(doc->layers().size(),2);
        QTRY_VERIFY(!canvas->previewBusy());for(auto *label:editor.findChildren<QLabel *>())QVERIFY(!label->isVisible()||!label->text().contains("Shift 添加"));
        const auto point=canvas->screenPoint({123,86});hover(*canvas,point);auto *horizontal=editor.findChild<QWidget *>("imageEditorHorizontalRuler"),*vertical=editor.findChild<QWidget *>("imageEditorVerticalRuler");QVERIFY(horizontal&&vertical);QVERIFY(horizontal->property("cursorVisible").toBool());QVERIFY(vertical->property("cursorVisible").toBool());const auto coordinates=canvas->documentPoint(point);QCOMPARE(horizontal->property("cursorValue").toDouble(),coordinates.x());QCOMPARE(vertical->property("cursorValue").toDouble(),coordinates.y());
        QCOMPARE(tree->viewport()->palette().color(QPalette::Base),editor.palette().color(QPalette::Window));QVERIFY(editor.grab().save("build/image-editor-interaction.png"));doc->history()->setClean();
    }
    void backgroundLocksAndDocumentIsolation(){
        QTemporaryDir temp;QVERIFY(temp.isValid());const auto firstPath=temp.filePath("first.png");QVERIFY(pixels({120,80},Qt::red).save(firstPath));ImageEditorWindow editor(pixels({120,80},Qt::red),firstPath);editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();auto *first=editor.document();auto *firstCanvas=editor.canvas();auto *firstTools=editor.tools();const auto background=firstTools->layer();QVERIFY(info(*first,background).locked);QVERIFY(editor.findChild<QAction *>("imageEditorLockLayer")->isChecked());editor.findChild<QAction *>("imageEditorDuplicateLayer")->trigger();QCOMPARE(first->layers().size(),2);const auto duplicate=firstTools->layer();QVERIFY(!info(*first,duplicate).locked);QVERIFY(info(*first,background).locked);first->history()->clear();first->history()->setClean();
        editor.findChild<QAction *>("imageEditorTool_"+QString::number(int(ImageEditorTool::Brush)))->trigger();firstTools->setBrushRadius(19);firstTools->setEditingMask(true);firstCanvas->actualSize();const auto firstZoom=firstCanvas->zoom();auto *text=editor.findChild<QLineEdit *>("imageEditorTextInput");text->setText("first text");auto *ratio=editor.findChild<QComboBox *>("imageEditorCropRatio");ratio->setCurrentIndex(6);editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioWidth")->setValue(5);editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioHeight")->setValue(4);editor.findChild<QDoubleSpinBox *>("imageEditorLayerOpacity")->setValue(72);QCOMPARE(first->history()->count(),1);
        editor.addImage(pixels({90,60},Qt::blue),"second.png");auto *second=editor.document();QCOMPARE(editor.documentCount(),2);QVERIFY(second!=first);QVERIFY(editor.canvas()!=firstCanvas);QCOMPARE(editor.canvas()->tool(),ImageEditorTool::Hand);QVERIFY(!editor.tools()->editingMask());QCOMPARE(editor.findChild<QUndoView *>("imageEditorHistory")->stack(),second->history());QVERIFY(!editor.findChild<QAction *>("imageEditorUndo")->isEnabled());editor.findChild<QAction *>("imageEditorLockLayer")->trigger();second->history()->clear();second->history()->setClean();editor.findChild<QDoubleSpinBox *>("imageEditorLayerOpacity")->setValue(45);QCOMPARE(second->history()->count(),1);auto *secondCanvas=editor.canvas();secondCanvas->actualSize();secondCanvas->zoomBy(2);const auto secondZoom=secondCanvas->zoom();
        editor.activateDocument(0);QCOMPARE(editor.document(),first);QCOMPARE(editor.tools(),firstTools);QCOMPARE(editor.canvas()->tool(),ImageEditorTool::Brush);QCOMPARE(editor.canvas()->zoom(),firstZoom);QCOMPARE(editor.tools()->brushRadius(),19.);QVERIFY(editor.tools()->editingMask());QVERIFY(editor.findChild<QCheckBox *>("imageEditorEditMask")->isChecked());QCOMPARE(text->text(),QString("first text"));QCOMPARE(ratio->currentIndex(),6);QCOMPARE(editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioWidth")->value(),5.);QCOMPARE(editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioHeight")->value(),4.);editor.findChild<QAction *>("imageEditorUndo")->trigger();QCOMPARE(info(*first,duplicate).opacity,1.f);QCOMPARE(info(*second,second->layers().first().id).opacity,.45f);QCOMPARE(editor.findChild<QUndoView *>("imageEditorHistory")->stack(),first->history());
        editor.addImage(pixels({2,2},Qt::green),firstPath);QCOMPARE(editor.documentCount(),2);QCOMPARE(editor.document(),first);QCOMPARE(first->size(),QSize(120,80));editor.activateDocument(1);QCOMPARE(editor.canvas(),secondCanvas);QCOMPARE(secondCanvas->zoom(),secondZoom);QVERIFY(!editor.tools()->editingMask());first->history()->setClean();text->setText("keep current text");ratio->setCurrentIndex(6);editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioWidth")->setValue(7);editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioHeight")->setValue(3);auto *tabs=editor.findChild<QTabBar *>("imageEditorDocumentTabs");QVERIFY(QMetaObject::invokeMethod(tabs,"tabCloseRequested",Qt::DirectConnection,Q_ARG(int,0)));QCOMPARE(editor.documentCount(),1);QCOMPARE(editor.document(),second);QCOMPARE(text->text(),QString("keep current text"));QCOMPARE(editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioWidth")->value(),7.);QCOMPARE(editor.findChild<QDoubleSpinBox *>("imageEditorCropRatioHeight")->value(),3.);second->history()->setClean();
        auto *psd=new ImageDocument({40,40},ImagePrecision::UInt16);const auto unlocked=psd->addLayer("Unlocked",pixels({20,20},Qt::white));const auto locked=psd->addLayer("Locked",pixels({20,20},Qt::black));auto lockedInfo=info(*psd,locked);lockedInfo.locked=true;psd->updateLayer(lockedInfo);psd->history()->clear();psd->history()->setClean();editor.addDocument(psd,"locks.psd");QVERIFY(info(*psd,locked).locked);QVERIFY(!info(*psd,unlocked).locked);
    }
    void blendingPreviewCancelResetAndUndo(){
        auto *doc=new ImageDocument({160,120},ImagePrecision::UInt16);doc->addLayer("Bottom",pixels(doc->size(),Qt::white));const auto top=doc->addLayer("Top",pixels({80,60},Qt::red),{20,20});doc->history()->clear();doc->history()->setClean();ImageEditorWindow editor(doc,"blend.psd");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTRY_VERIFY(!editor.canvas()->previewBusy());const auto original=info(*doc,top);auto *action=editor.findChild<QAction *>("imageEditorBlendingOptions");QVERIFY(action);bool visited=false,previewApplied=false,previewRestored=false;
        QTimer::singleShot(0,&editor,[&]{auto *dialog=editor.findChild<QDialog *>("imageEditorBlendingDialog");if(!dialog)return;visited=true;dialog->findChild<QDoubleSpinBox *>("imageEditorBlendingOpacity")->setValue(45);dialog->findChild<QDoubleSpinBox *>("imageEditorBlendingFill")->setValue(30);dialog->findChild<QCheckBox *>("imageEditorBlendingChannel_1")->setChecked(false);dialog->findChild<QSpinBox *>("imageEditorBlendSource_1")->setValue(70);dialog->findChild<QSpinBox *>("imageEditorBlendSource_0")->setValue(20);const auto changed=info(*doc,top);previewApplied=changed.opacity==.45f&&changed.fillOpacity==.3f&&changed.channels==5&&changed.blendIfSource[0][0]>0&&doc->history()->count()==0;dialog->findChild<QCheckBox *>("imageEditorBlendingPreview")->setChecked(false);previewRestored=info(*doc,top).opacity==original.opacity&&info(*doc,top).fillOpacity==original.fillOpacity&&info(*doc,top).channels==original.channels;dialog->reject();});action->trigger();QVERIFY(visited);QVERIFY(previewApplied);QVERIFY(previewRestored);QCOMPARE(info(*doc,top).opacity,original.opacity);QCOMPARE(info(*doc,top).fillOpacity,original.fillOpacity);QCOMPARE(info(*doc,top).channels,original.channels);QCOMPARE(info(*doc,top).blendIfSource,original.blendIfSource);QCOMPARE(doc->history()->count(),0);QVERIFY(doc->history()->isClean());
        bool resetCorrect=false;QTimer::singleShot(0,&editor,[&]{auto *dialog=editor.findChild<QDialog *>("imageEditorBlendingDialog");if(!dialog)return;dialog->findChild<QDoubleSpinBox *>("imageEditorBlendingOpacity")->setValue(20);dialog->findChild<QDoubleSpinBox *>("imageEditorBlendingFill")->setValue(15);dialog->findChild<QSpinBox *>("imageEditorBlendSource_1")->setValue(100);auto *buttons=dialog->findChild<QDialogButtonBox *>("imageEditorBlendingButtons");buttons->button(QDialogButtonBox::Reset)->click();const auto reset=info(*doc,top);resetCorrect=reset.opacity==1&&reset.fillOpacity==1&&reset.channels==7&&reset.blendIfSource==original.blendIfSource;dialog->findChild<QDoubleSpinBox *>("imageEditorBlendingOpacity")->setValue(67);dialog->findChild<QComboBox *>("imageEditorBlendingMode")->setCurrentIndex(int(ImageBlendMode::Multiply));dialog->findChild<QComboBox *>("imageEditorBlendIfChannel")->setCurrentIndex(1);dialog->findChild<QSpinBox *>("imageEditorBlendSource_1")->setValue(40);dialog->accept();});action->trigger();QVERIFY(resetCorrect);QCOMPARE(doc->history()->count(),1);QCOMPARE(info(*doc,top).opacity,.67f);QCOMPARE(info(*doc,top).blend,ImageBlendMode::Multiply);QCOMPARE(info(*doc,top).blendIfSource[0],original.blendIfSource[0]);QVERIFY(info(*doc,top).blendIfSource[1][1]>0);doc->history()->undo();QCOMPARE(info(*doc,top).opacity,original.opacity);QCOMPARE(info(*doc,top).blendIfSource,original.blendIfSource);doc->history()->setClean();
    }
    void realLargePsdLayerInteraction(){
        const auto path=qEnvironmentVariable("VSR_PSD_SAMPLE");if(path.isEmpty())QSKIP("Set VSR_PSD_SAMPLE for large PSD transform smoke");
        QString error;auto doc=ImagePsd::load(path,&error);QVERIFY2(doc,error.toUtf8());ImageEditorWindow editor(doc.release(),path);editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.resize(1440,960);editor.show();auto *canvas=editor.canvas();QSignalSpy errors(canvas,&ImageEditorCanvas::previewError);QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);
        auto *document=editor.document();ImageLayerInfo chosen;for(const auto &layer:document->layers())if(layer.visible&&!layer.group&&!layer.locked){chosen=layer;break;}QVERIFY(!chosen.id.isNull());editor.tools()->setLayer(chosen.id);canvas->setTool(ImageEditorTool::Move);const auto bounds=canvas->selectedLayerBounds();QVERIFY(!bounds.isEmpty());const auto local=document->layerBounds(chosen.id).center();const auto before=document->readRegion(chosen.id,QRect(local,QSize(8,8)));
        canvas->startFreeTransform();const auto center=bounds.center();drag(*canvas,center,center+QPointF(20,10));QTest::keyClick(canvas,Qt::Key_Escape);QCOMPARE(info(*document,chosen.id).offset,chosen.offset);QCOMPARE(document->readRegion(chosen.id,QRect(local,QSize(8,8))),before);
        canvas->actualSize();QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);canvas->zoomBy(2);QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);QCOMPARE(canvas->zoom(),2.);const QPointF at(canvas->width()/2.,canvas->height()/2.);const auto origin=canvas->screenPoint({});for(auto modifiers:{Qt::NoModifier,Qt::ShiftModifier}){QWheelEvent wheel(at,canvas->mapToGlobal(at.toPoint()),{},QPoint(0,120),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);QApplication::sendEvent(canvas,&wheel);}QCOMPARE(canvas->screenPoint({}),origin+QPointF(40,40));QVERIFY(!canvas->previewBusy());QCOMPARE(errors.count(),0);QCOMPARE(document->readRegion(chosen.id,QRect(local,QSize(8,8))),before);QVERIFY(editor.grab().save("build/image-editor-large-psd-zoom.png"));canvas->fitToWindow();
        QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);QCOMPARE(errors.count(),0);QVERIFY(editor.grab().save("build/image-editor-large-psd-interaction.png"));document->history()->setClean();editor.addImage(pixels({320,180},Qt::blue),"第二张图片.png");QCOMPARE(editor.documentCount(),2);editor.activateDocument(0);QCOMPARE(editor.document(),document);QCOMPARE(editor.tools()->layer(),chosen.id);QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);QVERIFY(editor.grab().save("build/image-editor-large-psd-tabs.png"));bool captured=false;QTimer::singleShot(0,&editor,[&]{auto *dialog=editor.findChild<QDialog *>("imageEditorBlendingDialog");if(!dialog)return;QTest::qWait(50);auto screenshot=editor.grab();{QPainter painter(&screenshot);painter.drawPixmap(dialog->mapToGlobal(QPoint())-editor.mapToGlobal(QPoint()),dialog->grab());}captured=screenshot.save("build/image-editor-large-psd-blending.png");dialog->reject();});editor.findChild<QAction *>("imageEditorBlendingOptions")->trigger();QVERIFY(captured);document->history()->setClean();
    }
};
QTEST_MAIN(TestImageEditorInteraction)
#include "TestImageEditorInteraction.moc"
