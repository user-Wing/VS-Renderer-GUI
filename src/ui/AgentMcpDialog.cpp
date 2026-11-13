#include "ui/AgentMcpDialog.h"
#include "agent/AgentMcp.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QUuid>
#include <QVBoxLayout>

namespace vsr {
namespace {
QString stateText(const QString &state)
{
    if(state=="connected")return QStringLiteral("已连接");if(state=="connecting")return QStringLiteral("连接中");if(state=="error")return QStringLiteral("连接失败");return QStringLiteral("未连接");
}
}
AgentMcpDialog::AgentMcpDialog(AgentMcpManager *manager,QWidget *parent):QDialog(parent),manager_(manager)
{
    setObjectName("agentMcpDialog");setWindowTitle(QStringLiteral("MCP 连接与 npm 组件"));resize(900,600);
    auto *layout=new QVBoxLayout(this);auto *runtime=new QHBoxLayout;
    runtimeStatus_=new QLabel(this);runtimeStatus_->setWordWrap(true);runtime->addWidget(runtimeStatus_,1);
    install_=new QPushButton(QStringLiteral("安装 / 修复 npm 组件"),this);install_->setObjectName("agentMcpInstall");runtime->addWidget(install_);layout->addLayout(runtime);
    auto *help=new QLabel(QStringLiteral("连接 stdio 或 Streamable HTTP MCP 服务后，其工具会加入 Agent。导入的来源保持断开；确认命令、参数和权限后再连接。"),this);help->setWordWrap(true);layout->addWidget(help);
    auto *splitter=new QSplitter(this);servers_=new QListWidget(splitter);servers_->setObjectName("agentMcpServers");tools_=new QListWidget(splitter);tools_->setObjectName("agentMcpTools");splitter->addWidget(servers_);splitter->addWidget(tools_);splitter->setStretchFactor(1,1);layout->addWidget(splitter,1);
    serverStatus_=new QLabel(this);serverStatus_->setWordWrap(true);layout->addWidget(serverStatus_);
    auto *actions=new QHBoxLayout;
    const auto button=[&](const QString &text,const std::function<void()> &action){auto *item=new QPushButton(text,this);actions->addWidget(item);connect(item,&QPushButton::clicked,this,action);};
    button(QStringLiteral("新增"),[this]{edit(true);});button(QStringLiteral("编辑"),[this]{edit(false);});
    button(QStringLiteral("删除"),[this]{const auto id=selectedId();if(id.isEmpty())return;auto configs=manager_->servers();for(int i=0;i<configs.size();++i)if(configs[i].id==id){configs.removeAt(i);break;}QString error;if(!manager_->setServers(configs,&error) || !manager_->save(&error))QMessageBox::warning(this,windowTitle(),error);refresh();});
    button(QStringLiteral("连接"),[this]{const auto id=selectedId();if(!id.isEmpty())manager_->connectServer(id);});
    button(QStringLiteral("断开"),[this]{const auto id=selectedId();if(!id.isEmpty())manager_->disconnectServer(id);});
    button(QStringLiteral("刷新工具"),[this]{manager_->refreshTools(selectedId());});actions->addStretch();
    button(QStringLiteral("导入"),[this]{const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("导入 MCP 配置"),{},"JSON (*.json)");if(path.isEmpty())return;QString error;if(!manager_->importConfiguration(path,&error))QMessageBox::warning(this,windowTitle(),error);refresh();});
    button(QStringLiteral("导出"),[this]{const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("导出 MCP 配置（不含环境变量与请求头）"),"mcp-servers.json","JSON (*.json)");if(path.isEmpty())return;QString error;if(!manager_->exportConfiguration(path,&error))QMessageBox::warning(this,windowTitle(),error);});layout->addLayout(actions);
    auto *close=new QDialogButtonBox(QDialogButtonBox::Close,this);connect(close,&QDialogButtonBox::rejected,this,&QDialog::reject);layout->addWidget(close);
    connect(servers_,&QListWidget::currentRowChanged,this,[this]{const auto id=selectedId();tools_->clear();for(const auto &value:manager_->serverTools(id)){const auto tool=value.toObject();auto *item=new QListWidgetItem(tool.value("name").toString(),tools_);item->setToolTip(tool.value("description").toString());}serverStatus_->setText(id.isEmpty()?QStringLiteral("选择来源以查看连接状态与可用工具。"):QStringLiteral("状态：%1 · %2 个工具").arg(stateText(manager_->state(id))).arg(tools_->count()));});
    connect(manager_,&AgentMcpManager::serverChanged,this,[this](const QString &){refresh();});connect(manager_,&AgentMcpManager::toolsChanged,this,&AgentMcpDialog::refresh);
    connect(manager_,&AgentMcpManager::errorOccurred,this,[this](const QString &error){serverStatus_->setText(error);});
    connect(install_,&QPushButton::clicked,this,[this]{manager_->runtime()->install();refresh();});
    connect(manager_->runtime(),&AgentNodeRuntime::statusChanged,this,[this](const QString &status){runtimeStatus_->setText(status);install_->setEnabled(false);});
    connect(manager_->runtime(),&AgentNodeRuntime::installFinished,this,[this](bool,const QString &message){refresh();runtimeStatus_->setText(message);});refresh();
}

QString AgentMcpDialog::selectedId() const{return servers_->currentItem()?servers_->currentItem()->data(Qt::UserRole).toString():QString();}

