#include "agent/AgentContext.h"
#include "agent/AgentSession.h"
#include "agent/AgentTaskQueue.h"

#include <QJsonDocument>
#include <QPointer>
#include <QSet>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <memory>

using namespace vsr;

namespace {
QByteArray sseEvent(const QJsonObject &object)
{
    return "data: " + QJsonDocument(object).toJson(QJsonDocument::Compact) + "\r\n\r\n";
}

QByteArray response(const QString &text, const QJsonObject &usage = {})
{
    const QJsonObject choice{{"index", 0}, {"delta", QJsonObject{{"content", text}}}, {"finish_reason", "stop"}};
    auto bytes = sseEvent({{"choices", QJsonArray{choice}}});
    if (!usage.isEmpty()) bytes += sseEvent({{"choices", QJsonArray{}}, {"usage", usage}});
    return bytes + "data: [DONE]\r\n\r\n";
}

QByteArray toolResponse(const QStringList &names)
{
    QJsonArray calls;
    for (int i = 0; i < names.size(); ++i)
        calls.append(QJsonObject{{"index", i}, {"id", "call_" + QString::number(i)}, {"type", "function"},
            {"function", QJsonObject{{"name", names[i]}, {"arguments", "{}"}}}});
    return sseEvent({{"choices", QJsonArray{QJsonObject{{"index", 0}, {"delta", QJsonObject{{"tool_calls", calls}}}, {"finish_reason", "tool_calls"}}}}}) + "data: [DONE]\r\n\r\n";
}

class Endpoint final : public QTcpServer {
public:
    QList<QJsonObject> requests;
    QList<QPointer<QTcpSocket>> sockets;
    std::function<QByteArray(int)> reply;
    bool hold = false;

    Endpoint()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection();
                auto bytes = std::make_shared<QByteArray>();
                auto parsed = std::make_shared<bool>(false);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes, parsed] {
                    if (*parsed) return;
                    *bytes += socket->readAll();
                    const int split = bytes->indexOf("\r\n\r\n");
                    if (split < 0) return;
                    int length = 0;
                    for (const auto &line : bytes->left(split).split('\n'))
                        if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                    if (bytes->size() < split + 4 + length) return;
                    *parsed = true;
                    requests.append(QJsonDocument::fromJson(bytes->mid(split + 4, length)).object());
                    sockets.append(socket);
                    if (!hold) release(requests.size() - 1, reply ? reply(requests.size()) : response("done"));
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        listen(QHostAddress::LocalHost);
    }

    bool release(int index, const QByteArray &body)
    {
        if (index < 0 || index >= sockets.size() || !sockets[index] || sockets[index]->state() != QAbstractSocket::ConnectedState) return false;
        const QByteArray prefix = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n";
        sockets[index]->write(prefix + body); sockets[index]->disconnectFromHost();
        return true;
    }

    AgentProvider provider() const
    {
        return {"mock", "Mock", QString("http://127.0.0.1:%1/v1").arg(serverPort()), "test-key", "mock-model", ""};
    }
};

QJsonArray history()
{
    return {QJsonObject{{"role", "user"}, {"content", "OLD_FIRST_TURN"}},
        QJsonObject{{"role", "assistant"}, {"content", "first answer"}},
        QJsonObject{{"role", "user"}, {"content", "second task"}},
        QJsonObject{{"role", "assistant"}, {"content", "second answer"}},
        QJsonObject{{"role", "user"}, {"content", "latest task"}},
        QJsonObject{{"role", "assistant"}, {"content", "latest answer"}}};
}

QSet<QString> toolNames(const QJsonObject &request)
{
    QSet<QString> names;
    for (const auto &value : request.value("tools").toArray()) names.insert(value.toObject().value("function").toObject().value("name").toString());
    return names;
}

QString state(const AgentTaskQueue &queue, const QString &id) { return queue.task(id).value("state").toString(); }

int requestFor(const Endpoint &endpoint, const QString &prompt)
{
    for (int i = 0; i < endpoint.requests.size(); ++i)
        if (endpoint.requests[i].value("messages").toArray().last().toObject().value("content").toString() == prompt) return i;
    return -1;
}

void registerWorkspaceTools(AgentSession &session)
{
    const QJsonObject schema{{"type", "object"}, {"additionalProperties", false}};
    for (const auto &name : {"get_state", "list_filters", "add_filter"})
        session.registerTool(name, "Dummy workspace tool", schema, [](const QJsonObject &) { return QJsonObject{{"ok", true}}; });
}
}

