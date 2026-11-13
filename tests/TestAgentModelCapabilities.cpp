#include "agent/AgentModelCapabilities.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <memory>

using namespace vsr;

namespace {
QJsonObject model(const QString &id, const QJsonArray &levels, const QString &defaultLevel)
{
    return {{"id", id}, {"effort", QJsonObject{{"supported_levels", levels}, {"default_level", defaultLevel}}},
        {"context_window", 1048576}, {"max_output_tokens", 393216}};
}

class MetadataEndpoint final : public QTcpServer {
public:
    struct Request { QByteArray path, method, headers; QJsonObject body; QPointer<QTcpSocket> socket; };
    QList<Request> requests;
    QJsonObject document{{"data", QJsonArray{model("custom", {"low", "high"}, "high")}}};
    QByteArray serverName;
    int status = 200;
    bool hold = false;

    MetadataEndpoint()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection(); auto bytes = std::make_shared<QByteArray>(); auto parsed = std::make_shared<bool>(false);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes, parsed] {
                    if (*parsed) return;
                    *bytes += socket->readAll(); const int split = bytes->indexOf("\r\n\r\n"); if (split < 0) return;
                    const auto headers = bytes->left(split); int length = 0;
                    for (const auto &line : headers.split('\n')) if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                    if (bytes->size() < split + 4 + length) return;
                    *parsed = true; const auto first = headers.left(headers.indexOf("\r\n")).split(' ');
                    requests.append({first.value(1), first.value(0), headers, QJsonDocument::fromJson(bytes->mid(split + 4, length)).object(), socket});
                    if (!hold) release(requests.size() - 1, document, status);
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        listen(QHostAddress::LocalHost);
    }

    QString url() const { return QString("http://127.0.0.1:%1/v1").arg(serverPort()); }
    void release(int index, const QJsonObject &data, int code = 200)
    {
        if (index < 0 || index >= requests.size() || !requests[index].socket) return;
        const auto bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
        const QByteArray header = "HTTP/1.1 " + QByteArray::number(code) + " Response\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(bytes.size()) +
            "\r\nConnection: close\r\n" + (serverName.isEmpty() ? QByteArray() : "Server: " + serverName + "\r\n") + "\r\n";
        requests[index].socket->write(header + bytes); requests[index].socket->disconnectFromHost();
    }
};
}

class TestAgentModelCapabilities final : public QObject {
    Q_OBJECT
private slots:
    void imageInputRequiresExplicitMetadata()
    {
        QVERIFY(!AgentModelCapabilities::conservative("https://api.openai.com/v1","gpt-4o").imageInputSupported);
        const QList<QJsonObject> declared{
            {{"input_modalities",QJsonArray{"text","image"}}},{{"inputModalities",QJsonArray{"image"}}},
            {{"modalities",QJsonObject{{"input",QJsonArray{"image","text"}}}}},
            {{"capabilities",QJsonObject{{"vision",true}}}},{{"capabilities",QJsonArray{"completion","vision"}}},
            {{"architecture",QJsonObject{{"input_modalities",QJsonArray{"text","image"}}}}}
        };
        for(const auto &metadata:declared){const auto cap=AgentModelCapabilities::fromMetadata("https://compatible.example/v1","custom",metadata);QVERIFY(cap.imageInputSupported);QCOMPARE(cap.imageInputSource,QString("metadata"));const auto restored=AgentModelCapability::fromJson(cap.toJson());QVERIFY(restored.imageInputSupported);QCOMPARE(restored.imageInputSource,QString("metadata"));}
        for(const auto &metadata:QList<QJsonObject>{{},{{"input_modalities",QJsonArray{"text"}}},{{"capabilities",QJsonObject{{"vision","true"}}}},{{"id","other"},{"input_modalities",QJsonArray{"image"}}},{{"input_modalities",QJsonArray{"image"}},{"capabilities",QJsonObject{{"vision",false}}}}})
            QVERIFY(!AgentModelCapabilities::fromMetadata("https://compatible.example/v1","custom",metadata).imageInputSupported);
    }

