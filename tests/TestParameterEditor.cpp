#include "ui/ParameterEditor.h"
#include "ui/ThemePackage.h"
#include "ui/PageTheme.h"
#include "ui/WrappedFileCombo.h"
#include <QScrollBar>
#include <QTemporaryDir>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QImage>
#include "graph/FilterCatalog.h"
#include "graph/FilterGraph.h"
#include "app/Style.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTest>
#include <QAbstractItemView>
using namespace vsr;
class TestParameterEditor final : public QObject {
    Q_OBJECT
private slots:
    void folderFileNamesWrapWithoutElision(){
        QWidget host;WrappedFileCombo combo(&host);combo.resize(180,30);const auto name=QString("Long.Animation.Movie.")+QString(250,'X')+".2160p48.AV1.SC_TC_JP_EN.mkv";combo.addItem(name);host.show();combo.showPopup();QTest::qWait(80);auto *view=qobject_cast<QListView*>(combo.view());QVERIFY(view);QVERIFY(view->sizeHintForRow(0)>combo.fontMetrics().height()*2);QCOMPARE(view->horizontalScrollBar()->maximum(),0);QCOMPARE(combo.currentText(),name);view->window()->grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("wrapped-folder-filename.png"));combo.hidePopup();
    }
    void pageThemePaintsAndReloadsImmediately(){
        QTemporaryDir dir;QImage red(100,100,QImage::Format_RGB32);red.fill(Qt::red);QVERIFY(red.save(dir.filePath("red.png")));QImage blue=red;blue.fill(Qt::blue);QVERIFY(blue.save(dir.filePath("blue.png")));
        QSettings settings(dir.filePath("themes.ini"),QSettings::IniFormat);settings.setValue("VSR-Main/image","red.png");settings.setValue("VSR-Main/transparency",0);settings.setValue("VSR-Main/brightness",100);settings.sync();
        QWidget page;page.resize(400,300);auto *layout=new QVBoxLayout(&page);auto *container=new QWidget(&page);layout->addWidget(container);applyPageTheme(&page,settings.fileName(),"VSR-Main");page.show();QTest::qWait(50);
        const auto a=page.grab().toImage().pixelColor(200,150);QVERIFY2(a.red()>a.blue()+50,"Theme is hidden behind styled page containers");
        settings.setValue("VSR-Main/image","blue.png");settings.sync();applyPageTheme(&page,settings.fileName(),"VSR-Main");QTest::qWait(30);const auto b=page.grab().toImage().pixelColor(200,150);QVERIFY(b.blue()>b.red()+50);
    }
    void shaderDescriptionTracksScrollViewportWidth(){
        FilterGraph graph;const int index=graph.add("anime4k");QWidget panel;auto *layout=new QVBoxLayout(&panel);layout->setContentsMargins(0,0,0,0);auto *header=new QLabel("高级参数",&panel);header->setFixedHeight(32);layout->addWidget(header);
        auto *scroll=new QScrollArea(&panel);scroll->setWidgetResizable(true);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);auto *editor=new ParameterEditor;QFont font("Comic Sans MS");font.setPixelSize(13);editor->setFont(font);editor->setNode(FilterCatalog::find("anime4k"),graph.at(index));scroll->setWidget(editor);layout->addWidget(scroll);panel.show();
        for(int width:{330,260,290,243}){panel.resize(width,480);QTest::qWait(60);auto *text=editor->findChild<QLabel*>("parameterDescription");QVERIFY(text);QVERIFY(text->sizePolicy().hasHeightForWidth());QVERIFY2(text->height()>=text->heightForWidth(text->width()),"The shader description must keep its full wrapped height inside the scroll area");}
        panel.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath("shader-description-viewport.png"));
    }
    void narrowPanelKeepsParameterText(){
        for(const auto &id:{"ncnn_still","anime4k"}){FilterGraph graph;const int index=graph.add(id);ParameterEditor editor;editor.setNode(FilterCatalog::find(id),graph.at(index));editor.resize(260,1400);editor.show();QTest::qWait(50);
            for(auto *label:editor.findChildren<QLabel*>()){QVERIFY2(label->geometry().right()<editor.width(),qPrintable(label->text()));const int flags=label->wordWrap()?Qt::TextWordWrap:Qt::TextSingleLine;const auto bounds=label->fontMetrics().boundingRect(QRect(0,0,label->width(),10000),flags,label->text());QVERIFY2(label->height()>=bounds.height(),qPrintable(label->text()));}
            editor.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QString("parameters-%1-followup.png").arg(id)));
        }
    }
    void themePackageRoundTrip(){
        QTemporaryDir original,moved;QImage image(32,24,QImage::Format_RGB32);image.fill(Qt::red);QVERIFY(image.save(original.filePath("background.png")));
        QSettings source(original.filePath("themes.ini"),QSettings::IniFormat);source.setValue("skin_name","roundtrip");source.setValue("VSP-Main/image","background.png");source.setValue("VSP-Main/brightness",42);source.setValue("VSP-Settings/color","#123456");source.sync();
        QString error,installed;const auto zip=original.filePath("theme.zip");QVERIFY2(ThemePackage::exportPack(source.fileName(),zip,error),qPrintable(error));
        QVERIFY2(ThemePackage::importPack(zip,moved.filePath("themes.ini"),installed,error),qPrintable(error));QSettings result(installed,QSettings::IniFormat);QCOMPARE(result.value("VSP-Main/brightness").toInt(),42);QCOMPARE(result.value("VSP-Settings/color").toString(),"#123456");
        const auto background=QDir(QFileInfo(installed).absolutePath()).filePath(result.value("VSP-Main/image").toString());QCOMPARE(QImage(background),image);
        source.setValue("VSP-Main/image","../outside.png");source.sync();QVERIFY(QFile::copy(source.fileName(),original.filePath("skin.ini")));QVERIFY(!ThemePackage::importPack(original.path(),moved.filePath("other.ini"),installed,error));
    }
    void initTestCase(){QFont font("Segoe UI Variable");font.setPixelSize(13);qApp->setFont(font);qApp->setStyleSheet(applicationStyleSheet());}
    void shaderCategoryPreservesPathsAndWidth() {
        auto definition=*FilterCatalog::find("anime4k");
        const QString longPath="mpv-shaders/example--repository/a/very/long/folder/NVScaler_a_very_long_shader_filename_which_must_not_expand_the_control.glsl";
        for(auto &p:definition.parameters)if(p.id=="mode")p.choices.append(longPath);
        FilterGraph graph;const int index=graph.add("anime4k");graph.setParameter(index,"mode",longPath);
        ParameterEditor editor;editor.resize(243,800);editor.setNode(&definition,graph.at(index));editor.show();QTest::qWait(20);
        auto *mode=editor.findChild<QComboBox *>("parameter_mode");auto *category=editor.findChild<QComboBox *>("parameter_category");
        QCOMPARE(mode->currentData().toString(),longPath);QCOMPARE(mode->width(),120);QVERIFY(mode->toolTip().contains(longPath));QVERIFY(mode->toolTip().contains("非 N 卡独占"));QCOMPARE(mode->view()->maximumWidth(),520);
        const auto *label=editor.findChild<QLabel *>("parameterLabel_mode");QVERIFY(label->geometry().right()<mode->geometry().left());QVERIFY(label->width()>=label->fontMetrics().horizontalAdvance(label->text()));QVERIFY(label->height()>=label->fontMetrics().height());
        for(const auto *text:editor.findChildren<QLabel *>())if(text->objectName().startsWith("parameterLabel_") && !text->wordWrap())QVERIFY2(text->width()>=text->fontMetrics().horizontalAdvance(text->text()),qPrintable(text->objectName()));
        editor.grab().save(QCoreApplication::applicationDirPath()+"/parameter-shaders-long-path-1.0.4.png");
        QSignalSpy changed(&editor,&ParameterEditor::parameterChanged);
        category->setCurrentText("基础锐化");QVERIFY(mode->count()>0);for(int i=0;i<mode->count();++i)QCOMPARE(FilterCatalog::shaderCategory(mode->itemData(i).toString()),QString("基础锐化"));
        category->setCurrentText("自定义 GLSL/HLSL");QCOMPARE(mode->currentData().toString(),QString("自定义 GLSL"));QCOMPARE(mode->width(),120);
        QCOMPARE(changed.last().at(0).toString(),QString("mode"));QCOMPARE(changed.last().at(1).toString(),QString("自定义 GLSL"));
        editor.grab().save(QCoreApplication::applicationDirPath()+"/parameter-shaders-1.0.4.png");
    }
    void hlslOptionPersistsInGraph(){
        FilterGraph graph;const int index=graph.add("anime4k");ParameterEditor editor;editor.setNode(FilterCatalog::find("anime4k"),graph.at(index));
        auto *hlsl=editor.findChild<QCheckBox *>("parameter_hlsl");QVERIFY(hlsl);QVERIFY(hlsl->isChecked());
        connect(&editor,&ParameterEditor::parameterChanged,&editor,[&](const QString &key,const QVariant &value){graph.setParameter(index,key,value);});
        hlsl->setChecked(false);QVERIFY(!graph.at(index)->parameters.value("hlsl").toBool());editor.setNode(FilterCatalog::find("anime4k"),graph.at(index));QVERIFY(!editor.findChild<QCheckBox *>("parameter_hlsl")->isChecked());
    }
    void linkedResolutionUsesLastEditedDimension() {
        FilterGraph graph;const int index=graph.add("anime4k");ParameterEditor editor;
        editor.setNode(FilterCatalog::find("anime4k"),graph.at(index));editor.setInputSize(QSize(1920,1080));
        auto *output=editor.findChild<QComboBox *>("parameter_output_mode");auto *width=editor.findChild<QSpinBox *>("parameter_width");auto *height=editor.findChild<QSpinBox *>("parameter_height");auto *lock=editor.findChild<QCheckBox *>("parameter_keep_aspect");
        QVERIFY(!width->isEnabled());output->setCurrentText("指定分辨率");QVERIFY(width->isEnabled());
        width->setValue(2560);QCOMPARE(height->value(),1440);height->setValue(720);QCOMPARE(width->value(),1280);
        lock->setChecked(false);width->setValue(1000);QCOMPARE(height->value(),720);
        lock->setChecked(true);QCOMPARE(height->value(),563);
        editor.setInputSize(QSize(1080,1920));QCOMPARE(height->value(),1778);
    }
};
QTEST_MAIN(TestParameterEditor)
#include "TestParameterEditor.moc"
