#include "agent/AgentLocalImages.h"
#include "agent/AgentSession.h"
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using namespace vsr;
namespace {
class Endpoint final : public QTcpServer {
public:
    QJsonArray calls;
    QList<QJsonObject> requests;
    Endpoint() {
        listen(QHostAddress::LocalHost);connect(this,&QTcpServer::newConnection,this,[this]{while(hasPendingConnections()){
            auto *socket=nextPendingConnection();auto bytes=std::make_shared<QByteArray>();auto parsed=std::make_shared<bool>(false);
            connect(socket,&QTcpSocket::readyRead,this,[this,socket,bytes,parsed]{
                if(*parsed)return;*bytes+=socket->readAll();const int split=bytes->indexOf("\r\n\r\n");if(split<0)return;int length=0;
                for(const auto &line:bytes->left(split).split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();if(bytes->size()<split+4+length)return;
                *parsed=true;requests.append(QJsonDocument::fromJson(bytes->mid(split+4,length)).object());const bool first=requests.size()==1;
                const QJsonObject delta=first?QJsonObject{{"tool_calls",calls}}:QJsonObject{{"content","Mock completed the image round."}};
                const auto event=QJsonDocument(QJsonObject{{"choices",QJsonArray{QJsonObject{{"delta",delta},{"finish_reason",first?"tool_calls":"stop"}}}}}).toJson(QJsonDocument::Compact);
                const auto body=QByteArray("data: ")+event+"\n\ndata: [DONE]\n\n";socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }});
    }
    QString url() const { return QString("http://127.0.0.1:%1/v1").arg(serverPort()); }
    void add(const QString &name,const QString &path) { const int index=calls.size();calls.append(QJsonObject{{"index",index},{"id",QString("call-%1").arg(index)},{"type","function"},{"function",QJsonObject{{"name",name},{"arguments",QString::fromUtf8(QJsonDocument(QJsonObject{{"path",path}}).toJson(QJsonDocument::Compact))}}}}); }
};
void prepare(AgentSession &session,const QString &directory,const QString &url)
{
    session.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(session.setWorkingDirectory(directory));AgentProvider provider;provider.id="image-source";provider.name="Image test";provider.baseUrl=url;provider.model="explicit-vision";provider.modelCapability={{"endpoint",url},{"model",provider.model},{"imageInputSupported",true}};session.setProvider(provider);
}
QJsonObject run(AgentSession &session,const QString &name,const QJsonObject &arguments)
{
    struct State { bool done=false;QJsonObject result{{"ok",false},{"error","Image test timeout"}}; };auto state=std::make_shared<State>();QEventLoop loop;const QPointer<QEventLoop> guard(&loop);
    session.executeToolAsync(name,arguments,[state,guard](QJsonObject result){state->done=true;state->result=result;if(guard)guard->quit();});if(!state->done){QTimer::singleShot(10000,&loop,&QEventLoop::quit);loop.exec();}return state->result;
}
bool hasTool(const AgentSession &session,const QString &name) { for(const auto &value:session.tools())if(value.toObject().value("function").toObject().value("name")==QJsonValue(name))return true;return false; }
QList<QString> images(const QJsonArray &messages)
{
    QList<QString> found;for(const auto &value:messages)for(const auto &item:value.toObject().value("content").toArray()){const auto block=item.toObject();if(block.value("type")==QJsonValue("image_url"))found.append(block.value("image_url").toObject().value("url").toString());}return found;
}
QByteArray fileBytes(const QString &path) { QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{}; }
QByteArray imageBytes(const QString &url) { return QByteArray::fromBase64(url.mid(url.indexOf(',')+1).toLatin1()); }
QJsonObject toolResult(const QJsonArray &history,int index=0)
{
    for(const auto &value:history){const auto message=value.toObject();if(message.value("role")==QJsonValue("tool") && index--==0)return QJsonDocument::fromJson(message.value("content").toString().toUtf8()).object();}return {};
}
}

class TestAgentLocalImages final : public QObject {
    Q_OBJECT
private slots:
    void explicitImageRouteAndEnvironmentGates() {
        QTemporaryDir dir;AgentSession session;Endpoint endpoint;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();QVERIFY(hasTool(session,"read_image"));QVERIFY(hasTool(session,"view_image"));
        session.setAccessMode(AgentAccessMode::SoftwareOnly);QVERIFY(!hasTool(session,"read_image"));QVERIFY(!run(session,"view_image",{{"path","anything.png"}}).value("ok").toBool());session.setAccessMode(AgentAccessMode::FullEnvironment);
        auto provider=session.provider();provider.modelCapability.remove("imageInputSupported");session.setProvider(provider);QVERIFY(!hasTool(session,"read_image"));provider.model="gpt-4o";session.setProvider(provider);QVERIFY(!hasTool(session,"view_image"));provider.modelCapability={{"endpoint",provider.baseUrl},{"model",provider.model},{"imageInputSupported",true}};session.setProvider(provider);QVERIFY(hasTool(session,"read_image"));
        provider.modelCapability.insert("model","different");session.setProvider(provider);QVERIFY(!hasTool(session,"read_image"));AgentSession child;session.copyToolsTo(&child);QVERIFY(!hasTool(child,"read_image"));
    }

    void pngWithoutExtensionUsesRealImageBlocksAfterCompleteToolGroup() {
        QTemporaryDir dir;const auto path=dir.filePath("extensionless");QImage source(2048,1024,QImage::Format_ARGB32);source.fill(QColor(40,110,170,160));QVERIFY(source.save(path,"PNG"));const auto original=fileBytes(path);
        Endpoint endpoint;endpoint.add("read_image","extensionless");endpoint.calls.append(QJsonObject{{"index",1},{"id","call-1"},{"type","function"},{"function",QJsonObject{{"name","get_state"},{"arguments","{}"}}}});
        AgentSession session;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();session.registerTool("get_state","State",{{"type","object"},{"properties",QJsonObject{}}},[](const QJsonObject &){return QJsonObject{{"ok",true}};});QSignalSpy finished(&session,&AgentSession::finished);session.send("Inspect the image.");QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,10000);QCOMPARE(endpoint.requests.size(),2);
        const auto requestMessages=endpoint.requests.last().value("messages").toArray();const auto urls=images(requestMessages);QCOMPARE(urls.size(),1);QVERIFY(urls.first().startsWith("data:image/png;base64,"));QVERIFY(urls.first().toUtf8().size()<=512*1024);const auto attached=QImage::fromData(imageBytes(urls.first()));QCOMPARE(attached.size(),QSize(1024,512));QVERIFY(attached.hasAlphaChannel());
        int tools=0,imageAt=-1,lastTool=-1;for(int i=0;i<requestMessages.size();++i){const auto message=requestMessages[i].toObject();if(message.value("role")==QJsonValue("tool")){++tools;lastTool=i;QVERIFY(!message.value("content").toString().contains("data:image"));}if(!images(QJsonArray{message}).isEmpty())imageAt=i;}QCOMPARE(tools,2);QVERIFY(imageAt>lastTool);
        const auto result=toolResult(session.messages());QVERIFY2(result.value("ok").toBool(),qPrintable(result.value("error").toString()));QVERIFY(result.value("attachment_queued").toBool());QVERIFY(result.value("resized").toBool());QCOMPARE(result.value("original_width").toInt(),2048);QVERIFY(!result.contains("dataUrl"));QCOMPARE(fileBytes(path),original);
        AgentSession restored;QString error;QVERIFY2(restored.importConversation(session.exportConversation(),&error),qPrintable(error));QCOMPARE(images(restored.messages()),images(session.messages()));
    }

    void opaqueTextureFitsBudgetAndAliasUsesCwd() {
        QTemporaryDir dir;QVERIFY(QDir().mkdir(dir.filePath("images")));const auto path=dir.filePath("images/noise.png");QImage source(1024,1024,QImage::Format_RGB32);QRandomGenerator random(17);for(int y=0;y<source.height();++y){auto *row=reinterpret_cast<QRgb*>(source.scanLine(y));for(int x=0;x<source.width();++x)row[x]=0xff000000|random.generate();}QVERIFY(source.save(path,"PNG"));const auto hash=QCryptographicHash::hash(fileBytes(path),QCryptographicHash::Sha256);
        Endpoint endpoint;endpoint.add("view_image",path);AgentSession session;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();QSignalSpy finished(&session,&AgentSession::finished);session.send("Inspect the texture.");QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,10000);const auto urls=images(endpoint.requests.last().value("messages").toArray());QCOMPARE(urls.size(),1);QVERIFY(urls.first().toUtf8().size()<=512*1024);QVERIFY(urls.first().startsWith("data:image/jpeg;base64,"));const auto decoded=QImage::fromData(imageBytes(urls.first()));QVERIFY(!decoded.isNull());QVERIFY(decoded.width()<=1024 && decoded.height()<=1024);QCOMPARE(hash,QCryptographicHash::hash(fileBytes(path),QCryptographicHash::Sha256));
        const auto denied=run(session,"read_image",{{"cwd",dir.filePath("images")},{"path","noise.png"}});QVERIFY(!denied.value("ok").toBool());QVERIFY(denied.value("error").toString().contains("active tool group"));
    }

