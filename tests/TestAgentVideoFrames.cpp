#include "app/AgentVideoFrames.h"
#include <QImage>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

using namespace vsr;
namespace {
class Endpoint final : public QTcpServer {
public:
    QJsonObject arguments;
    QList<QJsonObject> requests;
    Endpoint() {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            auto *socket = nextPendingConnection(); auto bytes = std::make_shared<QByteArray>();
            connect(socket, &QTcpSocket::readyRead, this, [=, this] {
                *bytes += socket->readAll(); const int split = bytes->indexOf("\r\n\r\n"); if (split < 0) return;
                int length = 0; for (const auto &line : bytes->left(split).split('\n')) if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                if (bytes->size() < split+4+length) return;
                requests.append(QJsonDocument::fromJson(bytes->mid(split+4,length)).object()); bytes->clear();
                QJsonObject message{{"role", "assistant"}, {"content", "Inspected sampled frame."}};
                if (requests.size() == 1) {
                    const QJsonObject function{{"name", "inspect_video_frame"}, {"arguments", QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))}};
                    const QJsonObject call{{"id", "frame-call"}, {"type", "function"}, {"function", function}};
                    message = {{"role", "assistant"}, {"tool_calls", QJsonArray{call}}};
                }
                const auto body = QJsonDocument(QJsonObject{{"choices", QJsonArray{QJsonObject{{"message", message}, {"finish_reason", requests.size()==1 ? "tool_calls" : "stop"}}}}}).toJson(QJsonDocument::Compact);
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body); socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }
    AgentProvider provider() const {
        AgentProvider p; p.baseUrl=QString("http://127.0.0.1:%1/v1").arg(serverPort()); p.model="vision-fixture";
        p.modelCapability={{"endpoint",p.baseUrl},{"model",p.model},{"imageInputSupported",true}}; return p;
    }
};
QString script(bool processed) {
    return QString("import vapoursynth as vs\nc=vs.core\na=c.std.BlankClip(width=128,height=72,format=vs.RGB48,length=1,fpsnum=24,fpsden=1,color=[65535,0,0])\nb=c.std.BlankClip(a,color=[0,65535,0])\nd=c.std.BlankClip(a,color=[0,0,65535])\nclip=a+b+d\n%1\nclip.set_output(0)\n").arg(processed ? "clip=c.resize.Point(clip[::-1],width=64,height=36)" : "");
}
QJsonObject result(const AgentSession &session) {
    for (const auto &v : session.messages()) { const auto m=v.toObject(); if (m.value("role")==QJsonValue("tool")) return QJsonDocument::fromJson(m.value("content").toString().toUtf8()).object(); } return {};
}
QList<QImage> images(const Endpoint &endpoint) {
    QList<QImage> result;
    if (endpoint.requests.isEmpty()) return result;
    for (const auto &v : endpoint.requests.last().value("messages").toArray()) for (const auto &b : v.toObject().value("content").toArray()) {
        const auto block=b.toObject(); if (block.value("type")!=QJsonValue("image_url")) continue;
        const auto url=block.value("image_url").toObject().value("url").toString(); result.append(QImage::fromData(QByteArray::fromBase64(url.mid(url.indexOf(',')+1).toLatin1())));
    }
    return result;
}
}
class TestAgentVideoFrames final : public QObject {
    Q_OBJECT
private slots:
    void exactFrameAndSeconds_data() {
        QTest::addColumn<QJsonObject>("args"); QTest::addColumn<QColor>("color"); QTest::addColumn<QSize>("size"); QTest::addColumn<int>("frame");
        QTest::newRow("source-frame") << QJsonObject{{"frame",1}} << QColor(Qt::green) << QSize(128,72) << 1;
        QTest::newRow("source-seconds") << QJsonObject{{"seconds",2./24}} << QColor(Qt::blue) << QSize(128,72) << 2;
        QTest::newRow("processed-timeline") << QJsonObject{{"frame",0},{"target","processed"}} << QColor(Qt::blue) << QSize(64,36) << 0;
        QTest::newRow("crop") << QJsonObject{{"frame",0},{"crop",QJsonObject{{"x",5},{"y",6},{"width",17},{"height",11}}}} << QColor(Qt::red) << QSize(17,11) << 0;
    }
    void exactFrameAndSeconds() {
        QFETCH(QJsonObject,args); QFETCH(QColor,color); QFETCH(QSize,size); QFETCH(int,frame);
        Endpoint endpoint; endpoint.arguments=args; AgentSession session; session.setProvider(endpoint.provider());
        AgentVideoFrames module(&session, [](bool processed,QString *) { return script(processed); });
        QSignalSpy finished(&session,&AgentSession::finished); session.send("Inspect this frame before suggesting filters.");
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,20000);
        const auto r=result(session); QVERIFY2(r.value("ok").toBool(),qPrintable(r.value("error").toString())); QCOMPARE(r.value("frame").toInt(),frame);
        QCOMPARE(r.value("totalFrames").toInt(),3); QCOMPARE(r.value("fpsNumerator").toInt(),24); QCOMPARE(images(endpoint).size(),1);
        QCOMPARE(images(endpoint).first().size(),size); QCOMPARE(images(endpoint).first().pixelColor(0,0).rgb(),color.rgb());
        QCOMPARE(session.accessMode(),AgentAccessMode::SoftwareOnly); QVERIFY(!r.contains("dataUrl"));
    }
    void invalidPositionsAndCrop_data() {
        QTest::addColumn<QJsonObject>("args");
        QTest::newRow("missing") << QJsonObject{};
        QTest::newRow("ambiguous") << QJsonObject{{"frame",1},{"seconds",0}};
        QTest::newRow("negative") << QJsonObject{{"frame",-1}};
        QTest::newRow("outside") << QJsonObject{{"frame",3}};
        QTest::newRow("fractional") << QJsonObject{{"frame",0.5}};
        QTest::newRow("crop-outside") << QJsonObject{{"frame",0},{"crop",QJsonObject{{"x",120},{"y",0},{"width",20},{"height",10}}}};
    }
    void invalidPositionsAndCrop() {
        QFETCH(QJsonObject,args); Endpoint endpoint; endpoint.arguments=args; AgentSession session; session.setProvider(endpoint.provider());
        AgentVideoFrames module(&session, [](bool processed,QString *) { return script(processed); });
        QSignalSpy finished(&session,&AgentSession::finished); session.send("Inspect."); QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,20000);
        QVERIFY(!result(session).value("ok").toBool()); QVERIFY(images(endpoint).isEmpty());
    }
    void hdrRejected() {
        Endpoint endpoint; endpoint.arguments={{"frame",0}}; AgentSession session; session.setProvider(endpoint.provider());
        AgentVideoFrames module(&session, [](bool,QString *) { return script(false)+"vs.core.std.SetFrameProps(clip,_Transfer=16,_Primaries=9).set_output(0)\n"; });
        QSignalSpy finished(&session,&AgentSession::finished); session.send("Inspect."); QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,20000);
        QVERIFY(!result(session).value("ok").toBool()); QVERIFY(result(session).value("error").toString().contains("HDR")); QVERIFY(images(endpoint).isEmpty());
    }
    void capabilitiesAndCancellation() {
        Endpoint endpoint; AgentSession session; session.setProvider(endpoint.provider());
        AgentVideoFrames module(&session, [](bool,QString *) { return script(false); }); QVERIFY(session.toolAvailable("inspect_video_frame"));
        auto provider=endpoint.provider(); provider.modelCapability={}; session.setProvider(provider); QVERIFY(!session.toolAvailable("inspect_video_frame"));
        session.setProvider(endpoint.provider()); bool done=false; QJsonObject reply;
        session.executeToolAsync("inspect_video_frame",{{"frame",0}},[&](QJsonObject r){reply=r;done=true;});
        emit session.toolExecutionCancelled(); QTRY_VERIFY(done); QVERIFY(reply.value("cancelled").toBool());
    }
};
QTEST_GUILESS_MAIN(TestAgentVideoFrames)
#include "TestAgentVideoFrames.moc"
