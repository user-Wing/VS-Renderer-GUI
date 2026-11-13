#include "agent/AgentLocalProcess.h"
#include "agent/AgentSession.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace vsr;

namespace {
void print(FILE *handle, const QByteArray &data)
{
    fwrite(data.constData(), 1, size_t(data.size()), handle); fflush(handle);
}
int fixture(const QStringList &arguments)
{
    const auto mode = arguments.value(2);
    if (mode == "identity") { print(stdout, QCoreApplication::applicationFilePath().toUtf8()); return 0; }
    if (mode == "echo") {
        QJsonArray args; for (int i = 3; i < arguments.size(); ++i) args.append(arguments[i]);
        print(stdout, QJsonDocument(QJsonObject{{"cwd", QDir::currentPath()}, {"env", qEnvironmentVariable("VSR_PROCESS_FIXTURE")}, {"args", args}}).toJson(QJsonDocument::Compact));
        print(stderr, QStringLiteral("错误输出✓").toUtf8()); return 7;
    }
    if (mode == "stdin") {
        char line[8192]; while (fgets(line, sizeof(line), stdin)) print(stdout, "echo:" + QByteArray(line));
        print(stdout, "EOF\n"); return 0;
    }
    if (mode == "flood") { print(stdout, QByteArray(512 * 1024, 'o')); print(stderr, QByteArray(512 * 1024, 'e')); return 0; }
    if (mode == "tree" || mode == "tree-exit") {
        QProcess child; child.start(QCoreApplication::applicationFilePath(), {"--fixture", "sleep"});
        if (!child.waitForStarted(5000)) return 10;
        print(stdout, QByteArray::number(child.processId()) + "\n");
#ifdef Q_OS_WIN
        if (mode == "tree-exit") { QThread::msleep(200); ExitProcess(0); }
#endif
        child.waitForFinished(-1); return 0;
    }
    print(stdout, "ready\n"); QThread::msleep(arguments.value(3, "30000").toULong()); print(stdout, "done\n"); return 0;
}
QJsonObject call(AgentSession &session, const QString &name, const QJsonObject &arguments, int timeout = 35000)
{
    struct Reply { bool done = false; QJsonObject object; };
    const auto reply = std::make_shared<Reply>();
    session.executeToolAsync(name, arguments, [reply](QJsonObject result) { reply->object = result; reply->done = true; });
    QElapsedTimer timer; timer.start();
    while (!reply->done && timer.elapsed() < timeout) QTest::qWait(5);
    return reply->done ? reply->object : QJsonObject{{"ok", false}, {"error", "Test timed out waiting for completion."}};
}
QJsonObject startArgs(const QString &mode, int yield = 1000, bool background = false)
{
    return {{"executable", QCoreApplication::applicationFilePath()}, {"arguments", QJsonArray{"--fixture", mode}}, {"yield_time_ms", yield}, {"background", background}};
}
QJsonObject poll(AgentSession &session, const QJsonObject &started, int yield = 1000)
{
    return call(session, "write_stdin", {{"session_id", started.value("session_id")}, {"yield_time_ms", yield}});
}
#ifdef Q_OS_WIN
bool stopped(HANDLE process, int timeout = 5000)
{
    QElapsedTimer timer; timer.start();
    while (WaitForSingleObject(process, 0) == WAIT_TIMEOUT && timer.elapsed() < timeout) QTest::qWait(5);
    return WaitForSingleObject(process, 0) == WAIT_OBJECT_0;
}
#endif
}

class TestAgentLocalProcess final : public QObject {
    Q_OBJECT
private slots:
    void permissionsAndDelegation();
    void powershellAndExitCode();
    void executableArgumentsEnvironmentAndWorkdir();
    void asynchronousPollingAndStdin();
    void boundedOutput();
    void timeoutsCancellationAndOwnerIsolation();
    void processTreeCleanup();
    void installedCommands();
    void portableRuntimePath();
    void completionCanDestroyOwner();
    void hostPathPrecedesPortablePython();
    void relativeExecutableUsesWorkdir();
};

