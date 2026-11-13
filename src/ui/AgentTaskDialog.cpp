#include "ui/AgentTaskDialog.h"
#include "ui/AgentMarkdownView.h"
#include "agent/AgentTaskQueue.h"
#include "agent/AgentSession.h"
#include <QDialogButtonBox>
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace vsr {
AgentTaskDialog::AgentTaskDialog(AgentTaskQueue *queue, AgentSession *parentSession, QWidget *parent) : QDialog(parent)
{
    setObjectName("agentTaskDialog"); setWindowTitle(QStringLiteral("子智能体会话")); resize(1100, 760);
    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(QStringLiteral("在主会话中说“使用子智能体完成…”即可由 AI 自动分工、创建和协调。此处查看实际会话，也可向选中子智能体继续发信。"), this);
    hint->setWordWrap(true); layout->addWidget(hint);
    auto *split = new QSplitter(Qt::Vertical, this); layout->addWidget(split, 1);
    auto *table = new QTreeWidget(split); table->setObjectName("agentTaskList"); table->setHeaderLabels({QStringLiteral("会话 / 任务"), QStringLiteral("状态"), QStringLiteral("模型"), QStringLiteral("Token")});
    for(int column=0;column<table->columnCount();++column)table->headerItem()->setToolTip(column,table->headerItem()->text(column));
    table->header()->setSectionResizeMode(QHeaderView::Interactive);table->header()->setStretchLastSection(false);table->header()->setMinimumSectionSize(60);
    auto sized=std::make_shared<bool>(false);
    connect(table->header(),&QHeaderView::sectionResized,this,[sized,table](int,int,int){if(table->isVisible() && QApplication::mouseButtons().testFlag(Qt::LeftButton))*sized=true;});
    auto *detail=new QStackedWidget(split);auto *empty=new QLabel(QStringLiteral("暂无子智能体会话"),detail);empty->setObjectName("agentTaskEmpty");empty->setAlignment(Qt::AlignCenter);detail->addWidget(empty);
    auto *view = new AgentMarkdownView(detail); view->setObjectName("agentTaskResult");detail->addWidget(view);split->setSizes({180, 420});
    auto *row = new QHBoxLayout; auto *message = new QLineEdit(this); message->setObjectName("agentTaskMessage"); message->setPlaceholderText(QStringLiteral("给选中的子智能体补充内容…（Enter 发送）")); row->addWidget(message, 1);
    auto *send = new QPushButton(QStringLiteral("发送给子智能体"), this); send->setObjectName("agentTaskSend"); row->addWidget(send); layout->addLayout(row);
    auto *status = new QLabel(this); status->setWordWrap(true); layout->addWidget(status);
    auto previous = std::make_shared<QByteArray>();
    const auto render = [queue, table, view, detail, empty, message, send, previous] {
        if (!table->currentItem()) { previous->clear();view->clear();detail->setCurrentWidget(empty);message->setEnabled(false);send->setEnabled(false);return; }
        detail->setCurrentWidget(view);message->setEnabled(true);send->setEnabled(true);
        const auto task = queue->task(table->currentItem()->data(0, Qt::UserRole).toString());
        const auto snapshot = QJsonDocument(task).toJson(QJsonDocument::Compact); if (*previous == snapshot) return; *previous = snapshot;
        auto messages = task.value("messages").toArray();
        if (messages.isEmpty()) {
            messages.append(QJsonObject{{"role", "user"}, {"content", task.value("prompt")}});
            if (!task.value("result").toString().isEmpty()) messages.append(QJsonObject{{"role", "assistant"}, {"content", task.value("result")}});
        }
        if (!task.value("error").toString().isEmpty()) messages.append(QJsonObject{{"role", "assistant"}, {"content", QStringLiteral("**任务错误**\n\n")+task.value("error").toString()}});
        view->showMessages(messages, task.value("streamText").toString(), task.value("pendingMessages").toArray(),task.value("state").toString()=="running");
    };
    const auto refresh = [queue, table, status, render, sized] {
        QString selected; if (table->currentItem()) selected = table->currentItem()->data(0, Qt::UserRole).toString();
        const QSignalBlocker block(table); table->clear();
        const QMap<QString, QString> names{{"queued", QStringLiteral("等待")}, {"running", QStringLiteral("交流中")}, {"completed", QStringLiteral("本轮完成")}, {"failed", QStringLiteral("失败")}, {"cancelled", QStringLiteral("已取消")}, {"interrupted", QStringLiteral("待恢复")}};
        for (const auto &value : queue->tasks()) {
            const auto task = value.toObject(); auto *item = new QTreeWidgetItem(table, {task.value("name").toString(), names.value(task.value("state").toString(), task.value("state").toString()), task.value("model").toString(), QString::number(qint64(task.value("usage").toObject().value("totalTokens").toDouble()))});
            item->setData(0, Qt::UserRole, task.value("id")); if (task.value("id").toString() == selected) table->setCurrentItem(item);
            for(int column=0;column<table->columnCount();++column)item->setToolTip(column,QStringLiteral("<qt>%1</qt>").arg(item->text(column).toHtmlEscaped()));
        }
        if (!table->currentItem() && table->topLevelItemCount()) table->setCurrentItem(table->topLevelItem(0));
        if(!*sized && table->topLevelItemCount())QTimer::singleShot(0,table,[table,sized]{if(*sized)return;*sized=true;const QSignalBlocker block(table->header());int width=0;for(int column=0;column<table->columnCount();++column){table->resizeColumnToContents(column);width+=table->columnWidth(column);}if(width<table->viewport()->width())table->setColumnWidth(0,table->columnWidth(0)+table->viewport()->width()-width);});
        if (!status->property("hasError").toBool()) status->setText(queue->summary());
        render();
    };
    connect(table, &QTreeWidget::currentItemChanged, this, [render](QTreeWidgetItem *) { render(); });
    connect(queue, &AgentTaskQueue::changed, this, refresh); refresh();
    auto *timer = new QTimer(this); timer->setInterval(500); connect(timer, &QTimer::timeout, this, refresh); timer->start();
    const auto sendMessage = [queue, table, message, status] {
        if (!table->currentItem() || message->text().trimmed().isEmpty()) return;
        QString error; if (!queue->sendMessage(table->currentItem()->data(0, Qt::UserRole).toString(), message->text(), &error)) { status->setProperty("hasError", true); status->setText(error); }
        else { status->setProperty("hasError", false); message->clear(); }
    };
    connect(send, &QPushButton::clicked, this, sendMessage); connect(message, &QLineEdit::returnPressed, this, sendMessage);
    auto *actions = new QHBoxLayout;
    auto *run = new QPushButton(QStringLiteral("恢复 / 重试"), this), *cancel = new QPushButton(QStringLiteral("停止选中"), this), *cancelAll = new QPushButton(QStringLiteral("停止全部"), this);
    auto *clear = new QPushButton(QStringLiteral("清理已结束"), this), *import = new QPushButton(QStringLiteral("导入队列"), this), *exportButton = new QPushButton(QStringLiteral("导出队列"), this);
    for (auto *button : {run, cancel, cancelAll, clear, import, exportButton}) actions->addWidget(button);
    layout->addLayout(actions);
    connect(run, &QPushButton::clicked, this, [=] { if (table->currentItem()) queue->resume(table->currentItem()->data(0, Qt::UserRole).toString(), parentSession->provider()); });
    connect(cancel, &QPushButton::clicked, this, [=] { if (table->currentItem()) queue->cancel(table->currentItem()->data(0, Qt::UserRole).toString()); });
    connect(cancelAll, &QPushButton::clicked, queue, &AgentTaskQueue::cancelAll); connect(clear, &QPushButton::clicked, queue, &AgentTaskQueue::clearFinished);
    connect(import, &QPushButton::clicked, this, [=] {
        const auto path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("导入子会话队列"), {}, "JSON (*.json)"); if (path.isEmpty()) return;
        QFile file(path); QString error;
        if (!file.open(QIODevice::ReadOnly) || file.size() > 16*1024*1024 || !queue->importTasks(QJsonDocument::fromJson(file.readAll()).object(), &error)) { status->setProperty("hasError", true); status->setText(error.isEmpty() ? QStringLiteral("队列文件读取失败。") : error); }
    });
    connect(exportButton, &QPushButton::clicked, this, [=] {
        const auto path = QFileDialog::getSaveFileName(nullptr, QStringLiteral("导出子会话队列"), "agent-tasks.json", "JSON (*.json)"); if (path.isEmpty()) return;
        QSaveFile file(path); const auto data = QJsonDocument(queue->exportTasks()).toJson();
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) { status->setProperty("hasError", true); status->setText(QStringLiteral("队列导出失败。")); }
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject); layout->addWidget(buttons);
}
}
