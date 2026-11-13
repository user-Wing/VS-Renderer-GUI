#include "agent/AgentSession.h"
#include "agent/AgentContext.h"
#include <QJsonDocument>
#include <QPointer>
#include <QSet>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <memory>

using namespace vsr;

namespace {
QString imageUrl() { return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+XhS8AAAAASUVORK5CYII="; }
QJsonObject attachment() { return {{"mime","image/png"},{"dataUrl",imageUrl()},{"path","C:/images/local.png"},{"width",1},{"height",1}}; }
QJsonObject imageBlock(const QString &url=imageUrl()) { return {{"type","image_url"},{"image_url",QJsonObject{{"url",url}}}}; }
QJsonObject imageMessage(const QString &caption="Local image",const QString &url=imageUrl()) {
    return {{"role","user"},{"source",QStringLiteral("本地图像")},{"displayContent",caption},{"content",QJsonArray{QJsonObject{{"type","text"},{"text",caption}},imageBlock(url)}}};
}
QByteArray event(const QJsonObject &value) { return "data: "+QJsonDocument(value).toJson(QJsonDocument::Compact)+"\r\n\r\n"; }
QByteArray answer(const QString &text="done") {
    return event({{"choices",QJsonArray{QJsonObject{{"index",0},{"delta",QJsonObject{{"content",text}}},{"finish_reason","stop"}}}}})+"data: [DONE]\r\n\r\n";
}
QByteArray calls(const QStringList &names) {
    QJsonArray tools;for(int i=0;i<names.size();++i)tools.append(QJsonObject{{"index",i},{"id","call_"+QString::number(i)},{"type","function"},{"function",QJsonObject{{"name",names[i]},{"arguments","{}"}}}});
    return event({{"choices",QJsonArray{QJsonObject{{"index",0},{"delta",QJsonObject{{"tool_calls",tools}}},{"finish_reason","tool_calls"}}}}})+"data: [DONE]\r\n\r\n";
}
class Endpoint final : public QTcpServer {
public:
    QList<QJsonObject> requests;std::function<QByteArray(int)> reply;
    Endpoint() {
        connect(this,&QTcpServer::newConnection,this,[this]{while(hasPendingConnections()){
            auto *socket=nextPendingConnection();auto bytes=std::make_shared<QByteArray>();auto parsed=std::make_shared<bool>(false);
            connect(socket,&QTcpSocket::readyRead,this,[this,socket,bytes,parsed]{
                if(*parsed)return;*bytes+=socket->readAll();const auto split=bytes->indexOf("\r\n\r\n");if(split<0)return;int length=0;
                for(const auto &line:bytes->left(split).split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();
                if(bytes->size()<split+4+length)return;*parsed=true;requests.append(QJsonDocument::fromJson(bytes->mid(split+4,length)).object());const auto body=reply?reply(requests.size()):answer();
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }});listen(QHostAddress::LocalHost);
    }
    AgentProvider provider(bool vision=true) const {
        AgentProvider source;source.id="image-test";source.name="Image test";source.baseUrl=QString("http://127.0.0.1:%1/v1").arg(serverPort());source.model="vision-model";
        source.modelCapability={{"endpoint",source.baseUrl},{"model",source.model},{"imageInputSupported",vision}};return source;
    }
};
void configure(AgentSession &session,Endpoint &endpoint) { session.setProvider(endpoint.provider());session.setAccessMode(AgentAccessMode::FullEnvironment); }
QSet<QString> names(const QJsonArray &tools) { QSet<QString> result;for(const auto &value:tools)result.insert(value.toObject().value("function").toObject().value("name").toString());return result; }
bool hasImages(const QJsonArray &messages) { for(const auto &value:messages)if(value.toObject().value("content").isArray())return true;return false; }
}

class TestAgentImageMessages final : public QObject {
    Q_OBJECT
private slots:
    void realRequestAppendsImagesAfterTheCompleteToolGroup() {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn==1?calls({"read_image","following"}):answer("Image received");};AgentSession session;configure(session,endpoint);QList<QString> executed;bool queued=false;
        session.registerTool("read_image","Read image",{},[&](const QJsonObject &){executed.append("image");queued=session.queueToolImageAttachment(attachment());return QJsonObject{{"ok",queued},{"path","C:/images/local.png"}};});session.setToolEnvironmentAccess("read_image");session.setToolImageInputRequired("read_image");
        session.registerTool("following","Following call",{},[&](const QJsonObject &){executed.append("following");return QJsonObject{{"ok",true}};});QSignalSpy done(&session,&AgentSession::finished);session.send("Inspect local image");QTRY_COMPARE(done.count(),1);QVERIFY(queued);QCOMPARE(executed,QStringList({"image","following"}));QCOMPARE(endpoint.requests.size(),2);
        const auto sent=endpoint.requests[1].value("messages").toArray();QCOMPARE(sent.size(),6);QCOMPARE(sent[2].toObject().value("role").toString(),QString("assistant"));QCOMPARE(sent[3].toObject().value("tool_call_id").toString(),QString("call_0"));QCOMPARE(sent[4].toObject().value("tool_call_id").toString(),QString("call_1"));const auto image=sent[5].toObject();QCOMPARE(image.value("role").toString(),QString("user"));QVERIFY(!image.contains("source"));QVERIFY(!image.contains("displayContent"));const auto blocks=image.value("content").toArray();QCOMPARE(blocks.size(),2);QCOMPARE(blocks[1],QJsonValue(imageBlock()));QVERIFY(blocks[0].toObject().value("text").toString().contains("1 × 1"));
        const auto saved=session.messages()[4].toObject();QCOMPARE(saved.value("source").toString(),QStringLiteral("本地图像"));QVERIFY(!saved.value("displayContent").toString().contains("base64"));AgentSession restored;QString error;QVERIFY2(restored.importConversation(session.exportConversation(),&error),qPrintable(error));QCOMPARE(restored.messages(),session.messages());QCOMPARE(restored.accessMode(),AgentAccessMode::SoftwareOnly);
    }

    void groupLimitsAndInvalidAttachmentsDoNotConsumeSlots() {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn==1?calls({"images"}):answer();};AgentSession session;configure(session,endpoint);QList<bool> results;
        session.registerTool("images","Queue images",{},[&](const QJsonObject &){
            auto invalid=attachment();invalid["mime"]="image/jpeg";results.append(session.queueToolImageAttachment(invalid));invalid=attachment();invalid["width"]=0;results.append(session.queueToolImageAttachment(invalid));invalid=attachment();invalid["dataUrl"]="data:image/png;base64,"+QString(512*1024,'A');results.append(session.queueToolImageAttachment(invalid));
            for(int i=0;i<5;++i)results.append(session.queueToolImageAttachment(attachment()));return QJsonObject{{"ok",true}};
        });QSignalSpy done(&session,&AgentSession::finished);session.send("Read images");QTRY_COMPARE(done.count(),1);QCOMPARE(results,QList<bool>({false,false,false,true,true,true,true,false}));const auto blocks=endpoint.requests[1].value("messages").toArray().last().toObject().value("content").toArray();QCOMPARE(blocks.size(),5);QVERIFY(!session.queueToolImageAttachment(attachment()));
    }

    void imageQueuesResetBetweenToolGroups() {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn<3?calls({"image"}):answer();};AgentSession session;configure(session,endpoint);int accepted=0;session.registerTool("image","Image",{},[&](const QJsonObject &){accepted+=session.queueToolImageAttachment(attachment());return QJsonObject{{"ok",true}};});QSignalSpy done(&session,&AgentSession::finished);session.send("Two image rounds");QTRY_COMPARE(done.count(),1);QCOMPARE(accepted,2);QCOMPARE(endpoint.requests.size(),3);int images=0;for(const auto &message:endpoint.requests[2].value("messages").toArray())if(message.toObject().value("content").isArray()){++images;QCOMPARE(message.toObject().value("content").toArray().size(),2);}QCOMPARE(images,2);
    }

    void cancellationDropsUncommittedImagesAndLateCallbacks() {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn==1?calls({"slow_image","following"}):answer();};AgentSession session;configure(session,endpoint);AgentSession::Completion pending;bool accepted=false;int following=0;
        session.registerAsyncTool("slow_image","Delayed image",{},[&](const QJsonObject &,AgentSession::Completion done){accepted=session.queueToolImageAttachment(attachment());pending=std::move(done);});session.registerTool("following","Following",{},[&](const QJsonObject &){++following;return QJsonObject{{"ok",true}};});session.send("Read then wait");QTRY_VERIFY(bool(pending));QVERIFY(accepted);session.cancel();pending({{"ok",true}});QTest::qWait(20);QVERIFY(!hasImages(session.messages()));QCOMPARE(following,0);AgentSession imported;QString error;QVERIFY2(imported.importConversation(session.exportConversation(),&error),qPrintable(error));
        session.clear();QVERIFY(session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{}}}));QSignalSpy done(&session,&AgentSession::finished);session.send("New unrelated task");QTRY_COMPARE(done.count(),1);QVERIFY(!hasImages(endpoint.requests.last().value("messages").toArray()));
    }

    void softwareAndUnverifiedRoutesCannotQueueImages() {
        Endpoint endpoint;endpoint.reply=[](int turn){return turn%2?calls({"probe"}):answer();};AgentSession session;session.setProvider(endpoint.provider());QList<bool> attempts;
        session.registerTool("probe","Probe queue",{},[&](const QJsonObject &){attempts.append(session.queueToolImageAttachment(attachment()));return QJsonObject{{"ok",true}};});QSignalSpy done(&session,&AgentSession::finished);session.send("Software mode");QTRY_COMPARE(done.count(),1);session.setAccessMode(AgentAccessMode::FullEnvironment);session.setProvider(endpoint.provider(false));session.send("No vision");QTRY_COMPARE(done.count(),2);QCOMPARE(attempts,QList<bool>({false,false}));QVERIFY(!hasImages(session.messages()));
    }

    void imageToolsRequireTheBoundVisionCapabilityWithoutChangingPreferences() {
        Endpoint endpoint;AgentSession session;session.setAccessMode(AgentAccessMode::FullEnvironment);int runs=0;session.registerTool("read_image","Image",{},[&](const QJsonObject &){++runs;return QJsonObject{{"ok",true}};});session.setToolImageInputRequired("read_image");session.setToolEnvironmentAccess("read_image");
        auto provider=endpoint.provider();provider.modelCapability={};session.setProvider(provider);QVERIFY(!names(session.tools()).contains("read_image"));QVERIFY(!session.executeTool("read_image",{}).value("ok").toBool());provider=endpoint.provider();provider.modelCapability["endpoint"]="http://other.invalid/v1";session.setProvider(provider);QVERIFY(!session.toolAvailable("read_image"));provider=endpoint.provider();provider.modelCapability["model"]="another-model";session.setProvider(provider);QVERIFY(!session.toolAvailable("read_image"));
        session.setProvider(endpoint.provider());QVERIFY(session.toolAvailable("read_image"));QVERIFY(session.executeTool("read_image",{}).value("ok").toBool());QCOMPARE(runs,1);session.setToolEnabled("read_image",false);session.setProvider(endpoint.provider(false));session.setProvider(endpoint.provider());QVERIFY(!session.toolEnabled("read_image"));AgentSession copied;session.copyToolsTo(&copied);copied.setAccessMode(AgentAccessMode::FullEnvironment);copied.setProvider(endpoint.provider(false));copied.setToolEnabled("read_image",true);QVERIFY(!copied.toolAvailable("read_image"));
    }

    void importedImagesRoundTripAndRejectMalformedBlocks() {
        AgentSession session;const QJsonArray original{imageMessage()};QString error;QVERIFY2(session.importConversation({{"schemaVersion",1},{"messages",original}},&error),qPrintable(error));QCOMPARE(session.messages(),original);QCOMPARE(session.exportConversation().value("messages").toArray(),original);
        const QStringList badUrls{"https://example.com/image.png","data:image/gif;base64,AA==","data:image/svg+xml;base64,AA==","data:image/png;base64,not base64","data:image/png;base64,AB==","data:image/png;base64,AA==\n","data:image/png;base64,"+QString(512*1024,'A')};
        for(const auto &url:badUrls){QVERIFY(!session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{imageMessage("Bad",url)}}},&error));QCOMPARE(session.messages(),original);}
        for(const auto &role:{"assistant","tool","system","developer"}){auto message=imageMessage();message["role"]=role;QVERIFY(!session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{message}}},&error));}
        auto message=imageMessage();auto blocks=message.value("content").toArray();for(int i=0;i<4;++i)blocks.append(imageBlock());message["content"]=blocks;QVERIFY(!session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{message}}},&error));
        blocks=QJsonArray{QJsonObject{{"type","unknown"}},imageBlock()};message["content"]=blocks;QVERIFY(!session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{message}}},&error));QCOMPARE(session.messages(),original);
        const QJsonObject call{{"id","a"},{"type","function"},{"function",QJsonObject{{"name","read_image"},{"arguments","{}"}}}};QVERIFY(!session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{QJsonObject{{"role","assistant"},{"content",QJsonValue::Null},{"tool_calls",QJsonArray{call}}},imageMessage(),QJsonObject{{"role","tool"},{"tool_call_id","a"},{"content","{}"}}}}},&error));
    }

    void base64DoesNotInflateTextTokenEstimates() {
        AgentContext context;const QJsonArray tiny{imageMessage()};const auto largeUrl="data:image/png;base64,"+QString::fromLatin1(QByteArray(300000,'x').toBase64());const QJsonArray large{imageMessage("Local image",largeUrl)};QCOMPARE(context.estimateRequest(tiny,"",{}),context.estimateRequest(large,"",{}));const auto estimate=context.estimateRequest(large,"",{});QVERIFY(estimate>=4096 && estimate<5000);QVERIFY(context.usage(estimate,32768).value("estimated").toBool());
    }

    void unsupportedSourceRefusesImportedImageHistoryWithoutNetworkRequest() {
        Endpoint endpoint;AgentSession session;session.setProvider(endpoint.provider(false));QVERIFY(session.importConversation({{"schemaVersion",1},{"messages",QJsonArray{imageMessage()}}}));QSignalSpy errors(&session,&AgentSession::errorOccurred);session.send("Continue");QCOMPARE(errors.count(),1);QVERIFY(errors.first().first().toString().contains(QStringLiteral("图像输入")));QVERIFY(endpoint.requests.isEmpty());QCOMPARE(session.messages().first().toObject(),imageMessage());
        session.setProvider(endpoint.provider());QSignalSpy done(&session,&AgentSession::finished);session.send("Use visual source");QTRY_COMPARE(done.count(),1);QCOMPARE(endpoint.requests.size(),1);QVERIFY(hasImages(endpoint.requests[0].value("messages").toArray()));
    }

    void compactionUsesImageMetadataAndKeepsRecentImageBlocks() {
        Endpoint endpoint;endpoint.reply=[](int turn){return answer(turn==1?"FACTUAL_SUMMARY":"Follow-up result");};AgentSession session;configure(session,endpoint);
        const QJsonArray original{QJsonObject{{"role","user"},{"content","OLD_FIRST"}},imageMessage("OLD_IMAGE_METADATA"),QJsonObject{{"role","assistant"},{"content","Old result"}},QJsonObject{{"role","user"},{"content","Second human task"}},QJsonObject{{"role","assistant"},{"content","Second result"}},QJsonObject{{"role","user"},{"content","Recent human task"}},imageMessage("RECENT_IMAGE_METADATA"),QJsonObject{{"role","assistant"},{"content","Recent result"}}};QString error;QVERIFY2(session.importConversation({{"schemaVersion",1},{"messages",original}},&error),qPrintable(error));AgentContext context;QCOMPARE(context.compressionBoundary(original),3);
        QSignalSpy done(&session,&AgentSession::finished);session.compactContext();QTRY_COMPARE(done.count(),1);const auto summary=QJsonDocument(endpoint.requests[0]).toJson();QVERIFY(summary.contains("OLD_IMAGE_METADATA"));QVERIFY(!summary.contains("base64"));QVERIFY(!summary.contains("iVBOR"));QVERIFY(!hasImages(endpoint.requests[0].value("messages").toArray()));QCOMPARE(session.messages(),original);
        session.send("Continue recent image task");QTRY_COMPARE(done.count(),2);const auto sent=endpoint.requests[1].value("messages").toArray();QVERIFY(hasImages(sent));QVERIFY(QJsonDocument(sent).toJson().contains("RECENT_IMAGE_METADATA"));QVERIFY(!QJsonDocument(sent).toJson().contains("OLD_IMAGE_METADATA"));QVERIFY(QJsonDocument(session.exportConversation()).toJson().contains("OLD_IMAGE_METADATA"));
    }
};
QTEST_GUILESS_MAIN(TestAgentImageMessages)
#include "TestAgentImageMessages.moc"