void TestAgentLocalProcess::permissionsAndDelegation()
{
    AgentSession parent; AgentLocalProcess local(&parent); local.registerTools();
    QCOMPARE(parent.allTools().size(), 4); QVERIFY(!parent.toolAvailable("exec_command"));
    QVERIFY(!call(parent, "exec_command", startArgs("sleep")).value("ok").toBool());
    AgentSession child; parent.copyToolsTo(&child); QVERIFY(child.allTools().isEmpty());
    parent.setAccessMode(AgentAccessMode::FullEnvironment);
    QVERIFY(parent.toolAvailable("exec_command")); QVERIFY(parent.toolWritesEnvironment("exec_command")); QVERIFY(!parent.toolWritesEnvironment("list_processes"));
    QVERIFY(!call(parent, "exec_command", {{"cmd", "x"}, {"executable", "y"}}).value("ok").toBool());
    QVERIFY(!call(parent, "exec_command", {{"cmd", "x"}, {"env", QJsonObject{{"BAD", 42}}}}).value("ok").toBool());
    QVERIFY(!call(parent, "exec_command", {{"cmd", "x"}, {"workdir", "Z:/nonexistent-vsr-test"}}).value("ok").toBool());
    const auto missing = call(parent, "exec_command", {{"executable", "vsr-nonexistent-program-41981"}});
    QVERIFY(!missing.value("ok").toBool()); QVERIFY(!missing.value("running").toBool()); QVERIFY(!missing.value("error").toString().isEmpty());
    for (const auto id : {0.0, -1.0, 0.5, 9007199254740992.0, 1e300}) {
        for (const auto &tool : {"write_stdin", "terminate_process"}) {
            const auto invalid = call(parent, tool, {{"session_id", id}});
            QVERIFY(!invalid.value("ok").toBool()); QVERIFY(!invalid.value("error").toString().contains("Unknown process"));
        }
    }
}

void TestAgentLocalProcess::powershellAndExitCode()
{
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    const auto value = call(session, "exec_command", {{"cmd", QStringLiteral("[Console]::Write('中文✓'); [Console]::Error.Write('stderr✓'); exit 9")}, {"yield_time_ms", 10000}});
    QCOMPARE(value.value("exit_code").toInt(), 9); QCOMPARE(value.value("stdout").toString(), QStringLiteral("中文✓")); QCOMPARE(value.value("stderr").toString(), QStringLiteral("stderr✓"));
    QVERIFY(!value.value("ok").toBool()); QVERIFY(!value.value("running").toBool());
    const auto thrown = call(session, "exec_command", {{"cmd", "throw 'fixture failure'"}, {"yield_time_ms", 10000}});
    QVERIFY(!thrown.value("ok").toBool()); QVERIFY(thrown.value("exit_code").toInt() != 0);
}

void TestAgentLocalProcess::executableArgumentsEnvironmentAndWorkdir()
{
    QTemporaryDir temp; QVERIFY(temp.isValid());
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); QVERIFY(session.setWorkingDirectory(temp.path()));
    AgentLocalProcess local(&session); local.registerTools();
    auto args = startArgs("echo"); const QJsonArray expected{QStringLiteral("a b"), QStringLiteral("quote\"and'中文")};
    args["arguments"] = QJsonArray{"--fixture", "echo", expected[0], expected[1]}; args["env"] = QJsonObject{{"VSR_PROCESS_FIXTURE", QStringLiteral("环境✓")}};
    const auto value = call(session, "exec_command", args);
    QCOMPARE(value.value("exit_code").toInt(), 7);
    const auto document = QJsonDocument::fromJson(value.value("stdout").toString().toUtf8()).object();
    QCOMPARE(document.value("cwd").toString(), QDir::cleanPath(temp.path())); QCOMPARE(document.value("env").toString(), QStringLiteral("环境✓")); QCOMPARE(document.value("args").toArray(), expected);
    QCOMPARE(value.value("stderr").toString(), QStringLiteral("错误输出✓"));
    auto relative = args; relative["workdir"] = "."; QVERIFY(!call(session, "exec_command", relative).value("running").toBool());
}

