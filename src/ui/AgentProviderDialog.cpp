#include "ui/AgentProviderDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFontInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonArray>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QUuid>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
AgentProviderDialog::AgentProviderDialog(QList<AgentProvider> providers, QString configPath,
                                         const QString &selectedId, QWidget *parent)
    : QDialog(parent), providers_(std::move(providers)), path_(std::move(configPath))
{
    setObjectName("agentProviderDialog"); setWindowTitle(QStringLiteral("Agent 来源设置"));
    setStyleSheet(QStringLiteral(
        "QDialog#agentProviderDialog QSpinBox { min-height:34px; padding:0 36px 0 8px; }"
        "QDialog#agentProviderDialog QSpinBox QLineEdit { min-height:0; padding:0; border:0; background:transparent; }"
        "QDialog#agentProviderDialog QSpinBox::up-button { subcontrol-origin:border; subcontrol-position:top right; width:28px; height:17px; }"
        "QDialog#agentProviderDialog QSpinBox::down-button { subcontrol-origin:border; subcontrol-position:bottom right; width:28px; height:17px; }"));
    resize(940, 650); setMinimumSize(780, 580);
    auto *layout = new QVBoxLayout(this); layout->setContentsMargins(20, 18, 20, 16); layout->setSpacing(16);
    auto *heading = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("来源与模型"), this);title->setObjectName("agentProviderHeading");auto font = title->font();font.setPixelSize(std::max(16,QFontInfo(font).pixelSize()+4));font.setBold(true);title->setFont(font);heading->addWidget(title);heading->addStretch();
    auto *add = new QPushButton(QStringLiteral("＋ 添加来源"), this); add->setObjectName("agentProviderAdd"); heading->addWidget(add); layout->addLayout(heading);
    auto *body = new QHBoxLayout;
    auto *sidebar = new QWidget(this); auto *side = new QVBoxLayout(sidebar); side->setContentsMargins(0,0,12,0);
    list_ = new QListWidget(sidebar); list_->setObjectName("agentProviderList"); list_->setMinimumWidth(180); list_->setMaximumWidth(230); side->addWidget(list_,1);
    auto *remove = new QPushButton(QStringLiteral("删除所选来源"), sidebar); remove->setObjectName("agentProviderRemove"); side->addWidget(remove); body->addWidget(sidebar);
    auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    form_ = new QWidget(scroll); scroll->setWidget(form_); auto *form = new QFormLayout(form_); form->setContentsMargins(10,4,4,4); form->setVerticalSpacing(16); form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    name_ = new QLineEdit(form_); name_->setObjectName("agentProviderName"); form->addRow(QStringLiteral("来源名称"),name_);
    endpoint_ = new QLineEdit(form_); endpoint_->setObjectName("agentProviderUrl"); endpoint_->setPlaceholderText("https://api.deepseek.com/v1"); form->addRow(QStringLiteral("API 端点"),endpoint_);
    key_ = new QLineEdit(form_); key_->setObjectName("agentProviderKey"); key_->setEchoMode(QLineEdit::Password); form->addRow(QStringLiteral("API Key"),key_);
    model_ = new QLineEdit(form_); model_->setObjectName("agentProviderModel"); model_->setPlaceholderText("Model ID"); form->addRow(QStringLiteral("默认模型"),model_);
    reasoning_ = new QComboBox(form_); reasoning_->setObjectName("agentProviderReasoning"); reasoning_->addItem(QStringLiteral("端点默认"),""); reasoning_->setToolTip(QStringLiteral("应用来源后，可在主界面按模型实际能力选择思考强度。")); form->addRow(QStringLiteral("推理级别"),reasoning_);
    context_ = new QSpinBox(form_); context_->setObjectName("agentProviderContext"); context_->setRange(4096,2000000); context_->setSingleStep(4096); context_->setSuffix(" tokens"); form->addRow(QStringLiteral("上下文上限"),context_);
    output_ = new QSpinBox(form_); output_->setObjectName("agentProviderMaxOutput"); output_->setRange(0,2000000); output_->setSingleStep(1024); output_->setSpecialValueText(QStringLiteral("端点默认")); form->addRow(QStringLiteral("最大输出 tokens"),output_);
    auto *hint = new QLabel(QStringLiteral("兼容 OpenAI Chat Completions。上下文达到上限的 85% 时自动整理；输出设为“端点默认”时不发送输出长度参数。密钥仅在当前 Windows 用户下加密保存。"),form_); hint->setWordWrap(true); form->addRow(hint); body->addWidget(scroll,1); layout->addLayout(body,1);
    auto *exchange = new QHBoxLayout;
    auto *ffui = new QPushButton(QStringLiteral("导入 3FUI 配置…"),this), *import = new QPushButton(QStringLiteral("导入来源…"),this), *exportButton = new QPushButton(QStringLiteral("导出来源…"),this);
    ffui->setObjectName("agentProviderImportFfui"); import->setObjectName("agentProviderImport"); exportButton->setObjectName("agentProviderExport"); exportButton->setToolTip(QStringLiteral("导出全部来源，不包含 API Key。")); exchange->addWidget(ffui);exchange->addWidget(import);exchange->addWidget(exportButton);exchange->addStretch();layout->addLayout(exchange);
    status_ = new QLabel(this);status_->setObjectName("agentProviderStatus");status_->setWordWrap(true);layout->addWidget(status_);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply,this);buttons->setObjectName("agentProviderButtons");buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));buttons->button(QDialogButtonBox::Apply)->setText(QStringLiteral("应用"));layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,this,[this]{if(apply())accept();});connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this]{apply();});
    connect(list_,&QListWidget::currentRowChanged,this,&AgentProviderDialog::selectProvider);
    connect(name_,&QLineEdit::textChanged,this,[this](const QString &text){if(active_>=0 && active_<list_->count())list_->item(active_)->setText(text.isEmpty()?QStringLiteral("未命名来源"):text);});
    connect(context_,&QSpinBox::valueChanged,output_,&QSpinBox::setMaximum);
    connect(add,&QPushButton::clicked,this,[this]{storeForm();AgentProvider source;source.id=QUuid::createUuid().toString(QUuid::WithoutBraces);source.name=QStringLiteral("新来源");source.baseUrl="https://api.deepseek.com/v1";source.model="deepseek-flash";providers_.append(source);refreshList(source.id);name_->setFocus();name_->selectAll();});
    connect(remove,&QPushButton::clicked,this,[this]{if(active_<0)return;storeForm();const int row=active_;providers_.removeAt(row);refreshList(providers_.isEmpty()?QString():providers_[qMin(row,int(providers_.size())-1)].id);});
    connect(ffui,&QPushButton::clicked,this,[this]{importProviders(true);});connect(import,&QPushButton::clicked,this,[this]{importProviders(false);});connect(exportButton,&QPushButton::clicked,this,&AgentProviderDialog::exportProviders);
    refreshList(selectedId);
}