    void imageMetadataRefreshAndOllamaShowPreserveReasoning()
    {
        MetadataEndpoint endpoint;auto entry=model("custom",{"high"},"high");entry.insert("input_modalities",QJsonArray{"text","image"});endpoint.document={{"data",QJsonArray{entry}}};AgentModelCapabilities resolver;QSignalSpy ready(&resolver,&AgentModelCapabilities::resolved);
        resolver.resolve(endpoint.url(),{},"custom");QTRY_COMPARE(ready.count(),1);QVERIFY(ready.last()[2].toJsonObject().value("imageInputSupported").toBool());
        entry.insert("input_modalities",QJsonArray{"text"});endpoint.document={{"data",QJsonArray{entry}}};resolver.resolve(endpoint.url(),{},"custom",true);QTRY_COMPARE(ready.count(),2);QVERIFY(!ready.last()[2].toJsonObject().value("imageInputSupported").toBool());
        MetadataEndpoint ollama;ollama.hold=true;ollama.serverName="Ollama";AgentModelCapabilities local;QSignalSpy localReady(&local,&AgentModelCapabilities::resolved);local.resolve(ollama.url(),{},"custom");QTRY_COMPARE(ollama.requests.size(),1);ollama.release(0,{{"data",QJsonArray{model("custom",{"low","high"},"high")}}});QTRY_COMPARE(ollama.requests.size(),2);QCOMPARE(ollama.requests.last().path,QByteArray("/api/show"));ollama.release(1,{{"capabilities",QJsonArray{"completion","vision"}}});QTRY_COMPARE(localReady.count(),1);const auto cap=localReady.first()[2].toJsonObject();QVERIFY(cap.value("imageInputSupported").toBool());QCOMPARE(cap.value("levels").toArray(),QJsonArray({"low","high"}));
    }

    void deepseekUsesActualEffortMetadataAndThinkingToggle()
    {
        const auto capability = AgentModelCapabilities::fromMetadata("https://api.deepseek.com/v1", "deepseek-flash", model("deepseek-flash", {"low", "high", "max"}, "high"));
        QCOMPARE(capability.source, QString("metadata")); QCOMPARE(capability.levels, QStringList({"low", "high", "max"}));
        QCOMPARE(capability.defaultLevel, QString("high")); QVERIFY(capability.supportsOff);
        QCOMPARE(capability.contextWindow, qint64(1048576)); QCOMPARE(capability.maxOutputTokens, qint64(393216));
        QString error;
        QVERIFY(capability.requestOptions("auto", &error).isEmpty()); QVERIFY(error.isEmpty());
        QCOMPARE(capability.requestOptions("off"), QJsonObject({{"thinking", QJsonObject{{"type", "disabled"}}}}));
        QCOMPARE(capability.requestOptions("max"), QJsonObject({{"thinking", QJsonObject{{"type", "enabled"}}}, {"reasoning_effort", "max"}}));
        QVERIFY(capability.requestOptions("medium", &error).isEmpty()); QVERIFY(!error.isEmpty());
        const auto restored = AgentModelCapability::fromJson(capability.toJson());
        QCOMPARE(restored.requestOptions("high"), capability.requestOptions("high"));
        QCOMPARE(capability.toJson().value("reasoningLevels").toArray().size(), 5);
    }

