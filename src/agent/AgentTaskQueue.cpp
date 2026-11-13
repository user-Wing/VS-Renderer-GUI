#include "agent/AgentTaskQueue.h"
#include "agent/AgentSession.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QUuid>

namespace vsr {
namespace {
QString state(const QJsonObject &task) { return task.value("state").toString(); }
QJsonObject objectSchema(const QJsonObject &properties, const QJsonArray &required = {}) {
    return {{"type", "object"}, {"properties", properties}, {"required", required}, {"additionalProperties", false}};
}
const QStringList controlTools{"spawn_agent", "enqueue_agent_task", "list_agent_tasks", "read_agent_task", "cancel_agent_task", "wait_agent_tasks", "send_agent_message"};
}
AgentTaskQueue::AgentTaskQueue(AgentSession *session, const QString &path, QObject *parent)
    : QObject(parent), parentSession_(session), path_(path) {
    connect(session,&AgentSession::pendingMessagesChanged,this,[this](const QJsonArray &){QTimer::singleShot(0,this,[this]{for(const auto &task:tasks_)report(task);});});
    connect(session,&AgentSession::accessModeChanged,this,[this](AgentAccessMode mode){if(mode==AgentAccessMode::SoftwareOnly){parentSession_->cancel();cancelAll();}});
    connect(session,&AgentSession::conversationReset,this,[this]{cancelAll();for(const auto &task:tasks_)task->data.insert("reportedRevision",task->data.value("revision").toInt());save();});
}
AgentTaskQueue::~AgentTaskQueue() { shuttingDown_ = true; cancelAll(); }
std::shared_ptr<AgentTaskQueue::Task> AgentTaskQueue::find(const QString &id) const {
    for (const auto &task : tasks_) if (task->data.value("id").toString() == id) return task;
    return {};
}
QString AgentTaskQueue::enqueue(const QString &name, const QString &prompt, const AgentProvider &provider, bool writes, QString *error)
{
    if (!provider.validate(error)) return {};
    if (prompt.trimmed().isEmpty() || prompt.size() > 128000 || prompt.toUtf8().size() > 256 * 1024 || tasks_.size() >= 128) {
        if (error) *error = QStringLiteral("任务不能为空、超过128000字符或使队列超过128项。"); return {};
    }
    auto task = std::make_shared<Task>(); task->provider = provider;task->accessMode=parentSession_->accessMode();task->workingDirectory=parentSession_->workingDirectory();
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task->data = {{"id", id}, {"name", name.trimmed().left(80)}, {"prompt", prompt}, {"providerId", provider.id},
        {"providerName", provider.name}, {"model", provider.model}, {"allowWrites", writes}, {"state", "queued"},
        {"createdAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}, {"result", ""}};
    tasks_.append(task); save(); emit changed(); QTimer::singleShot(0, this, &AgentTaskQueue::schedule); return id;
}
QString AgentTaskQueue::enqueue(const QString &name,const QString &prompt,bool writes,QString *error)
{
    return enqueue(name,prompt,parentSession_->provider(),writes,error);
}
bool AgentTaskQueue::sendMessage(const QString &id,const QString &message,QString *error)
{
    return deliver(id,message,QStringLiteral("父智能体补充"),error);
}
bool AgentTaskQueue::deliver(const QString &id,const QString &message,const QString &source,QString *error)
{
    const auto task=find(id);
    if(!task || message.trimmed().isEmpty() || message.toUtf8().size()>256*1024){if(error)*error=QStringLiteral("子任务不存在，或消息为空/超过256 KiB。");return false;}
    if(task->session){
        if(task->session->postMessage(message,source).isEmpty()){if(error)*error=QStringLiteral("子智能体未接受该消息，请检查等待队列。");return false;}
    }else{
        if(task->inbox.size()>=128){if(error)*error=QStringLiteral("子任务等待消息已达到128条。");return false;}
        if(!task->provider.validate()){task->provider=parentSession_->provider();task->accessMode=parentSession_->accessMode();task->workingDirectory=parentSession_->workingDirectory();}
        if(!task->provider.validate(error))return false;
        task->inbox.append(QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"content",message},{"displayContent",message},{"source",source},{"createdAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
        task->data.insert("state","queued");task->data.remove("error");task->data.remove("finishedAt");
    }
    save();emit changed();QTimer::singleShot(0,this,&AgentTaskQueue::schedule);return true;
}
void AgentTaskQueue::schedule()
{
    if (shuttingDown_) return;
    int running = 0; bool writing = false;
    for (const auto &task : tasks_) if (state(task->data) == "running") { ++running; writing |= task->data.value("allowWrites").toBool(); }
    if (writing || running >= 2) return;
    for (const auto &task : tasks_) {
        if (state(task->data) != "queued") continue;
        if (task->data.value("allowWrites").toBool()) { if (!running) start(task); return; }
        start(task); if (++running == 2) return;
    }
}
void AgentTaskQueue::start(const std::shared_ptr<Task> &task)
{
    auto *session = new AgentSession(this); task->session = session; task->data.insert("state", "running");
    session->setToolPermissionParent(parentSession_);
    session->setAccessMode(task->accessMode==AgentAccessMode::FullEnvironment && parentSession_->accessMode()==AgentAccessMode::FullEnvironment?AgentAccessMode::FullEnvironment:AgentAccessMode::SoftwareOnly);
    session->setWorkingDirectory(task->workingDirectory.isEmpty()?parentSession_->workingDirectory():task->workingDirectory);
    task->data.insert("accessMode",session->accessMode()==AgentAccessMode::FullEnvironment?"full":"software");task->data.insert("workingDirectory",session->workingDirectory());
    task->data.insert("providerId",task->provider.id);task->data.insert("providerName",task->provider.name);task->data.insert("model",task->provider.model);
    task->data.insert("revision",task->data.value("revision").toInt()+1);task->data.insert("result","");task->streamText.clear();task->data.remove("error");
    parentSession_->copyToolsTo(session, controlTools);
    emit sessionCreated(session, task->data.value("allowWrites").toBool());
    for(const auto &tool:session->allTools()){const auto name=tool.toObject().value("function").toObject().value("name").toString();if(!parentSession_->toolEnabled(name))session->setToolEnabled(name,false);}
    const QStringList readOnly{"get_state", "list_filters", "get_script", "list_files", "list_skills", "read_skill", "get_token_usage", "get_environment"};
    if (!task->data.value("allowWrites").toBool())
        for (const auto &tool : session->allTools()) { const auto name = tool.toObject().value("function").toObject().value("name").toString(); session->setToolEnabled(name, session->toolEnabled(name) && (readOnly.contains(name) || (session->toolRequiresEnvironment(name) && !session->toolWritesEnvironment(name)))); }
    session->setProvider(task->provider);
    session->setSystemPrompt(parentSession_->systemPrompt() + QStringLiteral("\n你是一个子智能体。只完成当前指定任务，使用独立会话；给出可核验的结果及未完成项。不能创建更多子任务。可用send_agent_message向parent或现有任务发出有用发现、问题或协作请求；不要互相发送无内容确认。完成后的结果会自动交给父智能体，不需要再次用通信工具重复终稿。")
        + (task->data.value("allowWrites").toBool() ? QStringLiteral("本任务允许软件状态修改，队列会串行执行修改任务。") : QStringLiteral("本任务只允许查询和分析。")));
    session->registerTool("get_token_usage", QStringLiteral("读取本子智能体Token用量。"), objectSchema({}), [session](const QJsonObject &) { return session->usage(); });
    registerCommunicationTools(session,task->data.value("id").toString());
    connect(session, &AgentSession::textDelta, this, [task](const QString &text) { task->streamText=(task->streamText+text).left(128000);task->data.insert("result",task->streamText); });
    connect(session, &AgentSession::errorOccurred, this, [task](const QString &error) { task->data.insert("error", error); });
    connect(session,&AgentSession::messageAdded,this,[this,task,session](const QJsonObject &message){
        if(message.value("role").toString()=="assistant" && !message.contains("tool_calls")){task->data.insert("rounds",task->data.value("rounds").toInt()+1);task->data.insert("result",message.value("content"));}
        if(message.value("role").toString()=="assistant")task->streamText.clear();
        task->conversation=session->exportConversation();emit taskMessage(task->data.value("id").toString(),message);save();emit changed();
    });
    connect(session,&AgentSession::pendingMessagesChanged,this,[this,task,session](const QJsonArray &){task->conversation=session->exportConversation();save();emit changed();});
    connect(session, &AgentSession::finished, this, [this, task, session] {
        if(task->session!=session)return;
        if (state(task->data) == "running") task->data.insert("state", task->data.contains("error") ? "failed" : "completed");
        task->conversation=session->exportConversation();
        task->data.insert("usage", session->usage()); task->data.insert("finishedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        task->session = nullptr; session->deleteLater(); save(); emit changed();report(task); QTimer::singleShot(0, this, &AgentTaskQueue::schedule);
    });
    QString error;
    if(!task->conversation.isEmpty() && !session->importConversation(task->conversation,&error)){
        task->data.insert("error",error);task->data.insert("state","failed");task->session=nullptr;session->deleteLater();save();emit changed();report(task);QTimer::singleShot(0,this,&AgentTaskQueue::schedule);return;
    }
    save(); emit changed();
    if(session->messages().isEmpty() && session->pendingMessages().isEmpty())session->send(task->data.value("prompt").toString());
    if(task->session!=session)return;
    if(!task->inbox.isEmpty()){
        if(!session->isBusy())session->cancel();const auto queued=task->inbox;task->inbox={};
        for(const auto &value:queued){const auto message=value.toObject();session->postMessage(message.value("content").toString(),message.value("source").toString());}
        session->resumePending();
    }else if(!session->isBusy() && !session->pendingMessages().isEmpty())session->resumePending();
    if (task->session==session && !session->isBusy()) { task->data.insert("state", "failed");task->data.insert("error",QStringLiteral("子任务未能启动模型请求。"));task->conversation=session->exportConversation(); task->session = nullptr; session->deleteLater(); save(); emit changed();report(task); QTimer::singleShot(0, this, &AgentTaskQueue::schedule); }
}
bool AgentTaskQueue::resume(const QString &id, const AgentProvider &provider)
{
    const auto task = find(id); if (!task || state(task->data) == "running" || state(task->data) == "queued" || !provider.validate()) return false;
    task->provider = provider; task->data.insert("providerId", provider.id); task->data.insert("providerName", provider.name); task->data.insert("model", provider.model);
    task->accessMode=parentSession_->accessMode();task->workingDirectory=parentSession_->workingDirectory();
    task->data.insert("state", "queued"); task->data.insert("result", ""); task->data.remove("error"); task->data.remove("usage");
    if(!task->conversation.isEmpty() && task->conversation.value("pendingMessages").toArray().isEmpty() && task->inbox.isEmpty())task->inbox.append(QJsonObject{{"content",task->data.value("prompt")},{"source",QStringLiteral("用户恢复任务")}});
    save(); emit changed(); QTimer::singleShot(0, this, &AgentTaskQueue::schedule); return true;
}
void AgentTaskQueue::cancel(const QString &id)
{
    const auto task = find(id); if (!task || (state(task->data) != "queued" && state(task->data) != "running")) return;
    task->data.insert("state", "cancelled"); if (task->session) task->session->cancel();
    save(); emit changed(); QTimer::singleShot(0, this, &AgentTaskQueue::schedule);
}
void AgentTaskQueue::cancelAll() { const bool before=shuttingDown_;shuttingDown_=true;for (const auto &task : tasks_) cancel(task->data.value("id").toString());shuttingDown_=before; }
void AgentTaskQueue::clearFinished() {
    tasks_.removeIf([](const auto &task) { return state(task->data) != "queued" && state(task->data) != "running"; }); save(); emit changed();
}
QJsonArray AgentTaskQueue::tasks() const { QJsonArray result; for (const auto &task : tasks_) {auto data=task->data;const auto conversation=task->session?task->session->exportConversation():task->conversation;data.insert("messageCount",conversation.value("messages").toArray().size());data.insert("pendingCount",task->session?task->session->pendingMessages().size():task->inbox.size()+conversation.value("pendingMessages").toArray().size());result.append(data);} return result; }
QJsonObject AgentTaskQueue::task(const QString &id) const {
    const auto found=find(id);if(!found)return QJsonObject{{"ok",false},{"error","Unknown task"}};auto data=found->data;
    const auto conversation=found->session?found->session->exportConversation():found->conversation;data.insert("conversation",conversation);data.insert("messages",found->session?found->session->messages():conversation.value("messages").toArray());
    auto pending=found->session?found->session->pendingMessages():conversation.value("pendingMessages").toArray();for(const auto &message:found->inbox)pending.append(message);data.insert("pendingMessages",pending);data.insert("streamText",found->streamText);return data;
}
QString AgentTaskQueue::summary() const {
    int queued = 0, running = 0; for (const auto &task : tasks_) { queued += state(task->data) == "queued"; running += state(task->data) == "running"; }
    return QStringLiteral("子 Agent · %1 运行 / %2 等待").arg(running).arg(queued);
}
void AgentTaskQueue::save() {
    QDir().mkpath(QFileInfo(path_).absolutePath()); QSaveFile file(path_); const auto bytes = QJsonDocument(exportTasks()).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) emit errorOccurred(QStringLiteral("子智能体队列保存失败。"));
}
QJsonObject AgentTaskQueue::exportTasks() const {
    QJsonArray entries;for(const auto &task:tasks_){auto data=task->data;data.insert("conversation",task->session?task->session->exportConversation():task->conversation);data.insert("inbox",task->inbox);entries.append(data);}return {{"schemaVersion",1},{"tasks",entries}};
}
bool AgentTaskQueue::load(QString *error) {
    QFile file(path_); if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024) { if (error) *error = QStringLiteral("无法读取子智能体队列。"); return false; }
    return importTasks(QJsonDocument::fromJson(file.readAll()).object(), error);
}
bool AgentTaskQueue::importTasks(const QJsonObject &document, QString *error)
{
    const auto entries = document.value("tasks").toArray();
    if (document.value("schemaVersion").toInt() != 1 || !document.value("tasks").isArray() || tasks_.size() + entries.size() > 128) {
        if (error) *error = QStringLiteral("队列格式不正确或超过128项。"); return false;
    }
    QList<std::shared_ptr<Task>> imported; QSet<QString> ids;
    for (const auto &existing : tasks_) ids.insert(existing->data.value("id").toString());
    for (const auto &entry : entries) {
        const auto data = entry.toObject();
        if (data.value("prompt").toString().trimmed().isEmpty() || data.value("prompt").toString().size() > 128000) { if (error) *error = QStringLiteral("任务内容无效。"); return false; }
        auto task = std::make_shared<Task>();
        for (const auto &key : {"id", "name", "prompt", "providerId", "providerName", "model", "allowWrites", "state", "createdAt", "result", "error", "usage", "finishedAt", "revision", "reportedRevision", "rounds"})
            if (data.contains(key)) task->data.insert(key, data.value(key));
        auto id = data.value("id").toString(); if (QUuid(id).isNull() || ids.contains(id)) id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        ids.insert(id); task->data.insert("id", id);
        if (state(data) == "queued" || state(data) == "running" || state(data).isEmpty()) task->data.insert("state", "interrupted");
        // Imported terminal records are history, not fresh completion events from this queue.
        task->data.insert("reportedRevision",qMax(0,task->data.value("revision").toInt()));
        if(data.value("conversation").isObject() && !data.value("conversation").toObject().isEmpty()){
            AgentSession restored;if(!restored.importConversation(data.value("conversation").toObject(),error))return false;task->conversation=restored.exportConversation();
        }
        if(data.contains("inbox")){
            AgentSession restored;if(!restored.importConversation(QJsonObject{{"schemaVersion",1},{"messages",QJsonArray{}},{"pendingMessages",data.value("inbox")}},error))return false;task->inbox=restored.pendingMessages();
        }
        imported.append(task);
    }
    tasks_.append(imported); save(); emit changed(); return true;
}
void AgentTaskQueue::registerTools()
{
    const auto spawn=[this](const QJsonObject &args){QString error;const auto id=enqueue(args.value("name").toString(QStringLiteral("子任务")),args.value("prompt").toString(),args.value("allowWrites").toBool(),&error);return id.isEmpty()?QJsonObject{{"ok",false},{"error",error}}:QJsonObject{{"ok",true},{"taskId",id},{"state","queued"}};};
    const auto spawnSchema=
        objectSchema({{"name", QJsonObject{{"type", "string"}}}, {"prompt", QJsonObject{{"type", "string"}}}, {"allowWrites", QJsonObject{{"type", "boolean"}}}}, {"prompt"});
    parentSession_->registerTool("enqueue_agent_task",QStringLiteral("实际创建独立子智能体，自动继承当前来源；无需用户配置。默认只读，只有用户明确授权修改时设置allowWrites。创建后应等待/读取结果再汇总。"),spawnSchema,spawn);
    parentSession_->registerTool("spawn_agent",QStringLiteral("创建子智能体并启动任务（与enqueue_agent_task相同）；自动继承当前来源，默认只读。"),spawnSchema,spawn);
    registerCommunicationTools(parentSession_,"parent");
    parentSession_->registerTool("cancel_agent_task", QStringLiteral("取消排队或正在运行的子智能体。"), objectSchema({{"id", QJsonObject{{"type", "string"}}}}, {"id"}), [this](const QJsonObject &args) {
        const auto id = args.value("id").toString(); if (!find(id)) return QJsonObject{{"ok", false}, {"error", "Unknown task"}}; cancel(id); return QJsonObject{{"ok", true}};
    });
    parentSession_->registerAsyncTool("wait_agent_tasks", QStringLiteral("等待子智能体结束，最长60秒；返回队列状态和已有结果。"), objectSchema({{"seconds", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 60}}}}), [this](const QJsonObject &args, AgentSession::Completion completion) {
        auto *timer = new QTimer(this); timer->setInterval(100); timer->setProperty("remaining", args.value("seconds").toInt(30) * 10);
        connect(timer, &QTimer::timeout, this, [this, timer, completion] {
            bool running = false; for (const auto &task : tasks_) running |= state(task->data) == "running" || state(task->data) == "queued";
            const int remaining = timer->property("remaining").toInt()-1; timer->setProperty("remaining", remaining);
            if (!running || remaining <= 0) { timer->stop(); timer->deleteLater(); completion(QJsonObject{{"ok", true}, {"tasks", tasks()}, {"timedOut", running}}); }
        }); timer->start();
    });
}
void AgentTaskQueue::registerCommunicationTools(AgentSession *session,const QString &sender)
{
    session->registerTool("list_agent_tasks",QStringLiteral("读取已有子智能体的状态、结果摘要及会话轮数；不创建任务。"),objectSchema({}),[this](const QJsonObject &){return QJsonObject{{"ok",true},{"tasks",tasks()}};});
    session->registerTool("read_agent_task",QStringLiteral("读取指定子任务的实际会话messages、待接收消息、结果及Token用量。"),objectSchema({{"id",QJsonObject{{"type","string"}}}},{"id"}),[this](const QJsonObject &args){return task(args.value("id").toString());});
    session->registerTool("send_agent_message",QStringLiteral("给已有子智能体发送补充要求或协作信息，运行中只进入待接收队列，不打断模型/工具。子智能体可发给id=parent；避免重复确认和无意义来回。"),
        objectSchema({{"id",QJsonObject{{"type","string"}}},{"message",QJsonObject{{"type","string"}}}},{"id","message"}),[this,sender](const QJsonObject &args){
            const auto id=args.value("id").toString(),message=args.value("message").toString();
            if(id==sender)return QJsonObject{{"ok",false},{"error",QStringLiteral("不能向当前智能体自己发送消息。")}};
            if(message.trimmed().isEmpty() || message.toUtf8().size()>256*1024)return QJsonObject{{"ok",false},{"error",QStringLiteral("消息为空或超过256 KiB。")}};
            const auto source=sender=="parent"?QStringLiteral("父智能体补充"):QStringLiteral("子智能体 %1 的协作消息").arg(sender);
            if(id=="parent"){
                const auto pending=parentSession_->postMessage(message,source);return pending.isEmpty()?QJsonObject{{"ok",false},{"error",QStringLiteral("父智能体等待队列已满。")}}:QJsonObject{{"ok",true},{"messageId",pending},{"state","queued"}};
            }
            QString error;if(!deliver(id,message,source,&error))return QJsonObject{{"ok",false},{"error",error}};
            return QJsonObject{{"ok",true},{"taskId",id},{"state",state(find(id)->data)}};
        });
    if(session!=parentSession_){
        const QStringList controlled{"list_agent_tasks","read_agent_task","send_agent_message","get_token_usage"};
        for(const auto &value:parentSession_->allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();if(controlled.contains(name) && !parentSession_->toolEnabled(name))session->setToolEnabled(name,false);}
    }
}
void AgentTaskQueue::report(const std::shared_ptr<Task> &task)
{
    if(shuttingDown_)return;const auto status=state(task->data);
    if(status!="completed" && status!="failed" && status!="cancelled")return;
    const int revision=task->data.value("revision").toInt(),reported=task->data.value("reportedRevision").toInt();if(revision<=reported)return;
    task->data.insert("reportedRevision",revision);
    const auto fullResult=task->data.value("result").toString();const bool truncated=fullResult.size()>16000;
    QJsonObject result{{"taskId",task->data.value("id")},{"name",task->data.value("name")},{"revision",revision},{"state",status},{"rounds",task->data.value("rounds")},{"result",fullResult.left(16000)},{"resultTruncated",truncated}};
    if(truncated)result.insert("resultNotice",QStringLiteral("自动汇报仅展示前16000字符；完整结果与会话已保留，可通过read_agent_task读取，或打开子会话查看器查看。"));
    if(task->data.contains("error"))result.insert("error",task->data.value("error"));
    const auto message=QStringLiteral("子任务本轮已结束。以下为实际状态与输出数据；按taskId和revision只消化一次，核实后继续原任务或汇总，不要再发无内容确认。\n")+QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
    if(parentSession_->postMessage(message,QStringLiteral("子智能体完成汇报")).isEmpty()){
        task->data.insert("reportedRevision",reported);
        emit errorOccurred(parentSession_->pendingMessages().size()>=128?QStringLiteral("子任务结果已保存，但父智能体等待队列已满；移除消息后会重试投递。")
            :QStringLiteral("子任务结果已保存，但父智能体未接受自动汇报；可通过read_agent_task读取完整结果。"));
    }
    save();
}
}