    void malformedMissingOversizedAndArgumentsFailHonestly() {
        QTemporaryDir dir;Endpoint endpoint;AgentSession session;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();QFile invalid(dir.filePath("invalid.png"));QVERIFY(invalid.open(QIODevice::WriteOnly));invalid.write("not an image");invalid.close();QFile huge(dir.filePath("huge.png"));QVERIFY(huge.open(QIODevice::WriteOnly));QVERIFY(huge.resize(33*1024*1024));huge.close();
        for(const auto &path:{"missing.png","invalid.png","huge.png"})QVERIFY(!run(session,"read_image",{{"path",path}}).value("ok").toBool());QVERIFY(!run(session,"read_image",{{"path","invalid.png"},{"permissions","full"}}).value("ok").toBool());QVERIFY(!run(session,"read_image",{{"path","invalid.png"},{"cwd","does-not-exist"}}).value("ok").toBool());QVERIFY(!run(session,"read_image",{{"path",""}}).value("ok").toBool());QVERIFY(images(session.messages()).isEmpty());
    }

    void completeImageGroupEnforcesFourAttachmentLimit() {
        QTemporaryDir dir;QImage source(32,32,QImage::Format_RGB32);source.fill(Qt::cyan);QVERIFY(source.save(dir.filePath("small.png")));Endpoint endpoint;for(int i=0;i<5;++i)endpoint.add("read_image","small.png");AgentSession session;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();QSignalSpy finished(&session,&AgentSession::finished);session.send("Inspect these images.");QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,10000);QCOMPARE(endpoint.requests.size(),2);QCOMPARE(images(endpoint.requests.last().value("messages").toArray()).size(),4);for(int i=0;i<4;++i)QVERIFY(toolResult(session.messages(),i).value("ok").toBool());QVERIFY(!toolResult(session.messages(),4).value("ok").toBool());
    }

