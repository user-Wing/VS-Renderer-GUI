#include "ui/AgentMarkdownView.h"
#include "app/Style.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>
#include <QTest>
#include <QTextBlock>
#include <QTextTable>

using namespace vsr;

namespace {
QJsonObject call(const QString &id,const QString &name,const QString &arguments)
{
    return {{"id",id},{"type","function"},{"function",QJsonObject{{"name",name},{"arguments",arguments}}}};
}
QJsonArray history()
{
    return {
        QJsonObject{{"role","user"},{"content","HIDDEN_REFERENCE_BODY"},{"displayContent",QStringLiteral("检查滤镜链，保留当前设置。")}},
        QJsonObject{{"role","assistant"},{"content",QStringLiteral("INTERMEDIATE_PROGRESS：我先读取当前设置。")}},
        QJsonObject{{"role","assistant"},{"content",QStringLiteral("先核对源尺寸和滤镜顺序。")},{"tool_calls",QJsonArray{call("a","get_state","{\"query\":\"ARGUMENT_A\"}"),call("b","list_filters","{\"query\":\"ARGUMENT_B\"}")}}},
        QJsonObject{{"role","tool"},{"tool_call_id","a"},{"content","{\"ok\":true,\"value\":\"RESULT_A\"}"}},
        QJsonObject{{"role","tool"},{"tool_call_id","b"},{"content","{\"ok\":true,\"value\":\"RESULT_B\"}"}},
        QJsonObject{{"role","user"},{"source",QStringLiteral("子智能体完成汇报")},{"content","CHILD_LARGE_JSON"}},
        QJsonObject{{"role","assistant"},{"content",QStringLiteral("## 最终结果\n\n滤镜顺序正确，当前设置已保留。\n\n- 尺寸：1920 × 1080\n- 不需要额外修改。")}}
    };
}
void toggle(AgentMarkdownView &view,const QString &key)
{
    QVERIFY(QMetaObject::invokeMethod(&view,"anchorClicked",Qt::DirectConnection,Q_ARG(QUrl,QUrl(key))));
}
int bubbles(const AgentMarkdownView &view)
{
    int count=0;for(auto *frame:view.document()->rootFrame()->childFrames())if(auto *table=qobject_cast<QTextTable*>(frame))count+=table->format().alignment()==Qt::AlignRight;return count;
}
void screenshot(AgentMarkdownView &view,const QString &name)
{
    const auto root=qEnvironmentVariable("VSR_AGENT_ARTIFACTS");if(root.isEmpty())return;
    QDir().mkpath(root);QTest::qWait(30);QVERIFY(view.grab().save(QDir(root).filePath(name)));
}
}