    void officialRulesAreScopedToExactProviderAndModel()
    {
        const auto deepseek = AgentModelCapabilities::conservative("https://api.deepseek.com", "deepseek-v4-pro");
        QCOMPARE(deepseek.source, QString("official-rule")); QVERIFY(deepseek.supportsOff);
        const auto unknown = AgentModelCapabilities::conservative("https://api.deepseek.com.example.org", "deepseek-flash");
        QCOMPARE(unknown.source, QString("unknown")); QVERIFY(unknown.levels.isEmpty()); QVERIFY(!unknown.supportsOff);
        QVERIFY(unknown.requestOptions("auto").isEmpty());
        QString error; QVERIFY(unknown.requestOptions("high", &error).isEmpty()); QVERIFY(!error.isEmpty());
        QVERIFY(AgentModelCapabilities::conservative("https://api.deepseek.com", "future-model").levels.isEmpty());
        QVERIFY(AgentModelCapabilities::conservative("https://user:password@api.deepseek.com", "deepseek-flash").levels.isEmpty());
        QVERIFY(AgentModelCapabilities::conservative("https://compatible.example/v1", "gpt-5.2").levels.isEmpty());
        const auto old = AgentModelCapabilities::conservative("https://api.openai.com/v1", "gpt-5-2025-08-07");
        QVERIFY(old.levels.contains("minimal")); QVERIFY(!old.supportsOff);
        const auto newer = AgentModelCapabilities::conservative("https://api.openai.com", "gpt-5.2");
        QVERIFY(newer.levels.contains("xhigh")); QCOMPARE(newer.requestOptions("off").value("reasoning_effort").toString(), QString("none"));
        const auto astra = AgentModelCapabilities::conservative("https://api.openai.com", "gpt-6-astra");
        QVERIFY(!astra.chatToolsSupported); QVERIFY(!astra.supportsOff);
        const auto luna = AgentModelCapabilities::conservative("https://api.openai.com", "gpt-6-luna");
        QVERIFY(luna.chatToolsRequiresOff); QVERIFY(luna.supportsOff);
    }

    void metadataOverridesFallbackWithoutAdvertisingUnsupportedLevels()
    {
        auto metadata = model("gpt-5.2", {"low", "high"}, "low");
        const auto result = AgentModelCapabilities::fromMetadata("https://api.openai.com", "gpt-5.2", metadata);
        QCOMPARE(result.source, QString("metadata")); QCOMPARE(result.defaultLevel, QString("low"));
        QCOMPARE(result.levels, QStringList({"low", "high"})); QVERIFY(!result.supportsOff);
        const QJsonObject generic{{"id", "custom"}, {"capabilities", QJsonObject{{"reasoning_effort", QJsonObject{{"enum", QJsonArray{"none", "low"}}, {"default", "none"}}}}}};
        const auto discovered = AgentModelCapabilities::fromMetadata("https://compatible.example/v1", "custom", generic);
        QCOMPARE(discovered.levels, QStringList({"low"})); QVERIFY(discovered.supportsOff); QCOMPARE(discovered.defaultLevel, QString("off"));
        const auto disabled = AgentModelCapabilities::fromMetadata("https://api.openai.com", "gpt-5.2", {{"reasoning", false}});
        QVERIFY(disabled.levels.isEmpty()); QVERIFY(!disabled.supportsOff); QCOMPARE(disabled.source, QString("metadata"));
        QVERIFY(AgentModelCapabilities::fromMetadata("https://compatible.example", "different-model", generic).levels.isEmpty());
    }

    void discoversModelsCachesAndRefreshesMetadata()
    {
        MetadataEndpoint endpoint; endpoint.document = {{"data", QJsonArray{model("custom", {"low", "high"}, "high"), QJsonObject{{"id", "plain"}}}}};
        AgentModelCapabilities resolver; QSignalSpy resolved(&resolver, &AgentModelCapabilities::resolved);
        resolver.resolve(endpoint.url(), "fake-test-key", "custom"); QTRY_COMPARE(resolved.count(), 1);
        QCOMPARE(endpoint.requests.size(), 1); QCOMPARE(endpoint.requests.first().method, QByteArray("GET")); QCOMPARE(endpoint.requests.first().path, QByteArray("/v1/models"));
        QVERIFY(endpoint.requests.first().headers.contains("Authorization: Bearer fake-test-key"));
        auto capability = resolved.last()[2].toJsonObject();
        QCOMPARE(capability.value("source").toString(), QString("metadata")); QCOMPARE(capability.value("models").toArray(), QJsonArray({"custom", "plain"}));
        resolver.resolve(endpoint.url(), "fake-test-key", "custom"); QTRY_COMPARE(resolved.count(), 2); QCOMPARE(endpoint.requests.size(), 1);
        endpoint.document = {{"data", QJsonArray{model("custom", {"max"}, "max")}}};
        resolver.resolve(endpoint.url() + "/chat/completions", "fake-test-key", "custom", true); QTRY_COMPARE(resolved.count(), 3); QCOMPARE(endpoint.requests.size(), 2);
        capability = resolved.last()[2].toJsonObject(); QCOMPARE(capability.value("levels").toArray(), QJsonArray({"max"}));
        QCOMPARE(resolver.cached(endpoint.url(), "custom").value("levels"), capability.value("levels"));
        resolver.resolve(endpoint.url(), "", "unlisted"); QTRY_COMPARE(resolved.count(), 4);
        QCOMPARE(resolved.last()[2].toJsonObject().value("reasoningLevels").toArray().size(), 1);
    }

