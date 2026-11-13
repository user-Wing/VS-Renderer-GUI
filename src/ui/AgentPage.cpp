#include "ui/AgentPage.h"
#include "ui/AgentComposer.h"
#include "ui/AgentChoicePopup.h"
#include "ui/AgentMarkdownView.h"
#include "ui/AgentProviderDialog.h"
#include "agent/AgentSession.h"
#include "agent/AgentToolLibrary.h"
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QInputDialog>
#include <QRegularExpression>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStyle>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
namespace {
QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
bool writeJson(const QString &path, const QJsonObject &object, QString &error)
{
    QSaveFile file(path);
    const auto data = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        error = file.errorString(); return false;
    }
    return true;
}
QJsonObject readJson(const QString &path, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return {}; }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON 格式无效：%1").arg(parse.errorString()); return {};
    }
    return document.object();
}
}

AgentPage::AgentPage(AgentSession *session, QWidget *parent)
    : QWidget(parent), session_(session), toolLibrary_(std::make_unique<AgentToolLibrary>(session))
{
    setObjectName("agentPage");
    directory_ = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent");
    QDir().mkpath(QDir(directory_).filePath("conversations"));
    setStyleSheet(QStringLiteral(R"(
        QWidget#agentSidebar { background:rgba(128,128,128,12); border:1px solid #738098; border-radius:14px; }
        QSplitter#agentSplitter::handle { background:transparent; }
        QListWidget#agentConversations { background:transparent; border:0; outline:0; }
        QListWidget#agentConversations::item { border-radius:8px; padding:7px 10px; margin:2px 0; }
        QListWidget#agentConversations::item:selected { background:#315474; color:#ffffff; border:1px solid #9bb9da; }
        QTextBrowser#agentTranscript { background:transparent; border:0; padding:6px 10px; }
        QWidget#agentComposer { background:rgba(34,39,48,240); border:1px solid #8e9db4; border-radius:18px; }
        QWidget#agentComposerBody { background:transparent; }
        QLabel#agentMarkdownHint { color:#b9c6d8; background:transparent; }
        QPlainTextEdit#agentInput { background:#222730; color:#f2f5fb; border:0; padding:3px 2px; font-size:14px; selection-background-color:#3569a1; }
        QPlainTextEdit#agentInput QWidget { background:#222730; color:#f2f5fb; }
        QTextBrowser#agentInputPreview { background:#222b38; color:#f2f5fb; border:1px solid #8293ad; border-radius:8px; padding:8px; }
        QTextBrowser#agentInputPreview QWidget { background:transparent; color:#f2f5fb; }
        QPushButton[agentControl="true"], QDialog QPushButton, QDialog QComboBox {
            background:#343e4e; color:#f5f8fc; border:1px solid #899bb6; border-radius:7px; padding:4px 8px; min-height:24px;
        }
        QPushButton[agentControl="true"]:hover, QDialog QPushButton:hover, QDialog QComboBox:hover { background:#465775; border-color:#c1d6ee; }
        QPushButton[agentControl="true"]:checked { background:#2d6098; border-color:#c1d6ee; }
        QPushButton[agentControl="true"]:disabled, QDialog QPushButton:disabled { background:#303642; color:#9da9bb; border-color:#596579; }
        QPushButton#agentAccessMode[agentAccessFull="true"] { background:#493425; color:#ffbb79; border-color:#d69b62; }
        QPushButton#agentAccessMode[agentAccessFull="true"]:hover { background:#62452d; border-color:#ffd0a0; }
        QPushButton#agentAccessMode[agentAccessFull="true"]:disabled { background:#39312a; color:#bc9877; border-color:#7f6852; }
        QPushButton#agentSend { background:#2676cf; color:white; border:1px solid #8dc4ff; border-radius:17px; min-height:32px; padding:0; }
        QListWidget#agentCompletionPopup { background:#253143; color:#f3f6fb; border:1px solid #9fb3cd; border-radius:7px; padding:5px; }
        QListWidget#agentCompletionPopup::item { padding:7px; min-height:24px; }
        QListWidget#agentCompletionPopup::item:selected { background:#3a6599; color:white; }
    )"));
    auto *layout = new QHBoxLayout(this); layout->setContentsMargins(12,12,18,12); layout->setSpacing(0);
    auto *splitter = new QSplitter(this); splitter->setObjectName("agentSplitter"); splitter->setHandleWidth(12); layout->addWidget(splitter);
    auto *sidebar = new QWidget(splitter); sidebar->setObjectName("agentSidebar"); sidebar->setMinimumWidth(190); sidebar->setMaximumWidth(300);
    auto *side = new QVBoxLayout(sidebar); side->setContentsMargins(12,16,12,12); side->setSpacing(10);
    auto *sideTitle = new QLabel(QStringLiteral("对话"),sidebar); auto titleFont=sideTitle->font(); titleFont.setBold(true); sideTitle->setFont(titleFont); side->addWidget(sideTitle);
    new_ = new QPushButton(QStringLiteral("＋ 新对话"),sidebar); new_->setObjectName("agentNewConversation"); new_->setProperty("agentControl",true); side->addWidget(new_);
    conversations_ = new QListWidget(sidebar); conversations_->setObjectName("agentConversations"); conversations_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); conversations_->setTextElideMode(Qt::ElideRight); conversations_->setWordWrap(false); side->addWidget(conversations_,1);
    auto *exchange = new QHBoxLayout;
    import_ = new QPushButton(QStringLiteral("导入"),sidebar); import_->setObjectName("agentImportConversation"); exchange->addWidget(import_);
    export_ = new QPushButton(QStringLiteral("导出"),sidebar); export_->setObjectName("agentExportConversation"); exchange->addWidget(export_); side->addLayout(exchange);
    remove_ = new QPushButton(QStringLiteral("删除对话"),sidebar); remove_->setObjectName("agentDeleteConversation"); side->addWidget(remove_);
    for(auto *button:{import_,export_,remove_})button->setProperty("agentControl",true);
    auto *room = new QWidget(splitter); auto *center = new QHBoxLayout(room); center->setContentsMargins(6,0,0,0);
    auto *column = new QWidget(room); auto *chat = new QVBoxLayout(column); chat->setContentsMargins(4,6,4,0); chat->setSpacing(12); center->addWidget(column,1);
    chatTitle_ = new QLabel(QStringLiteral("新对话"),column); chatTitle_->setObjectName("agentConversationTitle"); auto font=chatTitle_->font(); font.setBold(true); chatTitle_->setFont(font); chat->addWidget(chatTitle_);
    transcript_ = new AgentMarkdownView(column); chat->addWidget(transcript_,1);
    composer_=new AgentComposer(column); input_=composer_->editor(); auto *toolbar=composer_->toolbar();
    provider_=new AgentChoiceButton(composer_);provider_->setObjectName("agentProvider");provider_->setMinimumWidth(200);provider_->setMaximumWidth(280);toolbar->addWidget(provider_);
    settings_=new QPushButton(QStringLiteral("来源设置"),composer_);settings_->setObjectName("agentProviderSettings");toolbar->addWidget(settings_);
    features_=new QPushButton(QStringLiteral("功能"),composer_);features_->setObjectName("agentFeatureSettings");features_->setToolTip(QStringLiteral("Skills、全局指令和 MCP 设置"));toolbar->addWidget(features_);
    tools_=new QPushButton(QStringLiteral("工具"),composer_);tools_->setObjectName("agentTools");toolbar->addWidget(tools_);
    queue_=new QPushButton(QStringLiteral("自动队列"),composer_);queue_->setObjectName("agentQueue");toolbar->addWidget(queue_);toolbar->addStretch();
    modelButton_=new AgentChoiceButton(composer_);modelButton_->setText(QStringLiteral("选择模型"));modelButton_->setObjectName("agentModelButton");modelButton_->setMinimumWidth(110);modelButton_->setMaximumWidth(240);toolbar->addWidget(modelButton_);
    reasoning_=new AgentChoiceButton(composer_);reasoning_->setText(QStringLiteral("思考 · 自动"));reasoning_->setObjectName("agentReasoningButton");reasoning_->setMinimumWidth(110);reasoning_->setMaximumWidth(150);toolbar->addWidget(reasoning_);
    stop_=new QPushButton(QStringLiteral("■"),composer_);stop_->setObjectName("agentStop");stop_->setToolTip(QStringLiteral("停止当前请求，保留待接收消息"));stop_->setFixedWidth(34);toolbar->addWidget(stop_);
    send_=new QPushButton(QStringLiteral("↑"),composer_);send_->setObjectName("agentSend");send_->setToolTip(QStringLiteral("发送（Enter）；Ctrl+Enter 换行"));send_->setFixedSize(34,34);toolbar->addWidget(send_);
    toolbar->insertWidget(toolbar->indexOf(modelButton_),stop_);
    for(auto *button:{settings_,features_,tools_,queue_,modelButton_,reasoning_,stop_})button->setProperty("agentControl",true);
    auto *modeBar=composer_->modeBar();for(auto *button:{settings_,features_,tools_,queue_}){toolbar->removeWidget(button);modeBar->insertWidget(modeBar->count()-2,button);}
    toolbar->removeWidget(provider_);toolbar->insertWidget(toolbar->indexOf(modelButton_),provider_);
    access_=new AgentChoiceButton(composer_);access_->setObjectName("agentAccessMode");access_->setProperty("agentControl",true);access_->setMinimumWidth(130);toolbar->insertWidget(0,access_);
    workingDirectory_=new AgentChoiceButton(composer_);workingDirectory_->setObjectName("agentWorkingDirectory");workingDirectory_->setProperty("agentControl",true);workingDirectory_->setMinimumWidth(95);workingDirectory_->setMaximumWidth(200);modeBar->insertWidget(modeBar->count()-2,workingDirectory_);
    chat->addWidget(composer_);
    auto *footer=new QHBoxLayout;status_=new QLabel(column);status_->setObjectName("agentStatus");status_->setWordWrap(true);footer->addWidget(status_,1);
    pending_=new QPushButton(column);pending_->setObjectName("agentPendingMessages");pending_->setProperty("agentControl",true);footer->addWidget(pending_);
    usage_=new QPushButton(column);usage_->setObjectName("agentUsage");usage_->setProperty("agentControl",true);footer->addWidget(usage_);chat->addLayout(footer);
    splitter->setSizes({220,1050}); splitter->setStretchFactor(1,1);
    renderTimer_ = new QTimer(this); renderTimer_->setSingleShot(true); renderTimer_->setInterval(40);
    connect(renderTimer_, &QTimer::timeout, this, &AgentPage::renderConversation);
    connect(send_, &QPushButton::clicked, this, &AgentPage::send);
    connect(stop_, &QPushButton::clicked, session_, &AgentSession::cancel);
    connect(new_, &QPushButton::clicked, this, &AgentPage::newConversation);
    connect(import_, &QPushButton::clicked, this, &AgentPage::importConversation);
    connect(export_, &QPushButton::clicked, this, &AgentPage::exportConversation);
    connect(remove_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, QStringLiteral("删除对话"), QStringLiteral("删除当前对话的本地记录？")) != QMessageBox::Yes) return;
        const auto path = QDir(directory_).filePath("conversations/" + conversationId_ + ".json");
        if (QFile::exists(path) && !QFile::remove(path)) { report(QStringLiteral("无法删除对话文件。"), true); return; }
        session_->clear(); conversationId_.clear(); newConversation();
    });
    connect(conversations_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        if (item && item->data(Qt::UserRole).toString() != conversationId_) loadConversation(item->data(Qt::UserRole).toString());
    });
    connect(tools_, &QPushButton::clicked, this, &AgentPage::showTools);
    connect(settings_,&QPushButton::clicked,this,&AgentPage::showProviderSettings);
    connect(features_,&QPushButton::clicked,this,&AgentPage::showFeatureSettings);
    connect(queue_,&QPushButton::clicked,this,&AgentPage::queueRequested);
    connect(usage_,&QPushButton::clicked,this,&AgentPage::showUsage);
    connect(session_,&AgentSession::usageChanged,this,[this](const QJsonObject &){updateUsage();});
    connect(modelButton_,&QPushButton::clicked,this,&AgentPage::openModelMenu);
    connect(reasoning_,&QPushButton::clicked,this,&AgentPage::showReasoningMenu);
    connect(pending_,&QPushButton::clicked,this,&AgentPage::showPendingMessages);
    connect(access_,&QPushButton::clicked,this,&AgentPage::showAccessMenu);
    connect(workingDirectory_,&QPushButton::clicked,this,&AgentPage::chooseWorkingDirectory);
    connect(session_,&AgentSession::accessModeChanged,this,[this](AgentAccessMode mode){QSettings().setValue("agent/accessMode",mode==AgentAccessMode::FullEnvironment?"full":"software");updateAccessControls();});
    connect(session_,&AgentSession::workingDirectoryChanged,this,[this](const QString &path){QSettings().setValue("agent/workingDirectory",path);updateAccessControls();});
    connect(composer_,&AgentComposer::submitted,this,&AgentPage::send);
    connect(composer_,&AgentComposer::completionsRequested,this,&AgentPage::completionsRequested);
    connect(composer_,&AgentComposer::mentionActionRequested,this,&AgentPage::mentionActionRequested);
    connect(session_,&AgentSession::pendingMessagesChanged,this,[this](const QJsonArray &){updatePendingMessages();renderConversation();});
    connect(provider_,&QPushButton::clicked,this,&AgentPage::openProviderMenu);
    connect(session_, &AgentSession::textDelta, this, [this](const QString &text) { pendingText_ += text; if (!renderTimer_->isActive()) renderTimer_->start(); });
    connect(session_, &AgentSession::messageAdded, this, [this](const QJsonObject &) { pendingText_.clear(); renderConversation(); if (!session_->isBusy()) saveConversation(); });
    connect(session_, &AgentSession::busyChanged, this, &AgentPage::updateBusy);
    connect(session_, &AgentSession::errorOccurred, this, [this](const QString &error) { report(error, true); });
    connect(session_, &AgentSession::finished, this, [this] { pendingText_.clear(); renderConversation(); saveConversation(); });
    QString error;
    providers_ = AgentProviderStore::load(QDir(directory_).filePath("providers.json"), &error);
    if (!error.isEmpty()) report(error, true);
    refreshProviders(QSettings().value("agent/providerId").toString());
    const auto toolsPath = QDir(directory_).filePath("tools.json");
    if (QFile::exists(toolsPath)) { error.clear(); const auto document = readJson(toolsPath, error); if (!error.isEmpty() || !toolLibrary_->importTools(document, &error)) report(error, true); }
    for (const auto &name : QSettings().value("agent/disabledTools").toStringList()) session_->setToolEnabled(name, false);
    refreshConversations();
    const auto last = QSettings().value("agent/lastConversation").toString();
    bool restored = false;
    for (int i = 0; i < conversations_->count(); ++i) if (conversations_->item(i)->data(Qt::UserRole).toString() == last) { loadConversation(last); restored = true; break; }
    if (!restored) newConversation();
    session_->setAccessMode(QSettings().value("agent/accessMode","software").toString()=="full"?AgentAccessMode::FullEnvironment:AgentAccessMode::SoftwareOnly);
    const auto workingDirectory=QSettings().value("agent/workingDirectory").toString();if(!workingDirectory.isEmpty())session_->setWorkingDirectory(workingDirectory);
    updateAccessControls();
    updateBusy(session_->isBusy()); updateUsage();updatePendingMessages();
}

