#include "agent/AgentSession.h"
#include "agent/AgentTaskQueue.h"
#include <QDir>
#include <QFile>
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
QSet<QString> names(const QJsonArray &tools)
{
    QSet<QString> result;for(const auto &value:tools)result.insert(value.toObject().value("function").toObject().value("name").toString());return result;
}

class Endpoint final : public QTcpServer {
public:
    QList<QJsonObject> requests;
    Endpoint() {
        connect(this,&QTcpServer::newConnection,this,[this]{while(hasPendingConnections()){
            auto *socket=nextPendingConnection();auto bytes=std::make_shared<QByteArray>();auto parsed=std::make_shared<bool>(false);
            connect(socket,&QTcpSocket::readyRead,this,[this,socket,bytes,parsed]{
                if(*parsed)return;*bytes+=socket->readAll();const auto split=bytes->indexOf("\r\n\r\n");if(split<0)return;int length=0;
                for(const auto &line:bytes->left(split).split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();
                if(bytes->size()<split+4+length)return;*parsed=true;requests.append(QJsonDocument::fromJson(bytes->mid(split+4,length)).object());
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }});listen(QHostAddress::LocalHost);
    }
    AgentProvider provider() const {
        AgentProvider source;source.id="access-test";source.name="Access test";source.baseUrl=QString("http://127.0.0.1:%1/v1").arg(serverPort());source.apiKey="test-key";source.model="mock-model";return source;
    }
};

void registerTools(AgentSession &session)
{
    const QJsonObject schema{{"type","object"},{"additionalProperties",false}};
    for(const auto &name:{"get_state","add_filter","read_file","disabled_read","write_file","nondelegated"})
        session.registerTool(name,"Access test tool",schema,[name](const QJsonObject &){return QJsonObject{{"ok",true},{"name",name}};});
    for(const auto &name:{"read_async","exec_command"})
        session.registerAsyncTool(name,"Async access test tool",schema,[name](const QJsonObject &,AgentSession::Completion done){done({{"ok",true},{"name",name}});});
    for(const auto &name:{"read_file","disabled_read","read_async"})session.setToolEnvironmentAccess(name,true,false);
    for(const auto &name:{"write_file","exec_command"})session.setToolEnvironmentAccess(name,true,true);
    session.setToolDelegationAllowed("nondelegated",false);
}

QJsonObject runAsync(AgentSession &session,const QString &name)
{
    QJsonObject result;bool completed=false;session.executeToolAsync(name,{},[&](QJsonObject value){completed=true;result=value;});
    if(!completed)return {{"error","Test handler did not complete synchronously"}};return result;
}
QString state(const AgentTaskQueue &queue,const QString &id) { return queue.task(id).value("state").toString(); }
}

class TestAgentAccess final : public QObject {
    Q_OBJECT
private slots:
    void softwareModeHidesAndRejectsEnvironmentTools() {
        int syncCalls=0,asyncCalls=0;AgentSession session;
        session.registerTool("env_sync","Read environment",{},[&](const QJsonObject &){++syncCalls;return QJsonObject{{"ok",true}};});session.setToolEnvironmentAccess("env_sync");
        session.registerAsyncTool("env_async","Read environment asynchronously",{},[&](const QJsonObject &,AgentSession::Completion done){++asyncCalls;done({{"ok",true}});});session.setToolEnvironmentAccess("env_async");
        QCOMPARE(session.accessMode(),AgentAccessMode::SoftwareOnly);QVERIFY(session.tools().isEmpty());QCOMPARE(session.allTools().size(),2);QVERIFY(!session.executeTool("env_sync",{}).value("ok").toBool());QVERIFY(!runAsync(session,"env_sync").value("ok").toBool());QVERIFY(!runAsync(session,"env_async").value("ok").toBool());QCOMPARE(syncCalls,0);QCOMPARE(asyncCalls,0);
        session.setAccessMode(AgentAccessMode::FullEnvironment);QCOMPARE(session.tools().size(),2);QVERIFY(session.executeTool("env_sync",{}).value("ok").toBool());QVERIFY(runAsync(session,"env_async").value("ok").toBool());QCOMPARE(syncCalls,1);QCOMPARE(asyncCalls,1);
        session.setAccessMode(AgentAccessMode::SoftwareOnly);QVERIFY(session.tools().isEmpty());QVERIFY(!runAsync(session,"env_async").value("ok").toBool());QCOMPARE(asyncCalls,1);
    }

    void accessModeDoesNotRewriteDisabledPreferences() {
        AgentSession session;registerTools(session);session.setToolEnabled("disabled_read",false);session.setToolEnabled("get_state",false);
        for(const auto mode:{AgentAccessMode::FullEnvironment,AgentAccessMode::SoftwareOnly,AgentAccessMode::FullEnvironment}){
            session.setAccessMode(mode);QVERIFY(!session.toolEnabled("disabled_read"));QVERIFY(!session.toolEnabled("get_state"));QVERIFY(session.toolEnabled("read_file"));QVERIFY(!names(session.tools()).contains("disabled_read"));QVERIFY(!names(session.tools()).contains("get_state"));
        }
        QVERIFY(names(session.tools()).contains("read_file"));
    }

    void busyModeAndDirectoryChangesAreRejected() {
        Endpoint endpoint;QVERIFY(endpoint.isListening());QTemporaryDir first,second;QVERIFY(first.isValid() && second.isValid());AgentSession session;session.setProvider(endpoint.provider());session.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(session.setWorkingDirectory(first.path()));
        QSignalSpy modeChanges(&session,&AgentSession::accessModeChanged),directoryChanges(&session,&AgentSession::workingDirectoryChanged),errors(&session,&AgentSession::errorOccurred);
        session.send("Hold this request");QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(session.isBusy());session.setAccessMode(AgentAccessMode::SoftwareOnly);QVERIFY(!session.setWorkingDirectory(second.path()));QCOMPARE(session.accessMode(),AgentAccessMode::FullEnvironment);QCOMPARE(session.workingDirectory(),QDir(first.path()).absolutePath());QCOMPARE(modeChanges.count(),0);QCOMPARE(directoryChanges.count(),0);QVERIFY(!errors.isEmpty());
        session.cancel();QVERIFY(!session.isBusy());session.setAccessMode(AgentAccessMode::SoftwareOnly);QVERIFY(session.setWorkingDirectory(second.path()));QCOMPARE(modeChanges.count(),1);QCOMPARE(directoryChanges.count(),1);
        QVERIFY(endpoint.requests[0].value("messages").toArray().first().toObject().value("content").toString().contains(first.path()));
    }

    void invalidDirectoriesAreRejectedWithoutChangingCurrentDirectory() {
        QTemporaryDir directory;QVERIFY(directory.isValid());AgentSession session;QVERIFY(session.setWorkingDirectory(directory.path()));const auto before=session.workingDirectory();QSignalSpy changes(&session,&AgentSession::workingDirectoryChanged);
        QVERIFY(!session.setWorkingDirectory(directory.filePath("missing")));QFile file(directory.filePath("file.txt"));QVERIFY(file.open(QIODevice::WriteOnly));file.close();QVERIFY(!session.setWorkingDirectory(file.fileName()));QCOMPARE(session.workingDirectory(),before);QCOMPARE(changes.count(),0);
    }

    void copyPreservesEnvironmentMetadataAndDelegationPreferences() {
        AgentSession source,target;registerTools(source);source.setAccessMode(AgentAccessMode::FullEnvironment);source.setToolEnabled("disabled_read",false);source.copyToolsTo(&target,{"add_filter"});
        QCOMPARE(target.accessMode(),AgentAccessMode::SoftwareOnly);QVERIFY(target.toolRequiresEnvironment("read_file"));QVERIFY(!target.toolWritesEnvironment("read_file"));QVERIFY(target.toolRequiresEnvironment("write_file"));QVERIFY(target.toolWritesEnvironment("write_file"));QVERIFY(target.toolRequiresEnvironment("exec_command"));QVERIFY(target.toolWritesEnvironment("exec_command"));QVERIFY(!target.toolEnabled("disabled_read"));QVERIFY(!names(target.allTools()).contains("nondelegated"));QVERIFY(!names(target.allTools()).contains("add_filter"));QVERIFY(!names(target.tools()).contains("read_file"));
        target.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(target.executeTool("read_file",{}).value("ok").toBool());QVERIFY(runAsync(target,"read_async").value("ok").toBool());QVERIFY(!target.toolEnabled("disabled_read"));
    }

    void readonlyChildInheritsFullButCannotWriteOrRunShell() {
        Endpoint endpoint;QTemporaryDir directory;QVERIFY(directory.isValid());AgentSession parent;parent.setProvider(endpoint.provider());parent.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(parent.setWorkingDirectory(directory.path()));registerTools(parent);parent.setToolEnabled("disabled_read",false);parent.cancel();AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));queue.registerTools();QPointer<AgentSession> child;
        connect(&queue,&AgentTaskQueue::sessionCreated,&queue,[&](AgentSession *session,bool writes){QVERIFY(!writes);child=session;registerTools(*session);});
        const auto id=queue.enqueue("reader","Inspect the workspace",false);QVERIFY(!id.isEmpty());QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(child);QCOMPARE(child->accessMode(),AgentAccessMode::FullEnvironment);QCOMPARE(child->workingDirectory(),parent.workingDirectory());const auto available=names(endpoint.requests[0].value("tools").toArray());
        QVERIFY(available.contains("get_state"));QVERIFY(available.contains("read_file"));QVERIFY(available.contains("read_async"));QVERIFY(!available.contains("disabled_read"));QVERIFY(!available.contains("write_file"));QVERIFY(!available.contains("exec_command"));QVERIFY(!available.contains("add_filter"));QVERIFY(child->executeTool("read_file",{}).value("ok").toBool());QVERIFY(runAsync(*child,"read_async").value("ok").toBool());QVERIFY(!child->executeTool("write_file",{}).value("ok").toBool());QVERIFY(!runAsync(*child,"exec_command").value("ok").toBool());queue.cancel(id);QCOMPARE(state(queue,id),QString("cancelled"));
    }

