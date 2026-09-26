#include "ui/PreparedFiles.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

namespace vsr {

PreparedFiles::PreparedFiles(QWidget *parent) : QListWidget(parent)
{
    setAcceptDrops(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setToolTip(QStringLiteral("拖入多个视频；可拖动排序，重复文件自动忽略。"));
}

QStringList PreparedFiles::files() const
{
    QStringList result;
    for (int row = 0; row < count(); ++row) result.append(item(row)->text());
    return result;
}

void PreparedFiles::addFiles(const QStringList &paths)
{
    QStringList existing = files();
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.isFile()) continue;
        const QString absolute = info.canonicalFilePath();
        if (existing.contains(absolute, Qt::CaseInsensitive)) continue;
        addItem(absolute);
        existing.append(absolute);
    }
}

void PreparedFiles::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else QListWidget::dragEnterEvent(event);
}

void PreparedFiles::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else QListWidget::dragMoveEvent(event);
}

void PreparedFiles::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        QListWidget::dropEvent(event);
        return;
    }
    QStringList paths;
    for (const auto &url : event->mimeData()->urls())
        if (url.isLocalFile()) paths.append(url.toLocalFile());
    addFiles(paths);
    event->acceptProposedAction();
}

}