AgentPage::~AgentPage() { saveConversation(); }

void AgentPage::report(const QString &message, bool error)
{
    submissionRejected_=error;status_->setText(message); status_->setStyleSheet(error ? "color:#df5b5b;" : QString());
}

void AgentPage::refreshProviders(const QString &selected)
{
    providerIndex_=-1;for(int i=0;i<providers_.size();++i)if(providers_[i].id==selected){providerIndex_=i;break;}if(providerIndex_<0 && !providers_.isEmpty())providerIndex_=0;
    if(providerIndex_<0){provider_->setText(QStringLiteral("选择来源"));provider_->setToolTip(QStringLiteral("选择 Agent 来源"));modelButton_->setText(QStringLiteral("选择模型"));session_->setProvider({});report(QStringLiteral("请在“来源设置”中新增来源或导入 3FUI 配置。"));return;}
    selectProviderId(providers_[providerIndex_].id);
}

void AgentPage::selectProviderId(const QString &id)
{
    int index=-1;for(int i=0;i<providers_.size();++i)if(providers_[i].id==id){index=i;break;}if(index<0)return;providerIndex_=index;auto &source=providers_[index];source.modelCapability={};provider_->setText(source.name);provider_->setToolTip(source.name+"\n"+source.baseUrl);session_->setProvider(source);QSettings().setValue("agent/providerId",source.id);updateModelButtons();updateUsage();emit providerModelChanged(source);
}

