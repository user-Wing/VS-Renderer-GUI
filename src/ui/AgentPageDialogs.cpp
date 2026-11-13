#include "ui/AgentPage.h"
#include "ui/AgentProviderDialog.h"
#include "ui/AgentChoicePopup.h"
#include "agent/AgentSession.h"
#include "agent/AgentToolLibrary.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QStyle>
#include <QVBoxLayout>

namespace vsr {
namespace {
bool writeJson(const QString &path,const QJsonObject &object,QString &error){QSaveFile file(path);const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Indented);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()){error=file.errorString();return false;}return true;}
QJsonObject readJson(const QString &path,QString &error){QFile file(path);if(!file.open(QIODevice::ReadOnly)){error=file.errorString();return {};}QJsonParseError parse;const auto document=QJsonDocument::fromJson(file.readAll(),&parse);if(parse.error!=QJsonParseError::NoError || !document.isObject()){error=QStringLiteral("JSON 格式无效。");return {};}return document.object();}
}
void AgentPage::showAccessMenu()
{
    if(session_->isBusy())return;
    const QList<AgentChoice> choices{
        {"software",QStringLiteral("软件内操作"),QStringLiteral("使用 Renderer 工具；通用文件、命令和 MCP 环境工具关闭。")},
        {"full",QStringLiteral("环境完全访问"),QStringLiteral("可操作任意本地路径和命令，使用当前 Windows 账户权限。")}};
    auto *popup=new AgentChoicePopup(QStringLiteral("访问权限"),QStringLiteral("切换后应用于接下来的操作。"),choices,session_->accessMode()==AgentAccessMode::FullEnvironment?"full":"software",this);popup->setObjectName("agentAccessPopup");
    connect(popup,&AgentChoicePopup::chosen,this,[this](const QString &id){if(!session_->isBusy())session_->setAccessMode(id=="full"?AgentAccessMode::FullEnvironment:AgentAccessMode::SoftwareOnly);});popup->openFor(access_);
}

void AgentPage::chooseWorkingDirectory()
{
    if(session_->isBusy())return;
    const auto path=QFileDialog::getExistingDirectory(this,QStringLiteral("选择 Agent 工作目录"),session_->workingDirectory());
    if(!path.isEmpty() && !session_->isBusy() && !session_->setWorkingDirectory(path))report(QStringLiteral("该工作目录不可用。"),true);
}

void AgentPage::updateAccessControls()
{
    const bool full=session_->accessMode()==AgentAccessMode::FullEnvironment;
    access_->setText(full?QStringLiteral("环境完全访问"):QStringLiteral("软件内操作"));access_->setProperty("agentAccessFull",full);
    access_->setToolTip(full?QStringLiteral("环境完全访问：允许终端、文件及环境操作"):QStringLiteral("软件内操作：仅调用 VS Renderer 功能"));access_->setEnabled(!session_->isBusy());access_->style()->unpolish(access_);access_->style()->polish(access_);access_->update();
    const auto path=QDir::toNativeSeparators(session_->workingDirectory());auto name=QFileInfo(path).fileName();if(name.isEmpty())name=path;
    workingDirectory_->setText(QStringLiteral("目录 · ")+name);workingDirectory_->setToolTip(QStringLiteral("当前工作目录：\n")+path+QStringLiteral("\n点击切换"));workingDirectory_->setEnabled(!session_->isBusy());
}

void AgentPage::showProviderSettings()
{
    AgentProviderDialog dialog(providers_,QDir(directory_).filePath("providers.json"),providerIndex_>=0?providers_[providerIndex_].id:QString(),this);
    connect(&dialog,&AgentProviderDialog::applied,this,[this](const QList<AgentProvider> &sources,const QString &selected){providers_=sources;refreshProviders(selected);});
    dialog.exec();
}

void AgentPage::showFeatureSettings()
{
    QDialog dialog(this);dialog.setObjectName("agentFeatureDialog");dialog.setWindowTitle(QStringLiteral("Agent 功能设置"));auto *layout=new QVBoxLayout(&dialog);layout->setSpacing(14);
    auto *hint=new QLabel(QStringLiteral("管理 Agent 的指令、扩展连接和子任务。"),&dialog);hint->setWordWrap(true);layout->addWidget(hint);int selected=0;
    const QStringList labels{QStringLiteral("Skills / 全局 AGENTS"),QStringLiteral("MCP / npm 组件"),QStringLiteral("子 Agent 队列")};
    for(int i=0;i<labels.size();++i){auto *button=new QPushButton(labels[i],&dialog);button->setObjectName(QString("agentFeatureEntry%1").arg(i));layout->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[&,i]{selected=i+1;dialog.accept();});}
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.resize(420,240);dialog.exec();
    if(selected==1)emit skillsSettingsRequested();else if(selected==2)emit mcpSettingsRequested();else if(selected==3)emit queueRequested();
}

