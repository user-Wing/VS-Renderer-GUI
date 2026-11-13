#include "agent/AgentLocalProcess.h"
#include "agent/AgentSession.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QTimer>
#include <atomic>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace vsr {
namespace {
constexpr int BufferLimit = 131072;
constexpr int DefaultOutputLimit = 16000;
constexpr int MaximumOutputLimit = 60000;
std::atomic<qint64> nextProcessId{1};
QJsonObject failed(const QString &message) { return {{"ok", false}, {"error", message}}; }
QJsonObject parameter(const char *type, const QString &description = {})
{
    QJsonObject value{{"type", type}}; if (!description.isEmpty()) value.insert("description", description); return value;
}
QJsonObject schema(const QJsonObject &properties, const QJsonArray &required = {})
{
    return {{"type", "object"}, {"properties", properties}, {"required", required}, {"additionalProperties", false}};
}
QJsonObject processIdParameter()
{
    return {{"type", "integer"}, {"minimum", 1}, {"maximum", 9007199254740991.0}};
}
int outputLimit(const QJsonObject &arguments)
{
    if (arguments.contains("max_output_chars")) return qBound(256, arguments.value("max_output_chars").toInt(DefaultOutputLimit), MaximumOutputLimit);
    return qBound(256, int(qBound(64.0, arguments.value("max_output_tokens").toDouble(DefaultOutputLimit / 4), double(MaximumOutputLimit / 4)) * 4), MaximumOutputLimit);
}
QProcessEnvironment environment(const QJsonObject &overrides)
{
    auto env = QProcessEnvironment::systemEnvironment();
    // QProcess output is decoded as UTF-8. This affects child Python stdio only;
    // per-command overrides below remain authoritative.
    env.insert("PYTHONIOENCODING", "utf-8");
    for (auto it = overrides.begin(); it != overrides.end(); ++it) env.insert(it.key(), it.value().toString());
    const auto base = QCoreApplication::applicationDirPath();
    // Full environment access honors the caller's PATH. Embedded runtimes are
    // fallbacks: their reduced Python installation must not hide host Python.
    auto paths = env.value("PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts);
#ifdef Q_OS_WIN
    paths.append(QDir::toNativeSeparators(QDir(qEnvironmentVariable("SystemRoot", "C:/Windows")).filePath("System32")));
#endif
    for (const auto &relative : {".", "runtime/agent", "runtime/agent/npm", "runtime/agent/npm/bin", "runtime/python", "runtime/python/Scripts", "runtime/tools"}) {
        const auto path = QDir(base).absoluteFilePath(relative); if (QFileInfo(path).isDir()) paths.append(QDir::toNativeSeparators(path));
    }
    paths.removeDuplicates(); env.insert("PATH", paths.join(QDir::listSeparator()));
    return env;
}
QString findProgram(const QString &name, const QProcessEnvironment &env)
{
    return QStandardPaths::findExecutable(name, env.value("PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts));
}
QString npmCli(const QString &name, const QProcessEnvironment &env)
{
    const auto node = findProgram("node", env);
    if (node.isEmpty()) return {};
    const auto base = QFileInfo(node).absolutePath();
    for (const auto &relative : {"npm/bin/", "node_modules/npm/bin/"}) {
        const auto file = QDir(base).filePath(QString::fromLatin1(relative) + name + "-cli.js");
        if (QFileInfo(file).isFile()) return file;
    }
    return {};
}
QString psQuoted(QString value) { value.replace('\'', "''"); return "'" + value + "'"; }
QString powershellScript(const QString &command, const QProcessEnvironment &env)
{
    QString script = "[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); $OutputEncoding=[Console]::OutputEncoding; ";
    const auto node = findProgram("node", env);
    for (const auto &name : {QStringLiteral("npm"), QStringLiteral("npx")}) {
        const auto cli = npmCli(name, env);
        if (!node.isEmpty() && !cli.isEmpty()) script += "function " + name + " { & " + psQuoted(node) + " " + psQuoted(cli) + " @args }; ";
    }
    // The user's block retains normal PowerShell semantics, including explicit exit.
    script += "$global:LASTEXITCODE=$null; & {\n" + command + "\n}; $vsrCommandOk=$?; if ($LASTEXITCODE -is [int]) { exit $LASTEXITCODE }; if (-not $vsrCommandOk) { exit 1 }; exit 0";
    return script;
}
QString encodedPowershell(const QString &script)
{
    QByteArray utf16; utf16.reserve(script.size() * 2);
    for (const auto ch : script) { utf16.append(char(ch.unicode() & 255)); utf16.append(char(ch.unicode() >> 8)); }
    return QString::fromLatin1(utf16.toBase64());
}
}

struct AgentLocalProcess::Process {
    struct Waiter { QPointer<QTimer> timer; Completion completion; int limit; };
    qint64 id = 0, pid = 0, discarded = 0;
    QPointer<QProcess> process;
    QPointer<QTimer> timeout;
    QElapsedTimer elapsed;
    QString program, workdir, out, err, error;
    QStringDecoder outDecoder{QStringDecoder::Utf8}, errDecoder{QStringDecoder::Utf8};
    QList<Waiter> waiters;
    bool background = false, finished = false, timedOut = false, cancelled = false, truncated = false;
    int exitCode = 0;
    void *job = nullptr, *nativeInfo = nullptr;
};

AgentLocalProcess::AgentLocalProcess(AgentSession *session) : QObject(session), session_(session)
{
    Q_ASSERT(session);
    connect(session, &AgentSession::toolExecutionCancelled, this, &AgentLocalProcess::cancelForeground);
    connect(session, &AgentSession::pendingMessagesChanged, this, [this] { if (session_ && session_->pendingPaused()) cancelForeground(); });
    connect(session, &AgentSession::finished, this, [this] { if (session_ && session_->pendingPaused()) cancelForeground(); });
    connect(session, &AgentSession::accessModeChanged, this, [this](AgentAccessMode mode) { if (mode == AgentAccessMode::SoftwareOnly) shutdown(); });
}

AgentLocalProcess::~AgentLocalProcess()
{
    destroying_ = true; shutdown();
    for (const auto &entry : processes_) if (entry->process) {
        entry->process->disconnect(this); if (entry->process->state() != QProcess::NotRunning) entry->process->waitForFinished(1000);
        // A completion may destroy its owner inside QProcess::finished. Keep
        // that signal's sender alive until the current event has returned.
        entry->process->setParent(nullptr); entry->process->deleteLater();
    }
}

void AgentLocalProcess::registerTools()
{
    if (!session_) return;
    const QPointer<AgentLocalProcess> guard(this);
    QJsonObject execProperties{
        {"cmd", parameter("string", "PowerShell command text; use executable + arguments to bypass a shell.")},
        {"executable", parameter("string")}, {"arguments", QJsonObject{{"type", "array"}, {"items", parameter("string")}}},
        {"shell", parameter("string", "Optional shell executable. Default: PowerShell without a profile.")},
        {"workdir", parameter("string", "Working directory; defaults to this session's directory. May be outside the project.")},
        {"env", QJsonObject{{"type", "object"}, {"additionalProperties", parameter("string")}}},
        {"timeout_ms", parameter("integer", "0 disables the deadline. Default: 120000, or 0 for explicit background jobs.")},
        {"yield_time_ms", parameter("integer", "Return after at most this many milliseconds, retaining a running session_id. Default 1000; maximum 30000.")},
        {"max_output_chars", parameter("integer")}, {"max_output_tokens", parameter("integer")},
        {"background", parameter("boolean", "Return immediately and keep the process until it exits, is terminated, or this session closes/revokes environment access.")}
    };
    session_->registerAsyncTool("exec_command", "Run local commands with full environment access. No directory sandbox. Returns separate stdout/stderr, exit_code or a running session_id; poll using write_stdin. Keep long-running servers in the foreground of the launched process and set background=true; detached descendants are cleaned when it exits.", schema(execProperties), [guard](const QJsonObject &args, Completion completion) {
        if (guard) guard->execute(args, std::move(completion)); else completion(failed("Process owner no longer exists."));
    });
    session_->registerAsyncTool("write_stdin", "Poll/wait for a managed process or write UTF-8 text to its stdin. An empty chars polls. Only session_ids owned by this conversation are accepted.", schema({
        {"session_id", processIdParameter()}, {"chars", parameter("string")}, {"close_stdin", parameter("boolean")},
        {"yield_time_ms", parameter("integer")}, {"max_output_chars", parameter("integer")}, {"max_output_tokens", parameter("integer")}
    }, {"session_id"}), [guard](const QJsonObject &args, Completion completion) {
        if (guard) guard->write(args, std::move(completion)); else completion(failed("Process owner no longer exists."));
    });
    session_->registerAsyncTool("terminate_process", "Terminate a process session and its descendants owned by this conversation.", schema({{"session_id", processIdParameter()}}, {"session_id"}), [guard](const QJsonObject &args, Completion completion) {
        if (guard) guard->terminate(args, std::move(completion)); else completion(failed("Process owner no longer exists."));
    });
    session_->registerTool("list_processes", "List managed processes owned by this conversation, without exposing environment variables or full command lines.", schema({}), [guard](const QJsonObject &) {
        return guard ? QJsonObject{{"ok", true}, {"processes", guard->processes()}} : failed("Process owner no longer exists.");
    });
    for (const auto &name : {"exec_command", "write_stdin", "terminate_process", "list_processes"}) {
        session_->setToolDelegationAllowed(name, false);
        session_->setToolEnvironmentAccess(name, true, QString::fromLatin1(name) != "list_processes");
    }
}

void AgentLocalProcess::execute(const QJsonObject &arguments, Completion completion)
{
    if (!session_ || session_->accessMode() != AgentAccessMode::FullEnvironment) { completion(failed("Full environment access is required.")); return; }
    const auto command = arguments.value("cmd").toString();
    QString program = arguments.value("executable").toString();
    QStringList args;
    if (program.isEmpty() == command.trimmed().isEmpty()) { completion(failed("Provide either cmd or executable, but not both.")); return; }
    if (!program.isEmpty() && arguments.contains("shell")) { completion(failed("shell cannot be combined with executable.")); return; }
    for (const auto &arg : arguments.value("arguments").toArray()) args.append(arg.toString());
    if (program.isEmpty() && !args.isEmpty()) { completion(failed("arguments requires executable.")); return; }
    const auto overrides = arguments.value("env").toObject();
    for (auto it = overrides.begin(); it != overrides.end(); ++it) {
        if (it.key().isEmpty() || it.key().contains('=') || it.key().contains(QChar(0)) || !it.value().isString() || it.value().toString().contains(QChar(0))) {
            completion(failed("env must contain valid text names and values without NUL.")); return;
        }
    }
    const auto env = environment(overrides);
    if (program.isEmpty()) {
        program = arguments.value("shell").toString();
#ifdef Q_OS_WIN
        if (program.isEmpty()) program = findProgram("pwsh", env);
        if (program.isEmpty()) program = QDir(qEnvironmentVariable("SystemRoot", "C:/Windows")).filePath("System32/WindowsPowerShell/v1.0/powershell.exe");
#else
        if (program.isEmpty()) program = "/bin/sh";
#endif
        const auto name = QFileInfo(program).baseName().toLower();
        if (name == "powershell" || name == "pwsh") args = {"-NoLogo", "-NoProfile", "-NonInteractive", "-EncodedCommand", encodedPowershell(powershellScript(command, env))};
        else if (name == "cmd") args = {"/D", "/S", "/C", command};
        else args = {"-c", command};
    } else {
        const auto name = QFileInfo(program).fileName().toLower();
        if (!program.contains('/') && !program.contains('\\') && (name == "npm" || name == "npm.cmd" || name == "npx" || name == "npx.cmd")) {
            const auto cli = npmCli(name.startsWith("npx") ? "npx" : "npm", env);
            if (!cli.isEmpty()) { program = findProgram("node", env); args.prepend(cli); }
        }
    }
    if (program.contains(QChar(0)) || command.contains(QChar(0))) { completion(failed("Command text cannot contain NUL.")); return; }
    for (const auto &arg : args) if (arg.contains(QChar(0))) { completion(failed("Arguments cannot contain NUL.")); return; }
    const auto requested = arguments.value("workdir").toString();
    const auto workdir = QDir::cleanPath(QDir(session_->workingDirectory()).absoluteFilePath(requested.isEmpty() ? "." : requested));
    if (!QFileInfo(workdir).isDir() || requested.contains(QChar(0))) { completion(failed("Working directory does not exist.")); return; }
    if (QDir::isRelativePath(program)) {
        if (program.contains('/') || program.contains('\\')) program = QDir(workdir).absoluteFilePath(QDir::fromNativeSeparators(program));
        else { const auto resolved = findProgram(program, env); if (!resolved.isEmpty()) program = resolved; }
    }
    prune();
    const auto entry = std::make_shared<Process>(); entry->id = nextProcessId.fetch_add(1); entry->program = program; entry->workdir = workdir; entry->background = arguments.value("background").toBool();
#ifdef Q_OS_WIN
    const HANDLE job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{}; limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        if (job) CloseHandle(job); completion(failed("Cannot create the process-tree cleanup boundary; no command was started.")); return;
    }
    entry->job = job;
#endif
    auto *process = new QProcess(this); entry->process = process; entry->elapsed.start(); processes_.insert(entry->id, entry);
    process->setProcessEnvironment(env); process->setWorkingDirectory(workdir); process->setProcessChannelMode(QProcess::SeparateChannels);
#ifdef Q_OS_WIN
    process->setCreateProcessArgumentsModifier([entry](QProcess::CreateProcessArguments *native) {
        native->flags |= CREATE_SUSPENDED | CREATE_NO_WINDOW;
        entry->nativeInfo = native->processInformation;
    });
#endif
    connect(process, &QProcess::started, this, [this, entry] {
        entry->pid = entry->process->processId();
        if (entry->cancelled || entry->timedOut) { entry->nativeInfo = nullptr; stop(entry, entry->timedOut ? "timeout" : "cancelled"); return; }
#ifdef Q_OS_WIN
        const auto *native = static_cast<PROCESS_INFORMATION *>(entry->nativeInfo);
        const bool assigned = native && AssignProcessToJobObject(static_cast<HANDLE>(entry->job), native->hProcess);
        const bool resumed = assigned && ResumeThread(native->hThread) != DWORD(-1);
        entry->nativeInfo = nullptr;
        if (!resumed) { entry->error = "Cannot attach the suspended process to its cleanup boundary; command was not executed."; stop(entry, "boundary_failure"); return; }
#endif
        const QPointer<AgentLocalProcess> guard(this);
        if (entry->background) settle(entry);
        if (!guard) return;
        emit processesChanged();
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [this, entry] { read(entry); });
    connect(process, &QProcess::readyReadStandardError, this, [this, entry] { read(entry); });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this, entry](int code, QProcess::ExitStatus status) {
        read(entry); entry->finished = true; entry->exitCode = code;
        if (status == QProcess::CrashExit && entry->error.isEmpty() && !entry->cancelled && !entry->timedOut) entry->error = "Process crashed.";
        if (entry->timeout) entry->timeout->stop();
#ifdef Q_OS_WIN
        if (entry->job) { CloseHandle(static_cast<HANDLE>(entry->job)); entry->job = nullptr; }
#endif
        const QPointer<AgentLocalProcess> guard(this); settle(entry); if (!guard) return;
        if (entry->process) entry->process->deleteLater(); emit processesChanged();
    });
    connect(process, &QProcess::errorOccurred, this, [this, entry](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        entry->error = entry->process->errorString(); entry->finished = true; entry->exitCode = -1; entry->nativeInfo = nullptr;
        if (entry->timeout) entry->timeout->stop();
#ifdef Q_OS_WIN
        if (entry->job) { CloseHandle(static_cast<HANDLE>(entry->job)); entry->job = nullptr; }
#endif
        const QPointer<AgentLocalProcess> guard(this); settle(entry); if (!guard) return;
        if (entry->process) entry->process->deleteLater(); emit processesChanged();
    });
    const int deadline = qBound(0, arguments.value("timeout_ms").toInt(entry->background ? 0 : 120000), 24 * 60 * 60 * 1000);
    if (deadline) {
        auto *timer = new QTimer(this); timer->setSingleShot(true); entry->timeout = timer;
        connect(timer, &QTimer::timeout, this, [this, entry] { stop(entry, "timeout"); }); timer->start(deadline);
    }
    // Starting is asynchronous; a background reply still waits for successful
    // suspended-start/job assignment rather than claiming a failed launch worked.
    const QPointer<AgentLocalProcess> guard(this);
    process->start(program, args, QIODevice::ReadWrite);
    if (!guard) { completion(failed("Process owner no longer exists.")); return; }
    wait(entry, entry->background ? (entry->pid ? 0 : 30000) : qBound(0, arguments.value("yield_time_ms").toInt(1000), 30000), outputLimit(arguments), std::move(completion));
}

void AgentLocalProcess::read(const std::shared_ptr<Process> &entry)
{
    if (!entry->process || !entry->process->isOpen()) return;
    const auto append = [entry](QString &buffer, const QString &text) {
        buffer += text;
        if (buffer.size() > BufferLimit) { const auto extra = buffer.size() - BufferLimit; buffer.remove(0, extra); entry->discarded += extra; entry->truncated = true; }
    };
    append(entry->out, entry->outDecoder.decode(entry->process->readAllStandardOutput()));
    append(entry->err, entry->errDecoder.decode(entry->process->readAllStandardError()));
}

QJsonObject AgentLocalProcess::result(const std::shared_ptr<Process> &entry, int limit)
{
    read(entry);
    int outLimit = qMin(limit, int(entry->out.size()));
    int errLimit = qMin(limit - outLimit, int(entry->err.size()));
    if (!entry->err.isEmpty() && !entry->out.isEmpty()) {
        errLimit = qMin(int(entry->err.size()), limit / 2); outLimit = qMin(int(entry->out.size()), limit - errLimit);
    }
    const auto out = entry->out.left(outLimit), err = entry->err.left(errLimit);
    const qint64 omitted = entry->out.size() + entry->err.size() - out.size() - err.size();
    const bool truncated = entry->truncated || omitted > 0;
    QJsonObject value{{"ok", entry->error.isEmpty() && !entry->timedOut && !entry->cancelled && (!entry->finished || entry->exitCode == 0)},
        {"session_id", double(entry->id)}, {"pid", double(entry->pid)}, {"running", !entry->finished},
        {"exit_code", entry->finished ? QJsonValue(entry->exitCode) : QJsonValue(QJsonValue::Null)},
        {"stdout", out}, {"stderr", err}, {"output", out + (out.isEmpty() || err.isEmpty() ? QString() : QStringLiteral("\n")) + err},
        {"output_truncated", truncated}, {"discarded_characters", double(entry->discarded + omitted)},
        {"wall_time_seconds", entry->elapsed.elapsed() / 1000.0}, {"timed_out", entry->timedOut}, {"cancelled", entry->cancelled}};
    if (!entry->error.isEmpty()) value.insert("error", entry->error);
    else if (entry->timedOut) value.insert("error", "Process deadline exceeded.");
    else if (entry->cancelled) value.insert("error", "Process was cancelled or terminated.");
    entry->out.clear(); entry->err.clear(); entry->truncated = false; entry->discarded = 0;
    return value;
}

void AgentLocalProcess::wait(const std::shared_ptr<Process> &entry, int milliseconds, int limit, Completion completion)
{
    if (entry->finished || (milliseconds == 0 && entry->pid != 0)) { completion(result(entry, limit)); return; }
    auto *timer = new QTimer(this); timer->setSingleShot(true);
    entry->waiters.append({timer, std::move(completion), limit});
    connect(timer, &QTimer::timeout, this, [this, entry, timer] {
        for (int i = 0; i < entry->waiters.size(); ++i) if (entry->waiters[i].timer == timer) {
            auto waiter = entry->waiters.takeAt(i); timer->deleteLater();
            if (!destroying_) waiter.completion(result(entry, waiter.limit)); break;
        }
    });
    timer->start(milliseconds == 0 ? 1 : milliseconds);
}

void AgentLocalProcess::settle(const std::shared_ptr<Process> &entry)
{
    const QPointer<AgentLocalProcess> guard(this);
    auto waiters = std::move(entry->waiters); entry->waiters.clear();
    for (auto &waiter : waiters) {
        if (waiter.timer) { waiter.timer->stop(); waiter.timer->deleteLater(); }
        if (!destroying_) waiter.completion(result(entry, waiter.limit));
        if (!guard) return;
    }
}

void AgentLocalProcess::write(const QJsonObject &arguments, Completion completion)
{
    const auto entry = processes_.value(qint64(arguments.value("session_id").toDouble()));
    if (!entry) { completion(failed("Unknown process session_id for this conversation.")); return; }
    if (!entry->finished && entry->process) {
        const auto text = arguments.value("chars").toString().toUtf8();
        if (text.size() > 1024 * 1024 || entry->process->bytesToWrite() > 1024 * 1024) { completion(failed("Pending stdin exceeds 1 MiB; wait for the process to consume it.")); return; }
        if (!text.isEmpty() && entry->process->write(text) < 0) { completion(failed("Process stdin is closed.")); return; }
        if (arguments.value("close_stdin").toBool()) entry->process->closeWriteChannel();
    } else if (!arguments.value("chars").toString().isEmpty()) { completion(failed("Cannot write to a completed process.")); return; }
    wait(entry, qBound(0, arguments.value("yield_time_ms").toInt(1000), 30000), outputLimit(arguments), std::move(completion));
}

void AgentLocalProcess::terminate(const QJsonObject &arguments, Completion completion)
{
    const auto entry = processes_.value(qint64(arguments.value("session_id").toDouble()));
    if (!entry) { completion(failed("Unknown process session_id for this conversation.")); return; }
    stop(entry, "terminated"); wait(entry, 1000, DefaultOutputLimit, std::move(completion));
}

void AgentLocalProcess::stop(const std::shared_ptr<Process> &entry, const QString &reason)
{
    if (entry->finished) return;
    if (reason == "timeout") entry->timedOut = true; else if (reason != "boundary_failure") entry->cancelled = true;
#ifdef Q_OS_WIN
    if (entry->job) { TerminateJobObject(static_cast<HANDLE>(entry->job), 1); CloseHandle(static_cast<HANDLE>(entry->job)); entry->job = nullptr; }
#endif
    if (entry->process) entry->process->kill();
}

void AgentLocalProcess::cancelForeground()
{
    for (const auto &entry : processes_) if (!entry->background) stop(entry, "cancelled");
}

void AgentLocalProcess::shutdown()
{
    for (const auto &entry : processes_) stop(entry, "cancelled");
}

QJsonArray AgentLocalProcess::processes() const
{
    QJsonArray result;
    for (const auto &entry : processes_) result.append(QJsonObject{{"session_id", double(entry->id)}, {"pid", double(entry->pid)},
        {"program", QFileInfo(entry->program).fileName()}, {"workdir", entry->workdir}, {"background", entry->background},
        {"running", !entry->finished}, {"exit_code", entry->finished ? QJsonValue(entry->exitCode) : QJsonValue(QJsonValue::Null)},
        {"wall_time_seconds", entry->elapsed.elapsed() / 1000.0}});
    return result;
}

void AgentLocalProcess::prune()
{
    for (auto it = processes_.begin(); it != processes_.end() && processes_.size() >= 128;) {
        if (it.value()->finished) { if (it.value()->timeout) it.value()->timeout->deleteLater(); it = processes_.erase(it); } else ++it;
    }
}
}
