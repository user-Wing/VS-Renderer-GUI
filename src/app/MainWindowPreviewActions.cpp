#include "app/MainWindow.h"
#include "app/RenderRoute.h"
#include "graph/FilterCatalog.h"
#include "graph/HlslPreview.h"
#include "graph/PresetStore.h"
#include "player/PlayerMenu.h"
#include "ui/CompareView.h"
#include "ui/ShortcutTable.h"

#include <QAction>
#include <QAbstractItemModel>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <cmath>

namespace vsr {
namespace {

const QString shortcutKey = QStringLiteral("renderer/shortcuts");
const QString menuKey = QStringLiteral("renderer/previewContextMenus");

QList<ShortcutAction> rendererActions()
{
    return {{"area1", QStringLiteral("显示 / 隐藏功能区 1")},
        {"area2", QStringLiteral("显示 / 隐藏功能区 2")},
        {"add", QStringLiteral("加路")}, {"remove", QStringLiteral("移除当前路")},
        {"visibility", QStringLiteral("隐藏 / 显示当前路")},
        {"export", QStringLiteral("打开导出页面")}, {"presets", QStringLiteral("打开预设页面")},
        {"play", QStringLiteral("播放 / 暂停")}, {"render", QStringLiteral("渲染预览")},
        {"previous", QStringLiteral("上一帧")}, {"next", QStringLiteral("下一帧")},
        {"previous-key", QStringLiteral("上一个关键帧")}, {"next-key", QStringLiteral("下一个关键帧")}};
}

QList<ShortcutBinding> rendererBindings()
{
    QSettings settings;
    if (!settings.contains(shortcutKey))
        return {{"1","area1"},{"2","area2"},{"A","add"},{"S","remove"},{"D","visibility"},
                {"O","export"},{"P","presets"},{"Space","play"}};
    QList<ShortcutBinding> bindings;
    for (const auto &value : settings.value(shortcutKey).toList()) {
        const auto entry = value.toMap();
        bindings.append({entry.value("key").toString(), entry.value("action").toString()});
    }
    return bindings;
}

QJsonArray configuredMenus()
{
    return QJsonDocument::fromJson(QSettings().value(menuKey).toByteArray()).array();
}

class PreviewMenuItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qRound(size.height() * 1.3));
        return size;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        const auto name = static_cast<QLineEdit *>(editor)->text().trimmed();
        if (!name.isEmpty()) model->setData(index, name, Qt::EditRole);
    }
};

}

QWidget *MainWindow::buildShortcutSettings()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *table = new ShortcutTable(rendererActions(), page);
    table->setObjectName("rendererShortcuts");
    for (const auto &binding : rendererBindings()) table->append(binding);
    layout->addWidget(table);

    auto *buttons = new QHBoxLayout;
    auto *add = new QPushButton(QStringLiteral("添加快捷键"), page);
    auto *remove = new QPushButton(QStringLiteral("移除所选"), page);
    auto *save = new QPushButton(QStringLiteral("保存"), page);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    buttons->addWidget(save);
    layout->addLayout(buttons);
    connect(add, &QPushButton::clicked, table, [table] {
        table->append({{}, "play"});
        table->setCurrentCell(table->rowCount() - 1, 0);
        table->editItem(table->currentItem());
    });
    connect(remove, &QPushButton::clicked, table, [table] {
        if (table->currentRow() >= 0) table->removeRow(table->currentRow());
    });
    connect(save, &QPushButton::clicked, this, [this, table] {
        QVariantList bindings;
        for (int row = 0; row < table->rowCount(); ++row) {
            const auto binding = table->binding(row);
            bindings.append(QVariantMap{{"key", binding.key}, {"action", binding.action}});
        }
        QSettings().setValue(shortcutKey, bindings);
        setStatus(QStringLiteral("快捷键已保存。"));
    });
    return page;
}