void AgentPage::showUsage()
{
    QDialog dialog(this);dialog.setObjectName("agentUsageDialog");dialog.setWindowTitle(QStringLiteral("上下文与 Token 用量"));auto *layout=new QVBoxLayout(&dialog);auto *details=new QLabel(&dialog);details->setObjectName("agentUsageDetails");details->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(details);
    const auto refresh=[&]{const auto u=session_->usage();const auto lastModel=u.value("lastModel").toString();const auto lastInput=u.value("lastPromptTokens").toDouble(),lastCached=u.value("lastCachedTokens").toDouble();QString text=QStringLiteral("当前模型：%1\n上下文：%2 / %3 tokens（%4%）\n累计输入：%5\n累计输出：%6\n累计 tokens：%7\n缓存命中：%8\n推理 tokens：%9\n最近一轮：输入 %10 / 输出 %11\n最近缓存命中：%12 tokens（最近输入的 %13%）\n已整理上下文：%14 次").arg(lastModel.isEmpty()?session_->provider().model:lastModel).arg(u.value("estimatedContextTokens").toInteger()).arg(u.value("contextLimit").toInteger()).arg(u.value("contextPercent").toDouble(),0,'f',1).arg(u.value("promptTokens").toInteger()).arg(u.value("completionTokens").toInteger()).arg(u.value("totalTokens").toInteger()).arg(u.value("cachedTokens").toInteger()).arg(u.value("reasoningTokens").toInteger()).arg(u.value("lastPromptTokens").toInteger()).arg(u.value("lastCompletionTokens").toInteger()).arg(qint64(lastCached)).arg(lastInput>0?100.*lastCached/lastInput:0.,0,'f',1).arg(u.value("compactions").toInt());if(u.value("estimated").toBool())text+=QStringLiteral("\n\n当前上下文为估算值，账单以端点返回为准。");details->setText(text);};
    refresh();connect(session_,&AgentSession::usageChanged,&dialog,[&](const QJsonObject &){refresh();});auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog);auto *compact=buttons->addButton(QStringLiteral("整理上下文"),QDialogButtonBox::ActionRole);compact->setObjectName("agentCompactContext");compact->setEnabled(!session_->isBusy());connect(session_,&AgentSession::busyChanged,&dialog,[compact](bool busy){compact->setEnabled(!busy);});connect(compact,&QPushButton::clicked,session_,&AgentSession::compactContext);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(buttons);dialog.resize(490,390);dialog.exec();
}

