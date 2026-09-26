#include "ui/CompareView.h"

#include "ui/PreviewPane.h"

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>

#include <algorithm>

namespace vsr {

CompareView::CompareView(QWidget *parent, const QString &leftTitle, const QString &rightTitle)
    : QWidget(parent)
{
    setMinimumSize(640, 240);
    sourceClip_ = new QWidget(this);
    processedClip_ = new QWidget(this);
    sourceClip_->setAttribute(Qt::WA_NativeWindow);
    processedClip_->setAttribute(Qt::WA_NativeWindow);
    sourcePane_ = new PreviewPane(leftTitle, QStringLiteral("Fit"), sourceClip_);
    processedPane_ = new PreviewPane(rightTitle, QStringLiteral("VS · 待渲染"), processedClip_);
    sliderHandle_ = new QWidget(this);
    sliderHandle_->setFixedWidth(7);
    sliderHandle_->setCursor(Qt::SplitHCursor);
    sliderHandle_->setStyleSheet(QStringLiteral(
        "background:#0067c0;border-left:2px solid rgba(255,255,255,180);"
        "border-right:2px solid rgba(255,255,255,180);"));
    sliderHandle_->installEventFilter(this);
    updateLayout();
}

PreviewPane *CompareView::sourcePane() const { return sourcePane_; }
PreviewPane *CompareView::processedPane() const { return processedPane_; }
CompareMode CompareView::mode() const { return mode_; }
double CompareView::splitRatio() const { return splitRatio_; }

void CompareView::setMode(CompareMode mode)
{
    if (mode_ == mode)
        return;
    mode_ = mode;
    updateLayout();
}

void CompareView::setSplitRatio(double ratio)
{
    splitRatio_ = std::clamp(ratio, 0.05, 0.95);
    if (mode_ == CompareMode::Slider)
        updateLayout();
}

bool CompareView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != sliderHandle_)
        return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            dragging_ = true;
            return true;
        }
    } else if (event->type() == QEvent::MouseMove && dragging_) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        const QPoint point = mapFromGlobal(mouse->globalPosition().toPoint());
        setSplitRatio(width() > 0 ? static_cast<double>(point.x()) / width() : 0.5);
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton && dragging_) {
            dragging_ = false;
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CompareView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLayout();
}

void CompareView::updateLayout()
{
    const int w = width();
    const int h = height();
    if (mode_ == CompareMode::SideBySide) {
        const int leftWidth = w / 2;
        sourceClip_->setGeometry(0, 0, leftWidth, h);
        processedClip_->setGeometry(leftWidth + 1, 0, std::max(0, w - leftWidth - 1), h);
        sourcePane_->setGeometry(0, 0, leftWidth, h);
        processedPane_->setGeometry(0, 0, std::max(0, w - leftWidth - 1), h);
        sliderHandle_->hide();
        sourcePane_->show();
        processedPane_->show();
        return;
    }

    const int split = std::clamp(static_cast<int>(w * splitRatio_), 1, std::max(1, w - 1));
    // Clip native swap-chain children with disjoint native parents. QWidget masks
    // on overlapping panes do not reliably clip descendant HWND presentation.
    sourceClip_->setGeometry(0, 0, split, h);
    processedClip_->setGeometry(split, 0, w - split, h);
    sourcePane_->setGeometry(0, 0, w, h);
    processedPane_->setGeometry(-split, 0, w, h);
    sourcePane_->show();
    processedPane_->show();
    sliderHandle_->setGeometry(split - sliderHandle_->width() / 2, 0, sliderHandle_->width(), h);
    sliderHandle_->show();
    sliderHandle_->raise();
}

}