class TestAgentAdvanced final : public QObject {
    Q_OBJECT
private slots:
    void longEscapedChildReportIsBoundedWithoutChangingSavedConversation()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession parent;parent.setProvider(endpoint.provider());parent.cancel();QTemporaryDir directory;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QSignalSpy errors(&parent,&AgentSession::errorOccurred);
        const auto id=queue.enqueue("long child","produce findings");QVERIFY(!id.isEmpty());QTRY_COMPARE(endpoint.requests.size(),1);
        const auto full=QStringLiteral("\\\"\n\t").repeated(2000)+QString(90000,QChar(0x4e2d));QVERIFY(full.toUtf8().size()>256*1024);QVERIFY(endpoint.release(0,response(full)));QTRY_COMPARE(state(queue,id),QString("completed"));
        QCOMPARE(queue.task(id).value("result").toString(),full);QCOMPARE(queue.task(id).value("messages").toArray().last().toObject().value("content").toString(),full);QCOMPARE(parent.pendingMessages().size(),1);QVERIFY(errors.isEmpty());
        const auto report=parent.pendingMessages().first().toObject().value("content").toString();QVERIFY(report.toUtf8().size()<256*1024);const auto payload=QJsonDocument::fromJson(report.mid(report.indexOf('\n')+1).toUtf8()).object();
        QVERIFY(payload.value("resultTruncated").toBool());QCOMPARE(payload.value("result").toString(),full.left(16000));QVERIFY(payload.value("resultNotice").toString().contains("read_agent_task"));
        parent.postMessage("another pending input");QTest::qWait(30);QCOMPARE(parent.pendingMessages().size(),2);QCOMPARE(queue.task(id).value("result").toString(),full);
    }

    void clearingDuringCancellationCannotLeaveOrphanToolResults()
    {
        Endpoint endpoint;endpoint.reply=[](int){return toolResponse({"slow","after"});};AgentSession session;session.setProvider(endpoint.provider());AgentSession::Completion pending;
        session.registerAsyncTool("slow","Delayed",{},[&](const QJsonObject&,AgentSession::Completion done){pending=std::move(done);});session.registerTool("after","After",{},[](const QJsonObject&){return QJsonObject{};});
        session.send("run");QTRY_VERIFY(bool(pending));bool cleared=false;connect(&session,&AgentSession::messageAdded,&session,[&](const QJsonObject &message){if(!cleared && message.value("role")=="tool"){cleared=true;session.clear();}});
        session.cancel();QVERIFY(cleared);QVERIFY(session.messages().isEmpty());QVERIFY(!session.isBusy());AgentSession restored;QString error;QVERIFY2(restored.importConversation(session.exportConversation(),&error),qPrintable(error));
    }

    void importingCompletedTaskDoesNotInventNewCompletionNotifications()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession parent;parent.setProvider(endpoint.provider());parent.cancel();QTemporaryDir directory;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QString error;
        QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"name","imported"},{"prompt","old work"},{"state","completed"},{"revision",4},{"result","old result"}}}}},&error));
        parent.postMessage("one pending user message");QTest::qWait(60);QCOMPARE(parent.pendingMessages().size(),1);QCOMPARE(endpoint.requests.size(),0);
    }

    void importedContinuationUsesCurrentSourceAndDisabledPermissions()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession parent;parent.setProvider(endpoint.provider());parent.cancel();registerWorkspaceTools(parent);QTemporaryDir directory;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));queue.registerTools();parent.setToolEnabled("add_filter",false);parent.setToolEnabled("send_agent_message",false);parent.setToolEnabled("list_agent_tasks",false);
        const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);QVERIFY(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{QJsonObject{{"id",id},{"name","restored"},{"prompt","inspect"},{"state","interrupted"},{"allowWrites",true},{"providerName","forged source"},{"model","forged model"},{"apiKey","DO_NOT_LOAD_THIS_KEY"}}}}}));
        QVERIFY(queue.sendMessage(id,"continue inspection"));QTRY_COMPARE(endpoint.requests.size(),1);const auto names=toolNames(endpoint.requests[0]);QVERIFY(!names.contains("add_filter"));QVERIFY(!names.contains("send_agent_message"));QVERIFY(!names.contains("list_agent_tasks"));QCOMPARE(queue.task(id).value("model").toString(),QString("mock-model"));QVERIFY(!QJsonDocument(queue.exportTasks()).toJson().contains("DO_NOT_LOAD_THIS_KEY"));queue.cancel(id);
    }

    void runningModelAcceptsPendingInputWithoutInterruption()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession session;session.setProvider(endpoint.provider());QSignalSpy done(&session,&AgentSession::finished);
        session.sendWithContext("short display","FIRST_MODEL_CONTEXT");QTRY_COMPARE(endpoint.requests.size(),1);session.send("SECOND_INPUT");
        QCOMPARE(session.pendingMessages().size(),1);QCOMPARE(session.messages().size(),1);QVERIFY(endpoint.sockets[0] && endpoint.sockets[0]->state()==QAbstractSocket::ConnectedState);
        QVERIFY(endpoint.release(0,response("first answer")));QTRY_COMPARE(endpoint.requests.size(),2);QCOMPARE(done.size(),0);
        const auto history=endpoint.requests[1].value("messages").toArray();QCOMPARE(history.last().toObject().value("content").toString(),"SECOND_INPUT");QVERIFY(!QJsonDocument(history).toJson().contains("displayContent"));QVERIFY(!QJsonDocument(history).toJson().contains("\"source\""));
        QVERIFY(endpoint.release(1,response("second answer")));QTRY_COMPARE(done.size(),1);QCOMPARE(session.pendingMessages().size(),0);QCOMPARE(session.messages().size(),4);
        QCOMPARE(session.messages()[0].toObject().value("displayContent").toString(),"short display");AgentSession restored;QString error;QVERIFY2(restored.importConversation(session.exportConversation(),&error),qPrintable(error));QCOMPARE(restored.messages(),session.messages());
    }

    void pendingInputWaitsUntilAllToolResultsArePaired()
    {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn==1?toolResponse({"slow","after"}):response("done");};AgentSession session;session.setProvider(endpoint.provider());AgentSession::Completion pending;int after=0;
        session.registerAsyncTool("slow","Delayed",{},[&](const QJsonObject&,AgentSession::Completion done){pending=std::move(done);});session.registerTool("after","Following",{},[&](const QJsonObject&){++after;return QJsonObject{{"ok",true}};});
        QSignalSpy done(&session,&AgentSession::finished);session.send("run");QTRY_VERIFY(bool(pending));session.send("STEER_AFTER_TOOLS");QCOMPARE(session.messages().size(),2);QCOMPARE(session.pendingMessages().size(),1);
        const auto snapshot=session.exportConversation();AgentSession imported;QString error;QVERIFY2(imported.importConversation(snapshot,&error),qPrintable(error));QCOMPARE(session.messages().size(),2);QVERIFY(imported.pendingPaused());
        pending({{"ok",true}});QTRY_COMPARE(done.size(),1);QCOMPARE(after,1);QCOMPARE(endpoint.requests.size(),2);
        const auto history=endpoint.requests[1].value("messages").toArray();QCOMPARE(history[3].toObject().value("role").toString(),"tool");QCOMPARE(history[4].toObject().value("role").toString(),"tool");QCOMPARE(history[5].toObject().value("content").toString(),"STEER_AFTER_TOOLS");
        QVERIFY2(imported.importConversation(session.exportConversation(),&error),qPrintable(error));
    }

    void cancellationPreservesInboxAndBackgroundReportsStayPaused()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession session;session.setProvider(endpoint.provider());QSignalSpy done(&session,&AgentSession::finished);
        session.send("first");QTRY_COMPARE(endpoint.requests.size(),1);session.send("keep A");session.send("remove B");const auto entries=session.pendingMessages();session.cancel();QCOMPARE(done.size(),1);QVERIFY(session.pendingPaused());
        session.postMessage("background C","child");QTest::qWait(30);QCOMPARE(endpoint.requests.size(),1);QCOMPARE(session.pendingMessages().size(),3);
        QVERIFY(session.dropPendingMessage(entries[1].toObject().value("id").toString()));QCOMPARE(session.takePendingMessage(entries[0].toObject().value("id").toString()).value("content").toString(),"keep A");QCOMPARE(session.pendingMessages().size(),1);
        AgentSession restored;QString error;QVERIFY2(restored.importConversation(session.exportConversation(),&error),qPrintable(error));QVERIFY(restored.pendingPaused());QCOMPARE(restored.pendingMessages().size(),1);
        session.resumePending();QTRY_COMPARE(endpoint.requests.size(),2);QVERIFY(endpoint.requests[1].value("messages").toArray().last().toObject().value("content").toString().contains("background C"));QVERIFY(!endpoint.requests[1].value("messages").toArray().last().toObject().contains("source"));QVERIFY(endpoint.release(1,response("resumed")));QTRY_COMPARE(done.size(),2);
        QCOMPARE(session.messages()[1].toObject().value("source").toString(),"child");QVERIFY(restored.importConversation(session.exportConversation(),&error));QCOMPARE(restored.messages()[1].toObject().value("source").toString(),"child");
    }

    void cancellingWhileInjectingDoesNotLoseRemainingMessages()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession session;session.setProvider(endpoint.provider());session.cancel();session.postMessage("A");session.postMessage("B");
        connect(&session,&AgentSession::messageAdded,&session,[&](const QJsonObject &message){if(message.value("content").toString()=="A")session.cancel();});
        session.resumePending();QVERIFY(!session.isBusy());QVERIFY(session.pendingPaused());QCOMPARE(session.pendingMessages().size(),1);QCOMPARE(session.pendingMessages()[0].toObject().value("content").toString(),"B");QCOMPARE(endpoint.requests.size(),0);
    }

    void automaticSpawnAndContinuedChildConversation()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession parent;parent.setProvider(endpoint.provider());parent.cancel();registerWorkspaceTools(parent);QTemporaryDir directory;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));queue.registerTools();QPointer<AgentSession> child;connect(&queue,&AgentTaskQueue::sessionCreated,&queue,[&](AgentSession *session,bool){child=session;});
        const auto spawned=parent.executeTool("spawn_agent",{{"prompt","CHILD_FIRST"}});QVERIFY(spawned.value("ok").toBool());const auto id=spawned.value("taskId").toString();QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(child);
        const auto names=toolNames(endpoint.requests[0]);QVERIFY(!names.contains("spawn_agent"));QVERIFY(!names.contains("enqueue_agent_task"));QVERIFY(!names.contains("add_filter"));QVERIFY(names.contains("send_agent_message"));
        const auto communication=child->executeTool("send_agent_message",{{"id","parent"},{"message","intermediate finding"}});QVERIFY(communication.value("ok").toBool());QCOMPARE(parent.pendingMessages().size(),1);
        QVERIFY(queue.sendMessage(id,"CHILD_FOLLOWUP"));QVERIFY(endpoint.release(0,response("first child answer")));QTRY_COMPARE(endpoint.requests.size(),2);QCOMPARE(state(queue,id),QString("running"));QVERIFY(endpoint.requests[1].value("messages").toArray().last().toObject().value("content").toString().contains("CHILD_FOLLOWUP"));
        QVERIFY(endpoint.release(1,response("final child answer")));QTRY_COMPARE(state(queue,id),QString("completed"));QCOMPARE(queue.task(id).value("messages").toArray().size(),4);QCOMPARE(queue.task(id).value("rounds").toInt(),2);QCOMPARE(parent.pendingMessages().size(),2);QVERIFY(parent.pendingPaused());QCOMPARE(parent.messages().size(),0);
        QVERIFY(queue.sendMessage(id,"ONE_MORE"));QTRY_COMPARE(endpoint.requests.size(),3);QCOMPARE(endpoint.requests[2].value("messages").toArray().size(),6);QVERIFY(endpoint.release(2,response("third child answer")));QTRY_COMPARE(state(queue,id),QString("completed"));QCOMPARE(queue.task(id).value("messages").toArray().size(),6);QCOMPARE(parent.pendingMessages().size(),3);
        QTest::qWait(30);QCOMPARE(parent.pendingMessages().size(),3);QVERIFY(!QJsonDocument(queue.exportTasks()).toJson().contains("test-key"));
    }

    void parentWaitReceivesOneAutomaticReportBeforeSummarizing()
    {
        Endpoint endpoint;endpoint.hold=true;AgentSession parent;parent.setProvider(endpoint.provider());registerWorkspaceTools(parent);QTemporaryDir directory;AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));queue.registerTools();QSignalSpy finished(&parent,&AgentSession::finished);
        parent.send(QStringLiteral("使用子智能体完成调查并汇总"));QTRY_COMPARE(endpoint.requests.size(),1);
        QJsonArray calls{QJsonObject{{"index",0},{"id","spawn"},{"type","function"},{"function",QJsonObject{{"name","spawn_agent"},{"arguments","{\"prompt\":\"DELEGATED_WORK\"}"}}}},QJsonObject{{"index",1},{"id","wait"},{"type","function"},{"function",QJsonObject{{"name","wait_agent_tasks"},{"arguments","{\"seconds\":2}"}}}}};
        QVERIFY(endpoint.release(0,sseEvent({{"choices",QJsonArray{QJsonObject{{"index",0},{"delta",QJsonObject{{"tool_calls",calls}}},{"finish_reason","tool_calls"}}}}})+"data: [DONE]\r\n\r\n"));
        QTRY_COMPARE(endpoint.requests.size(),2);QVERIFY(endpoint.release(1,response("verified child findings")));QTRY_COMPARE_WITH_TIMEOUT(endpoint.requests.size(),3,5000);
        const auto history=endpoint.requests[2].value("messages").toArray();int reports=0;for(const auto &value:history)if(value.toObject().value("role")=="user" && value.toObject().value("content").toString().contains("完成汇报"))++reports;QCOMPARE(reports,1);
        QCOMPARE(history[3].toObject().value("role").toString(),"tool");QCOMPARE(history[4].toObject().value("role").toString(),"tool");QVERIFY(endpoint.release(2,response("parent consolidated findings")));QTRY_COMPARE(finished.size(),1);QTest::qWait(150);QCOMPARE(endpoint.requests.size(),3);
    }

    void usageTailPromptAndOutputLimit()
    {
        Endpoint endpoint;
        endpoint.reply = [](int turn) {
            const QJsonObject usage{{"prompt_tokens", turn == 1 ? 100 : 160}, {"completion_tokens", turn == 1 ? 20 : 30},
                {"total_tokens", turn == 1 ? 120 : 190}, {"prompt_tokens_details", QJsonObject{{"cached_tokens", turn == 1 ? 60 : 100}}},
                {"completion_tokens_details", QJsonObject{{"reasoning_tokens", turn == 1 ? 5 : 8}}}};
            // A server may repeat its usage snapshot before the choices-empty final frame.
            return sseEvent({{"choices", QJsonArray{}}, {"usage", usage}}) + response("answer", usage);
        };
        AgentSession session; auto provider = endpoint.provider(); provider.contextLimit = 32768; provider.maxOutputTokens = 512;
        session.setProvider(provider); session.setSystemPrompt("GLOBAL_PROMPT_MARKER");
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred), changes(&session, &AgentSession::usageChanged);
        session.send("first"); QTRY_COMPARE(done.count(), 1);
        session.send("second"); QTRY_COMPARE(done.count(), 2);
        QCOMPARE(errors.count(), 0); QVERIFY(!changes.isEmpty());
        QCOMPARE(endpoint.requests.size(), 2);
        const auto request = endpoint.requests.first();
        QCOMPARE(request.value("max_tokens").toInt(), 512);
        QVERIFY(request.value("stream_options").toObject().value("include_usage").toBool());
        QVERIFY(request.value("messages").toArray().first().toObject().value("content").toString().contains("GLOBAL_PROMPT_MARKER"));
        const auto usage = session.usage();
        QCOMPARE(usage.value("promptTokens").toDouble(), 260.);
        QCOMPARE(usage.value("completionTokens").toDouble(), 50.);
        QCOMPARE(usage.value("totalTokens").toDouble(), 310.);
        QCOMPARE(usage.value("cachedTokens").toDouble(), 160.);
        QCOMPARE(usage.value("reasoningTokens").toDouble(), 13.);
        QCOMPARE(usage.value("lastPromptTokens").toDouble(), 160.);
        QVERIFY(usage.value("hasActualUsage").toBool());
        QCOMPARE(usage.value("contextLimit").toInt(), 32768);
    }

    void contextCheckpointAndUsageRoundTrip()
    {
        const auto messages = history(); const auto original = messages;
        AgentContext context;
        QCOMPARE(AgentContext::estimate(QStringLiteral("abcd中文")), 3);
        QCOMPARE(context.compressionBoundary(messages), 2);
        context.checkpoint(2, "FACTUAL_CHECKPOINT");
        context.addUsage({{"input_tokens", 200}, {"output_tokens", 40}, {"cache_read_input_tokens", 80},
            {"output_tokens_details", QJsonObject{{"reasoning_tokens", 10}}}}, "alternate-model");
        const auto projected = context.project(messages, "global");
        QCOMPARE(messages, original);
        QCOMPARE(projected.size(), 6);
        QVERIFY(projected[1].toObject().value("content").toString().contains("FACTUAL_CHECKPOINT"));
        QCOMPARE(projected[2], messages[2]);
        AgentContext restored; QVERIFY(restored.restore(context.save(), messages));
        QCOMPARE(restored.project(messages, "global"), projected);
        const auto usage = restored.usage(300, 4096);
        QCOMPARE(usage.value("totalTokens").toDouble(), 240.);
        QCOMPARE(usage.value("cachedTokens").toDouble(), 80.);
        QCOMPARE(usage.value("lastModel").toString(), QString("alternate-model"));
        QCOMPARE(usage.value("compactions").toInt(), 1);
    }

    void manualCompactionUsesSummaryWithoutDeletingHistory()
    {
        Endpoint endpoint;
        endpoint.reply = [](int turn) { return response(turn == 1 ? "FACTUAL_SUMMARY_ONLY" : "next answer"); };
        AgentSession session; session.setProvider(endpoint.provider()); session.setSystemPrompt("GLOBAL_PROMPT_MARKER");
        const auto original = history(); QString error;
        QVERIFY2(session.importConversation({{"schemaVersion", 1}, {"messages", original}}, &error), qPrintable(error));
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.compactContext(); QTRY_COMPARE(done.count(), 1);
        QCOMPARE(errors.count(), 0); QCOMPARE(endpoint.requests.size(), 1);
        QVERIFY(!endpoint.requests.first().contains("tools"));
        QVERIFY(QJsonDocument(endpoint.requests.first()).toJson().contains("OLD_FIRST_TURN"));
        QCOMPARE(session.messages(), original);
        QCOMPARE(session.exportConversation().value("messages").toArray(), original);
        QCOMPARE(session.usage().value("compactions").toInt(), 1);
        session.send("follow-up"); QTRY_COMPARE(done.count(), 2);
        QCOMPARE(endpoint.requests.size(), 2);
        const auto sent = endpoint.requests[1].value("messages").toArray();
        QVERIFY(sent.first().toObject().value("content").toString().contains("GLOBAL_PROMPT_MARKER"));
        QVERIFY(sent[1].toObject().value("content").toString().contains("FACTUAL_SUMMARY_ONLY"));
        QVERIFY(!QJsonDocument(sent).toJson().contains("OLD_FIRST_TURN"));
        QCOMPARE(sent.last().toObject().value("content").toString(), QString("follow-up"));
        QVERIFY(QJsonDocument(session.exportConversation()).toJson().contains("OLD_FIRST_TURN"));
        AgentSession restored; QVERIFY2(restored.importConversation(session.exportConversation(), &error), qPrintable(error));
        QCOMPARE(restored.messages(), session.messages());
    }

    void failedCompactionRetainsPreviousCheckpoint()
    {
        Endpoint endpoint;
        endpoint.reply = [](int) { return sseEvent({{"choices", QJsonArray{QJsonObject{{"index", 0}, {"delta", QJsonObject{{"content", "incomplete summary"}}}, {"finish_reason", "length"}}}}}); };
        AgentSession session; session.setProvider(endpoint.provider());
        AgentContext context; context.checkpoint(2, "KEEP_PREVIOUS_CHECKPOINT");
        QString error;
        QVERIFY(session.importConversation({{"schemaVersion", 1}, {"messages", history()}, {"context", context.save()}}, &error));
        const auto before = session.exportConversation();
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.compactContext(); QTRY_COMPARE(done.count(), 1);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(session.exportConversation().value("messages"), before.value("messages"));
        QCOMPARE(session.exportConversation().value("context").toObject().value("summary").toString(), QString("KEEP_PREVIOUS_CHECKPOINT"));
        QCOMPARE(session.exportConversation().value("context").toObject().value("covered").toInt(), 2);
    }

    void copiedAsyncToolWaitsBeforeFollowingCalls()
    {
        Endpoint endpoint;
        endpoint.reply = [](int turn) { return turn == 1 ? toolResponse({"slow", "after"}) : response("finished tools"); };
        AgentSession source, session;
        AgentSession::Completion pending;
        int slow = 0, following = 0;
        source.registerAsyncTool("slow", "Delayed tool", {{"type", "object"}}, [&](const QJsonObject &, AgentSession::Completion completion) { ++slow; pending = std::move(completion); });
        source.registerTool("after", "Following tool", {{"type", "object"}}, [&](const QJsonObject &) { ++following; return QJsonObject{{"ok", true}}; });
        source.registerTool("disabled", "Disabled", {}, [](const QJsonObject &) { return QJsonObject{}; }); source.setToolEnabled("disabled", false);
        source.registerTool("excluded", "Excluded", {}, [](const QJsonObject &) { return QJsonObject{}; });
        source.registerTool("no_delegate", "Not delegatable", {}, [](const QJsonObject &) { return QJsonObject{}; });
        source.setToolDelegationAllowed("no_delegate", false);
        source.copyToolsTo(&session, {"excluded"});
        QCOMPARE(session.allTools().size(), 3); QVERIFY(!session.toolEnabled("disabled"));
        for (const auto &tool : session.allTools())
            QVERIFY(tool.toObject().value("function").toObject().value("name").toString() != "no_delegate");
        session.setProvider(endpoint.provider()); QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("run slow and after"); QTRY_COMPARE(slow, 1);
        QVERIFY(session.isBusy()); QCOMPARE(following, 0); QCOMPARE(endpoint.requests.size(), 1); QVERIFY(bool(pending));
        pending({{"ok", true}, {"value", 7}});
        pending({{"ok", true}, {"value", 99}}); // The same asynchronous completion must settle only once.
        QTRY_COMPARE(done.count(), 1);
        QCOMPARE(following, 1); QCOMPARE(errors.count(), 0); QCOMPARE(endpoint.requests.size(), 2);
        const auto messages = session.messages(); QCOMPARE(messages.size(), 5);
        QCOMPARE(QJsonDocument::fromJson(messages[2].toObject().value("content").toString().toUtf8()).object().value("value").toInt(), 7);
        AgentSession imported; QString error; QVERIFY2(imported.importConversation(session.exportConversation(), &error), qPrintable(error));
    }

    void cancelledAsyncToolIgnoresLateCompletionAndKeepsPairs()
    {
        Endpoint endpoint; endpoint.reply = [](int) { return toolResponse({"slow", "after"}); };
        AgentSession session; session.setProvider(endpoint.provider());
        AgentSession::Completion pending; int following = 0;
        session.registerAsyncTool("slow", "Delayed", {}, [&](const QJsonObject &, AgentSession::Completion completion) { pending = std::move(completion); });
        session.registerTool("after", "Following", {}, [&](const QJsonObject &) { ++following; return QJsonObject{{"ok", true}}; });
        QSignalSpy done(&session, &AgentSession::finished), cancelled(&session, &AgentSession::toolExecutionCancelled);
        session.send("run"); QTRY_VERIFY(bool(pending));
        session.cancel(); QCOMPARE(done.count(), 1); QCOMPARE(cancelled.count(), 1); QVERIFY(!session.isBusy());
        const auto stopped = session.messages(); QCOMPARE(stopped.size(), 4);
        pending({{"ok", true}}); QTest::qWait(30);
        QCOMPARE(following, 0); QCOMPARE(endpoint.requests.size(), 1); QCOMPARE(session.messages(), stopped);
        AgentSession imported; QString error; QVERIFY2(imported.importConversation(session.exportConversation(), &error), qPrintable(error));
    }

    void readOnlyQueueRunsTwoIndependentSessions()
    {
        Endpoint endpoint; endpoint.hold = true; AgentSession parent; registerWorkspaceTools(parent);
        parent.setToolEnabled("get_state", false);
        parent.setSystemPrompt("SHARED_GLOBAL_INSTRUCTIONS");
        QVERIFY(parent.importConversation({{"schemaVersion", 1}, {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"content", "PARENT_HISTORY_PRIVATE"}}}}}));
        QTemporaryDir directory; AgentTaskQueue queue(&parent, directory.filePath("tasks.json")); queue.registerTools();
        const auto first = queue.enqueue("first", "TASK_ONE", endpoint.provider());
        const auto second = queue.enqueue("second", "TASK_TWO", endpoint.provider());
        const auto third = queue.enqueue("third", "TASK_THREE", endpoint.provider());
        QVERIFY(!first.isEmpty() && !second.isEmpty() && !third.isEmpty());
        QTRY_COMPARE(endpoint.requests.size(), 2);
        QCOMPARE(state(queue, first), QString("running")); QCOMPARE(state(queue, second), QString("running")); QCOMPARE(state(queue, third), QString("queued"));
        QSet<QString> prompts;
        for (const auto &request : endpoint.requests) {
            const auto messages = request.value("messages").toArray(); QCOMPARE(messages.size(), 2);
            QVERIFY(messages.first().toObject().value("content").toString().contains("SHARED_GLOBAL_INSTRUCTIONS"));
            QVERIFY(!QJsonDocument(messages).toJson().contains("PARENT_HISTORY_PRIVATE"));
            prompts.insert(messages.last().toObject().value("content").toString());
            const auto tools = toolNames(request); QVERIFY(!tools.contains("get_state")); QVERIFY(tools.contains("list_filters"));
            QVERIFY(!tools.contains("add_filter")); QVERIFY(!tools.contains("enqueue_agent_task"));
        }
        QCOMPARE(prompts, QSet<QString>({"TASK_ONE", "TASK_TWO"}));
        QVERIFY(endpoint.release(requestFor(endpoint, "TASK_ONE"), response("first result")));
        QTRY_COMPARE(state(queue, first), QString("completed")); QTRY_COMPARE(endpoint.requests.size(), 3);
        QCOMPARE(state(queue, third), QString("running"));
        QVERIFY(endpoint.release(requestFor(endpoint, "TASK_TWO"), response("second result"))); QVERIFY(endpoint.release(requestFor(endpoint, "TASK_THREE"), response("third result")));
        QTRY_COMPARE(state(queue, second), QString("completed")); QTRY_COMPARE(state(queue, third), QString("completed"));
        QCOMPARE(queue.task(first).value("result").toString(), QString("first result"));
        QCOMPARE(parent.messages().size(), 1);
    }

    void queueWritesAreExclusiveAndCancellationReleasesSlots()
    {
        Endpoint endpoint; endpoint.hold = true; AgentSession parent; registerWorkspaceTools(parent);
        QTemporaryDir directory; AgentTaskQueue queue(&parent, directory.filePath("tasks.json"));
        const auto reader = queue.enqueue("reader", "READ_FIRST", endpoint.provider());
        const auto writer = queue.enqueue("writer", "WRITE_EXCLUSIVE", endpoint.provider(), true);
        const auto cancelled = queue.enqueue("cancel pending", "NEVER_RUN", endpoint.provider());
        QTRY_COMPARE(endpoint.requests.size(), 1);
        QCOMPARE(state(queue, reader), QString("running")); QCOMPARE(state(queue, writer), QString("queued")); QCOMPARE(state(queue, cancelled), QString("queued"));
        queue.cancel(cancelled); QCOMPARE(state(queue, cancelled), QString("cancelled"));
        QVERIFY(endpoint.release(0, response("reader done"))); QTRY_COMPARE(endpoint.requests.size(), 2);
        QCOMPARE(state(queue, writer), QString("running"));
        QVERIFY(toolNames(endpoint.requests[1]).contains("add_filter"));
        const auto after = queue.enqueue("after", "AFTER_CANCEL", endpoint.provider());
        QTest::qWait(30); QCOMPARE(state(queue, after), QString("queued")); QCOMPARE(endpoint.requests.size(), 2);
        queue.cancel(writer); QCOMPARE(state(queue, writer), QString("cancelled"));
        QTRY_COMPARE(endpoint.requests.size(), 3); QCOMPARE(state(queue, after), QString("running"));
        QCOMPARE(endpoint.requests.last().value("messages").toArray().last().toObject().value("content").toString(), QString("AFTER_CANCEL"));
        QVERIFY(endpoint.release(2, response("after done"))); QTRY_COMPARE(state(queue, after), QString("completed"));
        QVERIFY(!QJsonDocument(queue.exportTasks()).toJson().contains("test-key"));
    }

    void importedQueueRequiresExplicitResume()
    {
        Endpoint endpoint; endpoint.hold = true; AgentSession parent; registerWorkspaceTools(parent);
        QTemporaryDir directory; const auto path = directory.filePath("tasks.json");
        const auto first = QUuid::createUuid().toString(QUuid::WithoutBraces), second = QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            AgentTaskQueue imported(&parent, path); QString error;
            QVERIFY2(imported.importTasks({{"schemaVersion", 1}, {"tasks", QJsonArray{
                QJsonObject{{"id", first}, {"name", "restored queued"}, {"prompt", "RESUME_ME"}, {"state", "queued"}},
                QJsonObject{{"id", second}, {"name", "restored running"}, {"prompt", "DO_NOT_AUTORUN"}, {"state", "running"}}}}}, &error), qPrintable(error));
            QCOMPARE(state(imported, first), QString("interrupted")); QCOMPARE(state(imported, second), QString("interrupted"));
            QTest::qWait(30); QCOMPARE(endpoint.requests.size(), 0);
        }
        AgentTaskQueue restored(&parent, path); QString error; QVERIFY2(restored.load(&error), qPrintable(error));
        QTest::qWait(30); QCOMPARE(endpoint.requests.size(), 0); QCOMPARE(restored.tasks().size(), 2);
        QVERIFY(restored.resume(first, endpoint.provider())); QTRY_COMPARE(endpoint.requests.size(), 1);
        QCOMPARE(state(restored, first), QString("running")); QCOMPARE(state(restored, second), QString("interrupted"));
        QVERIFY(endpoint.release(0, response("resumed result"))); QTRY_COMPARE(state(restored, first), QString("completed"));
        QCOMPARE(restored.task(first).value("result").toString(), QString("resumed result"));
    }
};

QTEST_GUILESS_MAIN(TestAgentAdvanced)
#include "TestAgentAdvanced.moc"
