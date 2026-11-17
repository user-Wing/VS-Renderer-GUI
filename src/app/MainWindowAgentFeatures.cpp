#include "app/MainWindow.h"
#include "agent/AgentSession.h"
#include "agent/AgentInstructions.h"
#include "agent/AgentMcp.h"
#include "agent/AgentTaskQueue.h"
#include "agent/AgentLocalFiles.h"
#include "agent/AgentLocalProcess.h"
#include "agent/AgentLocalImages.h"
#include "ui/AgentPage.h"
#include "ui/AgentInstructionsDialog.h"
#include "ui/AgentMcpDialog.h"
#include "ui/AgentTaskDialog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QSettings>
#include <QTimer>
#include <QSysInfo>

namespace vsr {
void MainWindow::initializeAgentFeatures()
{
    const auto directory = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent");
    agentInstructions_ = new AgentInstructions(directory, this);
    agentMcp_ = new AgentMcpManager(QDir(directory).filePath("mcp.json"), QDir(QCoreApplication::applicationDirPath()).filePath("runtime/agent"), this);
    QString error; if (!agentMcp_->load(&error)) QTimer::singleShot(0, this, [this, error] { setStatus(error, true); });
    agentQueue_=configureAgentConversation(agentSession_,agentSession_->property("agentConversationId").toString());
    agentPage_ = new AgentPage(agentSession_, pages_); pages_->addWidget(agentPage_);
    agentPage_->setSessionFactory([this](const QString &id){auto *session=new AgentSession(this);registerAgentWorkspace(session);configureAgentConversation(session,id);return session;});
    connect(agentPage_,&AgentPage::activeSessionChanged,this,[this](AgentSession *session){agentSession_=session;agentQueue_=session->findChild<AgentTaskQueue*>();agentPage_->setQueueSummary(agentQueue_->summary());refreshAgentCompletions();});
    connect(agentPage_, &AgentPage::skillsSettingsRequested, this, [this] { AgentInstructionsDialog dialog(agentInstructions_, agentPage_); dialog.exec(); });
    connect(agentPage_, &AgentPage::mcpSettingsRequested, this, [this] { AgentMcpDialog dialog(agentMcp_, agentPage_); dialog.exec(); });
    connect(agentPage_, &AgentPage::queueRequested, this, [this] { AgentTaskDialog dialog(agentQueue_, agentSession_, agentPage_); dialog.exec(); });
    connect(agentMcp_, &AgentMcpManager::errorOccurred, this, [this](const QString &message) { setStatus(message, true); });
    agentPage_->setQueueSummary(agentQueue_->summary());
    initializeAgentInteractions();
    QTimer::singleShot(0, agentMcp_, &AgentMcpManager::startEnabled);
}
AgentTaskQueue *MainWindow::configureAgentConversation(AgentSession *session,const QString &id)
{
    const auto refreshPrompt = [this,session] { session->setSystemPrompt(agentInstructions_->systemPrompt() + QStringLiteral(
        "\n\n视频画质初检：视觉模型用 inspect_video_frame 按用户指定帧号或秒数直接读取VS帧。先检查原片并汇报缺陷的位置、程度、置信度及抽样范围，再建议滤镜；未经用户要求不要自动修改处理链。"
        "检查色带、噪点、色块、振铃、锯齿、模糊、过锐、压缩痕迹；细微问题用原尺寸crop复核，时间性问题需多个相邻帧，不能由单帧断言。"
        "只有工具成功提供的图像才能作为观察证据；没有视觉能力或抽帧失败时明确说明，不能凭文件名/参数编造瑕疵。")); };
    connect(agentInstructions_, &AgentInstructions::changed, session, refreshPrompt); refreshPrompt();
    const auto object = [](QJsonObject properties, QJsonArray required = {}) { return QJsonObject{{"type", "object"}, {"properties", properties}, {"required", required}, {"additionalProperties", false}}; };
    const auto registerEnvironment = [object](AgentSession *session) {
        auto *files=new AgentLocalFiles(session->workingDirectory(),session);files->registerTools(session);
        auto *processes=new AgentLocalProcess(session);processes->registerTools();
        auto *images=new AgentLocalImages(session);images->registerTools();
        connect(session,&AgentSession::conversationReset,processes,&AgentLocalProcess::shutdown);
        session->registerTool("get_environment",QStringLiteral("读取当前访问模式、工作目录、操作系统和程序目录，不包含环境变量密钥。"),object({}),[session](const QJsonObject &){return QJsonObject{{"ok",true},{"accessMode",session->accessMode()==AgentAccessMode::FullEnvironment?"full":"software"},{"workingDirectory",session->workingDirectory()},{"platform",QSysInfo::prettyProductName()},{"applicationDirectory",QCoreApplication::applicationDirPath()}};});
        session->setToolDelegationAllowed("get_environment",false);
        session->registerTool("set_working_directory",QStringLiteral("切换当前智能体的工作目录，不改变其他智能体。相对路径以当前目录为基准。"),object({{"path",QJsonObject{{"type","string"}}}},{"path"}),[session](const QJsonObject &args){const auto path=QDir(session->workingDirectory()).absoluteFilePath(args.value("path").toString());return session->setWorkingDirectory(path,true)?QJsonObject{{"ok",true},{"workingDirectory",session->workingDirectory()}}:QJsonObject{{"ok",false},{"error",QStringLiteral("工作目录不存在或不可用。")}};});
        session->setToolEnvironmentAccess("set_working_directory",true,false);session->setToolDelegationAllowed("set_working_directory",false);
    };
    registerEnvironment(session);
    session->setToolEnvironmentAccess("list_files",true,false);
    session->registerTool("list_skills", QStringLiteral("列出技能名称、说明和启用状态。技能正文按需read_skill读取。"), object({}), [this](const QJsonObject &) {
        QString error; if (!agentInstructions_->refresh(&error)) return QJsonObject{{"ok", false}, {"error", error}};
        return QJsonObject{{"ok", true}, {"skills", agentInstructions_->skills()}};
    });
    session->registerTool("read_skill", QStringLiteral("按名称读取已启用技能的SKILL.md，或其包内相对路径资料。"),
        object({{"name", QJsonObject{{"type", "string"}}}, {"path", QJsonObject{{"type", "string"}}}}, {"name"}), [this](const QJsonObject &args) {
            return agentInstructions_->readSkill(args.value("name").toString(), args.value("path").toString("SKILL.md"));
        });
    session->registerTool("get_token_usage", QStringLiteral("读取当前会话的实际Token累计、缓存/推理用量及预计上下文占用。"), object({}), [session](const QJsonObject &) { return session->usage(); });
    agentMcp_->registerTools(session);
    connect(agentMcp_, &AgentMcpManager::toolsChanged, session, [session] {
        for(const auto &name:QSettings().value("agent/disabledTools").toStringList())session->setToolEnabled(name,false);
    });
    const auto directory=QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent");
    const auto queues=QDir(directory).filePath("conversation-tasks");const bool migrate=!QDir(queues).exists();QDir().mkpath(queues);
    const auto path=QDir(queues).filePath(id+".json");if(migrate)QFile::copy(QDir(directory).filePath("tasks.json"),path);
    auto *queue=new AgentTaskQueue(session,path,session);QString error;
    if(!queue->load(&error))QTimer::singleShot(0,this,[this,error]{setStatus(error,true);});queue->registerTools();
    connect(queue,&AgentTaskQueue::sessionCreated,session,[this,registerEnvironment](AgentSession *child,bool writes){registerEnvironment(child);if(writes)agentMcp_->registerTools(child);});
    connect(queue,&AgentTaskQueue::changed,this,[this,queue]{if(agentPage_&&agentQueue_==queue){agentPage_->setQueueSummary(queue->summary());refreshAgentCompletions();}});
    connect(queue,&AgentTaskQueue::errorOccurred,this,[this](const QString &message){setStatus(message,true);});
    for(const auto &name:QSettings().value("agent/disabledTools").toStringList())session->setToolEnabled(name,false);
    return queue;
}
}
