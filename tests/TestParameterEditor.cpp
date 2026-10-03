#include "ui/ParameterEditor.h"
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
    void initTestCase(){QFont font("Segoe UI Variable");font.setPixelSize(13);qApp->setFont(font);qApp->setStyleSheet(applicationStyleSheet());}
    void shaderCategoryPreservesPathsAndWidth() {
        auto definition=*FilterCatalog::find("anime4k");
        const QString longPath="mpv-shaders/example--repository/a/very/long/folder/NVScaler_a_very_long_shader_filename_which_must_not_expand_the_control.glsl";
        for(auto &p:definition.parameters)if(p.id=="mode")p.choices.append(longPath);
        FilterGraph graph;const int index=graph.add("anime4k");graph.setParameter(index,"mode",longPath);
        ParameterEditor editor;editor.resize(243,800);editor.setNode(&definition,graph.at(index));editor.show();QTest::qWait(20);
        auto *mode=editor.findChild<QComboBox *>("parameter_mode");auto *category=editor.findChild<QComboBox *>("parameter_category");
        QCOMPARE(mode->currentData().toString(),longPath);QCOMPARE(mode->width(),140);QVERIFY(mode->toolTip().contains(longPath));QVERIFY(mode->toolTip().contains("非 N 卡独占"));QCOMPARE(mode->view()->maximumWidth(),520);
        const auto *label=editor.findChild<QLabel *>("parameterLabel_mode");QVERIFY(label->geometry().right()<mode->geometry().left());QVERIFY(label->width()>=label->fontMetrics().horizontalAdvance(label->text()));QVERIFY(label->height()>=label->fontMetrics().height());
        for(const auto *text:editor.findChildren<QLabel *>())if(text->objectName().startsWith("parameterLabel_"))QVERIFY2(text->width()>=text->fontMetrics().horizontalAdvance(text->text()),qPrintable(text->objectName()));
        editor.grab().save(QCoreApplication::applicationDirPath()+"/parameter-shaders-long-path-1.0.4.png");
        QSignalSpy changed(&editor,&ParameterEditor::parameterChanged);
        category->setCurrentText("基础锐化");QVERIFY(mode->count()>0);for(int i=0;i<mode->count();++i)QCOMPARE(FilterCatalog::shaderCategory(mode->itemData(i).toString()),QString("基础锐化"));
        category->setCurrentText("自定义 GLSL");QCOMPARE(mode->currentData().toString(),QString("自定义 GLSL"));QCOMPARE(mode->width(),140);
        QCOMPARE(changed.last().at(0).toString(),QString("mode"));QCOMPARE(changed.last().at(1).toString(),QString("自定义 GLSL"));
        editor.grab().save(QCoreApplication::applicationDirPath()+"/parameter-shaders-1.0.4.png");
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
