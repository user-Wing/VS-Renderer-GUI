#include "agent/AgentMcp.h"
#include "agent/AgentSession.h"
#include "ui/AgentMcpDialog.h"

#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QDir>
#include <QElapsedTimer>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace vsr;

namespace {
QString runtimePath(){return QFileInfo(QFINDTESTDATA("../tools/agent-runtime/bridge.mjs")).absolutePath();}
AgentMcpConfig fixtureConfig(AgentMcpManager &manager)
{
    AgentMcpConfig config;config.id="fixture";config.name="Fixture";config.command=manager.runtime()->nodePath();config.arguments={QDir(runtimePath()).filePath("fixture-server.mjs")};return config;
}
}

class TestAgentMcp final:public QObject{
    Q_OBJECT
private slots:
    void configRoundTripAndImportStaysDisconnected(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());auto config=fixtureConfig(manager);config.enabled=true;config.environment={{"TOKEN","secret-fixture-value"}};config.headers={{"Authorization","Bearer local-token"}};QString error;
        QVERIFY2(manager.setServers({config},&error),qPrintable(error));QVERIFY2(manager.save(&error),qPrintable(error));QFile file(directory.filePath("mcp.json"));QVERIFY(file.open(QIODevice::ReadOnly));const auto saved=file.readAll();file.close();
#ifdef Q_OS_WIN
        QVERIFY(!saved.contains("secret-fixture-value"));QVERIFY(!saved.contains("Bearer local-token"));
#endif
        AgentMcpManager loaded(directory.filePath("mcp.json"),runtimePath());QVERIFY2(loaded.load(&error),qPrintable(error));QCOMPARE(loaded.servers().size(),1);QCOMPARE(loaded.servers()[0].environment,config.environment);QCOMPARE(loaded.state("fixture"),"disconnected");
        const auto exported=directory.filePath("export.json");QVERIFY(loaded.exportConfiguration(exported,&error));QFile output(exported);QVERIFY(output.open(QIODevice::ReadOnly));const auto data=output.readAll();QVERIFY(!data.contains("secret-fixture-value"));
        QVERIFY2(loaded.importConfiguration(exported,&error),qPrintable(error));QCOMPARE(loaded.servers().size(),2);QVERIFY(!loaded.servers()[1].enabled);QCOMPARE(loaded.state(loaded.servers()[1].id),"disconnected");
    }
    void stdioRegistersAndCallsAsyncWithoutBlockingUi(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());QVERIFY(manager.runtime()->ready());const auto config=fixtureConfig(manager);QVERIFY(manager.setServers({config}));AgentSession session;manager.registerTools(&session);manager.connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager.state(config.id),"connected",15000);QCOMPARE(manager.serverTools(config.id).size(),3);QCOMPARE(session.allTools().size(),3);
        QString add;for(const auto &value:session.allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();if(name.endsWith("_add"))add=name;}QVERIFY(!add.isEmpty());
        bool completed=false;QJsonObject result;session.executeToolAsync(add,{{"a",11},{"b",31}},[&](QJsonObject value){completed=true;result=value;});QVERIFY(completed);QVERIFY(!result.value("ok").toBool());
        session.setAccessMode(AgentAccessMode::FullEnvironment);completed=false;session.executeToolAsync(add,{{"a",11},{"b",31}},[&](QJsonObject value){completed=true;result=value;});QTRY_VERIFY_WITH_TIMEOUT(completed,5000);QVERIFY(result.value("ok").toBool());QCOMPARE(result.value("structuredContent").toObject().value("sum").toInt(),42);
        session.setToolEnabled(add,false);QSignalSpy changed(&manager,&AgentMcpManager::toolsChanged);manager.refreshTools(config.id);QTRY_VERIFY(!changed.isEmpty());QVERIFY(!session.toolEnabled(add));
        bool waiting=false;int ticks=0;QTimer heartbeat;heartbeat.setInterval(10);connect(&heartbeat,&QTimer::timeout,&session,[&]{++ticks;});heartbeat.start();manager.callTool(config.id,"wait",{{"milliseconds",250}},[&](QJsonObject value){waiting=true;result=value;},&session);QTRY_VERIFY_WITH_TIMEOUT(waiting,5000);QVERIFY(ticks>=3);QVERIFY(result.value("ok").toBool());
        manager.disconnectServer(config.id);QCOMPARE(manager.state(config.id),"disconnected");QCOMPARE(session.allTools().size(),0);
    }
    void cancelPendingToolKeepsConnectionReusable(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());const auto config=fixtureConfig(manager);QVERIFY(manager.setServers({config}));AgentSession session;manager.registerTools(&session);manager.connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager.state(config.id),"connected",15000);
        bool completed=false;QJsonObject result;manager.callTool(config.id,"wait",{{"milliseconds",3000}},[&](QJsonObject value){completed=true;result=value;},&session);QTest::qWait(50);emit session.toolExecutionCancelled();QTRY_VERIFY(completed);QVERIFY(!result.value("ok").toBool());QCOMPARE(manager.state(config.id),"connected");
        completed=false;manager.callTool(config.id,"add",{{"a",1},{"b",2}},[&](QJsonObject value){completed=true;result=value;});QTRY_VERIFY(completed);QCOMPARE(result.value("structuredContent").toObject().value("sum").toInt(),3);
    }
    void dialogShowsRuntimeAndSource(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());QVERIFY(manager.setServers({fixtureConfig(manager)}));AgentMcpDialog dialog(&manager);QCOMPARE(dialog.objectName(),"agentMcpDialog");QVERIFY(dialog.findChild<QObject*>("agentMcpInstall"));QVERIFY(dialog.findChild<QObject*>("agentMcpServers"));
    }
    void copiedChildOwnsItsCancellation(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());const auto config=fixtureConfig(manager);QVERIFY(manager.setServers({config}));AgentSession parent,child;manager.registerTools(&parent);manager.connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager.state(config.id),"connected",15000);
        parent.setAccessMode(AgentAccessMode::FullEnvironment);child.setAccessMode(AgentAccessMode::FullEnvironment);parent.copyToolsTo(&child);manager.registerTools(&child);QString wait;for(const auto &value:child.allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();if(name.endsWith("_wait"))wait=name;}QVERIFY(!wait.isEmpty());
        bool parentDone=false,childDone=false;QJsonObject parentResult,childResult;
        parent.executeToolAsync(wait,{{"milliseconds",350}},[&](QJsonObject result){parentDone=true;parentResult=result;});
        child.executeToolAsync(wait,{{"milliseconds",3000}},[&](QJsonObject result){childDone=true;childResult=result;});
        QTest::qWait(50);emit child.toolExecutionCancelled();QTRY_VERIFY_WITH_TIMEOUT(childDone,500);QVERIFY(!childResult.value("ok").toBool());QTRY_VERIFY_WITH_TIMEOUT(parentDone,3000);QVERIFY(parentResult.value("ok").toBool());QCOMPARE(manager.state(config.id),"connected");
    }
    void reconnectedChildCannotBypassParentToolPreferenceOrMode(){
        QTemporaryDir directory;AgentMcpManager manager(directory.filePath("mcp.json"),runtimePath());QVERIFY(manager.runtime()->ready());const auto config=fixtureConfig(manager);QVERIFY(manager.setServers({config}));AgentSession parent;parent.setAccessMode(AgentAccessMode::FullEnvironment);manager.registerTools(&parent);manager.connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager.state(config.id),"connected",15000);
        QString add;for(const auto &value:parent.allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();if(name.endsWith("_add"))add=name;}QVERIFY(!add.isEmpty());
        const auto available=[&](const AgentSession &session){for(const auto &value:session.tools())if(value.toObject().value("function").toObject().value("name")==QJsonValue(add))return true;return false;};
        parent.setToolEnabled(add,false);manager.disconnectServer(config.id);QCOMPARE(manager.state(config.id),"disconnected");QCOMPARE(parent.allTools().size(),0);
        AgentSession child;child.setAccessMode(AgentAccessMode::FullEnvironment);child.setToolPermissionParent(&parent);manager.registerTools(&child);manager.connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager.state(config.id),"connected",15000);QTRY_VERIFY(child.toolEnabled(add));QVERIFY(!parent.toolEnabled(add));QVERIFY(!available(child));
        bool done=false;QJsonObject result;child.executeToolAsync(add,{{"a",20},{"b",22}},[&](QJsonObject value){result=value;done=true;});QVERIFY(done);QVERIFY(!result.value("ok").toBool());QVERIFY(child.toolEnabled(add));
        parent.setToolEnabled(add,true);QVERIFY(available(child));done=false;child.executeToolAsync(add,{{"a",20},{"b",22}},[&](QJsonObject value){result=value;done=true;});QTRY_VERIFY_WITH_TIMEOUT(done,5000);QVERIFY(result.value("ok").toBool());QCOMPARE(result.value("structuredContent").toObject().value("sum").toInt(),42);
        AgentSession later;later.setAccessMode(AgentAccessMode::FullEnvironment);later.setToolPermissionParent(&parent);manager.registerTools(&later);QVERIFY(available(later));QVERIFY(later.toolEnabled(add));
        parent.setAccessMode(AgentAccessMode::SoftwareOnly);QVERIFY(!available(child));QVERIFY(!available(later));QVERIFY(child.toolEnabled(add));QVERIFY(later.toolEnabled(add));done=false;later.executeToolAsync(add,{{"a",1},{"b",2}},[&](QJsonObject value){result=value;done=true;});QVERIFY(done);QVERIFY(!result.value("ok").toBool());
    }
    void exitReapsStubbornServerAndGrandchild(){
#ifdef Q_OS_WIN
        QTemporaryDir directory;auto manager=std::make_unique<AgentMcpManager>(directory.filePath("mcp.json"),runtimePath());auto config=fixtureConfig(*manager);config.arguments.append("--stubborn");QVERIFY(manager->setServers({config}));manager->connectServer(config.id);QTRY_COMPARE_WITH_TIMEOUT(manager->state(config.id),"connected",15000);
        bool done=false;QJsonObject result;manager->callTool(config.id,"echo",{{"text","pid"}},[&](QJsonObject value){done=true;result=value;});QTRY_VERIFY(done);const auto data=result.value("structuredContent").toObject();
        const auto pid=DWORD(data.value("pid").toInt()),childPid=DWORD(data.value("childPid").toInt());QVERIFY(pid);QVERIFY(childPid);
        HANDLE server=OpenProcess(SYNCHRONIZE,FALSE,pid),child=OpenProcess(SYNCHRONIZE,FALSE,childPid);QVERIFY(server);QVERIFY(child);QElapsedTimer elapsed;elapsed.start();manager.reset();
        const auto serverExit=WaitForSingleObject(server,1000),childExit=WaitForSingleObject(child,1000);CloseHandle(server);CloseHandle(child);QCOMPARE(serverExit,DWORD(WAIT_OBJECT_0));QCOMPARE(childExit,DWORD(WAIT_OBJECT_0));QVERIFY(elapsed.elapsed()<6500);
#endif
    }
    void installsLockedPortableRuntime(){
        if(!qEnvironmentVariableIsSet("VSR_TEST_NPM_INSTALL"))QSKIP("Set VSR_TEST_NPM_INSTALL=1 for the network installation check.");
        QTemporaryDir directory;AgentNodeRuntime runtime(directory.path());QSignalSpy finished(&runtime,&AgentNodeRuntime::installFinished);runtime.install(runtimePath());QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,120000);QVERIFY2(finished[0][0].toBool(),qPrintable(finished[0][1].toString()));QVERIFY(runtime.ready());QVERIFY(QFileInfo::exists(directory.filePath("npm/bin/npx-cli.js")));
    }
};
QTEST_MAIN(TestAgentMcp)
#include "TestAgentMcp.moc"