void TestAgentLocalProcess::asynchronousPollingAndStdin()
{
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    int heartbeats = 0; QTimer heartbeat; connect(&heartbeat, &QTimer::timeout, this, [&] { ++heartbeats; }); heartbeat.start(5);
    auto args = startArgs("sleep", 30); args["arguments"] = QJsonArray{"--fixture", "sleep", "150"};
    const auto first = call(session, "exec_command", args); QVERIFY(first.value("running").toBool()); QVERIFY(first.value("exit_code").isNull());
    const auto last = poll(session, first); QVERIFY(!last.value("running").toBool()); QCOMPARE(last.value("exit_code").toInt(), 0); QVERIFY(heartbeats > 5);
    QElapsedTimer timer; timer.start(); const auto background = call(session, "exec_command", startArgs("stdin", 1000, true));
    QVERIFY(background.value("running").toBool()); QVERIFY(timer.elapsed() < 1000);
    const auto echoed = call(session, "write_stdin", {{"session_id", background.value("session_id")}, {"chars", QStringLiteral("你好\n")}, {"yield_time_ms", 100}});
    QVERIFY(echoed.value("running").toBool()); QVERIFY(echoed.value("stdout").toString().contains(QStringLiteral("echo:你好")));
    const auto eof = call(session, "write_stdin", {{"session_id", background.value("session_id")}, {"close_stdin", true}});
    QCOMPARE(eof.value("exit_code").toInt(-1), 0); QVERIFY(eof.value("stdout").toString().contains("EOF"));
    const auto listing = session.executeTool("list_processes", {}).value("processes").toArray(); QCOMPARE(listing.size(), 2);
    QVERIFY(!call(session, "write_stdin", {{"session_id", background.value("session_id")}, {"chars", "after exit"}}).value("ok").toBool());
}

void TestAgentLocalProcess::boundedOutput()
{
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    auto args = startArgs("flood"); args["max_output_chars"] = 2000;
    const auto result = call(session, "exec_command", args);
    QVERIFY(result.value("ok").toBool()); QVERIFY(result.value("output_truncated").toBool()); QVERIFY(result.value("discarded_characters").toInt() > 500000);
    QVERIFY(result.value("stdout").toString().size() <= 1000); QVERIFY(result.value("stderr").toString().size() <= 1000); QVERIFY(QJsonDocument(result).toJson().size() < 10000);
    const auto again = poll(session, result, 0); QVERIFY(again.value("stdout").toString().isEmpty()); QVERIFY(again.value("stderr").toString().isEmpty());
}

void TestAgentLocalProcess::timeoutsCancellationAndOwnerIsolation()
{
    AgentSession a, b; a.setAccessMode(AgentAccessMode::FullEnvironment); b.setAccessMode(AgentAccessMode::FullEnvironment);
    AgentLocalProcess first(&a), second(&b); first.registerTools(); second.registerTools();
    auto shortDeadline = startArgs("sleep"); shortDeadline["timeout_ms"] = 60;
    const auto timed = call(a, "exec_command", shortDeadline); QVERIFY(timed.value("timed_out").toBool()); QVERIFY(!timed.value("running").toBool());
    const auto activeA = call(a, "exec_command", startArgs("sleep", 20)); const auto activeB = call(b, "exec_command", startArgs("sleep", 20));
    QVERIFY(activeA.value("running").toBool()); QVERIFY(activeB.value("running").toBool()); QVERIFY(activeA.value("session_id") != activeB.value("session_id"));
    QVERIFY(!call(b, "terminate_process", {{"session_id", activeA.value("session_id")}}).value("ok").toBool());
    const auto background = call(a, "exec_command", startArgs("sleep", 20, true)); a.cancel();
    const auto cancelled = poll(a, activeA); QVERIFY(cancelled.value("cancelled").toBool()); QVERIFY(!cancelled.value("running").toBool());
    QVERIFY(poll(b, activeB, 0).value("running").toBool()); QVERIFY(poll(a, background, 0).value("running").toBool());
    a.setAccessMode(AgentAccessMode::SoftwareOnly); QTest::qWait(30);
    const auto records = first.processes(); for (const auto &entry : records) QVERIFY(!entry.toObject().value("running").toBool());
    const auto terminated = call(b, "terminate_process", {{"session_id", activeB.value("session_id")}}); QVERIFY(terminated.value("cancelled").toBool()); QVERIFY(!terminated.value("running").toBool());
}