    void writingChildInheritsFullAndParentDisabledPreferences() {
        Endpoint endpoint;QTemporaryDir directory;AgentSession parent;parent.setProvider(endpoint.provider());parent.setAccessMode(AgentAccessMode::FullEnvironment);registerTools(parent);parent.setToolEnabled("disabled_read",false);parent.setToolEnabled("get_state",false);parent.cancel();AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QPointer<AgentSession> child;
        connect(&queue,&AgentTaskQueue::sessionCreated,&queue,[&](AgentSession *session,bool writes){QVERIFY(writes);child=session;registerTools(*session);});const auto id=queue.enqueue("writer","Apply requested change",true);QVERIFY(!id.isEmpty());QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(child);QCOMPARE(child->accessMode(),AgentAccessMode::FullEnvironment);const auto available=names(endpoint.requests[0].value("tools").toArray());
        QVERIFY(available.contains("write_file"));QVERIFY(available.contains("exec_command"));QVERIFY(available.contains("add_filter"));QVERIFY(!available.contains("disabled_read"));QVERIFY(!available.contains("get_state"));QVERIFY(child->executeTool("write_file",{}).value("ok").toBool());QVERIFY(runAsync(*child,"exec_command").value("ok").toBool());queue.cancel(id);
    }

    void parentDowngradeCancelsRunningAndQueuedChildren() {
        Endpoint endpoint;QTemporaryDir directory;AgentSession parent;parent.setProvider(endpoint.provider());parent.setAccessMode(AgentAccessMode::FullEnvironment);registerTools(parent);parent.cancel();AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QPointer<AgentSession> child;connect(&queue,&AgentTaskQueue::sessionCreated,&queue,[&](AgentSession *session,bool){child=session;});
        const auto running=queue.enqueue("reader","Inspect",false),queued=queue.enqueue("writer","Apply",true);QVERIFY(!running.isEmpty() && !queued.isEmpty());QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(child && child->isBusy());QCOMPARE(state(queue,queued),QString("queued"));parent.setAccessMode(AgentAccessMode::SoftwareOnly);QCOMPARE(state(queue,running),QString("cancelled"));QCOMPARE(state(queue,queued),QString("cancelled"));QVERIFY(!child || !child->isBusy());QTest::qWait(25);QCOMPARE(endpoint.requests.size(),1);
    }