void AgentPage::openProviderMenu()
{
    if(providers_.isEmpty()){showProviderSettings();return;}QList<AgentChoice> choices;for(const auto &source:providers_)choices.append({source.id,source.name,source.model});auto *popup=new AgentChoicePopup(QStringLiteral("选择来源"),QStringLiteral("切换连接，当前会话上下文保留。"),choices,providers_[providerIndex_].id,this);popup->setObjectName("agentProviderPopup");connect(popup,&AgentChoicePopup::chosen,this,[this](const QString &id){if(providerIndex_<0 || providers_[providerIndex_].id!=id)selectProviderId(id);});popup->openFor(provider_);
}

bool AgentPage::saveProviders()
{
    QString error;
    if (!AgentProviderStore::save(QDir(directory_).filePath("providers.json"), providers_, &error)) { report(error, true); return false; }
    return true;
}



void AgentPage::setCompletionItems(const QJsonArray &items) { composer_->setCompletionItems(items); }
void AgentPage::insertInputText(const QString &text) { input_->insertPlainText(text);input_->setFocus(); }
void AgentPage::showNotice(const QString &text,bool error) { report(text,error); }
void AgentPage::startNewConversation() { newConversation(); }
void AgentPage::openUsage() { showUsage(); }
void AgentPage::openTools() { showTools(); }