void TestAgentLocalProcess::processTreeCleanup()
{
#ifndef Q_OS_WIN
    QSKIP("Windows JobObject process-tree regression.");
#else
    for (int cleanup = 0; cleanup < 5; ++cleanup) {
        auto session = std::make_unique<AgentSession>(); session->setAccessMode(AgentAccessMode::FullEnvironment);
        auto *local = new AgentLocalProcess(session.get()); local->registerTools();
        auto args = startArgs(cleanup == 3 ? "tree-exit" : "tree", 100, true);
        if (cleanup == 4) args["timeout_ms"] = 300;
        auto started = call(*session, "exec_command", args); QString output = started.value("stdout").toString();
        for (int i = 0; i < 30 && output.trimmed().isEmpty(); ++i) output += poll(*session, started, 20).value("stdout").toString();
        const auto childPid = output.trimmed().toULong(); QVERIFY2(childPid != 0, qPrintable(QJsonDocument(started).toJson()));
        HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, childPid); QVERIFY(child != nullptr);
        HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, DWORD(started.value("pid").toDouble())); QVERIFY(parent != nullptr);
        if (cleanup == 0) call(*session, "terminate_process", {{"session_id", started.value("session_id")}});
        else if (cleanup == 1) session->setAccessMode(AgentAccessMode::SoftwareOnly);
        else if (cleanup == 2) session.reset();
        else {
            const auto completed = poll(*session, started);
            QVERIFY(!completed.value("running").toBool());
            if (cleanup == 4) QVERIFY(completed.value("timed_out").toBool());
        }
        const bool parentStopped = stopped(parent), childStopped = stopped(child); CloseHandle(parent); CloseHandle(child);
        QVERIFY(parentStopped); QVERIFY(childStopped);
    }
#endif
}

void TestAgentLocalProcess::installedCommands()
{
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    for (const auto &command : {"node --version", "npm --version", "python --version", "git --version"}) {
        if (QStandardPaths::findExecutable(QString::fromLatin1(command).section(' ', 0, 0)).isEmpty()) continue;
        const auto result = call(session, "exec_command", {{"cmd", command}, {"yield_time_ms", 10000}});
        QVERIFY2(result.value("ok").toBool(), qPrintable(command + QStringLiteral(": ") + QString::fromUtf8(QJsonDocument(result).toJson())));
        QVERIFY(!result.value("output").toString().trimmed().isEmpty());
        if (QString::fromLatin1(command).startsWith("python ")) {
            const QString script = "import codecs,sys; print(sys.executable); print(codecs.lookup('utf-8-sig').name); print('\\u4e2d\\u6587\\u2713')";
            const auto library = call(session, "exec_command", {{"executable", "python"}, {"arguments", QJsonArray{"-c", script}}, {"yield_time_ms", 10000}});
            QVERIFY(library.value("ok").toBool()); QVERIFY(library.value("stdout").toString().contains("utf-8-sig")); QVERIFY(library.value("stdout").toString().contains(QStringLiteral("中文✓")));
            const auto overridden = call(session, "exec_command", {{"executable", "python"}, {"arguments", QJsonArray{"-c", "print('\\u4e2d\\u6587\\u2713')"}}, {"env", QJsonObject{{"PYTHONIOENCODING", "ascii:backslashreplace"}}}, {"yield_time_ms", 10000}});
            QVERIFY(overridden.value("ok").toBool()); QCOMPARE(overridden.value("stdout").toString().trimmed(), QString("\\u4e2d\\u6587\\u2713"));
        }
    }
    QString ffmpeg = QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");
    if (!QFileInfo::exists(ffmpeg)) ffmpeg = QStandardPaths::findExecutable("ffmpeg");
    if (QFileInfo::exists(ffmpeg)) {
        const auto result = call(session, "exec_command", {{"executable", ffmpeg}, {"arguments", QJsonArray{"-version"}}, {"yield_time_ms", 10000}});
        QVERIFY(result.value("ok").toBool()); QVERIFY(result.value("output").toString().contains("ffmpeg version"));
    }
    if (!QStandardPaths::findExecutable("npm").isEmpty()) {
        const auto npm = call(session, "exec_command", {{"executable", "npm"}, {"arguments", QJsonArray{"--version"}}, {"yield_time_ms", 10000}});
        QVERIFY(npm.value("ok").toBool()); QVERIFY(!npm.value("output").toString().trimmed().isEmpty());
    }
}

void TestAgentLocalProcess::portableRuntimePath()
{
    const auto runtime = QDir(QCoreApplication::applicationDirPath()).filePath("runtime/agent");
    if (!QFileInfo::exists(QDir(runtime).filePath("node.exe"))) QSKIP("Portable runtime is not staged beside this test executable.");
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    const auto system = qEnvironmentVariable("SystemRoot", "C:/Windows");
    const QJsonObject env{{"PATH", QDir::toNativeSeparators(QDir(system).filePath("System32"))}};
    const auto versions = call(session, "exec_command", {{"cmd", "node --version; npm --version; npx --version"}, {"env", env}, {"yield_time_ms", 10000}});
    QVERIFY2(versions.value("ok").toBool(), qPrintable(QString::fromUtf8(QJsonDocument(versions).toJson())));
    const auto lines = versions.value("stdout").toString().trimmed().split('\n', Qt::SkipEmptyParts);
    QCOMPARE(lines.size(), 3); QVERIFY(lines[0].startsWith('v')); QVERIFY(lines[1][0].isDigit()); QVERIFY(lines[2][0].isDigit());
    const auto npm = call(session, "exec_command", {{"executable", "npm"}, {"arguments", QJsonArray{"--version"}}, {"env", env}, {"yield_time_ms", 10000}});
    QVERIFY(npm.value("ok").toBool()); QCOMPARE(npm.value("stdout").toString().trimmed(), lines[1].trimmed());
}

