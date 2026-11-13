#include "agent/AgentNodeRuntime.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace vsr {
namespace {
QString systemNode()
{
    const auto configured = qEnvironmentVariable("VSR_AGENT_NODE");
    if (QFileInfo(configured).isExecutable()) return configured;
    const auto found = QStandardPaths::findExecutable("node");
    if (!found.isEmpty()) return found;
    const auto installed = QDir(qEnvironmentVariable("ProgramFiles", "C:/Program Files")).filePath("nodejs/node.exe");
    return QFileInfo(installed).isExecutable() ? installed : QString();
}
}

AgentNodeRuntime::AgentNodeRuntime(const QString &directory, QObject *parent) : QObject(parent), directory_(directory), install_(this)
{
    connect(&install_, &QProcess::readyReadStandardOutput, this, [this] { install_.readAllStandardOutput(); });
    connect(&install_, &QProcess::readyReadStandardError, this, [this] { install_.readAllStandardError(); });
    connect(&install_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) emit installFinished(false, QStringLiteral("无法启动本地 Node/npm。"));
    });
    connect(&install_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        const bool ok = code == 0 && status == QProcess::NormalExit && ready();
        emit installFinished(ok, ok ? QStringLiteral("MCP npm 组件已安装。") : QStringLiteral("npm 安装失败，请检查网络与运行目录权限。"));
    });
}

QString AgentNodeRuntime::nodePath() const
{
    const auto portable = QDir(directory_).filePath("node.exe");
    return QFileInfo(portable).isExecutable() ? portable : systemNode();
}

QString AgentNodeRuntime::bridgePath() const { return QDir(directory_).filePath("bridge.mjs"); }

bool AgentNodeRuntime::ready() const
{
    return !nodePath().isEmpty() && QFileInfo::exists(bridgePath())
        && QFileInfo::exists(QDir(directory_).filePath("node_modules/@modelcontextprotocol/client/package.json"));
}

void AgentNodeRuntime::install(const QString &sourceDirectory)
{
    if (isInstalling()) return;
    QString source = sourceDirectory;
    if (source.isEmpty()) source = qEnvironmentVariable("VSR_AGENT_RUNTIME_SOURCE");
    if (source.isEmpty() && QFileInfo::exists(QDir(directory_).filePath("package-lock.json"))) source = directory_;
    if (source.isEmpty()) source = QDir::current().filePath("tools/agent-runtime");
    const auto node = nodePath();
    if (node.isEmpty()) { emit installFinished(false, QStringLiteral("未找到 Node.js 20+，请安装 Node.js LTS 后重试。")); return; }
    auto npm = QDir(QFileInfo(node).absolutePath()).filePath("npm/bin/npm-cli.js");
    if(!QFileInfo::exists(npm))npm=QDir(QFileInfo(node).absolutePath()).filePath("node_modules/npm/bin/npm-cli.js");
    if (!QFileInfo::exists(npm)) { emit installFinished(false, QStringLiteral("Node.js 目录中缺少 npm。")); return; }
    if (!QDir().mkpath(directory_)) { emit installFinished(false, QStringLiteral("无法创建 Agent 运行目录。")); return; }
    for (const auto &name : {QStringLiteral("package.json"), QStringLiteral("package-lock.json"), QStringLiteral("bridge.mjs"), QStringLiteral("install.mjs")}) {
        const auto input = QDir(source).filePath(name), output = QDir(directory_).filePath(name);
        if (!QFileInfo::exists(input)) { emit installFinished(false, QStringLiteral("缺少已锁定的 Agent npm 组件文件。")); return; }
        if (QFileInfo(input).absoluteFilePath() == QFileInfo(output).absoluteFilePath()) continue;
        QFile file(input); if (!file.open(QIODevice::ReadOnly)) { emit installFinished(false, QStringLiteral("无法读取 Agent npm 组件文件。")); return; }
        QFile target(output); if (!target.open(QIODevice::WriteOnly) || target.write(file.readAll()) < 0) { emit installFinished(false, QStringLiteral("无法准备 Agent npm 组件文件。")); return; }
    }
    const auto portableNode = QDir(directory_).filePath("node.exe");
    if (!QFileInfo::exists(portableNode) && !QFile::copy(node, portableNode)) { emit installFinished(false, QStringLiteral("无法准备便携 Node.js。")); return; }
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("PATH", QFileInfo(node).absolutePath() + QDir::listSeparator() + environment.value("PATH"));
    install_.setProcessEnvironment(environment); install_.setWorkingDirectory(directory_);
    install_.start(node, {QDir(directory_).filePath("install.mjs"),npm});
    emit statusChanged(QStringLiteral("正在安装锁定的 MCP npm 组件…"));
}

}