void AgentPage::setModelCapabilities(const QJsonObject &capabilities)
{
    const int index=providerIndex_;if(index<0)return;auto checked=capabilities;auto levels=capabilities.value("reasoningLevels").toArray();
    if(capabilities.value("chatToolsRequiresOff").toBool()){levels=QJsonArray{QJsonObject{{"id","auto"},{"label",QStringLiteral("自动")}},QJsonObject{{"id","off"},{"label",QStringLiteral("关闭")}}};checked.insert("reasoningLevels",levels);}
    auto &source=providers_[index];bool supported=source.reasoningEffort.isEmpty() || source.reasoningEffort=="auto";for(const auto &item:levels)supported|=item.toObject().value("id").toString()==source.reasoningEffort;if(!supported){source.reasoningEffort="auto";saveProviders();}
    source.modelCapability=checked;session_->setProvider(source);updateModelButtons();
    if(capabilities.contains("chatToolsSupported") && !capabilities.value("chatToolsSupported").toBool())report(QStringLiteral("当前 API 未提供 Chat 工具调用能力，软件工具操作不可用。"));
    else if(capabilities.value("chatToolsRequiresOff").toBool())report(QStringLiteral("此模型需要关闭思考才能调用工具；自动模式会采用兼容设置。"));
}

void AgentPage::updateModelButtons()
{
    const int index=providerIndex_;if(index<0)return;const auto &source=providers_[index];modelButton_->setText(source.model);modelButton_->setToolTip(source.model);
    QString label=source.reasoningEffort=="off"?QStringLiteral("关闭"):QStringLiteral("自动");for(const auto &item:source.modelCapability.value("reasoningLevels").toArray()){const auto level=item.toObject();if(level.value("id").toString()==source.reasoningEffort)label=level.value("label").toString(source.reasoningEffort);}
    auto defaultLevel=source.modelCapability.value("defaultLevel").toString();for(const auto &item:source.modelCapability.value("reasoningLevels").toArray()){const auto level=item.toObject();if(level.value("id").toString()==defaultLevel)defaultLevel=level.value("label").toString(defaultLevel);}
    const auto status=source.modelCapability.value("status").toString();const QMap<QString,QString> states{{"ready",QStringLiteral("已确认")},{"unknown",QStringLiteral("未识别")},{"error",QStringLiteral("探测失败")},{"stale",QStringLiteral("缓存结果")}};
    reasoning_->setText(QStringLiteral("思考 · ")+label);reasoning_->setToolTip(source.modelCapability.value("label").toString(QStringLiteral("模型思考能力"))+(defaultLevel.isEmpty()?QString():QStringLiteral("\n默认思考：")+defaultLevel)+QStringLiteral("\n探测状态：")+states.value(status,status.isEmpty()?QStringLiteral("正在探测"):status));
}