void AgentProviderDialog::storeForm()
{
    if(active_<0 || active_>=providers_.size())return;
    auto &source=providers_[active_];source.name=name_->text().trimmed();source.baseUrl=endpoint_->text().trimmed();source.apiKey=key_->text().trimmed();source.model=model_->text().trimmed();source.reasoningEffort=reasoning_->currentData().toString();source.contextLimit=context_->value();source.maxOutputTokens=output_->value();
}

void AgentProviderDialog::selectProvider(int row)
{
    storeForm();active_=row;form_->setEnabled(row>=0 && row<providers_.size());
    if(!form_->isEnabled()){name_->clear();endpoint_->clear();key_->clear();model_->clear();return;}
    const auto source=providers_[row];name_->setText(source.name);endpoint_->setText(source.baseUrl);key_->setText(source.apiKey);model_->setText(source.model);
    reasoning_->clear();reasoning_->addItem(QStringLiteral("端点默认"),"");
    for(const auto &value:source.modelCapability.value("reasoningLevels").toArray()){const auto level=value.toObject();if(level.value("id").toString()!="auto")reasoning_->addItem(level.value("label").toString(),level.value("id"));}
    if(!source.reasoningEffort.isEmpty() && reasoning_->findData(source.reasoningEffort)<0)reasoning_->addItem(source.reasoningEffort=="auto"?QStringLiteral("自动"):source.reasoningEffort+QStringLiteral("（应用后验证）"),source.reasoningEffort);
    reasoning_->setCurrentIndex(std::max(0,reasoning_->findData(source.reasoningEffort)));context_->setValue(source.contextLimit);output_->setMaximum(source.contextLimit);output_->setValue(source.maxOutputTokens);
}

void AgentProviderDialog::refreshList(const QString &selectedId)
{
    int selected=-1;
    {const QSignalBlocker blocker(list_);list_->clear();active_=-1;for(int i=0;i<providers_.size();++i){list_->addItem(providers_[i].name);if(providers_[i].id==selectedId)selected=i;}if(selected<0 && !providers_.isEmpty())selected=0;list_->setCurrentRow(selected);}
    selectProvider(selected);
}

bool AgentProviderDialog::apply()
{
    storeForm();QString error;
    for(int i=0;i<providers_.size();++i){if(providers_[i].name.isEmpty())error=QStringLiteral("请填写来源名称。");else providers_[i].validate(&error);if(!error.isEmpty()){list_->setCurrentRow(i);status_->setText(error);return false;}}
    if(!AgentProviderStore::save(path_,providers_,&error)){status_->setText(error);return false;}
    status_->setText(QStringLiteral("来源设置已保存。"));emit applied(providers_,active_>=0?providers_[active_].id:QString());return true;
}

void AgentProviderDialog::importProviders(bool ffui)
{
    const auto path=QFileDialog::getOpenFileName(this,ffui?QStringLiteral("选择 3FUI Settings.json"):QStringLiteral("导入 Agent 来源"),{},"JSON (*.json)");if(path.isEmpty())return;
    QString error;auto imported=ffui?AgentProviderStore::importFfui(path,&error):AgentProviderStore::load(path,&error);if(!error.isEmpty() || imported.isEmpty()){status_->setText(error.isEmpty()?QStringLiteral("文件中没有来源配置。"):error);return;}
    storeForm();QString selected;
    for(auto &source:imported){auto existing=std::find_if(providers_.begin(),providers_.end(),[&](const auto &entry){return entry.baseUrl==source.baseUrl && entry.model==source.model;});if(existing!=providers_.end()){source.id=existing->id;if(source.apiKey.isEmpty())source.apiKey=existing->apiKey;*existing=source;}else{source.id=QUuid::createUuid().toString(QUuid::WithoutBraces);providers_.append(source);}selected=source.id;}
    refreshList(selected);status_->setText(QStringLiteral("已导入 %1 个来源，点击应用保存。").arg(imported.size()));
}

void AgentProviderDialog::exportProviders()
{
    storeForm();const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("导出来源（不含密钥）"),"agent-providers.json","JSON (*.json)");if(path.isEmpty())return;
    auto exported=providers_;for(auto &source:exported)source.apiKey.clear();QString error;if(!AgentProviderStore::save(path,exported,&error))status_->setText(error);else status_->setText(QStringLiteral("已导出全部来源，文件不包含 API Key。"));
}
}