bool MainWindow::handleRendererShortcut(QKeyEvent *key, bool execute)
{
    const auto sequence = QKeySequence(key->keyCombination()).toString(QKeySequence::PortableText);
    for (const auto &binding : rendererBindings()) {
        if (binding.key.isEmpty() || binding.key != sequence) continue;
        if (!execute || key->isAutoRepeat()) return true;
        const auto &action = binding.action;
        if (action == "area1" || action == "area2") {
            if (auto *toggle = findChild<QAction *>(QStringLiteral("rendererToggleArea") + action.back()))
                toggle->trigger();
        } else if (action == "add") addRoute();
        else if (action == "remove") confirmRemoveRoute();
        else if (action == "visibility") setRouteVisibility(!routes_[routeIndex_].visible);
        else if (action == "export") exportCurrentResult();
        else if (action == "presets") showPresets();
        else if (action == "play") togglePlayback();
        else if (action == "render") validateScript();
        else if (action == "previous" || action == "previous-key") stepPreview(-1, action.endsWith("-key"));
        else if (action == "next" || action == "next-key") stepPreview(1, action.endsWith("-key"));
        return true;
    }
    return false;
}

QWidget *MainWindow::buildContextMenuSettings()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *hint = new QLabel(QStringLiteral("子菜单 / 滤镜   ·   新建并选中子菜单，再从下方加入滤镜。并排预览中右击画面即可使用，滤镜只加入播放头所在的时间组。"), page);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto *tree = new QTreeWidget(page);
    tree->setObjectName("rendererContextMenuSettings");
    tree->setAccessibleName(QStringLiteral("子菜单 / 滤镜"));
    tree->setHeaderHidden(true);
    tree->setItemDelegate(new PreviewMenuItemDelegate(tree));
    tree->setStyleSheet(QStringLiteral("QTreeWidget QLineEdit { min-height: 0; }"));
    tree->setUniformRowHeights(true);
    layout->addWidget(tree);

    const auto appendFilter = [](QTreeWidgetItem *group, const QString &id) {
        const auto *definition = FilterCatalog::find(id);
        auto *item = new QTreeWidgetItem(group, {definition ? definition->name : id});
        item->setData(0, Qt::UserRole, id);
    };
    const auto appendGroup = [tree](const QString &name) {
        auto *group = new QTreeWidgetItem(tree, {name});
        group->setFlags(group->flags() | Qt::ItemIsEditable);
        group->setExpanded(true);
        return group;
    };
    const auto load = [tree, appendGroup, appendFilter](const QJsonArray &menus) {
        tree->clear();
        for (const auto &value : menus) {
            const auto entry = value.toObject();
            auto *group = appendGroup(entry.value("name").toString());
            for (const auto &filter : entry.value("filters").toArray()) appendFilter(group, filter.toString());
        }
    };
    const auto serialize = [tree] {
        QJsonArray menus;
        for (int row = 0; row < tree->topLevelItemCount(); ++row) {
            const auto *group = tree->topLevelItem(row);
            const auto name = group->text(0).trimmed();
            if (name.isEmpty()) continue;
            QJsonArray filters;
            for (int i = 0; i < group->childCount(); ++i) filters.append(group->child(i)->data(0, Qt::UserRole).toString());
            menus.append(QJsonObject{{"name", name}, {"filters", filters}});
        }
        return menus;
    };
    const auto persist = [serialize] {
        QSettings().setValue(menuKey, QJsonDocument(serialize()).toJson(QJsonDocument::Compact));
    };
    load(configuredMenus());

    auto *controls = new QHBoxLayout;
    const auto button = [page, controls](const QString &title) {
        auto *created = new QPushButton(title, page);
        controls->addWidget(created);
        return created;
    };
    auto *addGroup = button(QStringLiteral("新建子菜单"));
    auto *rename = button(QStringLiteral("重命名"));
    rename->setEnabled(false);
    auto *filters = new QComboBox(page);
    for (const auto &definition : FilterCatalog::all()) filters->addItem(definition.name, definition.id);
    controls->addWidget(filters, 1);
    auto *addFilter = button(QStringLiteral("加入滤镜"));
    auto *remove = button(QStringLiteral("移除所选"));
    auto *importMenu = button(QStringLiteral("导入"));
    auto *exportMenu = button(QStringLiteral("导出"));
    layout->addLayout(controls);
    connect(tree, &QTreeWidget::itemChanged, page, [persist] { persist(); });
    connect(tree, &QTreeWidget::currentItemChanged, rename, [rename](QTreeWidgetItem *item) {
        rename->setEnabled(item != nullptr);
    });
    connect(rename, &QPushButton::clicked, page, [tree] {
        auto *group = tree->currentItem();
        if (!group) return;
        if (group->parent()) group = group->parent();
        tree->setCurrentItem(group);
        tree->editItem(group);
    });
    connect(addGroup, &QPushButton::clicked, page, [tree, appendGroup, persist] {
        auto *group = appendGroup(QStringLiteral("滤镜"));
        tree->setCurrentItem(group);
        tree->editItem(group);
        persist();
    });
    connect(addFilter, &QPushButton::clicked, page, [tree, filters, appendFilter, persist] {
        auto *group = tree->currentItem();
        if (!group) return;
        if (group->parent()) group = group->parent();
        appendFilter(group, filters->currentData().toString());
        group->setExpanded(true);
        persist();
    });
    connect(remove, &QPushButton::clicked, page, [tree, persist] {
        delete tree->currentItem();
        persist();
    });
    connect(importMenu, &QPushButton::clicked, page, [page, load, persist] {
        const auto path = QFileDialog::getOpenFileName(page, QStringLiteral("导入右键菜单"), {}, QStringLiteral("JSON (*.json)"));
        if (path.isEmpty()) return;
        QFile file(path);
        QJsonDocument document;
        if (file.open(QIODevice::ReadOnly) && file.size() <= 1024 * 1024) document = QJsonDocument::fromJson(file.readAll());
        const auto root = document.object();
        const auto menus = root.value("menus").toArray();
        bool valid = root.value("version").toInt() == 1 && root.value("menus").isArray();
        for (const auto &value : menus) {
            const auto group = value.toObject();
            valid = valid && !group.value("name").toString().trimmed().isEmpty() && group.value("filters").isArray();
            for (const auto &filter : group.value("filters").toArray())
                valid = valid && filter.isString() && !filter.toString().isEmpty();
        }
        if (!valid) {
            QMessageBox::warning(page, QStringLiteral("导入右键菜单"), QStringLiteral("无法读取这份右键菜单配置。"));
            return;
        }
        if (!configuredMenus().isEmpty() && QMessageBox::question(page, QStringLiteral("导入右键菜单"),
                QStringLiteral("用导入的配置替换当前自定义菜单？")) != QMessageBox::Yes) return;
        load(menus);
        persist();
    });
    connect(exportMenu, &QPushButton::clicked, page, [page, serialize] {
        const auto path = QFileDialog::getSaveFileName(page, QStringLiteral("导出右键菜单"),
            QStringLiteral("vsr-preview-menu.json"), QStringLiteral("JSON (*.json)"));
        if (path.isEmpty()) return;
        QSaveFile file(path);
        const auto bytes = QJsonDocument(QJsonObject{{"version", 1}, {"menus", serialize()}}).toJson();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            QMessageBox::warning(page, QStringLiteral("导出右键菜单"), file.errorString());
    });
    return page;
}