void AgentPage::selectModel(const QString &model)
{
    const int index=providerIndex_;if(index<0 || model.trimmed().isEmpty())return;auto &source=providers_[index];if(source.model==model.trimmed())return;source.model=model.trimmed();source.modelCapability={};
    auto history=QSettings().value("agent/models/"+source.id).toStringList();history.removeAll(source.model);history.prepend(source.model);while(history.size()>20)history.removeLast();QSettings().setValue("agent/models/"+source.id,history);saveProviders();session_->setProvider(source);updateModelButtons();emit providerModelChanged(source);
}

void AgentPage::openModelMenu()
{
    const int index=providerIndex_;if(index<0){showProviderSettings();return;}const auto source=providers_[index];QStringList models{source.model};for(const auto &value:source.modelCapability.value("models").toArray())if(!value.toString().isEmpty() && !models.contains(value.toString()))models.append(value.toString());for(const auto &model:QSettings().value("agent/models/"+source.id).toStringList())if(!models.contains(model))models.append(model);
    QList<AgentChoice> choices;for(const auto &model:models)choices.append({model,model,{}});choices.append({"__custom_model__",QStringLiteral("填写模型 ID…"),QStringLiteral("使用端点支持的其他模型")});
    auto *popup=new AgentChoicePopup(QStringLiteral("选择模型"),source.name,choices,source.model,this);popup->setObjectName("agentModelPopup");connect(popup,&AgentChoicePopup::chosen,this,[this,source](const QString &model){if(model!="__custom_model__"){selectModel(model);return;}bool ok=false;const auto custom=QInputDialog::getText(this,QStringLiteral("选择模型"),QStringLiteral("模型 ID"),QLineEdit::Normal,source.model,&ok);if(ok)selectModel(custom);});popup->openFor(modelButton_);
}