class TestAgentTranscript final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QFont font;font.setFamilies({"Comic Sans MS","Microsoft YaHei UI"});font.setPixelSize(13);qApp->setFont(font);
    }
    void init() { qApp->setStyleSheet(applicationStyleSheet(true)); }

    void defaultFoldAndIndependentDetails() {
        AgentMarkdownView view;view.resize(1000,760);view.show();const auto messages=history();const auto original=QJsonDocument(messages).toJson();view.showMessages(messages,{});
        auto text=view.toPlainText();QVERIFY(text.contains(QStringLiteral("执行过程 · 2 次工具调用")));QVERIFY(text.contains(QStringLiteral("最终结果")));QVERIFY(!text.contains("INTERMEDIATE_PROGRESS"));QVERIFY(!text.contains("HIDDEN_REFERENCE_BODY"));QVERIFY(!text.contains("ARGUMENT_A"));QVERIFY(!text.contains("RESULT_A"));QVERIFY(!text.contains("CHILD_LARGE_JSON"));QCOMPARE(bubbles(view),1);
        screenshot(view,"transcript-collapsed-dark.png");
        toggle(view,"process:0");text=view.toPlainText();QVERIFY(text.contains("INTERMEDIATE_PROGRESS"));QVERIFY(text.contains("get_state"));QVERIFY(text.contains("list_filters"));QVERIFY(!text.contains("ARGUMENT_A"));QVERIFY(!text.contains("CHILD_LARGE_JSON"));
        toggle(view,"detail:2-0-args");QVERIFY(view.toPlainText().contains("ARGUMENT_A"));QVERIFY(!view.toPlainText().contains("ARGUMENT_B"));QVERIFY(!view.toPlainText().contains("RESULT_A"));QVERIFY(view.document()->find("ARGUMENT_A").currentTable());
        toggle(view,"detail:2-0-result");QVERIFY(view.toPlainText().contains("RESULT_A"));QVERIFY(!view.toPlainText().contains("RESULT_B"));screenshot(view,"transcript-expanded-dark.png");
        toggle(view,"process:0");QVERIFY(!view.toPlainText().contains("RESULT_A"));toggle(view,"process:0");QVERIFY(view.toPlainText().contains("RESULT_A"));QCOMPARE(QJsonDocument(messages).toJson(),original);
    }

    void childNotificationsStayInsideTheUserTurn() {
        AgentMarkdownView view;view.resize(1000,760);view.show();auto messages=history();messages.append(QJsonObject{{"role","user"},{"source","child-2"},{"content","CHILD_SECOND_PAYLOAD"}});messages.append(QJsonObject{{"role","assistant"},{"content","LATEST_FINAL_BODY"}});messages.append(QJsonObject{{"role","user"},{"source","child-3"},{"content","CHILD_AFTER_FINAL"}});view.showMessages(messages,{});
        auto text=view.toPlainText();QCOMPARE(text.count(QStringLiteral("执行过程")),1);QCOMPARE(bubbles(view),1);QVERIFY(text.contains("LATEST_FINAL_BODY"));QVERIFY(!text.contains(QStringLiteral("最终结果")));QVERIFY(!text.contains("CHILD_AFTER_FINAL"));QVERIFY(text.contains(QStringLiteral("3 条任务通知")));
        toggle(view,"process:0");QVERIFY(!view.toPlainText().contains("CHILD_SECOND_PAYLOAD"));toggle(view,"detail:7-notice");QVERIFY(view.toPlainText().contains("CHILD_SECOND_PAYLOAD"));
        messages.append(QJsonObject{{"role","user"},{"content","SECOND_USER_TASK"}});messages.append(QJsonObject{{"role","assistant"},{"tool_calls",QJsonArray{call("c","get_state","{}")}}});messages.append(QJsonObject{{"role","tool"},{"tool_call_id","c"},{"content","{\"ok\":true}"}});messages.append(QJsonObject{{"role","assistant"},{"content","SECOND_FINAL"}});view.showMessages(messages,{});QCOMPARE(view.toPlainText().count(QStringLiteral("执行过程")),2);QCOMPARE(bubbles(view),2);QVERIFY(view.toPlainText().contains("SECOND_FINAL"));
    }

    void errorsAndCancelledCallsStayVisibleCollapsed() {
        AgentMarkdownView view;auto messages=history();messages[3]=QJsonObject{{"role","tool"},{"tool_call_id","a"},{"content",QString::fromUtf8(QJsonDocument(QJsonObject{{"ok",false},{"error",QStringLiteral("操作已取消")}}).toJson())}};messages[4]=QJsonObject{{"role","tool"},{"tool_call_id","b"},{"content","{\"isError\":true}"}};view.showMessages(messages,{}, {},true);
        QVERIFY(view.toPlainText().contains(QStringLiteral("2 项错误")));QVERIFY(view.toPlainText().contains(QStringLiteral("操作已取消")));QVERIFY(view.toPlainText().contains(QStringLiteral("正在处理")));QVERIFY(!view.toPlainText().contains("get_state"));
        view.showMessages(messages,{});QVERIFY(!view.toPlainText().contains(QStringLiteral("正在处理")));QVERIFY(view.toPlainText().contains(QStringLiteral("2 项错误")));
        auto running=history();while(running.size()>3)running.removeLast();view.showMessages(running,QStringLiteral("正在核对输入尺寸。"),{},true);QVERIFY(view.toPlainText().contains(QStringLiteral("正在核对输入尺寸")));toggle(view,"process:0");QVERIFY(view.toPlainText().contains(QStringLiteral("结果尚未返回")));
    }

    void expansionSurvivesAppendButNotConversationReplacement() {
        AgentMarkdownView view;auto messages=history();view.showMessages(messages,{});toggle(view,"process:0");toggle(view,"detail:2-0-args");messages.append(QJsonObject{{"role","user"},{"source","child"},{"content","NOTICE"}});view.showMessages(messages,{}, {},true);QVERIFY(view.toPlainText().contains("ARGUMENT_A"));
        view.showMessages({},{});view.showMessages(messages,{});QVERIFY(!view.toPlainText().contains("ARGUMENT_A"));toggle(view,"process:0");toggle(view,"detail:2-0-args");
        view.setTextCursor(view.document()->find("ARGUMENT_A"));QVERIFY(view.textCursor().hasSelection());auto replacement=messages;replacement[1]=QJsonObject{{"role","assistant"},{"content","DIFFERENT_CONVERSATION"}};view.showMessages(replacement,{});QVERIFY(!view.toPlainText().contains("ARGUMENT_A"));QVERIFY(!view.toPlainText().contains("DIFFERENT_CONVERSATION"));QVERIFY(!view.textCursor().hasSelection());
    }

    void streamingUpdatesPreserveSelectionAndReadingPosition() {
        AgentMarkdownView view;view.resize(900,540);view.show();auto messages=history();QString answer;for(int i=0;i<70;++i)answer+=QString("paragraph-%1: This is a stable explanation of the current filter chain.\n\n").arg(i);messages[6]=QJsonObject{{"role","assistant"},{"content",answer}};view.showMessages(messages,{});QTest::qWait(30);
        auto selected=view.document()->find("paragraph-30");QVERIFY(!selected.isNull());view.setTextCursor(selected);view.ensureCursorVisible();view.verticalScrollBar()->setValue(view.verticalScrollBar()->value()-80);const auto selectedText=view.textCursor().selectedText();const auto top=view.cursorForPosition(QPoint(8,20)).block().text();
        messages.append(QJsonObject{{"role","user"},{"source","child"},{"content","NEW_CHILD_REPORT"}});view.showMessages(messages,"Streaming tail",{},true);QCOMPARE(view.textCursor().selectedText(),selectedText);QCOMPARE(view.cursorForPosition(QPoint(8,20)).block().text(),top);QVERIFY(view.verticalScrollBar()->value()<view.verticalScrollBar()->maximum()-100);
    }

    void sourcePendingMessagesAreCompactAndExpandable() {
        AgentMarkdownView view;const QJsonArray pending{QJsonObject{{"id","child-pending"},{"source","child"},{"content","PENDING_CHILD_PAYLOAD"}},QJsonObject{{"id","user-pending"},{"content","USER_PENDING_BODY"}}};view.showMessages(history(),{},pending,true);QVERIFY(!view.toPlainText().contains("PENDING_CHILD_PAYLOAD"));QVERIFY(view.toPlainText().contains("USER_PENDING_BODY"));QCOMPARE(bubbles(view),2);toggle(view,"process:pending-child-pending");QVERIFY(view.toPlainText().contains("PENDING_CHILD_PAYLOAD"));
    }

    void toolUpdatesPreserveSelectionAndWaitForMouseRelease() {
        AgentMarkdownView view;view.resize(900,680);view.show();auto messages=history();while(messages.size()>3)messages.removeLast();view.showMessages(messages,{}, {},true);toggle(view,"process:0");toggle(view,"detail:2-0-args");
        view.setTextCursor(view.document()->find("ARGUMENT_A"));view.ensureCursorVisible();const auto selected=view.textCursor().selectedText();messages.append(history()[3]);view.showMessages(messages,{}, {},true);QCOMPARE(view.textCursor().selectedText(),selected);
        const auto point=view.cursorRect(view.document()->find("ARGUMENT_A")).center();QTest::mousePress(view.viewport(),Qt::LeftButton,Qt::NoModifier,point);
        messages.append(QJsonObject{{"role","tool"},{"tool_call_id","b"},{"content","{\"ok\":false,\"error\":\"MOUSE_UPDATE_ERROR\"}"}});view.showMessages(messages,{}, {},true);QVERIFY(!view.toPlainText().contains("MOUSE_UPDATE_ERROR"));
        QTest::mouseRelease(view.viewport(),Qt::LeftButton,Qt::NoModifier,point);QTRY_VERIFY(view.toPlainText().contains("MOUSE_UPDATE_ERROR"));
    }

    void importedOrphanResultsAndLightThemeRemainReadable() {
        AgentMarkdownView view;view.resize(1000,760);view.show();qApp->setStyleSheet(applicationStyleSheet(false));view.showMessages(QJsonArray{QJsonObject{{"role","user"},{"content",QStringLiteral("检查旧会话")}},QJsonObject{{"role","tool"},{"tool_call_id","legacy"},{"content","{\"ok\":false,\"error\":\"LEGACY_ERROR\"}"}},QJsonObject{{"role","assistant"},{"content",QStringLiteral("已定位旧会话中的失败，原始记录保留。")}}},{});QVERIFY(view.toPlainText().contains("LEGACY_ERROR"));QVERIFY(view.toPlainText().contains(QStringLiteral("1 次工具调用")));toggle(view,"process:0");toggle(view,"detail:1-result");QVERIFY(view.toPlainText().contains("\"ok\""));screenshot(view,"transcript-expanded-light.png");
    }

    void userBubblesHaveRoundedPixelsAndKeepMarkdownSelection() {
        AgentMarkdownView view;view.resize(860,520);view.show();QString answer;for(int i=0;i<40;++i)answer+=QString("Explanation paragraph %1 with stable readable text.\n\n").arg(i);
        const QJsonArray messages{QJsonObject{{"role","user"},{"content","**USER_MARKDOWN** with selectable text\n\n- first item\n- second item"}},QJsonObject{{"role","assistant"},{"content",answer}}};
        const auto verifyRounded=[&]{
            view.verticalScrollBar()->setValue(0);QTest::qWait(30);QTextTable *bubble=nullptr;for(auto *frame:view.document()->rootFrame()->childFrames())if(auto *table=qobject_cast<QTextTable*>(frame);table && table->format().alignment()==Qt::AlignRight){bubble=table;break;}QVERIFY(bubble);
            const auto body=view.cursorRect(bubble->cellAt(0,0).firstCursorPosition());const auto padding=bubble->format().cellPadding();const QRect rect((QPointF(body.topLeft())-QPointF(padding,padding)).toPoint(),view.document()->documentLayout()->frameBoundingRect(bubble).size().toSize());
            const auto image=view.viewport()->grab().toImage().scaled(view.viewport()->size());const QColor blue("#224e86");QVERIFY(image.rect().contains(rect));QCOMPARE(image.pixelColor(rect.center().x(),rect.top()+3),blue);QVERIFY(image.pixelColor(rect.left()+1,rect.top()+1)!=blue);QCOMPARE(image.pixelColor(rect.left()+2,rect.center().y()),blue);
            const auto user=view.document()->find("USER_MARKDOWN");QVERIFY(user.charFormat().fontWeight()>=QFont::Bold);QVERIFY(rect.adjusted(8,8,-8,-8).contains(view.cursorRect(user).center()));QVERIFY(view.toPlainText().contains("first item"));
            view.setTextCursor(user);QCOMPARE(view.textCursor().selectedText(),QString("USER_MARKDOWN"));view.grab();QCOMPARE(view.textCursor().selectedText(),QString("USER_MARKDOWN"));
        };
        view.showMessages(messages,{});verifyRounded();screenshot(view,"transcript-rounded-dark.png");view.resize(640,520);QTest::qWait(30);verifyRounded();view.zoomIn(1);verifyRounded();
        qApp->setStyleSheet(applicationStyleSheet(false));QTest::qWait(30);verifyRounded();screenshot(view,"transcript-rounded-light.png");view.setStyleSheet("background:#193d56;color:#f0f2f6;border:0;padding:6px 10px");QTest::qWait(30);verifyRounded();screenshot(view,"transcript-rounded-background.png");
        view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());view.grab();view.verticalScrollBar()->setValue(0);verifyRounded();
    }

    void imageNotificationsShowMetadataWithoutBase64() {
        AgentMarkdownView view;const QString data="data:image/png;base64,IMAGE_BYTES_MUST_NOT_BE_TEXT";
        const QJsonArray content{QJsonObject{{"type","text"},{"text","C:/images/local.png (1 × 1)"}},QJsonObject{{"type","image_url"},{"image_url",QJsonObject{{"url",data}}}}};
        view.showMessages(QJsonArray{QJsonObject{{"role","user"},{"content","Inspect image"}},QJsonObject{{"role","user"},{"source",QStringLiteral("本地图像")},{"content",content}},QJsonObject{{"role","assistant"},{"content","Inspection complete"}}},{});
        QCOMPARE(bubbles(view),1);QVERIFY(!view.toPlainText().contains("local.png"));toggle(view,"process:0");toggle(view,"detail:1-notice");QVERIFY(view.toPlainText().contains("local.png"));QVERIFY(view.toPlainText().contains(QStringLiteral("[本地图像]")));QVERIFY(!view.toPlainText().contains("base64"));QVERIFY(!view.toPlainText().contains("IMAGE_BYTES"));
    }
};
QTEST_MAIN(TestAgentTranscript)
#include "TestAgentTranscript.moc"