void MainWindow::addFilterToRoute(const QString &id)
{
    const int row = graph_.add(id);
    if (row < 0) return;
    activePreset_.clear();
    refreshPipeline(row);
    setStatus(QStringLiteral("已添加滤镜，VPY 待验证。"));
}

void MainWindow::addFilterAtPlayhead(const QString &id)
{
    const auto source = sourcePlayer_->snapshot();
    const auto position = timelinePressed_ || timelineSeekPending_
        ? source.duration100ns * timeline_->value() / timeline_->maximum()
        : source.position100ns;
    const bool fpsKnown = sourceFpsNumerator_ > 0 && sourceFpsDenominator_ > 0;
    int group = 0;
    for (int i = 1; i < timeGroups_.size(); ++i) {
        const auto &candidate = timeGroups_[i];
        const bool reached = fpsKnown
            ? position >= std::llround(groupFrameBoundary(candidate) * 10000000.L * sourceFpsDenominator_ / sourceFpsNumerator_)
            : candidate.startFrame < 0 && position / 10000000 >= candidate.startSeconds;
        if (!reached) break;
        group = i;
    }
    if (group != timeGroupIndex_) {
        timeGroups_ = currentTimeGroups();
        refreshTimeGroups(group);
    }
    addFilterToRoute(id);
    setStatus(QStringLiteral("已添加滤镜到「%1」，VPY 待验证。").arg(timeGroups_[group].name));
}

void MainWindow::connectPreviewContextMenu(PreviewPane *pane)
{
    pane->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pane, &QWidget::customContextMenuRequested, this,
        [this, pane](const QPoint &position) { showPreviewContextMenu(pane, position); });
}