    void cancellationAndRouteChangesNeverQueueOldImages() {
        QTemporaryDir dir;const auto path=dir.filePath("source.png");QImage source(2048,2048,QImage::Format_RGB32);source.fill(Qt::green);QVERIFY(source.save(path));const auto original=fileBytes(path);Endpoint endpoint;AgentSession session;prepare(session,dir.path(),endpoint.url());AgentLocalImages module(&session);module.registerTools();bool done=false;QJsonObject result;
        session.executeToolAsync("read_image",{{"path",path}},[&](QJsonObject value){result=value;done=true;});emit session.toolExecutionCancelled();QTRY_VERIFY_WITH_TIMEOUT(done,10000);QVERIFY(result.value("cancelled").toBool());QVERIFY(images(session.messages()).isEmpty());
        done=false;session.executeToolAsync("view_image",{{"path",path}},[&](QJsonObject value){result=value;done=true;});auto provider=session.provider();provider.model="other-model";provider.modelCapability={};session.setProvider(provider);QTRY_VERIFY_WITH_TIMEOUT(done,10000);QVERIFY(!result.value("ok").toBool());QVERIFY(!result.contains("dataUrl"));QVERIFY(images(session.messages()).isEmpty());QCOMPARE(fileBytes(path),original);
    }
};
QTEST_GUILESS_MAIN(TestAgentLocalImages)
#include "TestAgentLocalImages.moc"
