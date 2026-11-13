#include "ui/AgentInstructionsDialog.h"
#include "agent/AgentInstructions.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QVBoxLayout>

namespace vsr {
AgentInstructionsDialog::AgentInstructionsDialog(AgentInstructions *instructions, QWidget *parent) : QDialog(parent)
{
    setObjectName("agentInstructionsDialog");
    setWindowTitle(QStringLiteral("Agent 技能与全局指令"));
    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this); layout->addWidget(tabs, 1);
    auto *skillsPage = new QWidget(tabs); auto *skillsLayout = new QVBoxLayout(skillsPage);
    auto *hint = new QLabel(QStringLiteral("勾选启用技能。每轮只载入名称和说明，Agent 在需要时读取 SKILL.md 及包内资料；导入会保留完整文件夹。"), skillsPage);
    hint->setWordWrap(true); skillsLayout->addWidget(hint);
    auto *list = new QListWidget(skillsPage); list->setObjectName("agentSkillsList"); skillsLayout->addWidget(list, 1);
    auto *detail = new QLabel(skillsPage); detail->setWordWrap(true); detail->setTextInteractionFlags(Qt::TextSelectableByMouse); skillsLayout->addWidget(detail);
    auto *actions = new QHBoxLayout;
    auto *importFile = new QPushButton(QStringLiteral("导入技能文件…"), skillsPage); importFile->setObjectName("agentImportSkillFile"); actions->addWidget(importFile);
    auto *importFolder = new QPushButton(QStringLiteral("导入技能文件夹…"), skillsPage); importFolder->setObjectName("agentImportSkillFolder"); actions->addWidget(importFolder);
    auto *remove = new QPushButton(QStringLiteral("删除技能"), skillsPage); remove->setObjectName("agentRemoveSkill"); actions->addWidget(remove);
    actions->addStretch(); skillsLayout->addLayout(actions);
    tabs->addTab(skillsPage, QStringLiteral("Skills"));
    auto *promptPage = new QWidget(tabs); auto *promptLayout = new QVBoxLayout(promptPage);
    auto *promptPath = new QLabel(instructions->globalPromptPath(), promptPage); promptPath->setWordWrap(true); promptPath->setTextInteractionFlags(Qt::TextSelectableByMouse); promptLayout->addWidget(promptPath);
    auto *prompt = new QPlainTextEdit(promptPage); prompt->setObjectName("agentGlobalPrompt"); promptLayout->addWidget(prompt, 1);
    promptLayout->addWidget(new QLabel(QStringLiteral("保存后用于后续请求及新建子智能体；不会修改 Codex 的指令文件。"), promptPage));
    tabs->addTab(promptPage, QStringLiteral("全局 AGENTS.md"));
    auto *status = new QLabel(this); status->setObjectName("agentInstructionsStatus"); status->setWordWrap(true); layout->addWidget(status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto *reload = buttons->addButton(QStringLiteral("重新读取"), QDialogButtonBox::ActionRole); reload->setObjectName("agentReloadInstructions");
    auto *save = buttons->addButton(QStringLiteral("保存并应用全局指令"), QDialogButtonBox::ActionRole); save->setObjectName("agentSaveGlobalPrompt");
    layout->addWidget(buttons);
    const auto refreshList = [instructions, list] {
        const auto selected = list->currentItem() ? list->currentItem()->data(Qt::UserRole).toString() : QString();
        const QSignalBlocker blocker(list); list->clear();
        for (const auto &value : instructions->skills()) {
            const auto skill = value.toObject(); const auto name = skill.value("name").toString();
            auto *item = new QListWidgetItem(name + " — " + skill.value("description").toString(), list);
            item->setData(Qt::UserRole, name); item->setData(Qt::UserRole + 1, skill.value("path").toString()); item->setFlags(item->flags() | Qt::ItemIsUserCheckable); item->setCheckState(skill.value("enabled").toBool() ? Qt::Checked : Qt::Unchecked);
            if (name == selected) list->setCurrentItem(item);
        }
    };
    const auto reloadAll = [instructions, prompt, status] {
        QString error;
        if (!instructions->refresh(&error)) { status->setText(error); return; }
        prompt->setPlainText(instructions->loadGlobalPrompt(&error));
        status->setText(error.isEmpty() ? QStringLiteral("已读取本地技能与全局指令。") : error);
    };
    connect(instructions, &AgentInstructions::changed, this, refreshList);
    connect(list, &QListWidget::currentItemChanged, this, [detail, remove](QListWidgetItem *item) { remove->setEnabled(item); detail->setText(item ? item->data(Qt::UserRole + 1).toString() : QString()); });
    connect(list, &QListWidget::itemChanged, this, [instructions, list, status](QListWidgetItem *item) {
        const auto name = item->data(Qt::UserRole).toString(); const bool enabled = item->checkState() == Qt::Checked; QString error;
        if (!instructions->setSkillEnabled(name, enabled, &error)) {
            const QSignalBlocker blocker(list); item->setCheckState(enabled ? Qt::Unchecked : Qt::Checked); status->setText(error);
        } else status->setText(QStringLiteral("技能启用状态已保存。"));
    });
    const auto import = [this, instructions, status](bool folder) {
        const auto path = folder ? QFileDialog::getExistingDirectory(this, QStringLiteral("选择包含 SKILL.md 的技能文件夹")) : QFileDialog::getOpenFileName(this, QStringLiteral("选择技能 Markdown"), {}, "Markdown (*.md)");
        if (path.isEmpty()) return;
        QString error; status->setText(instructions->importSkill(path, &error) ? QStringLiteral("技能已导入，可在列表中启用或停用。") : error);
    };
    connect(importFile, &QPushButton::clicked, this, [import] { import(false); });
    connect(importFolder, &QPushButton::clicked, this, [import] { import(true); });
    connect(remove, &QPushButton::clicked, this, [this, instructions, list, status] {
        if (!list->currentItem()) return;
        const auto name = list->currentItem()->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, QStringLiteral("删除技能"), QStringLiteral("删除本软件中已导入的技能“%1”？原始导入文件保留。").arg(name)) != QMessageBox::Yes) return;
        QString error; status->setText(instructions->removeSkill(name, &error) ? QStringLiteral("技能已删除。") : error);
    });
    connect(reload, &QPushButton::clicked, this, reloadAll);
    connect(save, &QPushButton::clicked, this, [instructions, prompt, status] {
        QString error; status->setText(instructions->saveGlobalPrompt(prompt->toPlainText(), &error) ? QStringLiteral("全局指令已保存并应用。") : error);
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    remove->setEnabled(false); reloadAll(); refreshList(); resize(880, 650);
}
}