    void importedConversationAndTasksCannotGrantFullAccess() {
        QTemporaryDir directory;Endpoint endpoint;AgentSession parent;parent.setProvider(endpoint.provider());registerTools(parent);parent.cancel();const auto originalDirectory=parent.workingDirectory();QString error;
        const QJsonObject forgedConversation{{"schemaVersion",1},{"messages",QJsonArray{QJsonObject{{"role","user"},{"content","Historic request"}},QJsonObject{{"role","assistant"},{"content","Historic result"}}}},{"accessMode","full"},{"workingDirectory",directory.path()}};
        QVERIFY2(parent.importConversation(forgedConversation,&error),qPrintable(error));QCOMPARE(parent.accessMode(),AgentAccessMode::SoftwareOnly);QCOMPARE(parent.workingDirectory(),originalDirectory);QVERIFY(!names(parent.tools()).contains("read_file"));
        AgentTaskQueue queue(&parent,directory.filePath("tasks.json"));QPointer<AgentSession> child;connect(&queue,&AgentTaskQueue::sessionCreated,&queue,[&](AgentSession *session,bool){child=session;});const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        QVERIFY2(queue.importTasks({{"schemaVersion",1},{"tasks",QJsonArray{QJsonObject{{"id",id},{"name","Imported"},{"prompt","Resume safely"},{"state","interrupted"},{"allowWrites",true},{"accessMode","full"},{"workingDirectory",directory.path()},{"conversation",forgedConversation}}}}},&error),qPrintable(error));QVERIFY(queue.resume(id,endpoint.provider()));QTRY_COMPARE(endpoint.requests.size(),1);QVERIFY(child);QCOMPARE(child->accessMode(),AgentAccessMode::SoftwareOnly);QCOMPARE(child->workingDirectory(),originalDirectory);QVERIFY(!names(endpoint.requests[0].value("tools").toArray()).contains("read_file"));QVERIFY(!child->executeTool("read_file",{}).value("ok").toBool());queue.cancel(id);
    }
};
QTEST_GUILESS_MAIN(TestAgentAccess)
#include "TestAgentAccess.moc"