    void ollamaShowDiscoversBooleanAndNamedThinkingWithoutGeneration()
    {
        MetadataEndpoint endpoint; endpoint.serverName = "Ollama";
        endpoint.document = {{"data", QJsonArray{QJsonObject{{"id", "custom"}}}}}; endpoint.hold = true;
        AgentModelCapabilities resolver; QSignalSpy resolved(&resolver, &AgentModelCapabilities::resolved);
        resolver.resolve(endpoint.url(), "", "custom"); QTRY_COMPARE(endpoint.requests.size(), 1);
        endpoint.release(0, endpoint.document); QTRY_COMPARE(endpoint.requests.size(), 2);
        QCOMPARE(endpoint.requests[1].path, QByteArray("/api/show")); QCOMPARE(endpoint.requests[1].method, QByteArray("POST"));
        QCOMPARE(endpoint.requests[1].body.value("model").toString(), QString("custom"));
        endpoint.release(1, {{"thinking", QJsonObject{{"values", QJsonArray{false, true}}, {"default", true}}}}); QTRY_COMPARE(resolved.count(), 1);
        auto capability = AgentModelCapability::fromJson(resolved.last()[2].toJsonObject());
        QVERIFY(capability.supportsOff); QCOMPARE(capability.levels, QStringList({"on"})); QCOMPARE(capability.defaultLevel, QString("on"));
        QCOMPARE(capability.requestOptions("off").value("reasoning_effort").toString(), QString("none"));
        QCOMPARE(capability.requestOptions("on").value("reasoning_effort").toString(), QString("high"));
        const auto named = AgentModelCapabilities::fromMetadata(endpoint.url(), "custom", {{"thinking", QJsonObject{{"values", QJsonArray{"low", "medium", "high"}}, {"default", "medium"}}}});
        QCOMPARE(named.levels, QStringList({"low", "medium", "high"})); QVERIFY(!named.supportsOff);
        QCOMPARE(named.requestOptions("low").value("reasoning_effort").toString(), QString("low"));
    }

    void errorsRetainKnownChoicesAndUnknownEndpointsStayDefault()
    {
        MetadataEndpoint endpoint; AgentModelCapabilities resolver; QSignalSpy resolved(&resolver, &AgentModelCapabilities::resolved);
        resolver.resolve(endpoint.url(), "", "custom"); QTRY_COMPARE(resolved.count(), 1);
        endpoint.status = 503; endpoint.document = {{"error", "Do not expose server diagnostics"}};
        resolver.resolve(endpoint.url(), "", "custom", true); QTRY_COMPARE(resolved.count(), 2);
        const auto stale = resolved.last()[2].toJsonObject(); QCOMPARE(stale.value("status").toString(), QString("stale"));
        QCOMPARE(stale.value("levels").toArray(), QJsonArray({"low", "high"}));
        QVERIFY(!QJsonDocument(stale).toJson().contains("server diagnostics"));
        resolver.resolve(endpoint.url(), "", "unknown", true); QTRY_COMPARE(resolved.count(), 3);
        const auto unknown = resolved.last()[2].toJsonObject(); QCOMPARE(unknown.value("status").toString(), QString("error"));
        QCOMPARE(unknown.value("reasoningLevels").toArray().size(), 1);
    }

