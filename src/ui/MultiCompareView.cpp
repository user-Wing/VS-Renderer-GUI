#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include <QComboBox>
#include <QToolButton>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <algorithm>
namespace vsr {
MultiCompareView::MultiCompareView(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(320, 200);
    for (int i = 0; i < 9; ++i) {
        clips_[i] = new QWidget(this);
        clips_[i]->setAttribute(Qt::WA_NativeWindow);
        panes_[i] = new PreviewPane(QStringLiteral("视频 %1").arg(i + 1), QStringLiteral("直接解码"), clips_[i]);
        panes_[i]->setMinimumSize(0, 0);
        panes_[i]->setPlaceholderText(QStringLiteral("等待视频"));
        clips_[i]->hide();
        selectors_[i] = new QComboBox(this);
        selectors_[i]->setMinimumWidth(0);
        selectors_[i]->setStyleSheet(QStringLiteral("min-height:0px; padding:0 4px;"));
        selectors_[i]->hide();
        remove_[i] = new QToolButton(this);
        remove_[i]->setStyleSheet(QStringLiteral("min-height:0px; padding:0px;"));
        remove_[i]->setText(QStringLiteral("×"));
        remove_[i]->setObjectName(QStringLiteral("removeVideo%1").arg(i));
        remove_[i]->setToolTip(QStringLiteral("移除此视频(不删除文件)"));
        remove_[i]->hide();
        connect(remove_[i], &QToolButton::clicked, this, [this, i] {
            if (i < slots_.size()) emit sourceRemoved(slots_[i]);
        });
        connect(selectors_[i], &QComboBox::currentIndexChanged, this, [this, i](int video) {
            if (video >= 0) emit sourceSelected(i, video);
        });
    }
    for (int i = 0; i < 5; ++i) {
        handles_[i] = new QWidget(this);
        handles_[i]->setObjectName(QStringLiteral("comparisonDivider%1").arg(i));
        handles_[i]->setStyleSheet(QStringLiteral("background:#0067c0;"));
        handles_[i]->installEventFilter(this);
        handles_[i]->hide();
    }
}
PreviewPane *MultiCompareView::pane(int video) const { return panes_.at(video); }
QRect MultiCompareView::cellRect(int slot) const { return cells_.value(slot); }
void MultiCompareView::removeSource(int video)
{
    clips_[video]->hide();
    panes_[video]->setSurfaceActive(false);
    panes_[video]->setPixelText({});
    panes_[video]->adoptView(1, 0, 0);
    std::rotate(clips_.begin() + video, clips_.begin() + video + 1, clips_.end());
    std::rotate(panes_.begin() + video, panes_.begin() + video + 1, panes_.end());
}
void MultiCompareView::setSources(const QStringList &names)
{
    for (auto *selector : selectors_) { QSignalBlocker block(selector); selector->clear(); selector->addItems(names); }
}
void MultiCompareView::setSlots(const QList<int> &assignments, Layout layout)
{
    slots_ = assignments;
    if (layout != layout_) { x_ = y_ = (layout == ThreeRow || layout == ThreeColumn || layout == Grid) ? 1.0 / 3 : .5; x2_ = y2_ = 2.0 / 3; }
    layout_ = layout;
    arrange();
}
void MultiCompareView::setDivision(double x, double y) { x_ = std::clamp(x, .1, .9); y_ = std::clamp(y, .1, .9); arrange(); }
void MultiCompareView::resizeEvent(QResizeEvent *) { arrange(); }
bool MultiCompareView::isWipe() const
{
    return layout_ == Wipe || layout_ == ThreeRow || layout_ == ThreeColumn ||
        layout_ == ThreeLeft || layout_ == ThreeRight || layout_ == FourWipe;
}

void MultiCompareView::arrange()
{
    const int n = slots_.size();
    const bool wipe = isWipe();
    const int top = wipe ? 28 : 0;
    const int w = width(), h = std::max(1, height() - top);
    const int x = int(w * x_), y = int(h * y_);
    std::array<bool, 5> usedHandles{};
    cells_.clear();
    auto handle = [this, top, &usedHandles](int id, QRect rectangle, Qt::CursorShape cursor) {
        usedHandles[id] = true;
        handles_[id]->setGeometry(rectangle.translated(0, top));
        handles_[id]->setCursor(cursor);
    };
    if (n == 1) {
        cells_ << QRect(0, 0, w, h);
    } else if (n == 2) {
        const int split = wipe ? x : w / 2;
        cells_ << QRect(0, 0, split, h) << QRect(split, 0, w - split, h);
        if (wipe) handle(0, QRect(x - 3, 0, 6, h), Qt::SplitHCursor);
    } else if (layout_ == ThreeNormal) {
        for (int i = 0; i < 3; ++i)
            cells_ << QRect(i * w / 3, 0, (i + 1) * w / 3 - i * w / 3, h);
    } else if (layout_ == ThreeRow) {
        const int x2 = int(w * x2_);
        cells_ << QRect(0, 0, x, h) << QRect(x, 0, x2 - x, h) << QRect(x2, 0, w - x2, h);
        handle(0, QRect(x - 3, 0, 6, h), Qt::SplitHCursor);
        handle(1, QRect(x2 - 3, 0, 6, h), Qt::SplitHCursor);
    } else if (layout_ == ThreeColumn) {
        const int y2 = int(h * y2_);
        cells_ << QRect(0, 0, w, y) << QRect(0, y, w, y2 - y) << QRect(0, y2, w, h - y2);
        handle(2, QRect(0, y - 3, w, 6), Qt::SplitVCursor);
        handle(3, QRect(0, y2 - 3, w, 6), Qt::SplitVCursor);
    } else if (layout_ == ThreeLeft || layout_ == ThreeRight) {
        if (layout_ == ThreeLeft)
            cells_ << QRect(0, 0, x, h) << QRect(x, 0, w - x, y) << QRect(x, y, w - x, h - y);
        else
            cells_ << QRect(x, 0, w - x, h) << QRect(0, 0, x, y) << QRect(0, y, x, h - y);
        handle(0, QRect(x - 3, 0, 6, h), Qt::SplitHCursor);
        handle(2, QRect(layout_ == ThreeLeft ? x : 0, y - 3, layout_ == ThreeLeft ? w - x : x, 6), Qt::SplitVCursor);
        handle(4, QRect(x - 7, y - 7, 14, 14), Qt::SizeAllCursor);
    } else if (layout_ == Four || layout_ == FourWipe) {
        const int splitX = wipe ? x : w / 2, splitY = wipe ? y : h / 2;
        cells_ << QRect(0, 0, splitX, splitY) << QRect(splitX, 0, w - splitX, splitY)
               << QRect(0, splitY, splitX, h - splitY) << QRect(splitX, splitY, w - splitX, h - splitY);
        if (wipe) {
            handle(0, QRect(x - 3, 0, 6, h), Qt::SplitHCursor);
            handle(2, QRect(0, y - 3, w, 6), Qt::SplitVCursor);
            handle(4, QRect(x - 7, y - 7, 14, 14), Qt::SizeAllCursor);
        }
    } else if (n > 0) {
        const int rows = (n + 2) / 3;
        for (int i = 0; i < n; ++i)
            cells_ << QRect((i % 3) * w / 3, (i / 3) * h / rows,
                            (i % 3 + 1) * w / 3 - (i % 3) * w / 3,
                            (i / 3 + 1) * h / rows - (i / 3) * h / rows);
    }
    for (int i = 0; i < 9; ++i) {
        if (!slots_.contains(i)) clips_[i]->hide();
        if (i >= n) { selectors_[i]->hide(); remove_[i]->hide(); }
    }
    for (int i = 0; i < n; ++i) {
        const int video = slots_[i];
        const QRect cell = cells_[i];
        // Selectors stay above the shared canvas, never inside its cropped pieces.
        const QRect selector = wipe ? QRect(i * w / n, 0, (i + 1) * w / n - i * w / n, 28)
                                    : QRect(cell.x(), cell.y(), cell.width(), 28);
        const int selectorWidth = std::min(240, std::max(1, selector.width() - 34));
        selectors_[i]->setGeometry(selector.x() + 3, selector.y() + 2, selectorWidth, 24);
        remove_[i]->setGeometry(selector.x() + selectorWidth + 6, selector.y() + 2, 24, 24);
        remove_[i]->show();
        {
            QSignalBlocker block(selectors_[i]);
            selectors_[i]->setCurrentIndex(video);
            selectors_[i]->setToolTip(selectors_[i]->currentText());
        }
        selectors_[i]->show();
        panes_[video]->setChromeVisible(!wipe);
        if (wipe) {
            // Same-sized native surfaces, one common origin. Only HWND parent clips move.
            clips_[video]->setGeometry(cell.translated(0, top));
            panes_[video]->setGeometry(-cell.x(), -cell.y(), w, h);
        } else {
            const QRect viewport = cell.adjusted(0, 28, 0, 0);
            clips_[video]->setGeometry(viewport);
            panes_[video]->setGeometry(0, 0, viewport.width(), viewport.height());
        }
        clips_[video]->show();
    }
    // Hiding a pressed handle releases mouse capture. Never hide/show it on a move.
    for (int i = 0; i < 5; ++i) {
        handles_[i]->setVisible(usedHandles[i]);
        if (usedHandles[i]) handles_[i]->raise();
    }
}

bool MultiCompareView::eventFilter(QObject *object, QEvent *event)
{
    int id = -1;
    for (int i = 0; i < 5; ++i) if (object == handles_[i]) id = i;
    if (id < 0) return QWidget::eventFilter(object, event);
    if (event->type() == QEvent::MouseButtonPress &&
        static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
        dragging_ = id;
        handles_[id]->grabMouse();
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease &&
        static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
        if (dragging_ >= 0) handles_[dragging_]->releaseMouse();
        dragging_ = -1;
        return true;
    }
    if (event->type() == QEvent::MouseMove && dragging_ >= 0) {
        const auto point = mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPosition().toPoint());
        const double x = std::clamp(double(point.x()) / std::max(1, width()), .1, .9);
        const double y = std::clamp(double(point.y() - 28) / std::max(1, height() - 28), .1, .9);
        if (id == 0 || id == 4) x_ = layout_ == ThreeRow ? std::min(x, x2_ - .1) : x;
        if (id == 1) x2_ = std::max(x, x_ + .1);
        if (id == 2 || id == 4) y_ = layout_ == ThreeColumn ? std::min(y, y2_ - .1) : y;
        if (id == 3) y2_ = std::max(y, y_ + .1);
        arrange();
        return true;
    }
    return QWidget::eventFilter(object, event);
}
}
