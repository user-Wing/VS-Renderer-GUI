#include "app/MainWindow.h"
#include "agent/AgentInput.h"
#include "agent/AgentInstructions.h"
#include "agent/AgentMcp.h"
#include "agent/AgentModelCapabilities.h"
#include "agent/AgentSession.h"
#include "agent/AgentTaskQueue.h"
#include "ui/AgentPage.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace vsr {
void MainWindow::initializeAgentInteractions()
{
    agentModels_ = new AgentModelCapabilities(this);
    connect(agentPage_, &AgentPage::providerModelChanged, agentModels_, [this](const AgentProvider &provider) {
        agentModels_->resolve(provider.baseUrl, provider.apiKey, provider.model, true);
    });
    connect(agentModels_, &AgentModelCapabilities::resolved, agentPage_, [this](const QString &base, const QString &model, const QJsonObject &capability) {
        if (agentSession_->provider().baseUrl == base && agentSession_->provider().model == model) agentPage_->setModelCapabilities(capability);
    });
    connect(agentPage_, &AgentPage::messageSubmitted, this, &MainWindow::submitAgentInput);
    connect(agentPage_, &AgentPage::commandSubmitted, this, &MainWindow::submitAgentInput);
    connect(agentPage_, &AgentPage::completionsRequested, this, [this](const QString &) { refreshAgentCompletions(); });
    connect(agentPage_, &AgentPage::mentionActionRequested, this, [this](const QString &action) {
        if (action != "file") return;
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("选择要引用的文本文件"), {}, QStringLiteral("文本与代码 (*.md *.txt *.json *.vpy *.py *.cpp *.h *.ini *.yaml *.yml *.csv);;所有文件 (*.*)"));
        if (!path.isEmpty()) agentPage_->insertInputText(AgentInput::mention("file", path)+" ");
    });
    connect(agentInstructions_, &AgentInstructions::changed, this, &MainWindow::refreshAgentCompletions);
    connect(agentQueue_, &AgentTaskQueue::changed, this, &MainWindow::refreshAgentCompletions);
    connect(agentMcp_, &AgentMcpManager::toolsChanged, this, &MainWindow::refreshAgentCompletions);
    refreshAgentCompletions();
    const auto provider = agentSession_->provider();
    if (!provider.model.isEmpty() && !provider.baseUrl.isEmpty()) agentModels_->resolve(provider.baseUrl, provider.apiKey, provider.model);
}
void MainWindow::refreshAgentCompletions()
{
    auto items = AgentInput::commands();
    items.append(QJsonObject{{"kind", "mention"}, {"label", QStringLiteral("@workspace 当前工作区")}, {"insertText", "@workspace "}, {"description", QStringLiteral("引用源、滤镜图和实际状态")}});
    items.append(QJsonObject{{"kind", "mention"}, {"label", QStringLiteral("@file 选择文本文件…")}, {"insertText", ""}, {"action", "file"}, {"description", QStringLiteral("选择后在发送时附上文件内容")}});
    for (const auto &value : agentInstructions_->skills()) {
        const auto skill = value.toObject(); if (!skill.value("enabled").toBool()) continue;
        const auto name = skill.value("name").toString(), description = skill.value("description").toString();
        items.append(QJsonObject{{"kind", "mention"}, {"label", "@skill:"+name}, {"insertText", AgentInput::mention("skill", name)+" "}, {"description", description}});
        items.append(QJsonObject{{"kind", "command"}, {"label", "/"+name}, {"insertText", "/"+name+" "}, {"description", description}});
    }
    for (const auto &value : agentQueue_->tasks()) {
        const auto task = value.toObject(); const auto id = task.value("id").toString();
        items.append(QJsonObject{{"kind", "mention"}, {"label", "@agent:"+task.value("name").toString()}, {"insertText", AgentInput::mention("agent", id)+" "}, {"description", task.value("state").toString()}});
    }
    for (const auto &server : agentMcp_->servers()) items.append(QJsonObject{{"kind", "mention"}, {"label", "@mcp:"+server.name}, {"insertText", AgentInput::mention("mcp", server.id)+" "}, {"description", agentMcp_->state(server.id)}});
    agentPage_->setCompletionItems(items);
}
void MainWindow::submitAgentInput(const QString &text)
{
    auto input = AgentInput::parse(text);
    const auto showError = [this](const QString &error) { agentPage_->showNotice(error, true); };
    QString body = text;
    if (!input.command.isEmpty()) {
        const auto command = input.command.toLower();
        if (command == "new" || command == "clear") {
            if (agentSession_->isBusy()) showError(QStringLiteral("当前轮次正在执行；请停止或等待完成后新建会话。"));
            else agentPage_->startNewConversation();
            return;
        }
        if (command == "compact") {
            if (agentSession_->isBusy()) showError(QStringLiteral("当前轮次正在执行；可继续输入补充内容，整理上下文请在本轮结束后操作。"));
            else agentSession_->compactContext();
            return;
        }
        if (command == "status" || command == "usage") { agentPage_->openUsage(); return; }
        if (command == "model" || command == "reasoning") {
            if (agentSession_->isBusy()) { showError(QStringLiteral("当前轮次正在执行；模型与思考设置请在本轮结束后调整。")); return; }
            if (command == "reasoning") agentPage_->selectReasoning(input.arguments.isEmpty() ? "auto" : input.arguments);
            else if (input.arguments.isEmpty()) agentPage_->openModelMenu();
            else agentPage_->selectModel(input.arguments);
            return;
        }
        if (command == "stop") { agentSession_->cancel(); return; }
        if (command == "resume") { agentSession_->resumePending(); return; }
        if (command == "mcp") { emit agentPage_->mcpSettingsRequested(); return; }
        if (command == "agents" || command == "queue") { emit agentPage_->queueRequested(); return; }
        if (command == "skills" && input.arguments.isEmpty()) { emit agentPage_->skillsSettingsRequested(); return; }
        if (command == "help") {
            QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("Agent 命令与引用")); auto *layout = new QVBoxLayout(&dialog); auto *view = new QTextBrowser(&dialog);
            QString help = QStringLiteral("# / 命令\n\n"); for (const auto &item : AgentInput::commands()) { const auto command = item.toObject(); help += "- `"+command.value("label").toString()+"`："+command.value("description").toString()+"\n"; }
            help += QStringLiteral("\n# @ 引用\n\n`@workspace` 当前工作区；`@skill:名称` 技能；`@file:\"绝对路径\"` 文本；`@agent:任务ID` 子会话；`@mcp:来源ID` 插件工具。\n\nEnter 发送，Ctrl+Enter 换行。运行中发送会排队，工具调用完成后接收。停止后可用 /resume 继续。代码块及 \\@ 按原文发送。\n");
            view->setMarkdown(help); layout->addWidget(view); auto *close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog); connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject); layout->addWidget(close); dialog.resize(680, 650); dialog.exec(); return;
        }
        if (command == "agent") {
            if (input.arguments.isEmpty()) { showError(QStringLiteral("请在 /agent 后写明任务，AI 会自动创建并协调子智能体。")); return; }
            body = QStringLiteral("请实际使用子智能体完成下面的任务，自行创建队列、分工、交流并汇总结果；无需让我手动配置：\n")+input.arguments;
        } else {
            auto name = input.command, task = input.arguments;
            if (command == "skills") { const auto separator = task.indexOf(QRegularExpression("\\s")); name = separator < 0 ? task : task.left(separator); task = separator < 0 ? QString() : task.mid(separator+1); }
            bool found = false; for (const auto &skill : agentInstructions_->skills()) found |= skill.toObject().value("name").toString() == name;
            if (!found) { showError(QStringLiteral("未知命令 /%1；输入 / 查看可用命令。" ).arg(input.command)); return; }
            body = AgentInput::mention("skill", name)+" "+(task.isEmpty() ? QStringLiteral("请按此技能处理当前任务。") : task);
        }
    }
    input = AgentInput::expand(body, [this](const QString &kind, const QString &name) -> QJsonObject {
        if (kind == "file") return AgentInput::readFile(name);
        if (kind == "workspace" || kind == "source") return agentSession_->executeTool("get_state", {});
        if (kind == "skill") return agentInstructions_->readSkill(name);
        if (kind == "agent") {
            const auto task = agentQueue_->task(name); if (task.contains("error") && !task.contains("id")) return task;
            return {{"ok", true}, {"id", name}, {"name", task.value("name")}, {"state", task.value("state")}, {"result", task.value("result")}, {"errorMessage", task.value("error")}, {"conversation", task.value("messages")}};
        }
        if (kind == "mcp") {
            for (const auto &server : agentMcp_->servers()) if (server.id == name || server.name == name) {
                QJsonArray tools; for (const auto &tool : agentMcp_->serverTools(server.id)) { const auto item = tool.toObject(); tools.append(QJsonObject{{"name", item.value("name")}, {"description", item.value("description")}}); }
                return {{"ok", true}, {"name", server.name}, {"state", agentMcp_->state(server.id)}, {"tools", tools}};
            }
            return {{"ok", false}, {"error", QStringLiteral("MCP 来源不存在。")}};
        }
        return {{"ok", false}, {"error", QStringLiteral("不支持的引用类型。")}};
    });
    if (!input.error.isEmpty()) { showError(input.error); return; }
    QString error; if (!agentSession_->provider().validate(&error)) { showError(error); return; }
    agentSession_->sendWithContext(text.trimmed(), input.modelText);
}
}
