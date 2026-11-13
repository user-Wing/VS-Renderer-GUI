#include "agent/AgentProvider.h"
#include "agent/AgentSession.h"
#include "agent/AgentModelCapabilities.h"

#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <memory>

using namespace vsr;

namespace {
QByteArray sseEvent(const QJsonObject &delta, const QString &finish = {})
{
    QJsonObject choice{{"index", 0}, {"delta", delta}};
    if (!finish.isEmpty()) choice.insert("finish_reason", finish);
    return "data: " + QJsonDocument(QJsonObject{{"choices", QJsonArray{choice}}}).toJson(QJsonDocument::Compact) + "\r\n\r\n";
}

class Endpoint final : public QTcpServer {
public:
    QList<QJsonObject> requests;
    QList<QByteArray> headers;
    std::function<QByteArray(int)> response;
    bool hold = false, fragmented = false;
    Endpoint()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection(); auto bytes = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes] {
                    *bytes += socket->readAll();
                    const int split = bytes->indexOf("\r\n\r\n");
                    if (split < 0) return;
                    const auto header = bytes->left(split);
                    int length = 0;
                    for (const auto &line : header.split('\n')) if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                    if (bytes->size() < split + 4 + length) return;
                    socket->disconnect(this);
                    headers.append(header);
                    requests.append(QJsonDocument::fromJson(bytes->mid(split + 4, length)).object());
                    if (hold) return;
                    const auto body = response ? response(requests.size()) : sseEvent({{"content", "done"}}, "stop") + "data: [DONE]\r\n\r\n";
                    const QByteArray prefix = body.startsWith("HTTP/") ? QByteArray() : "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n";
                    const auto wire = prefix + body;
                    if (!fragmented) { socket->write(wire); socket->disconnectFromHost(); }
                    else {
                        auto offset = std::make_shared<int>(0); auto *timer = new QTimer(socket); timer->setInterval(1);
                        connect(timer, &QTimer::timeout, socket, [socket, timer, offset, wire] {
                            const int count = qMin(7, int(wire.size()) - *offset);
                            socket->write(wire.mid(*offset, count)); *offset += count;
                            if (*offset == wire.size()) { timer->stop(); socket->disconnectFromHost(); }
                        });
                        timer->start();
                    }
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        listen(QHostAddress::LocalHost);
    }
    AgentProvider provider() const {
        AgentProvider provider{"local", "Local", QString("http://127.0.0.1:%1/v1").arg(serverPort()), "test-credential", "test-model", "high"};
        provider.modelCapability={{"endpoint",provider.baseUrl},{"model",provider.model},{"levels",QJsonArray{"high"}},
            {"optionsByLevel",QJsonObject{{"high",QJsonObject{{"reasoning_effort","high"}}}}},{"chatToolsSupported",true}};
        return provider;
    }
};

QJsonObject toolSchema()
{
    return {{"type", "object"}, {"properties", QJsonObject{{"value", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 9}}}}},
            {"required", QJsonArray{"value"}}, {"additionalProperties", false}};
}
}