void AgentPage::selectReasoning(const QString &level)
{
    const int index=providerIndex_;if(index<0)return;bool supported=level.isEmpty() || level=="auto";for(const auto &value:providers_[index].modelCapability.value("reasoningLevels").toArray())supported|=value.toObject().value("id").toString()==level;if(!supported){report(QStringLiteral("当前模型未提供这个思考选项。"),true);return;}providers_[index].reasoningEffort=level;saveProviders();session_->setProvider(providers_[index]);updateModelButtons();
}

void AgentPage::showReasoningMenu()
{
    const int index=providerIndex_;if(index<0)return;const auto source=providers_[index];auto defaultLevel=source.modelCapability.value("defaultLevel").toString();for(const auto &value:source.modelCapability.value("reasoningLevels").toArray()){const auto level=value.toObject();if(level.value("id").toString()==defaultLevel)defaultLevel=level.value("label").toString(defaultLevel);}
    QList<AgentChoice> choices{{"auto",QStringLiteral("自动"),defaultLevel.isEmpty()?QStringLiteral("跟随当前模型能力"):QStringLiteral("当前模型默认：%1").arg(defaultLevel)}};
    for(const auto &value:source.modelCapability.value("reasoningLevels").toArray()){const auto level=value.toObject();const auto id=level.value("id").toString();if(id.isEmpty() || id=="auto")continue;choices.append({id,level.value("label").toString(id),id=="off"?QStringLiteral("关闭模型思考"):QString()});}
    auto *popup=new AgentChoicePopup(QStringLiteral("思考强度"),source.modelCapability.value("label").toString(QStringLiteral("只显示当前 API 已确认的选项。")),choices,source.reasoningEffort.isEmpty()?QStringLiteral("auto"):source.reasoningEffort,this);popup->setObjectName("agentReasoningPopup");connect(popup,&AgentChoicePopup::chosen,this,&AgentPage::selectReasoning);popup->openFor(reasoning_);
}

