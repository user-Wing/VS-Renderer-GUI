#include "ui/AgentProviderDialog.h"
#include "ui/AgentTaskDialog.h"
#include "agent/AgentSession.h"
#include "agent/AgentTaskQueue.h"
#include "app/Style.h"
#include "ui/AgentMarkdownView.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QStyleOptionSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QWindow>
#include <QUuid>

using namespace vsr;

class TestAgentDialogUi final:public QObject {
    Q_OBJECT
    static QString artifacts() { const auto path=qEnvironmentVariable("VSR_AGENT_DIALOG_ARTIFACTS",QDir::current().filePath("build/agent-v4-dialogs"));QDir().mkpath(path);return path; }
    static void screenshot(QWidget &widget,const QString &name) { widget.grab().save(QDir(artifacts()).filePath(qEnvironmentVariable("VSR_DIALOG_STAGE","after")+"-"+name+".png")); }
    static AgentProvider provider() {AgentProvider value;value.id="dialog-test";value.name=QStringLiteral("本地测试来源");value.baseUrl="http://127.0.0.1:1/v1";value.model="dialog-model";value.contextLimit=131072;value.maxOutputTokens=8192;return value;}
    static QRect stepRect(QSpinBox *spin,QStyle::SubControl control) {
        QStyleOptionSpinBox option;option.initFrom(spin);option.rect=spin->rect();option.frame=spin->hasFrame();option.buttonSymbols=spin->buttonSymbols();option.stepEnabled=QAbstractSpinBox::StepUpEnabled|QAbstractSpinBox::StepDownEnabled;
        return spin->style()->subControlRect(QStyle::CC_SpinBox,&option,control,spin);
    }
    static void clickAt(QWidget &window,QWidget *widget,const QPoint &position) {
        QTest::mouseClick(window.windowHandle(),Qt::LeftButton,Qt::NoModifier,window.mapFromGlobal(widget->mapToGlobal(position)));
    }
private slots:
    void initTestCase() {
        QFont font(QStringLiteral("Comic Sans MS"));font.setPixelSize(13);QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{"Microsoft YaHei UI"});qApp->setFont(font);qApp->setStyleSheet(applicationStyleSheet(true));
    }
    void providerHeadingUsesInheritedPixelFont() {
        QTemporaryDir directory;AgentProviderDialog dialog({provider()},directory.filePath("providers.json"));dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QLabel *title=nullptr;
        for(auto *label:dialog.findChildren<QLabel*>())if(label->text()==QStringLiteral("来源与模型"))title=label;QVERIFY(title);screenshot(dialog,"provider");
        QVERIFY2(QFontInfo(title->font()).pixelSize()>=QFontInfo(qApp->font()).pixelSize()+2,qPrintable(QString("title=%1px app=%2px titlePoint=%3").arg(QFontInfo(title->font()).pixelSize()).arg(QFontInfo(qApp->font()).pixelSize()).arg(title->font().pointSizeF())));
    }
    void tokenArrowsReceiveActualWindowMouseClicks() {
        QTemporaryDir directory;AgentProviderDialog dialog({provider()},directory.filePath("providers.json"));dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QTest::qWait(30);
        QJsonArray trace;bool allWorked=true;
        for(const auto &name:{QStringLiteral("agentProviderContext"),QStringLiteral("agentProviderMaxOutput")}){
            auto *spin=dialog.findChild<QSpinBox*>(name);QVERIFY(spin);const int original=spin->value();
            for(const auto control:{QStyle::SC_SpinBoxUp,QStyle::SC_SpinBoxDown}){
                const auto rect=stepRect(spin,control);const auto point=rect.center();auto *hit=dialog.childAt(dialog.mapFromGlobal(spin->mapToGlobal(point)));const int before=spin->value();QSignalSpy changes(spin,&QSpinBox::valueChanged);clickAt(dialog,spin,point);QTest::qWait(10);const int expected=before+(control==QStyle::SC_SpinBoxUp?spin->singleStep():-spin->singleStep());allWorked&=spin->value()==expected && hit==spin;
                const auto edit=spin->findChild<QLineEdit*>()->geometry();QJsonArray values;for(const auto &change:changes)values.append(change[0].toInt());
                trace.append(QJsonObject{{"field",name},{"control",control==QStyle::SC_SpinBoxUp?"up":"down"},{"rect",QJsonArray{rect.x(),rect.y(),rect.width(),rect.height()}},{"editorRect",QJsonArray{edit.x(),edit.y(),edit.width(),edit.height()}},{"spinSize",QJsonArray{spin->width(),spin->height()}},{"hitClass",hit?hit->metaObject()->className():"none"},{"hitName",hit?hit->objectName():QString()},{"before",before},{"after",spin->value()},{"expected",expected},{"valueChanges",values}});
            }
            allWorked&=spin->value()==original;
        }
        QFile output(QDir(artifacts()).filePath(qEnvironmentVariable("VSR_DIALOG_STAGE","after")+"-spin-hits.json"));QVERIFY(output.open(QIODevice::WriteOnly));output.write(QJsonDocument(trace).toJson());output.close();screenshot(dialog,"provider-spin");QVERIFY2(allWorked,"Native QWindow mouse clicks did not reach one or more spinbox arrow hit rectangles.");
    }
    void tokenZeroBoundsAndApplySaveClickedValues() {
        QTemporaryDir directory;const auto path=directory.filePath("providers.json");AgentProviderDialog dialog({provider()},path);dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));auto *context=dialog.findChild<QSpinBox*>("agentProviderContext"),*output=dialog.findChild<QSpinBox*>("agentProviderMaxOutput");QVERIFY(context && output);
        output->setValue(0);clickAt(dialog,output,stepRect(output,QStyle::SC_SpinBoxUp).center());QCOMPARE(output->value(),1024);clickAt(dialog,output,stepRect(output,QStyle::SC_SpinBoxDown).center());QCOMPARE(output->value(),0);clickAt(dialog,output,stepRect(output,QStyle::SC_SpinBoxDown).center());QCOMPARE(output->value(),0);
        context->setValue(context->minimum());clickAt(dialog,context,stepRect(context,QStyle::SC_SpinBoxDown).center());QCOMPARE(context->value(),4096);context->setValue(context->maximum());clickAt(dialog,context,stepRect(context,QStyle::SC_SpinBoxUp).center());QCOMPARE(context->value(),2000000);
        context->setValue(65536);output->setValue(2048);clickAt(dialog,context,stepRect(context,QStyle::SC_SpinBoxUp).center());clickAt(dialog,output,stepRect(output,QStyle::SC_SpinBoxUp).center());QCOMPARE(context->value(),69632);QCOMPARE(output->value(),3072);
        auto *buttons=dialog.findChild<QDialogButtonBox*>("agentProviderButtons");QVERIFY(buttons);auto *apply=buttons->button(QDialogButtonBox::Apply);clickAt(dialog,apply,apply->rect().center());QString error;const auto saved=AgentProviderStore::load(path,&error);QVERIFY2(error.isEmpty(),qPrintable(error));QCOMPARE(saved.size(),1);QCOMPARE(saved[0].contextLimit,69632);QCOMPARE(saved[0].maxOutputTokens,3072);screenshot(dialog,"provider-saved");
    }
    void taskColumnsAllowDraggingAndKeepUserWidthsAcrossRefresh() {
        QTemporaryDir directory;AgentSession parent;parent.setProvider(provider());AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QString error;
        const auto task=[](const QString &name){return QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"name",name},{"prompt","inspect workspace"},{"model","deepseek-flash-long-model-name"},{"state","completed"},{"usage",QJsonObject{{"totalTokens",123456}}}};};
        QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{task(QStringLiteral("核对当前工作区并汇报实际结果"))}}},&error));AgentTaskDialog dialog(&queue,&parent);dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QTest::qWait(80);auto *table=dialog.findChild<QTreeWidget*>("agentTaskList");QVERIFY(table);auto *header=table->header();screenshot(dialog,"tasks-before-drag");
        const int before=header->sectionSize(0);const QPoint edge(header->sectionViewportPosition(0)+before-1,header->height()/2);const auto origin=dialog.mapFromGlobal(header->viewport()->mapToGlobal(edge));QTest::mousePress(dialog.windowHandle(),Qt::LeftButton,Qt::NoModifier,origin);QTest::mouseMove(dialog.windowHandle(),origin+QPoint(80,0),10);QTest::mouseRelease(dialog.windowHandle(),Qt::LeftButton,Qt::NoModifier,origin+QPoint(80,0));QTest::qWait(20);
        const int dragged=header->sectionSize(0);QVERIFY2(dragged>=before+50,"The task header did not permit a real mouse drag to resize its first column.");
        QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{task(QStringLiteral("后续刷新加入更长的任务名称，已经拖动过的列宽必须保持用户选择"))}}},&error));QTest::qWait(650);QCOMPARE(header->sectionSize(0),dragged);
        for(int column=0;column<header->count();++column)QCOMPARE(header->sectionResizeMode(column),QHeaderView::Interactive);screenshot(dialog,"tasks-after-drag");
    }
    void emptyTaskQueueShowsOnlyItsOwnHint() {
        QTemporaryDir directory;AgentSession parent;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));AgentTaskDialog dialog(&queue,&parent);dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QTest::qWait(80);screenshot(dialog,"tasks-empty");auto *hint=dialog.findChild<QLabel*>("agentTaskEmpty");QVERIFY(hint);QVERIFY(hint->isVisible());QVERIFY(hint->text().contains(QStringLiteral("暂无子智能体")));auto *view=dialog.findChild<AgentMarkdownView*>("agentTaskResult");QVERIFY(view);QVERIFY(!view->isVisible());dialog.resize(960,680);QTest::qWait(550);QVERIFY(hint->isVisible());QVERIFY(!view->isVisible());
    }
    void firstTaskAfterEmptyQueueStillFitsColumns() {
        QTemporaryDir directory;AgentSession parent;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));AgentTaskDialog dialog(&queue,&parent);dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QTest::qWait(50);auto *table=dialog.findChild<QTreeWidget*>("agentTaskList");QVERIFY(table);const int before=table->columnWidth(2);
        const auto model=QStringLiteral("deepseek-first-arrival-long-model-identifier");const auto name=QStringLiteral("空队列后由AI创建的首个任务");QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"name",name},{"prompt","inspect"},{"model",model},{"state","completed"}}}}}));
        QTRY_VERIFY(table->columnWidth(2)>=table->fontMetrics().horizontalAdvance(model));QVERIFY(table->columnWidth(2)>before);QVERIFY(table->topLevelItem(0)->toolTip(0).contains(name));QVERIFY(table->topLevelItem(0)->toolTip(2).contains(model));QCOMPARE(table->headerItem()->toolTip(2),QStringLiteral("模型"));screenshot(dialog,"tasks-first-arrival");
    }
    void draggingEmptyQueueHeaderPreservesWidthWhenFirstTaskArrives() {
        QTemporaryDir directory;AgentSession parent;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));AgentTaskDialog dialog(&queue,&parent);dialog.show();QVERIFY(QTest::qWaitForWindowExposed(&dialog));QTest::qWait(50);auto *table=dialog.findChild<QTreeWidget*>("agentTaskList");QVERIFY(table);auto *header=table->header();const int before=header->sectionSize(0);const QPoint point=dialog.mapFromGlobal(header->viewport()->mapToGlobal(QPoint(before-1,header->height()/2)));
        QTest::mousePress(dialog.windowHandle(),Qt::LeftButton,Qt::NoModifier,point);QTest::mouseMove(dialog.windowHandle(),point+QPoint(80,0),10);QTest::mouseRelease(dialog.windowHandle(),Qt::LeftButton,Qt::NoModifier,point+QPoint(80,0));const int chosen=header->sectionSize(0);QVERIFY(chosen>=before+50);
        QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"name",QStringLiteral("保持用户手动列宽")},{"prompt","inspect"},{"model","a-long-model-name"},{"state","completed"}}}}}));QTest::qWait(650);QCOMPARE(header->sectionSize(0),chosen);screenshot(dialog,"tasks-manual-before-first");
    }
};
QTEST_MAIN(TestAgentDialogUi)
#include "TestAgentDialogUi.moc"
