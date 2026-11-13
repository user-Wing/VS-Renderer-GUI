#include "ui/AgentPage.h"
#include "ui/AgentComposer.h"
#include "ui/AgentChoicePopup.h"
#include "ui/AgentMarkdownView.h"
#include "ui/PageTheme.h"
#include "agent/AgentSession.h"
#include "app/Style.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
#include <QInputMethodEvent>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTextTable>
#include <QTimer>
#include <QUuid>

using namespace vsr;

class TestAgentUi final : public QObject {
    Q_OBJECT
    static QJsonObject capability() {
        return {{"models",QJsonArray{"model-a","model-b"}},
            {"reasoningLevels",QJsonArray{QJsonObject{{"id","auto"},{"label",QStringLiteral("自动")}},QJsonObject{{"id","off"},{"label",QStringLiteral("关闭")}},QJsonObject{{"id","low"},{"label",QStringLiteral("低")}},QJsonObject{{"id","high"},{"label",QStringLiteral("高")}}}},
            {"optionsByLevel",QJsonObject{{"auto",QJsonObject{}},{"off",QJsonObject{}},{"low",QJsonObject{{"reasoning_effort","low"}}},{"high",QJsonObject{{"reasoning_effort","high"}}}}},{"chatToolsSupported",true},{"status","ready"}};
    }
    static bool prepare(const QString &endpoint="http://127.0.0.1:1/v1") {
        AgentProvider provider;provider.id="test-source";provider.name=QStringLiteral("测试来源");provider.baseUrl=endpoint;provider.model="model-a";
        const auto path=QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json");
        return AgentProviderStore::save(path,{provider});
    }
    static void screenshot(QWidget &widget,const QString &name) {
        const auto root=qEnvironmentVariable("VSR_AGENT_ARTIFACTS");if(root.isEmpty())return;QDir().mkpath(root);widget.grab().save(QDir(root).filePath(name));
    }
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QCoreApplication::setOrganizationName("VSRenderer-Agent-Ui-Tests");
        QCoreApplication::setApplicationName("Ui-"+QUuid::createUuid().toString(QUuid::WithoutBraces));QFont font("Comic Sans MS");font.setPixelSize(13);QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{"Microsoft YaHei UI"});qApp->setFont(font);
    }
    void init() { qApp->setStyleSheet(applicationStyleSheet(true));for(const auto &key:{"agent/lastConversation","agent/accessMode","agent/workingDirectory"})QSettings().remove(key); }

    void markdownLayoutAndSourceIdentity() {
        AgentMarkdownView view;view.resize(980,760);view.show();
        view.showMessages(QJsonArray{
            QJsonObject{{"role","user"},{"content","EXPANDED_ATTACHMENT_SHOULD_NOT_APPEAR"},{"displayContent",QStringLiteral("请检查 @workspace")}},
            QJsonObject{{"role","assistant"},{"content",QStringLiteral("## 处理建议\n\n第一段说明。\n\n第二段说明。\n\n- **检查来源**\n- 准备输出\n\n| 参数 | 值 |\n|---|---|\n| 宽度 | 1920 |\n\n```python\nwidth = 1920\nheight = 1080\n```")}},
            QJsonObject{{"role","user"},{"source",QStringLiteral("子智能体汇报")},{"content",QStringLiteral("只读检查已完成。")}}},{});
        QTest::qWait(50);QVERIFY(!view.toPlainText().contains("EXPANDED_ATTACHMENT"));
        int bubbles=0,tables=0;for(auto *frame:view.document()->rootFrame()->childFrames())if(auto *table=qobject_cast<QTextTable*>(frame)){++tables;if(table->format().alignment()==Qt::AlignRight){++bubbles;QCOMPARE(table->cellAt(0,0).format().background().style(),Qt::NoBrush);}}
        QCOMPARE(bubbles,1);QVERIFY(tables>=2);bool spacious=false;for(auto block=view.document()->begin();block.isValid();block=block.next())spacious|=block.blockFormat().lineHeight()>=150;QVERIFY(spacious);screenshot(view,"ui-markdown.png");
    }

    void composerKeyboardImeAndVisiblePreview() {
        AgentComposer composer;composer.resize(900,300);composer.show();auto *input=composer.editor();input->setFocus();QSignalSpy sends(&composer,&AgentComposer::submitted);
        input->setPlainText("line");input->moveCursor(QTextCursor::End);QTest::keyClick(input,Qt::Key_Return,Qt::ControlModifier);QCOMPARE(input->toPlainText(),QString("line\n"));QCOMPARE(sends.count(),0);
        QTest::keyClick(input,Qt::Key_Return);QCOMPARE(sends.count(),1);
        QInputMethodEvent preedit(QStringLiteral("候选"),{});QApplication::sendEvent(input,&preedit);QTest::keyClick(input,Qt::Key_Return);QCOMPARE(sends.count(),1);
        QInputMethodEvent commit;commit.setCommitString(QStringLiteral("中文"));QApplication::sendEvent(input,&commit);QTest::keyClick(input,Qt::Key_Return);QCOMPARE(sends.count(),1);QTest::qWait(1);QTest::keyClick(input,Qt::Key_Return);QCOMPARE(sends.count(),2);
        input->setPlainText(QStringLiteral("# 预览标题\n\n**粗体** 与普通文字\n\n- 列表项目"));composer.findChild<QPushButton*>("agentPreviewToggle")->click();
        QTest::qWait(50);auto *preview=composer.findChild<QTextBrowser*>("agentInputPreview");QVERIFY(preview->isVisible());QCOMPARE(preview->geometry().top(),input->geometry().top());QVERIFY(!preview->toPlainText().contains("**"));QVERIFY(preview->document()->find(QStringLiteral("粗体")).charFormat().fontWeight()>=QFont::Bold);screenshot(composer,"ui-markdown-preview.png");
    }

    void completionSelectionAndRouting() {
        QVERIFY(prepare());AgentSession session;AgentPage page(&session);page.resize(1200,850);page.show();QTest::qWait(40);auto *input=page.findChild<QPlainTextEdit*>("agentInput");input->setFocus();
        page.setCompletionItems(QJsonArray{QJsonObject{{"kind","command"},{"label","/help"},{"insertText","/help"}},QJsonObject{{"kind","mention"},{"label","@workspace"},{"insertText","@workspace"}},QJsonObject{{"kind","mention"},{"label","@file"},{"insertText",""},{"action","file"}}});
        QSignalSpy commands(&page,&AgentPage::commandSubmitted),messages(&page,&AgentPage::messageSubmitted),files(&page,&AgentPage::mentionActionRequested);
        QTest::keyClicks(input,"/");QTRY_VERIFY(page.findChild<QListWidget*>("agentCompletionPopup")->isVisible());QTest::keyClick(input,Qt::Key_Return);QCOMPARE(input->toPlainText(),QString("/help "));QCOMPARE(commands.count(),0);
        QTest::keyClick(input,Qt::Key_Return);QCOMPARE(commands.count(),1);QVERIFY(input->toPlainText().isEmpty());
        QTest::keyClicks(input,"@w");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(input->toPlainText(),QString("@workspace "));QTest::keyClick(input,Qt::Key_Return);QCOMPARE(messages.count(),1);
        QTest::keyClicks(input,"@f");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(files.count(),1);QCOMPARE(files.first().first().toString(),QString("file"));
        connect(&page,&AgentPage::commandSubmitted,&page,[&](const QString &){page.showNotice(QStringLiteral("无效命令"),true);});input->setPlainText("/invalid");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(input->toPlainText(),QString("/invalid"));
    }

    void dynamicModelReasoningAndToolbar() {
        QVERIFY(prepare());AgentSession session;AgentPage page(&session);page.resize(1120,800);page.show();page.setModelCapabilities(capability());page.selectReasoning("low");QCOMPARE(session.provider().reasoningEffort,QString("low"));
        QSignalSpy changed(&page,&AgentPage::providerModelChanged);page.selectModel("model-b");QCOMPARE(changed.count(),1);QVERIFY(session.provider().modelCapability.isEmpty());
        page.setModelCapabilities(capability());page.selectReasoning("off");page.setModelCapabilities(capability());QCOMPARE(session.provider().reasoningEffort,QString("off"));
        page.setModelCapabilities(QJsonObject{{"reasoningLevels",QJsonArray{QJsonObject{{"id","auto"},{"label",QStringLiteral("自动")}}}}});QCOMPARE(session.provider().reasoningEffort,QString("auto"));
        auto restricted=capability();restricted.insert("chatToolsRequiresOff",true);page.setModelCapabilities(restricted);QCOMPARE(session.provider().modelCapability.value("reasoningLevels").toArray().size(),2);
        auto *model=page.findChild<QPushButton*>("agentModelButton"),*reasoning=page.findChild<QPushButton*>("agentReasoningButton"),*send=page.findChild<QPushButton*>("agentSend");
        QVERIFY(model && reasoning && send);QVERIFY(model->mapTo(&page,QPoint()).x()<reasoning->mapTo(&page,QPoint()).x());QVERIFY(reasoning->mapTo(&page,QPoint()).x()<send->mapTo(&page,QPoint()).x());QVERIFY(page.width()<=1120);QVERIFY(!page.findChild<QPushButton*>("agentStop")->isVisible());page.resize(1800,900);QTest::qWait(40);const auto composerWidth=page.findChild<AgentComposer*>()->width(),sidebarWidth=page.findChild<QWidget*>("agentSidebar")->width();QVERIFY(composerWidth>=page.width()-sidebarWidth-100);if(page.width()>1500)QVERIFY(composerWidth>1200);screenshot(page,"ui-wide-1800.png");
    }

    void modernChoicePopupsAndSourceSwitch() {
        AgentProvider first;first.id="first";first.name=QStringLiteral("这是一个很长的来源名称，用来验证省略显示以及完整提示");first.baseUrl="http://127.0.0.1:1/v1";first.model="model-a";auto second=first;second.id="second";second.name=QStringLiteral("第二来源");second.model="model-b";
        const auto path=QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json");QVERIFY(AgentProviderStore::save(path,{first,second}));AgentSession session;AgentPage page(&session);page.resize(1120,800);page.show();QTest::qWait(40);
        auto *source=page.findChild<QPushButton*>("agentProvider"),*model=page.findChild<QPushButton*>("agentModelButton");QVERIFY(source && model);QVERIFY(source->width()>=200);QVERIFY(source->toolTip().contains(first.name));QVERIFY(source->mapTo(&page,QPoint()).x()+source->width()<=model->mapTo(&page,QPoint()).x());
        QSignalSpy changed(&page,&AgentPage::providerModelChanged);source->click();auto *popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);QCOMPARE(popup->findChild<QLabel*>("agentChoiceTitle")->text(),QStringLiteral("选择来源"));auto *list=popup->findChild<QListWidget*>("agentChoiceList");QCOMPARE(list->count(),2);QVERIFY(list->item(0)->data(Qt::UserRole+1).toBool());screenshot(*popup,"ui-source-popup.png");QTest::keyClick(list,Qt::Key_Down);QTest::keyClick(list,Qt::Key_Return);QCOMPARE(changed.count(),1);QCOMPARE(session.provider().id,QString("second"));QTest::qWait(20);
        page.setModelCapabilities(capability());model->click();popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);list=popup->findChild<QListWidget*>("agentChoiceList");QVERIFY(list->count()>=2);screenshot(*popup,"ui-model-popup.png");const auto item=list->item(1);QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,list->visualItemRect(item).center());QCOMPARE(changed.count(),2);QCOMPARE(session.provider().model,QString("model-a"));QTest::qWait(20);
        page.setModelCapabilities(capability());page.findChild<QPushButton*>("agentReasoningButton")->click();popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);list=popup->findChild<QListWidget*>("agentChoiceList");QCOMPARE(list->count(),4);screenshot(*popup,"ui-reasoning-popup.png");QTest::keyClick(list,Qt::Key_Escape);QTest::qWait(10);QVERIFY(!QApplication::activePopupWidget());qApp->setStyleSheet(applicationStyleSheet(false));source->click();popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);screenshot(*popup,"ui-source-popup-light.png");QTest::keyClick(popup->findChild<QListWidget*>("agentChoiceList"),Qt::Key_Escape);QTest::qWait(10);QVERIFY(page.width()<=1120);
    }

    void choicePopupScreenBoundsAndSingleActivation() {
        QPushButton anchor;anchor.resize(230,35);const auto screen=QGuiApplication::primaryScreen()->availableGeometry();anchor.move(screen.right()-240,screen.bottom()-45);anchor.show();QTest::qWait(20);
        auto *popup=new AgentChoicePopup(QStringLiteral("选择思考强度"),QString(300,'x'),{{"auto",QStringLiteral("自动"),QStringLiteral("跟随模型默认")},{"high",QStringLiteral("高"),{}}},"auto");QSignalSpy choices(popup,&AgentChoicePopup::chosen);popup->openFor(&anchor);QVERIFY(screen.contains(popup->frameGeometry()));auto *list=popup->findChild<QListWidget*>("agentChoiceList");auto *first=list->item(0);QMetaObject::invokeMethod(list,"itemClicked",Qt::DirectConnection,Q_ARG(QListWidgetItem*,first));QMetaObject::invokeMethod(list,"itemActivated",Qt::DirectConnection,Q_ARG(QListWidgetItem*,first));QCOMPARE(choices.count(),1);
    }

    void busyInputPendingCancelAndEdit() {
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));connect(&server,&QTcpServer::newConnection,&server,[&]{while(server.hasPendingConnections())server.nextPendingConnection()->setParent(&server);});
        QVERIFY(prepare(QString("http://127.0.0.1:%1/v1").arg(server.serverPort())));AgentSession session;AgentPage page(&session);page.resize(1200,850);page.show();auto *input=page.findChild<QPlainTextEdit*>("agentInput");input->setFocus();
        input->setPlainText(QStringLiteral("开始长任务"));QTest::keyClick(input,Qt::Key_Return);QVERIFY(session.isBusy());QVERIFY(page.findChild<QPushButton*>("agentStop")->isVisible());QVERIFY(input->isEnabled());QVERIFY(page.findChild<QPushButton*>("agentSend")->isEnabled());
        QVERIFY(!page.findChild<QPushButton*>("agentAccessMode")->isEnabled());QVERIFY(!page.findChild<QPushButton*>("agentWorkingDirectory")->isEnabled());const auto directory=session.workingDirectory();session.setAccessMode(AgentAccessMode::FullEnvironment);QCOMPARE(session.accessMode(),AgentAccessMode::SoftwareOnly);QVERIFY(!session.setWorkingDirectory(QDir::tempPath()));QCOMPARE(session.workingDirectory(),directory);
        page.showNotice(QStringLiteral("之前的临时错误"),true);input->setPlainText(QStringLiteral("运行中继续发送"));QTest::keyClick(input,Qt::Key_Return);QCOMPARE(session.pendingMessages().size(),1);QVERIFY(input->toPlainText().isEmpty());QVERIFY(!page.findChild<QLabel*>("agentStatus")->text().contains(QStringLiteral("临时错误")));
        auto *view=page.findChild<AgentMarkdownView*>();QVERIFY(view->toPlainText().contains(QStringLiteral("等待 Agent 接收")));page.findChild<QPushButton*>("agentStop")->click();QVERIFY(!session.isBusy());QVERIFY(!page.findChild<QPushButton*>("agentStop")->isVisible());QVERIFY(session.pendingPaused());QCOMPARE(session.pendingMessages().size(),1);
        input->setPlainText(QStringLiteral("保留的草稿"));bool opened=false;QTimer::singleShot(30,&page,[&]{auto *dialog=page.findChild<QDialog*>("agentPendingDialog");opened=dialog && dialog->findChild<QListWidget*>("agentPendingList")->count()==1;if(dialog)dialog->findChild<QPushButton*>("agentPendingEdit")->click();});page.findChild<QPushButton*>("agentPendingMessages")->click();
        QVERIFY(opened);QVERIFY(session.pendingMessages().isEmpty());QCOMPARE(input->toPlainText(),QStringLiteral("保留的草稿\n\n运行中继续发送"));screenshot(page,"ui-pending-edited.png");
    }

    void accessModeDirectoryAndPersistence() {
        QVERIFY(prepare());QTemporaryDir first,second;QVERIFY(first.isValid() && second.isValid());QSettings().setValue("agent/accessMode","full");QSettings().setValue("agent/workingDirectory",first.path());
        {
            AgentSession session;AgentPage page(&session);page.resize(1120,800);page.show();QTest::qWait(40);auto *access=page.findChild<QPushButton*>("agentAccessMode"),*directory=page.findChild<QPushButton*>("agentWorkingDirectory");
            QCOMPARE(session.accessMode(),AgentAccessMode::FullEnvironment);QCOMPARE(session.workingDirectory(),QDir(first.path()).absolutePath());QVERIFY(access->property("agentAccessFull").toBool());QVERIFY(directory->toolTip().contains(QDir::toNativeSeparators(first.path())));QVERIFY(page.width()<=1120);
            access->click();auto *popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);auto *list=popup->findChild<QListWidget*>("agentChoiceList");QCOMPARE(list->count(),2);list->setCurrentRow(0);QTest::keyClick(list,Qt::Key_Return);QCOMPARE(session.accessMode(),AgentAccessMode::SoftwareOnly);QCOMPARE(QSettings().value("agent/accessMode").toString(),QString("software"));QTest::qWait(10);
            access->click();popup=qobject_cast<AgentChoicePopup*>(QApplication::activePopupWidget());QVERIFY(popup);list=popup->findChild<QListWidget*>("agentChoiceList");screenshot(*popup,"ui-access-popup.png");QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,list->visualItemRect(list->item(1)).center());QCOMPARE(session.accessMode(),AgentAccessMode::FullEnvironment);QTest::qWait(10);
            bool chooserOpened=false;QTimer::singleShot(30,&page,[&]{auto *dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());chooserOpened=dialog;if(dialog){dialog->setDirectory(second.path());QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);}});directory->click();QVERIFY(chooserOpened);QCOMPARE(session.workingDirectory(),QDir(second.path()).absolutePath());QCOMPARE(QSettings().value("agent/workingDirectory").toString(),session.workingDirectory());screenshot(page,"ui-full-access-directory.png");
            const auto *provider=page.findChild<QPushButton*>("agentProvider");QVERIFY(access->mapTo(&page,QPoint()).x()<provider->mapTo(&page,QPoint()).x());QVERIFY(directory->geometry().right()<=directory->parentWidget()->width());
        }
        AgentSession restored;AgentPage page(&restored);QCOMPARE(restored.accessMode(),AgentAccessMode::FullEnvironment);QCOMPARE(restored.workingDirectory(),QDir(second.path()).absolutePath());
    }

    void lightDarkAndPageTheme() {
        QVERIFY(prepare());AgentSession session;AgentPage page(&session);page.resize(1250,900);page.show();QTest::qWait(40);screenshot(page,"ui-dark.png");
        auto *input=page.findChild<QPlainTextEdit*>("agentInput");input->setPlainText(QStringLiteral("输入在浅色与背景主题下保持可读。"));qApp->setStyleSheet(applicationStyleSheet(false));QTest::qWait(40);screenshot(page,"ui-light.png");
        auto *send=page.findChild<QPushButton*>("agentSend");QVERIFY(send->palette().color(QPalette::ButtonText).lightness()>160);
        QTemporaryDir dir;QSettings theme(dir.filePath("themes.ini"),QSettings::IniFormat);theme.setValue("VSR-Agent/color","#193d56");theme.sync();const auto before=page.grab().toImage();applyPageTheme(&page,theme.fileName(),"VSR-Agent");QTest::qWait(50);QVERIFY(page.grab().toImage()!=before);screenshot(page,"ui-background.png");
        QVERIFY(page.findChild<AgentComposer*>()->testAttribute(Qt::WA_StyledBackground));QVERIFY(input->isEnabled());
    }
};
QTEST_MAIN(TestAgentUi)
#include "TestAgentUi.moc"