void AgentPage::updatePendingMessages()
{
    const int count=session_->pendingMessages().size();pending_->setVisible(count>0);pending_->setText(QStringLiteral("待接收 %1%2").arg(count).arg(session_->pendingPaused()?QStringLiteral(" · 已暂停"):QString()));
}



void AgentPage::setQueueSummary(const QString &summary)
{
    queue_->setText(QStringLiteral("自动队列"));queue_->setToolTip(summary.isEmpty()?QStringLiteral("查看 Agent 自动安排的任务"):summary);
}

void AgentPage::updateUsage()
{
    const auto usage=session_->usage();const auto prefix=usage.value("estimated").toBool()?QStringLiteral("≈"):QString();
    usage_->setText(QStringLiteral("上下文 %1%2% · %3 tokens").arg(prefix).arg(usage.value("contextPercent").toDouble(),0,'f',0).arg(usage.value("totalTokens").toInteger()));
}





void AgentPage::refreshConversations()
{
    const QSignalBlocker blocker(conversations_); conversations_->clear();
    const QDir directory(QDir(directory_).filePath("conversations"));
    for (const auto &file : directory.entryInfoList({"*.json"}, QDir::Files, QDir::Time)) {
        QString error; const auto document = readJson(file.absoluteFilePath(), error); if (!error.isEmpty()) continue;
        auto *item = new QListWidgetItem(document.value("title").toString(QStringLiteral("对话")), conversations_); item->setData(Qt::UserRole, file.completeBaseName()); item->setToolTip(document.value("updatedAt").toString());
        if (file.completeBaseName() == conversationId_) conversations_->setCurrentItem(item);
    }
}

void AgentPage::newConversation()
{
    if (session_->isBusy()) return;
    saveConversation(); session_->clear(); pendingText_.clear(); conversationId_ = newId(); conversationTitle_ = QStringLiteral("新对话"); input_->clear();
    refreshConversations(); renderConversation(); report(QStringLiteral("已新建对话。"));
}

