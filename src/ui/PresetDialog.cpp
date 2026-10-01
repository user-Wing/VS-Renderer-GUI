#include "ui/PresetDialog.h"
#include "graph/PresetStore.h"
#include "graph/FilterCatalog.h"
#include <QAbstractItemModel>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>

namespace vsr {
PresetDialog::PresetDialog(std::function<QString(const QString &)> saveCurrent, std::function<void(const QString &)> load,
                           std::function<void()> reset, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("预设管理 · VPY")); resize(980, 640);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("双击或读取加载预设；拖动排序；Delete 删除到回收站。预设保存在便携目录 vpy。"), this));
    auto *split = new QSplitter(this);
    auto *list = new QListWidget(split); list->setObjectName("presetList"); list->setDragDropMode(QAbstractItemView::InternalMove);
    auto *overview = new QPlainTextEdit(split); overview->setReadOnly(true); overview->setPlaceholderText(QStringLiteral("参数总览"));
    auto *script = new QPlainTextEdit(split); script->setReadOnly(true); script->setPlaceholderText(QStringLiteral("VPY 脚本预览")); script->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(split, 1); split->setSizes({240, 280, 460});
    auto *name = new QLineEdit(this); name->setObjectName("presetName"); name->setPlaceholderText(QStringLiteral("预设名称"));
    auto *note = new QLineEdit(this); note->setPlaceholderText(QStringLiteral("预设备注"));
    layout->addWidget(name); layout->addWidget(note);
    const auto selected = [list] { return list->currentItem() ? list->currentItem()->data(Qt::UserRole).toString() : QString(); };
    const auto refresh = [list] {
        const QString previous = list->currentItem() ? list->currentItem()->data(Qt::UserRole).toString() : QString();
        list->clear(); QDir dir(PresetStore::directory()); dir.mkpath(".");
        QStringList files = dir.entryList({"*.vpy"}, QDir::Files, QDir::Name);
        QFile order(dir.filePath(".preset-order.txt"));
        if (order.open(QIODevice::ReadOnly)) {
            QStringList sorted;
            for (const auto &entry : QString::fromUtf8(order.readAll()).split('\n'))
                if (files.removeOne(entry)) sorted << entry;
            files = sorted + files;
        }
        for (const auto &file : files) {
            auto *item = new QListWidgetItem(QFileInfo(file).completeBaseName(), list);
            item->setData(Qt::UserRole, dir.filePath(file)); item->setToolTip(dir.filePath(file));
            if (dir.filePath(file) == previous) list->setCurrentItem(item);
        }
    };
    const auto selectPath = [list](const QString &path) { for (int i=0; i<list->count(); ++i) if (list->item(i)->data(Qt::UserRole).toString() == path) { list->setCurrentRow(i); break; } };
    const auto error = [this](const QString &message) { QMessageBox::warning(this, QStringLiteral("预设管理"), message); };
    connect(list, &QListWidget::currentItemChanged, this, [=] {
        if (selected().isEmpty()) { name->clear(); note->clear(); script->clear(); overview->clear(); return; }
        const auto result = PresetStore::load(selected());
        const auto data = PresetStore::metadata(result.script);
        name->setText(QFileInfo(selected()).completeBaseName()); note->setText(data.value("note").toString());
        script->setPlainText(result.script);
        QStringList summary;
        if (data.isEmpty()) summary << QStringLiteral("外部 VPY：直接运行脚本，不重建处理链。");
        else {
            summary << QStringLiteral("源滤镜：%1").arg(data.value("sourceFilter").toInt() == 0 ? "L-SMASH Works" : "FFMS2") << QStringLiteral("备注：%1").arg(data.value("note").toString()) << "";
            const auto graph = PresetStore::graph(data);
            for (const auto &node : graph.nodes()) {
                const auto *definition = FilterCatalog::find(node.definitionId); if (!definition) continue;
                summary << (node.enabled ? QStringLiteral("✓ ") : QStringLiteral("停用 · ")) + definition->name;
                for (const auto &parameter : definition->parameters) summary << "  " + parameter.label + "：" + node.parameters.value(parameter.id).toString();
                summary << "";
            }
        }
        overview->setPlainText(summary.join('\n'));
    });
    connect(list->model(), &QAbstractItemModel::rowsMoved, this, [=] {
        QStringList order; for (int i = 0; i < list->count(); ++i) order << QFileInfo(list->item(i)->data(Qt::UserRole).toString()).fileName();
        QString message; if (!PresetStore::write(QDir(PresetStore::directory()).filePath(".preset-order.txt"), order.join('\n'), &message)) error(message);
    });
    auto *buttons = new QHBoxLayout; layout->addLayout(buttons);
    const auto button = [=](const QString &title, auto action) {
        auto *b = new QPushButton(title, this); buttons->addWidget(b); connect(b, &QPushButton::clicked, this, action);
    };
    const auto safeName = [=] {
        const QString n = name->text().trimmed();
        if (n.isEmpty() || n == "." || n == ".." || n.contains(QRegularExpression("[<>:\"/\\\\|?*]")) || n.endsWith('.') || n.endsWith(' ')) return QString();
        return QDir(PresetStore::directory()).filePath(n + ".vpy");
    };
    const auto confirmOverwrite = [this](const QString &path) {
        return !QFileInfo::exists(path) || QMessageBox::question(this, QStringLiteral("覆盖预设"), QStringLiteral("覆盖 %1？").arg(QFileInfo(path).fileName())) == QMessageBox::Yes;
    };
    button(QStringLiteral("保存当前"), [=] {
        const QString path = safeName(); if (path.isEmpty()) { error(QStringLiteral("请输入有效预设名称。")); return; }
        if (!confirmOverwrite(path)) return;
        const auto text = saveCurrent(note->text()); if (text.isEmpty()) { error(QStringLiteral("无法读取当前脚本。")); return; }
        QString message; if (!PresetStore::write(path, text, &message)) error(message); else { refresh(); selectPath(path); }
    });
    button(QStringLiteral("读取"), [=] { if (!selected().isEmpty()) load(selected()); });
    connect(list, &QListWidget::itemDoubleClicked, this, [=] { load(selected()); });
    button(QStringLiteral("导入"), [=] {
        const auto files = QFileDialog::getOpenFileNames(this, QStringLiteral("导入 VPY"), {}, "VapourSynth (*.vpy)");
        for (const auto &source : files) {
            const QString target = QDir(PresetStore::directory()).filePath(QFileInfo(source).fileName());
            if (!confirmOverwrite(target)) continue;
            QFile f(source); QString message;
            if (!f.open(QIODevice::ReadOnly) || !PresetStore::write(target, QString::fromUtf8(f.readAll()), &message)) error(message);
        }
        refresh();
    });
    button(QStringLiteral("导出"), [=] {
        if (selected().isEmpty()) return;
        const QString target = QFileDialog::getSaveFileName(this, QStringLiteral("导出 VPY"), QFileInfo(selected()).fileName(), "VapourSynth (*.vpy)");
        if (target.isEmpty()) return;
        QFile f(selected()); QString message;
        if (!f.open(QIODevice::ReadOnly) || !PresetStore::write(target, QString::fromUtf8(f.readAll()), &message)) error(message);
    });
    button(QStringLiteral("变更名称"), [=] { const auto target = safeName(); if (target.isEmpty() || selected().isEmpty() || target == selected()) return;
        if (QFileInfo::exists(target)) { error(QStringLiteral("名称已存在。")); return; }
        if (!QFile::rename(selected(), target)) error(QStringLiteral("重命名失败。")); else { refresh(); selectPath(target); } });
    button(QStringLiteral("变更备注"), [=] {
        QFile f(selected()); if (!f.open(QIODevice::ReadOnly)) return;
        QString text = QString::fromUtf8(f.readAll()); auto data = PresetStore::metadata(text);
        if (data.isEmpty()) { error(QStringLiteral("外部 VPY 没有预设元数据。")); return; }
        data.insert("note", note->text()); QStringList lines = text.split('\n');
        for (auto &line : lines) if (line.startsWith("# VSR_PRESET ")) line = "# VSR_PRESET " + QString::fromLatin1(QJsonDocument(data).toJson(QJsonDocument::Compact).toBase64());
        QString message; if (!PresetStore::write(selected(), lines.join('\n'), &message)) error(message);
    });
    const auto remove = [=] { if (selected().isEmpty()) return; if (!QFile::moveToTrash(selected())) error(QStringLiteral("移入回收站失败。")); else refresh(); };
    button(QStringLiteral("删除"), remove); auto *del = new QShortcut(QKeySequence::Delete, list); connect(del, &QShortcut::activated, this, remove);
    button(QStringLiteral("重置处理链"), [=] { reset(); });
    button(QStringLiteral("打开目录"), [=] { QDesktopServices::openUrl(QUrl::fromLocalFile(PresetStore::directory())); });
    refresh();
}
}