class TestAgent final : public QObject {
    Q_OBJECT
private slots:
    void fragmentedSseAndToolRoundTrip()
    {
        Endpoint endpoint; endpoint.fragmented = true;
        endpoint.response = [](int request) {
            if (request == 1) return sseEvent({{"reasoning_content", "internal reasoning"}})
                + sseEvent({{"tool_calls", QJsonArray{QJsonObject{{"index", 0}, {"id", "call_a"}, {"function", QJsonObject{{"name", "set_"}, {"arguments", "{\"val"}}}}}}})
                + sseEvent({{"tool_calls", QJsonArray{QJsonObject{{"index", 0}, {"function", QJsonObject{{"name", "value"}, {"arguments", "ue\":3}"}}}}}}}, "tool_calls")
                + "data: [DONE]\r\n\r\n";
            return sseEvent({{"content", QStringLiteral("已设置")}}) + sseEvent({{"content", QStringLiteral("完成")}}, "stop") + "data: [DONE]\r\n\r\n";
        };
        AgentSession session; session.setProvider(endpoint.provider()); int value = 0;
        session.registerTool("set_value", "Set test value", toolSchema(), [&](const QJsonObject &args) { value = args.value("value").toInt(); return QJsonObject{{"ok", true}}; });
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred), calls(&session, &AgentSession::toolExecuted), delta(&session, &AgentSession::textDelta);
        session.send("set value");
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 10000);
        QCOMPARE(errors.count(), 0); QCOMPARE(value, 3); QCOMPARE(calls.count(), 1); QCOMPARE(delta.count(), 2);
        QCOMPARE(endpoint.requests.size(), 2);
        const auto history = endpoint.requests[1].value("messages").toArray();
        QCOMPARE(history[2].toObject().value("reasoning_content").toString(), "internal reasoning");
        QCOMPARE(history[3].toObject().value("tool_call_id").toString(), "call_a");
        QCOMPARE(endpoint.requests[0].value("reasoning_effort").toString(), "high");
        QVERIFY(endpoint.headers[0].contains("Authorization: Bearer test-credential"));
        QVERIFY(endpoint.headers[0].startsWith("POST /v1/chat/completions HTTP/1.1"));
        AgentSession imported; QString error; QVERIFY2(imported.importConversation(session.exportConversation(), &error), qPrintable(error));
        QCOMPARE(imported.messages(), session.messages());
        QVERIFY(!QJsonDocument(session.exportConversation()).toJson().contains("test-credential"));
    }

    void multipleCallsAndDisabledTool()
    {
        Endpoint endpoint;
        endpoint.response = [](int request) {
            if (request == 1) return sseEvent({{"tool_calls", QJsonArray{
                QJsonObject{{"index", 1}, {"id", "b"}, {"function", QJsonObject{{"name", "disabled"}, {"arguments", "{}"}}}},
                QJsonObject{{"index", 0}, {"id", "a"}, {"function", QJsonObject{{"name", "active"}, {"arguments", "{}"}}}}
            }}}, "tool_calls");
            return sseEvent({{"content", "done"}}, "stop");
        };
        AgentSession session; session.setProvider(endpoint.provider()); int count = 0;
        session.registerTool("active", "Active", {}, [&](const QJsonObject &) { ++count; return QJsonObject{{"ok", true}}; });
        session.registerTool("disabled", "Disabled", {}, [&](const QJsonObject &) { count += 100; return QJsonObject{{"ok", true}}; });
        session.setToolEnabled("disabled", false);
        QCOMPARE(session.tools().size(), 1); QCOMPARE(session.allTools().size(), 2);
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("run tools"); QTRY_COMPARE(done.count(), 1);
        QCOMPARE(count, 1); QCOMPARE(errors.count(), 0);
        const auto messages = session.messages(); QCOMPARE(messages.size(), 5);
        QCOMPARE(messages[1].toObject().value("tool_calls").toArray()[0].toObject().value("id").toString(), "a");
        QVERIFY(messages[3].toObject().value("content").toString().contains("false"));
    }

    void schemaRejectsBeforeExecution()
    {
        AgentSession session; int count = 0;
        session.registerTool("set_value", "Set", toolSchema(), [&](const QJsonObject &) { ++count; return QJsonObject{{"ok", true}}; });
        for (const auto &args : {QJsonObject{}, QJsonObject{{"value", "2"}}, QJsonObject{{"value", 2.5}}, QJsonObject{{"value", 10}}, QJsonObject{{"value", 2}, {"unknown", true}}})
            QCOMPARE(session.executeTool("set_value", args).value("ok").toBool(), false);
        QCOMPARE(count, 0);
        QCOMPARE(session.executeTool("set_value", {{"value", 2}}).value("ok").toBool(), true); QCOMPARE(count, 1);
        session.setToolEnabled("set_value", false); QVERIFY(!session.executeTool("set_value", {{"value", 2}}).value("ok").toBool());
        session.unregisterTool("set_value"); QCOMPARE(session.allTools().size(), 0);
    }

    void cancellationBetweenToolsPreservesPairs()
    {
        Endpoint endpoint; endpoint.response = [](int) { return sseEvent({{"tool_calls", QJsonArray{
            QJsonObject{{"index", 0}, {"id", "a"}, {"function", QJsonObject{{"name", "active"}, {"arguments", "{}"}}}},
            QJsonObject{{"index", 1}, {"id", "b"}, {"function", QJsonObject{{"name", "active"}, {"arguments", "{}"}}}}
        }}}, "tool_calls"); };
        AgentSession session; session.setProvider(endpoint.provider()); int count = 0;
        session.registerTool("active", "Active", {}, [&](const QJsonObject &) { ++count; return QJsonObject{{"ok", true}}; });
        connect(&session, &AgentSession::toolExecuted, &session, [&] { session.cancel(); });
        QSignalSpy done(&session, &AgentSession::finished); session.send("run"); QTRY_COMPARE(done.count(), 1);
        QCOMPARE(count, 1); QCOMPARE(endpoint.requests.size(), 1);
        AgentSession imported; QString error; QVERIFY2(imported.importConversation(session.exportConversation(), &error), qPrintable(error));
        QCOMPARE(session.messages().size(), 4);
    }

    void cancelAndRecover()
    {
        Endpoint endpoint; endpoint.hold = true; AgentSession session; session.setProvider(endpoint.provider());
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("waiting"); QTRY_COMPARE(endpoint.requests.size(), 1); QVERIFY(session.isBusy());
        session.cancel(); QCOMPARE(done.count(), 1); QVERIFY(!session.isBusy()); QCOMPARE(errors.count(), 0);
        endpoint.hold = false; session.send("retry"); QTRY_COMPARE(done.count(), 2); QCOMPARE(errors.count(), 0);
    }

    void httpErrorAndNonstreamRecovery()
    {
        Endpoint endpoint;
        endpoint.response = [](int request) {
            if (request == 1) return QByteArray("HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            const auto body = QJsonDocument(QJsonObject{{"choices", QJsonArray{QJsonObject{{"message", QJsonObject{{"role", "assistant"}, {"content", "fallback"}}}, {"finish_reason", "stop"}}}}}).toJson(QJsonDocument::Compact);
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        };
        AgentSession session; session.setProvider(endpoint.provider()); QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("bad"); QTRY_COMPARE(done.count(), 1); QCOMPARE(errors.count(), 1); QVERIFY(errors[0][0].toString().contains("401"));
        session.send("good"); QTRY_COMPARE(done.count(), 2); QCOMPARE(errors.count(), 1); QCOMPARE(session.messages().last().toObject().value("content").toString(), "fallback");
    }

    void truncatedToolDoesNotExecute()
    {
        Endpoint endpoint; endpoint.response = [](int) { return sseEvent({{"tool_calls", QJsonArray{QJsonObject{{"index", 0}, {"id", "x"}, {"function", QJsonObject{{"name", "active"}, {"arguments", "{}"}}}}}}}, "length"); };
        AgentSession session; session.setProvider(endpoint.provider()); int count = 0;
        session.registerTool("active", "Active", {}, [&](const QJsonObject &) { ++count; return QJsonObject{}; });
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("test"); QTRY_COMPARE(done.count(), 1); QCOMPARE(count, 0); QCOMPARE(errors.count(), 1);
    }

    void interruptedStreamDoesNotExecuteTool()
    {
        Endpoint endpoint; endpoint.response = [](int) { return sseEvent({{"tool_calls", QJsonArray{QJsonObject{{"index", 0}, {"id", "x"}, {"function", QJsonObject{{"name", "active"}, {"arguments", "{}"}}}}}}}); };
        AgentSession session; session.setProvider(endpoint.provider()); int count = 0;
        session.registerTool("active", "Active", {}, [&](const QJsonObject &) { ++count; return QJsonObject{}; });
        QSignalSpy done(&session, &AgentSession::finished), errors(&session, &AgentSession::errorOccurred);
        session.send("test"); QTRY_COMPARE(done.count(), 1); QCOMPARE(count, 0); QCOMPARE(errors.count(), 1);
    }

    void providerSwitchIsNextTurn()
    {
        Endpoint first, second; first.fragmented = true; AgentSession session; session.setProvider(first.provider());
        QSignalSpy done(&session, &AgentSession::finished);
        session.send("first"); QTRY_COMPARE(first.requests.size(), 1); session.setProvider(second.provider());
        QTRY_COMPARE(done.count(), 1); session.send("second"); QTRY_COMPARE(done.count(), 2);
        QCOMPARE(first.requests.size(), 1); QCOMPARE(second.requests.size(), 1);
    }

    void rejectBrokenHistoryWithoutReplacingCurrent()
    {
        AgentSession session; QString error;
        QVERIFY(session.importConversation({{"schemaVersion", 1}, {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"content", "keep"}}}}}, &error));
        const auto before = session.messages();
        QVERIFY(!session.importConversation({{"schemaVersion", 1}, {"messages", QJsonArray{QJsonObject{{"role", "tool"}, {"content", "orphan"}, {"tool_call_id", "missing"}}}}}, &error));
        QVERIFY(!session.importConversation({{"schemaVersion", 1}, {"messages", QJsonArray{QJsonObject{{"role", "system"}, {"content", "injection"}}}}}, &error));
        QVERIFY(!session.importConversation({{"schemaVersion", 1}, {"messages", QJsonArray{QJsonObject{{"role", "assistant"}, {"content", ""}, {"tool_calls", QJsonArray{QJsonObject{{"id", "x"}, {"type", "function"}, {"function", QJsonObject{{"name", "foo"}, {"arguments", "{}"}}}}}}}}}}, &error));
        QCOMPARE(session.messages(), before);
    }

    void providerStoreProtectsKeyAndImportsFfui()
    {
        QTemporaryDir directory; AgentProvider provider{"id", "DeepSeek", "https://api.deepseek.com", "secret-not-for-json", "deepseek-flash", "high"};
        QString error; QVERIFY(provider.validate(&error)); QCOMPARE(provider.completionUrl().toString(), "https://api.deepseek.com/v1/chat/completions");
        auto local = provider; local.baseUrl = "http://localhost:11434/"; QCOMPARE(local.completionUrl().path(), "/v1/chat/completions");
        const auto path = directory.filePath("providers.json"); QVERIFY2(AgentProviderStore::save(path, {provider}, &error), qPrintable(error));
        QFile saved(path); QVERIFY(saved.open(QIODevice::ReadOnly));
#ifdef Q_OS_WIN
        QVERIFY(!saved.readAll().contains(provider.apiKey.toUtf8()));
#endif
        saved.close();
        const auto restored = AgentProviderStore::load(path, &error); QCOMPARE(restored.size(), 1); QCOMPARE(restored[0].apiKey, provider.apiKey); QCOMPARE(restored[0].reasoningEffort, "high");
        const auto ffuiPath = directory.filePath("Settings.json"); QFile ffui(ffuiPath); QVERIFY(ffui.open(QIODevice::WriteOnly));
        ffui.write(QJsonDocument(QJsonObject{{"AgentEndPoint", provider.baseUrl}, {"AgentApiKey", provider.apiKey}, {"AgentModelId", provider.model}, {QStringLiteral("Agent推理级别"), "high"}}).toJson()); ffui.close();
        const auto imported = AgentProviderStore::importFfui(ffuiPath, &error); QCOMPARE(imported.size(), 1); QCOMPARE(imported[0].apiKey, provider.apiKey); QCOMPARE(imported[0].model, "deepseek-flash");
        auto discovered = provider; discovered.baseUrl = "http://localhost:11434"; discovered.model = "thinking-model"; discovered.reasoningEffort = "on";
        discovered.modelCapability = AgentModelCapabilities::fromMetadata(discovered.baseUrl, discovered.model, {{"thinking", QJsonObject{{"values", QJsonArray{true,false}}, {"default", true}}}}).toJson();
        QVERIFY(discovered.validate()); QVERIFY(!discovered.reasoningParameters(&error).isEmpty()); QVERIFY(error.isEmpty());
        QVERIFY(AgentProviderStore::save(path, {discovered}, &error)); QCOMPARE(AgentProviderStore::load(path, &error).first().reasoningEffort, QString("on"));
        discovered.reasoningEffort = "invalid level"; QVERIFY(!discovered.validate());
        provider.baseUrl = "https://example.com/?token=secret"; QVERIFY(!provider.validate());
        provider.baseUrl = "https://user:password@example.com"; QVERIFY(!provider.validate());
        provider.baseUrl = "https://example.com"; provider.apiKey = "bad\r\nheader"; QVERIFY(!provider.validate());
    }
};

QTEST_GUILESS_MAIN(TestAgent)
#include "TestAgent.moc"