    void refreshDoesNotPublishOlderInFlightMetadata()
    {
        MetadataEndpoint endpoint; endpoint.hold = true;
        AgentModelCapabilities resolver; QSignalSpy resolved(&resolver, &AgentModelCapabilities::resolved);
        resolver.resolve(endpoint.url(), "", "custom"); QTRY_COMPARE(endpoint.requests.size(), 1);
        resolver.resolve(endpoint.url(), "", "custom", true); QTRY_COMPARE(endpoint.requests.size(), 2);
        endpoint.release(1, {{"data", QJsonArray{model("custom", {"max"}, "max")}}}); QTRY_COMPARE(resolved.count(), 1);
        endpoint.release(0, {{"data", QJsonArray{model("custom", {"low"}, "low")}}}); QTest::qWait(30);
        QCOMPARE(resolved.count(), 1); QCOMPARE(resolver.cached(endpoint.url(), "custom").value("levels").toArray(), QJsonArray({"max"}));
    }

    void requestOptionsRejectStaleIdentityAndPayloadOverrides()
    {
        auto capability = AgentModelCapabilities::fromMetadata("https://compatible.example/v1", "custom", model("custom", {"high"}, "high")).toJson();
        QString error;
        QVERIFY(!AgentModelCapabilities::requestOptions("https://compatible.example/v1", "custom", "high", capability, &error).isEmpty());
        QVERIFY(AgentModelCapabilities::requestOptions("https://compatible.example/v1", "changed-model", "high", capability, &error).isEmpty()); QVERIFY(!error.isEmpty());
        auto options = capability.value("optionsByLevel").toObject(); options.insert("high", QJsonObject{{"reasoning_effort", "high"}, {"model", "injected"}, {"messages", QJsonArray{}}}); capability.insert("optionsByLevel", options);
        QVERIFY(AgentModelCapabilities::requestOptions("https://compatible.example/v1", "custom", "high", capability, &error).isEmpty()); QVERIFY(!error.isEmpty());
        QVERIFY(AgentModelCapabilities::requestOptions("https://compatible.example/v1", "custom", "auto", capability, &error).isEmpty()); QVERIFY(error.isEmpty());
        auto conflicting = AgentModelCapabilities::conservative("https://api.deepseek.com", "deepseek-flash").toJson();
        auto fields = conflicting.value("optionsByLevel").toObject();
        fields.insert("off", QJsonObject{{"thinking", QJsonObject{{"type", "enabled"}}}, {"reasoning_effort", "none"}}); conflicting.insert("optionsByLevel", fields);
        QVERIFY(AgentModelCapabilities::requestOptions("https://api.deepseek.com", "deepseek-flash", "off", conflicting, &error).isEmpty()); QVERIFY(!error.isEmpty());
    }

    void destructionCancelsBothMetadataRequestStages()
    {
        for (const bool showStage : {false, true}) {
            MetadataEndpoint endpoint; endpoint.hold = true;
            if (showStage) { endpoint.serverName = "Ollama"; endpoint.document = {{"data", QJsonArray{QJsonObject{{"id", "custom"}}}}}; }
            auto *resolver = new AgentModelCapabilities;
            QSignalSpy resolved(resolver, &AgentModelCapabilities::resolved);
            resolver->resolve(endpoint.url(), "", "custom"); QTRY_COMPARE(endpoint.requests.size(), 1);
            if (showStage) { endpoint.release(0, endpoint.document); QTRY_COMPARE(endpoint.requests.size(), 2); }
            const auto socket = endpoint.requests.last().socket;
            QPointer<AgentModelCapabilities> guard(resolver); delete resolver;
            QVERIFY(guard.isNull());
            QTRY_VERIFY(!socket || socket->state() == QAbstractSocket::UnconnectedState);
            QCOMPARE(resolved.count(), 0);
            QTest::qWait(30); QCOMPARE(resolved.count(), 0);
        }
    }

    void clearingCacheInvalidatesQueuedInvalidEndpointResult()
    {
        AgentModelCapabilities resolver; QSignalSpy resolved(&resolver, &AgentModelCapabilities::resolved);
        resolver.resolve("file:///invalid", "", "custom");
        resolver.clearCache(); QTest::qWait(30);
        QCOMPARE(resolved.count(), 0);
        QCOMPARE(resolver.cached("file:///invalid", "custom").value("status").toString(), QString("unknown"));
    }
};

QTEST_GUILESS_MAIN(TestAgentModelCapabilities)
#include "TestAgentModelCapabilities.moc"
