#include "image/ImageEditorWindow.h"
#include "image/ImageEditorCanvas.h"
#include "image/ImageAdjustments.h"
#include "image/ImagePsd.h"
#include "player/PlayerImage.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

using namespace vsr;
class TestImageEditorLayout final : public QObject {
    Q_OBJECT
private:
    static QAction *action(ImageEditorWindow &editor,const QString &name){return editor.findChild<QAction *>(name);}
    static QImage pixels(){QImage image(400,300,QImage::Format_RGBA64);image.fill(QColor::fromRgba64(1001,2003,3005,65535));image.setColorSpace(QColorSpace::SRgb);return image;}
private slots:
    void photoshopLayoutAndGroups(){
        ImageEditorWindow editor(pixels(),"layout.png");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.resize(1366,900);editor.show();QTest::qWait(50);
        auto *toolbar=editor.findChild<QToolBar *>("imageEditorToolBar");QVERIFY(toolbar);QVERIFY(toolbar->width()<=48);QVERIFY(editor.canvas()->width()>editor.width()*.6);QCOMPARE(editor.menuBar()->actions().size(),8);
        for(auto *button:toolbar->findChildren<QToolButton *>())if(button->objectName().startsWith("imageEditorToolGroup_")){QCOMPARE(button->toolButtonStyle(),Qt::ToolButtonIconOnly);QVERIFY(!button->icon().isNull());}
        auto *ellipse=action(editor,"imageEditorTool_"+QString::number(int(ImageEditorTool::Ellipse)));QVERIFY(ellipse);ellipse->trigger();QCOMPARE(editor.canvas()->tool(),ImageEditorTool::Ellipse);QCOMPARE(editor.findChild<QToolButton *>("imageEditorToolGroup_select")->defaultAction(),ellipse);
        auto *rulers=action(editor,"imageEditorRulers");QVERIFY(rulers);rulers->trigger();QVERIFY(!editor.findChild<QWidget *>("imageEditorHorizontalRuler")->isVisible());rulers->trigger();QVERIFY(editor.findChild<QWidget *>("imageEditorHorizontalRuler")->isVisible());
        QVERIFY(editor.findChild<QTabBar *>("imageEditorDocumentTabs"));QVERIFY(editor.grab().save("build/image-editor-photoshop-layout.png"));editor.document()->history()->setClean();
    }
    void groupChildrenEditableAndUndo(){
        auto *doc=new ImageDocument({400,300},ImagePrecision::UInt16);const auto group=doc->addLayer("Group");auto groupInfo=doc->layers().last();groupInfo.group=true;doc->updateLayer(groupInfo);const auto child=doc->addLayer("Child",pixels());auto childInfo=doc->layers().last();childInfo.parentId=group;doc->updateLayer(childInfo);doc->history()->clear();doc->history()->setClean();ImageEditorWindow editor(doc,"groups.psd");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTest::qWait(20);
        auto *tree=editor.findChild<QTreeWidget *>("imageEditorLayers");QVERIFY(tree);QCOMPARE(tree->topLevelItemCount(),1);auto *groupItem=tree->topLevelItem(0);QCOMPARE(groupItem->childCount(),1);auto *childItem=groupItem->child(0);tree->setCurrentItem(childItem);QCOMPARE(editor.tools()->layer(),child);
        action(editor,"imageEditorAdjustment_"+QString::number(int(ImageAdjustmentKind::Invert)))->trigger();QCOMPARE(doc->readRegion(child,{0,0,1,1}).pixelColor(0,0).rgba64().red(),quint16(65535-1001));action(editor,"imageEditorUndo")->trigger();QCOMPARE(doc->readRegion(child,{0,0,1,1}).pixelColor(0,0).rgba64().red(),quint16(1001));
        action(editor,"imageEditorDuplicateLayer")->trigger();QCOMPARE(doc->layers().size(),3);QCOMPARE(doc->layers().last().parentId,group);action(editor,"imageEditorUndo")->trigger();QCOMPARE(doc->layers().size(),2);
        tree->topLevelItem(0)->setExpanded(false);auto info=doc->layers().last();info.name="Renamed";doc->updateLayer(info);QVERIFY(!tree->topLevelItem(0)->isExpanded());doc->history()->setClean();
    }
    void adjustmentDialogAndMask(){
        ImageEditorWindow editor(pixels(),"adjust.png");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTest::qWait(20);action(editor,"imageEditorLockLayer")->trigger();editor.document()->history()->clear();editor.document()->history()->setClean();const auto id=editor.tools()->layer();
        QTimer::singleShot(0,&editor,[&editor]{auto *dialog=editor.findChild<QDialog *>("imageEditorAdjustmentDialog");QVERIFY(dialog);auto *exposure=dialog->findChild<QDoubleSpinBox *>("exposure");QVERIFY(exposure);exposure->setValue(1);dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();});action(editor,"imageEditorAdjustment_0")->trigger();
        // This sample stays in sRGB's linear toe after one stop. Allow the Qt colour LUT's
        // low-value approximation, and verify dialog commit/undo independently of D's tests.
        const auto red=editor.document()->readRegion(id,{0,0,1,1}).pixelColor(0,0).rgba64().red();QVERIFY(red>=1900&&red<=2200);QCOMPARE(editor.document()->history()->count(),1);
        action(editor,"imageEditorUndo")->trigger();QCOMPARE(editor.document()->readRegion(id,{0,0,1,1}).pixelColor(0,0).rgba64().red(),quint16(1001));auto *mask=editor.findChild<QCheckBox *>("imageEditorEditMask");QVERIFY(mask);mask->setChecked(true);QVERIFY(editor.tools()->editingMask());mask->setChecked(false);QVERIFY(!editor.tools()->editingMask());editor.document()->history()->setClean();
    }
    void openNativeHdrAvif(){
        const auto path=QFileInfo(".deps/image/libavif/libavif-1.4.2/tests/data/colors_hdr_p3.avif").absoluteFilePath();QVERIFY(QFile::exists(path));
        const bool previous=QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);const auto restore=qScopeGuard([previous]{QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,previous);});
        ImageEditorWindow editor(pixels(),"source.png");editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();
        QTimer::singleShot(0,&editor,[&]{auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());QVERIFY(dialog);dialog->selectFile(path);QVERIFY(QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection));});action(editor,"imageEditorOpen")->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(editor.documentCount(),2,10000);QCOMPARE(editor.findChild<QTabBar *>("imageEditorDocumentTabs")->currentIndex(),1);
        QCOMPARE(editor.document()->size(),QSize(200,200));QCOMPARE(editor.document()->precision(),ImagePrecision::UInt16);QCOMPARE(editor.document()->colorSpace().primaries(),QColorSpace::Primaries::DciP3D65);QCOMPARE(editor.document()->colorSpace().transferFunction(),QColorSpace::TransferFunction::St2084);QVERIFY(action(editor,"imageEditorHdrToneMap")->isChecked());QCOMPARE(editor.canvas()->hdrPreviewSettings().whitePoint,64.);QTRY_VERIFY(!editor.canvas()->previewBusy());editor.document()->history()->setClean();editor.activateDocument(0);QCOMPARE(editor.document()->size(),pixels().size());editor.document()->history()->setClean();
    }
    void backgroundPsdExportPreservesNewEdits(){
        const bool nativeDialogsDisabled=QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);const auto restoreDialogs=qScopeGuard([nativeDialogsDisabled]{QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,nativeDialogsDisabled);});
        QTemporaryDir directory;QVERIFY(directory.isValid());const auto sourcePath=directory.filePath("source.psd"),outputPath=directory.filePath("snapshot.psd");ImageDocument source({400,300},ImagePrecision::UInt16);source.setColorSpace(QColorSpace::SRgb);source.addLayer("Source",pixels());QVERIFY(ImagePsd::save(&source,sourcePath).isEmpty());QFile original(sourcePath);QVERIFY(original.open(QIODevice::ReadOnly));const auto originalBytes=original.readAll();original.close();
        QString error;auto loaded=ImagePsd::load(sourcePath,&error);QVERIFY2(loaded,error.toUtf8());ImageEditorWindow editor(loaded.release(),sourcePath);editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.show();QTest::qWait(20);auto *doc=editor.document();const auto id=editor.tools()->layer();QImage oldPixel(1,1,doc->pixelFormat());oldPixel.fill(Qt::green);oldPixel.setColorSpace(doc->colorSpace());doc->writeRegion(id,{0,0},oldPixel);QVERIFY(!doc->history()->isClean());auto *save=action(editor,"imageEditorSavePsd");QVERIFY(save);bool accepted=false;
        QTimer::singleShot(0,&editor,[&]{auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if(!dialog)dialog=editor.findChild<QFileDialog *>();QVERIFY(dialog);dialog->selectFile(outputPath);accepted=QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);});save->trigger();QVERIFY(accepted);QVERIFY(!save->isEnabled());
        editor.addImage(pixels(),"another-image.png");QCOMPARE(editor.documentCount(),2);auto *other=editor.document();QVERIFY(other!=doc);action(editor,"imageEditorLockLayer")->trigger();other->history()->clear();other->history()->setClean();QImage otherPixel(1,1,other->pixelFormat());otherPixel.fill(Qt::yellow);otherPixel.setColorSpace(other->colorSpace());other->writeRegion(editor.tools()->layer(),{0,0},otherPixel);QVERIFY(!other->history()->isClean());
        QImage newPixel(1,1,doc->pixelFormat());newPixel.fill(Qt::blue);newPixel.setColorSpace(doc->colorSpace());doc->writeRegion(id,{0,0},newPixel);QVERIFY(!doc->history()->isClean());QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(),30000);QVERIFY2(QFile::exists(outputPath),"Background PSD output is missing");auto saved=ImagePsd::load(outputPath,&error);QVERIFY2(saved,error.toUtf8());QCOMPARE(saved->readRegion(saved->layers().first().id,{0,0,1,1}),oldPixel);QCOMPARE(doc->readRegion(id,{0,0,1,1}),newPixel);QVERIFY(!doc->history()->isClean());QCOMPARE(editor.document(),other);QVERIFY(!other->history()->isClean());QVERIFY(original.open(QIODevice::ReadOnly));QCOMPARE(original.readAll(),originalBytes);
        PlayerImage loader;QSignalSpy viewed(&loader,&PlayerImage::loaded),failed(&loader,&PlayerImage::failed);loader.open(outputPath);QTRY_VERIFY(!viewed.isEmpty()||!failed.isEmpty());QVERIFY(failed.isEmpty());const auto image=qvariant_cast<QImage>(viewed.first().first());QCOMPARE(image.format(),QImage::Format_RGBA64);QCOMPARE(image.pixelColor(0,0).rgba64(),oldPixel.pixelColor(0,0).rgba64());other->history()->setClean();doc->history()->setClean();
    }
    void realLargePsdWindow(){
        const auto path=qEnvironmentVariable("VSR_PSD_SAMPLE");if(path.isEmpty())QSKIP("Set VSR_PSD_SAMPLE for the user's large PSD UI check");
        QString error;QStringList warnings;auto loaded=ImagePsd::load(path,&error,&warnings);QVERIFY2(loaded,error.toUtf8());QCOMPARE(loaded->size(),QSize(5950,8420));QCOMPARE(loaded->layers().size(),9);
        ImageEditorWindow editor(loaded.release(),path);editor.setAttribute(Qt::WA_DeleteOnClose,false);editor.resize(1440,960);auto *canvas=editor.canvas();QSignalSpy ready(canvas,&ImageEditorCanvas::previewReady),errors(canvas,&ImageEditorCanvas::previewError);editor.show();canvas->fitToWindow();QTRY_VERIFY_WITH_TIMEOUT(ready.count()>0&&!canvas->previewBusy(),180000);QCOMPARE(errors.count(),0);
        auto *doc=editor.document();auto *tree=editor.findChild<QTreeWidget *>("imageEditorLayers");QVERIFY(tree);int count=0;QTreeWidgetItem *editable=nullptr;QRect documentBounds;
        for(QTreeWidgetItemIterator i(tree);*i;++i){++count;auto *item=*i;const auto id=item->data(0,Qt::UserRole).toUuid();tree->setCurrentItem(item);QCOMPARE(editor.tools()->layer(),id);for(const auto &layer:doc->layers())if(layer.id==id&&!layer.group&&!layer.locked&&layer.visible){const auto bounds=doc->layerBounds(id).translated(layer.offset).intersected(QRect(QPoint(),doc->size()));if(!editable&&bounds.width()>32&&bounds.height()>32){editable=item;documentBounds=bounds;}}}
        QCOMPARE(count,9);QVERIFY(editable);tree->setCurrentItem(editable);const auto id=editor.tools()->layer();ImageLayerInfo selected;for(const auto &layer:doc->layers())if(layer.id==id)selected=layer;
        const auto at=canvas->screenPoint(documentBounds.center()).toPoint();const auto point=canvas->documentPoint(at).toPoint()-selected.offset;const QRect region(point-QPoint(12,12),QSize(25,25));const auto before=doc->readRegion(id,region);QVERIFY(!before.isNull());const int history=doc->history()->count();editor.tools()->setBrushColor(Qt::magenta);editor.tools()->setBrushRadius(6);action(editor,"imageEditorTool_"+QString::number(int(ImageEditorTool::Brush)))->trigger();QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,at);QCOMPARE(doc->history()->count(),history+1);QVERIFY(doc->readRegion(id,region)!=before);
        action(editor,"imageEditorUndo")->trigger();QCOMPARE(doc->readRegion(id,region),before);QTRY_VERIFY_WITH_TIMEOUT(!canvas->previewBusy(),180000);QCOMPARE(errors.count(),0);QVERIFY(doc->storageError().isEmpty());QVERIFY(editor.grab().save("build/image-editor-large-psd.png"));doc->history()->setClean();qInfo()<<"PSD UI rasterization notices:"<<warnings;
        PlayerImage viewer;QSignalSpy viewed(&viewer,&PlayerImage::loaded),failed(&viewer,&PlayerImage::failed);viewer.open(path);QTRY_VERIFY_WITH_TIMEOUT(!viewed.isEmpty()||!failed.isEmpty(),180000);QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));const auto viewedImage=qvariant_cast<QImage>(viewed.first().first());QCOMPARE(viewedImage.size(),doc->size());QCOMPARE(viewedImage.format(),QImage::Format_RGBA64);QCOMPARE(viewedImage.text("decoder"),QString("Native layered PSD/PSB"));QCOMPARE(viewedImage.pixelColor(5000,8000).rgba64(),doc->composite({5000,8000,1,1}).pixelColor(0,0).rgba64());
    }
};
QTEST_MAIN(TestImageEditorLayout)
#include "TestImageEditorLayout.moc"