void AgentPage::showTools()
{
    QDialog dialog(this); dialog.setObjectName("agentToolsDialog"); dialog.setWindowTitle(QStringLiteral("Agent 工具管理"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *hint = new QLabel(QStringLiteral("勾选允许 Agent 调用的功能。导入的工具组合只调用已有工具，参数可用 $参数名 传入。"), &dialog); hint->setWordWrap(true); layout->addWidget(hint);
    auto *scroll = new QScrollArea(&dialog); scroll->setWidgetResizable(true); auto *content = new QWidget(scroll); auto *list = new QVBoxLayout(content); scroll->setWidget(content); layout->addWidget(scroll, 1);
    const auto refresh = [&] {
        while (auto *item = list->takeAt(0)) { delete item->widget(); delete item; }
        for (const auto &entry : session_->allTools()) {
            const auto function = entry.toObject().value("function").toObject(); const auto name = function.value("name").toString();
            auto *row=new QWidget(content);auto *details=new QVBoxLayout(row);details->setContentsMargins(0,4,0,8);details->setSpacing(3);
            const auto label=name+(session_->toolRequiresEnvironment(name)?QStringLiteral("  ·  环境完全访问"):QString());
            auto *check = new QCheckBox(label, row); check->setObjectName("agentTool_" + name); check->setChecked(session_->toolEnabled(name));
            check->setToolTip(session_->toolAvailable(name)?QStringLiteral("当前可调用"):QStringLiteral("当前已禁用，或访问模式/模型能力不满足。勾选设置不会提升访问权限。"));details->addWidget(check);
            auto *description=new QLabel(function.value("description").toString(),row);description->setTextFormat(Qt::PlainText);description->setWordWrap(true);details->addWidget(description);list->addWidget(row);
            connect(check, &QCheckBox::toggled, &dialog, [this, name](bool enabled) {
                session_->setToolEnabled(name, enabled); QStringList disabled;
                for (const auto &tool : session_->allTools()) { const auto key = tool.toObject().value("function").toObject().value("name").toString(); if (!session_->toolEnabled(key)) disabled.append(key); }
                QSettings().setValue("agent/disabledTools", disabled);
            });
        }
        list->addStretch();
    };
    refresh();
    auto *actions = new QHBoxLayout; auto *import = new QPushButton(QStringLiteral("导入工具组合…"), &dialog), *exportTools = new QPushButton(QStringLiteral("导出工具组合…"), &dialog); import->setObjectName("agentImportTools"); exportTools->setObjectName("agentExportTools"); actions->addWidget(import); actions->addWidget(exportTools); actions->addStretch(); layout->addLayout(actions);
    connect(import, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getOpenFileName(&dialog, QStringLiteral("导入工具组合"), {}, "JSON (*.json)"); if (path.isEmpty()) return;
        QString error; const auto document = readJson(path, error);
        if (!error.isEmpty() || !toolLibrary_->importTools(document, &error)) { QMessageBox::warning(&dialog, QStringLiteral("导入失败"), error); return; }
        if (!writeJson(QDir(directory_).filePath("tools.json"), toolLibrary_->exportTools(), error)) report(error, true);
        refresh();
    });
    connect(exportTools, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getSaveFileName(&dialog, QStringLiteral("导出工具组合"), "agent-tools.json", "JSON (*.json)"); if (path.isEmpty()) return;
        QString error; if (!writeJson(path, toolLibrary_->exportTools(), error)) QMessageBox::warning(&dialog, QStringLiteral("导出失败"), error);
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject); layout->addWidget(buttons); dialog.resize(850, 560); dialog.exec();
}

void AgentPage::showPendingMessages()
{
    QDialog dialog(this);dialog.setObjectName("agentPendingDialog");dialog.setWindowTitle(QStringLiteral("待接收消息"));auto *layout=new QVBoxLayout(&dialog);
    auto *hint=new QLabel(QStringLiteral("消息会在完整工具调用组结束后交给 Agent。停止请求会保留并暂停这些消息。"),&dialog);hint->setWordWrap(true);layout->addWidget(hint);
    auto *list=new QListWidget(&dialog);list->setObjectName("agentPendingList");list->setWordWrap(true);layout->addWidget(list,1);
    auto *actions=new QHBoxLayout;auto *edit=new QPushButton(QStringLiteral("取回编辑"),&dialog),*remove=new QPushButton(QStringLiteral("删除"),&dialog),*resume=new QPushButton(QStringLiteral("继续接收"),&dialog);edit->setObjectName("agentPendingEdit");remove->setObjectName("agentPendingRemove");resume->setObjectName("agentPendingResume");for(auto *button:{edit,remove,resume})actions->addWidget(button);layout->addLayout(actions);
    const auto refresh=[&]{QString selected;if(list->currentItem())selected=list->currentItem()->data(Qt::UserRole).toString();const QSignalBlocker blocker(list);list->clear();for(const auto &value:session_->pendingMessages()){const auto item=value.toObject();auto *row=new QListWidgetItem(item.value(item.contains("displayContent")?"displayContent":"content").toString(),list);row->setData(Qt::UserRole,item.value("id"));if(item.value("id").toString()==selected)list->setCurrentItem(row);}if(!list->currentItem() && list->count())list->setCurrentRow(0);resume->setEnabled(session_->pendingPaused() && list->count());edit->setEnabled(list->count());remove->setEnabled(list->count());};
    connect(session_,&AgentSession::pendingMessagesChanged,&dialog,[&](const QJsonArray &){refresh();});refresh();
    connect(edit,&QPushButton::clicked,&dialog,[&]{if(!list->currentItem())return;const auto id=list->currentItem()->data(Qt::UserRole).toString();const auto message=session_->takePendingMessage(id);if(message.isEmpty())return;const auto restored=message.value(message.contains("displayContent")?"displayContent":"content").toString(),draft=input_->toPlainText();input_->setPlainText(draft.isEmpty()?restored:draft+"\n\n"+restored);dialog.accept();input_->setFocus();});
    connect(remove,&QPushButton::clicked,&dialog,[&]{if(list->currentItem())session_->dropPendingMessage(list->currentItem()->data(Qt::UserRole).toString());});connect(resume,&QPushButton::clicked,session_,&AgentSession::resumePending);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog);buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("关闭"));connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(buttons);dialog.resize(620,420);dialog.exec();
}
}