void AgentPage::saveConversation()
{
    if (conversationId_.isEmpty() || session_->messages().isEmpty() || session_->isBusy()) return;
    for (const auto &entry : session_->messages()) if (entry.toObject().value("role").toString() == "user") { const auto message=entry.toObject();conversationTitle_ = message.value(message.contains("displayContent")?"displayContent":"content").toString().simplified().left(42); break; }
    chatTitle_->setText(conversationTitle_);
    QString error;
    const QJsonObject document{{"title", conversationTitle_}, {"updatedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}, {"conversation", session_->exportConversation()}};
    if (!writeJson(QDir(directory_).filePath("conversations/" + conversationId_ + ".json"), document, error)) { report(error, true); return; }
    QSettings().setValue("agent/lastConversation", conversationId_); refreshConversations();
}

void AgentPage::loadConversation(const QString &id)
{
    if (session_->isBusy() || QUuid(id).isNull()) return;
    QString error; const auto document = readJson(QDir(directory_).filePath("conversations/" + id + ".json"), error);
    if (!error.isEmpty()) { report(error, true); return; }
    saveConversation();
    if (!session_->importConversation(document.value("conversation").toObject(), &error)) { report(error, true); return; }
    conversationId_ = id; conversationTitle_ = document.value("title").toString(); pendingText_.clear(); QSettings().setValue("agent/lastConversation", id); refreshConversations(); renderConversation(); report(QStringLiteral("已恢复对话。"));
}

void AgentPage::importConversation()
{
    const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("导入 Agent 对话"), {}, "JSON (*.json)"); if (path.isEmpty()) return;
    QString error; auto document = readJson(path, error); if (!error.isEmpty()) { report(error, true); return; }
    if (document.contains("conversation")) document = document.value("conversation").toObject();
    saveConversation();
    if (!session_->importConversation(document, &error)) { report(error, true); return; }
    conversationId_ = newId(); pendingText_.clear(); saveConversation(); renderConversation(); report(QStringLiteral("对话已导入。"));
}

void AgentPage::exportConversation()
{
    const auto path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 Agent 对话"), conversationTitle_ + ".json", "JSON (*.json)"); if (path.isEmpty()) return;
    QString error; if (!writeJson(path, session_->exportConversation(), error)) report(error, true); else report(QStringLiteral("对话已导出，不包含来源密钥。"));
}

void AgentPage::renderConversation()
{
    renderTimer_->stop();chatTitle_->setText(conversationTitle_);transcript_->showMessages(session_->messages(),pendingText_,session_->pendingMessages(),session_->isBusy());
}

void AgentPage::updateBusy(bool busy)
{
    updateAccessControls();
    send_->setEnabled(true);input_->setEnabled(true);stop_->setEnabled(busy);stop_->setVisible(busy);provider_->setEnabled(!busy);modelButton_->setEnabled(!busy);reasoning_->setEnabled(!busy);settings_->setEnabled(!busy);features_->setEnabled(!busy);tools_->setEnabled(!busy);conversations_->setEnabled(!busy);new_->setEnabled(!busy);remove_->setEnabled(!busy);import_->setEnabled(!busy);export_->setEnabled(!busy);
    if (busy) report(QStringLiteral("正在处理；继续发送会加入待接收队列。"));
    else if (status_->text() == QStringLiteral("正在处理；继续发送会加入待接收队列。")) report(QStringLiteral("本轮已结束。"));
}

void AgentPage::send()
{
    const auto text=input_->toPlainText().trimmed();if(text.isEmpty())return;if(text.toUtf8().size()>256*1024){report(QStringLiteral("消息超过 256 KiB，请缩短内容。"),true);return;}
    submissionRejected_=false;
    if(text.startsWith('/')){if(receivers(SIGNAL(commandSubmitted(QString))))emit commandSubmitted(text);else report(QStringLiteral("命令处理尚未连接。"),true);}
    else{const int index=providerIndex_;if(index<0){report(QStringLiteral("请先添加或导入 Agent 来源。"),true);return;}QString error;if(!providers_[index].validate(&error)){report(error,true);return;}session_->setProvider(providers_[index]);if(receivers(SIGNAL(messageSubmitted(QString))))emit messageSubmitted(text);else session_->send(text);}
    if(!submissionRejected_){input_->clear();if(session_->isBusy())report(QStringLiteral("正在处理；继续发送会加入待接收队列。"));}
}
}