void TestAgentLocalProcess::completionCanDestroyOwner()
{
    QPointer<AgentSession> session = new AgentSession; session->setAccessMode(AgentAccessMode::FullEnvironment);
    auto *local = new AgentLocalProcess(session); local->registerTools();
    bool completed = false;
    session->executeToolAsync("exec_command", startArgs("echo"), [&session, &completed](const QJsonObject &result) {
        completed = result.value("exit_code").toInt() == 7; delete session.data();
    });
    QTRY_VERIFY_WITH_TIMEOUT(completed, 5000); QVERIFY(session.isNull());
    QTest::qWait(10);
}

void TestAgentLocalProcess::hostPathPrecedesPortablePython()
{
    QTemporaryDir host; QVERIFY(host.isValid());
    const auto executable = host.filePath("python.exe"); QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), executable));
    const auto oldPath = qgetenv("PATH"); const auto restore = qScopeGuard([&] { qputenv("PATH", oldPath); });
    const auto hostPath = QDir::toNativeSeparators(host.path()) + QDir::listSeparator() + QString::fromLocal8Bit(oldPath);
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); AgentLocalProcess local(&session); local.registerTools();
    for (const bool inherited : {false, true}) {
        QJsonObject environment;
        if (inherited) qputenv("PATH", hostPath.toLocal8Bit()); else environment.insert("PATH", hostPath);
        const auto direct = call(session, "exec_command", {{"executable", "python"}, {"arguments", QJsonArray{"--fixture", "identity"}}, {"env", environment}, {"yield_time_ms", 5000}});
        QVERIFY2(direct.value("ok").toBool(), qPrintable(QString::fromUtf8(QJsonDocument(direct).toJson())));
        QCOMPARE(QDir::cleanPath(direct.value("stdout").toString()), QDir::cleanPath(executable));
        const auto shell = call(session, "exec_command", {{"cmd", "python --fixture identity"}, {"env", environment}, {"yield_time_ms", 5000}});
        QVERIFY(shell.value("ok").toBool()); QCOMPARE(QDir::cleanPath(shell.value("stdout").toString()), QDir::cleanPath(executable));
    }
}

void TestAgentLocalProcess::relativeExecutableUsesWorkdir()
{
    QTemporaryDir work, path; QVERIFY(work.isValid()); QVERIFY(path.isValid());
    QVERIFY(QDir().mkpath(work.filePath("scripts"))); QVERIFY(QDir().mkpath(path.filePath("scripts")));
    const auto expected = work.filePath("scripts/tool.exe");
    QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), expected));
    QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), path.filePath("scripts/tool.exe")));
    const auto searchPath = QDir::toNativeSeparators(path.path()) + QDir::listSeparator() + qEnvironmentVariable("PATH");
    AgentSession session; session.setAccessMode(AgentAccessMode::FullEnvironment); QVERIFY(session.setWorkingDirectory(work.path())); AgentLocalProcess local(&session); local.registerTools();
    for (const auto &program : {"./scripts/tool.exe", "scripts/tool.exe", "scripts\\tool.exe"}) {
        const auto result = call(session, "exec_command", {{"executable", program}, {"arguments", QJsonArray{"--fixture", "identity"}}, {"env", QJsonObject{{"PATH", searchPath}}}, {"yield_time_ms", 5000}});
        QVERIFY2(result.value("ok").toBool(), qPrintable(QString::fromUtf8(QJsonDocument(result).toJson())));
        QCOMPARE(QDir::cleanPath(result.value("stdout").toString()), QDir::cleanPath(expected));
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == "--fixture") return fixture(app.arguments());
    TestAgentLocalProcess test; return QTest::qExec(&test, argc, argv);
}

#include "TestAgentLocalProcess.moc"