void AgentMcpDialog::refresh()
{
    const auto selected=selectedId();servers_->clear();for(const auto &config:manager_->servers()){
        auto *item=new QListWidgetItem(QStringLiteral("%1 · %2").arg(config.name,stateText(manager_->state(config.id))),servers_);item->setData(Qt::UserRole,config.id);if(config.id==selected)servers_->setCurrentItem(item);
    }if(!servers_->currentItem() && servers_->count())servers_->setCurrentRow(0);
    runtimeStatus_->setText(manager_->runtime()->ready()?QStringLiteral("Node 与 MCP npm 组件已就绪。\n%1").arg(manager_->runtime()->directory()):QStringLiteral("MCP npm 组件未安装；安装只写入 Agent 独立运行目录。"));install_->setEnabled(!manager_->runtime()->isInstalling());
}

void AgentMcpDialog::edit(bool create)
{
    auto configs=manager_->servers();int index=-1;AgentMcpConfig config;config.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    if(!create){const auto id=selectedId();for(int i=0;i<configs.size();++i)if(configs[i].id==id){index=i;config=configs[i];break;}if(index<0)return;}
    QDialog dialog(this);dialog.setWindowTitle(create?QStringLiteral("新增 MCP 来源"):QStringLiteral("编辑 MCP 来源"));auto *form=new QFormLayout(&dialog);
    auto *name=new QLineEdit(config.name,&dialog);auto *transport=new QComboBox(&dialog);transport->addItem("stdio","stdio");transport->addItem("Streamable HTTP","http");transport->setCurrentIndex(config.transport=="http"?1:0);
    auto *command=new QLineEdit(config.command,&dialog),*cwd=new QLineEdit(config.workingDirectory,&dialog),*url=new QLineEdit(config.url,&dialog);
    auto *args=new QPlainTextEdit(&dialog),*environment=new QPlainTextEdit(&dialog),*headers=new QPlainTextEdit(&dialog);QJsonArray values;for(const auto &arg:config.arguments)values.append(arg);
    args->setPlainText(QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact)));environment->setPlainText(QString::fromUtf8(QJsonDocument(config.environment).toJson(QJsonDocument::Indented)));headers->setPlainText(QString::fromUtf8(QJsonDocument(config.headers).toJson(QJsonDocument::Indented)));
    for(auto *editor:{args,environment,headers})editor->setMaximumHeight(90);
    auto *enabled=new QCheckBox(QStringLiteral("启动时自动连接此来源"),&dialog);enabled->setChecked(config.enabled);
    form->addRow(QStringLiteral("名称"),name);form->addRow(QStringLiteral("传输"),transport);form->addRow(QStringLiteral("可执行文件"),command);form->addRow(QStringLiteral("参数 JSON 数组"),args);form->addRow(QStringLiteral("工作目录"),cwd);form->addRow(QStringLiteral("环境变量 JSON"),environment);form->addRow(QStringLiteral("MCP URL"),url);form->addRow(QStringLiteral("请求头 JSON"),headers);form->addRow(enabled);
    auto *note=new QLabel(QStringLiteral("stdio 会运行这里明确指定的程序。环境变量与请求头在 Windows 本地加密保存，导出时省略。"),&dialog);note->setWordWrap(true);form->addRow(note);
    const auto visibility=[&]{const bool http=transport->currentData()=="http";for(auto *widget:QList<QWidget*>{command,args,cwd,environment})form->setRowVisible(widget,!http);form->setRowVisible(url,http);form->setRowVisible(headers,http);};connect(transport,&QComboBox::currentIndexChanged,&dialog,visibility);visibility();
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        QJsonParseError parse;const auto arguments=QJsonDocument::fromJson(args->toPlainText().toUtf8(),&parse);if(parse.error!=QJsonParseError::NoError || !arguments.isArray()){QMessageBox::warning(&dialog,dialog.windowTitle(),QStringLiteral("参数须为 JSON 字符串数组。"));return;}
        QStringList parsedArgs;for(const auto &arg:arguments.array()){if(!arg.isString()){QMessageBox::warning(&dialog,dialog.windowTitle(),QStringLiteral("每个参数须为字符串。"));return;}parsedArgs.append(arg.toString());}
        const auto env=QJsonDocument::fromJson(environment->toPlainText().toUtf8(),&parse);if(parse.error!=QJsonParseError::NoError || !env.isObject()){QMessageBox::warning(&dialog,dialog.windowTitle(),QStringLiteral("环境变量须为 JSON 对象。"));return;}
        const auto requestHeaders=QJsonDocument::fromJson(headers->toPlainText().toUtf8(),&parse);if(parse.error!=QJsonParseError::NoError || !requestHeaders.isObject()){QMessageBox::warning(&dialog,dialog.windowTitle(),QStringLiteral("请求头须为 JSON 对象。"));return;}
        config.name=name->text().trimmed();config.transport=transport->currentData().toString();config.command=command->text().trimmed();config.arguments=parsedArgs;config.workingDirectory=cwd->text().trimmed();config.url=url->text().trimmed();config.environment=env.object();config.headers=requestHeaders.object();config.enabled=enabled->isChecked();QString error;
        if(!config.validate(&error)){QMessageBox::warning(&dialog,dialog.windowTitle(),error);return;}dialog.accept();
    });dialog.resize(650,dialog.sizeHint().height());if(dialog.exec()!=QDialog::Accepted)return;
    if(create)configs.append(config);else configs[index]=config;QString error;
    if(!manager_->setServers(configs,&error) || !manager_->save(&error)){QMessageBox::warning(this,windowTitle(),error);return;}refresh();if(config.enabled)manager_->connectServer(config.id);
}
}