void MainWindow::showPreviewContextMenu(PreviewPane *pane, const QPoint &position)
{
    if (compareView_->mode() != CompareMode::SideBySide) return;
    int route = -1;
    if (pane != sourcePane_) {
        for (int i = 0; i < routes_.size(); ++i)
            if (compareView_->processedPane(i) == pane) { route = i; break; }
        if (route < 0) return;
    }
    auto *menu = new PlayerMenu(this);
    menu->setObjectName("rendererPreviewMenu");
    menu->setAttribute(Qt::WA_DeleteOnClose);
    const bool hasSource = !sourcePath_->text().isEmpty();
    menu->addAction(route < 0 ? QStringLiteral("用 VSP 播放源视频") : QStringLiteral("用 VSP 播放该路处理结果"),
        this, [this, route] { openPreviewInPlayer(route); })->setEnabled(hasSource);
    const bool hasFrame = route < 0 ? sourcePlayer_->snapshot().presentedVideoFrames > 0 :
        route == 0 ? vsScriptReady_ && lastVsFrame_ >= 0 : additionalRoutes_[route - 1]->hasFrame();
    menu->addAction(route < 0 ? QStringLiteral("复制当前原画面") : QStringLiteral("复制当前 VS 画面"),
        this, [this, route] { copyPreviewFrame(route); })->setEnabled(hasSource && hasFrame);
    const auto menus = configuredMenus();
    if (!menus.isEmpty()) menu->addSeparator();
    for (const auto &value : menus) {
        const auto group = value.toObject();
        auto *submenu = PlayerMenu::add(menu, group.value("name").toString());
        for (const auto &filter : group.value("filters").toArray()) {
            const auto id = filter.toString();
            const auto *definition = FilterCatalog::find(id);
            if (!definition) continue;
            submenu->addAction(definition->name, this, [this, id] { addFilterAtPlayhead(id); });
        }
        submenu->setEnabled(!submenu->isEmpty());
    }
    menu->popup(pane->mapToGlobal(position));
}

void MainWindow::openPreviewInPlayer(int route)
{
    const auto source = sourcePath_->text();
    if (!QFileInfo(source).isFile()) return;
    QStringList arguments{"--renderer-preview", source};
    if (route >= 0) {
        if (route != routeIndex_) selectRoute(route);
        const auto result = currentScript();
        if (!result.errors.isEmpty() || result.script.isEmpty()) {
            setStatus(result.errors.isEmpty() ? QStringLiteral("该路没有可播放的脚本。") : result.errors.join('\n'), true);
            return;
        }
        const auto bytes = result.script.toUtf8();
        const auto hash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        const QDir directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        if (!QDir().mkpath(directory.path())) { setStatus(QStringLiteral("无法创建预览缓存。"), true); return; }
        const auto path = directory.filePath("player-preview-" + hash + ".vpy");
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
            setStatus(QStringLiteral("无法写入该路播放脚本。"), true);
            return;
        }
        arguments << path;
    }
    if (!QProcess::startDetached(QDir(QCoreApplication::applicationDirPath()).filePath("vs-player.exe"), arguments))
        setStatus(QStringLiteral("无法启动 Full VSP。"), true);
}

void MainWindow::copyPreviewFrame(int route)
{
    QImage image;
    if (route < 0) {
        image = sourcePlayer_->captureFullFrame({});
    } else if (route == 0 && vsScriptReady_ && lastVsFrame_ >= 0) {
        const auto snap = processedPlayer_->snapshot();
        const QSize decoded(snap.videoWidth, snap.videoHeight);
        image = processedPlayer_->captureFullFrame(hlslPreviewPackage_.isEmpty() ? QSize()
            : hlslOutputSize(hlslPreviewParameters_, decoded));
    } else if (route > 0) {
        image = additionalRoutes_.at(route - 1)->captureFrame();
    }
    if (image.isNull()) { setStatus(QStringLiteral("当前没有可复制的整帧。"), true); return; }
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) { setStatus(QStringLiteral("无法生成剪贴板 PNG。"), true); return; }
    auto *mime = new QMimeData;
    mime->setImageData(image);
    mime->setData("image/png", png);
    QApplication::clipboard()->setMimeData(mime);
    setStatus(QStringLiteral("已复制 %1 × %2 的 PNG。").arg(image.width()).arg(image.height()));
}

}
