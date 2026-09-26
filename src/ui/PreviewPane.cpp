#include "ui/PreviewPane.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>

namespace vsr {
namespace {

class NativeVideoSurface final : public QWidget {
public:
    explicit NativeVideoSurface(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
    }
    QPaintEngine *paintEngine() const override { return nullptr; }
protected:
    void paintEvent(QPaintEvent *) override {}
};

}


PreviewPane::PreviewPane(const QString &title, const QString &badge, QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setObjectName(QStringLiteral("previewPane"));
    redrawTimer_ = new QTimer(this);
    redrawTimer_->setSingleShot(true);
    redrawTimer_->setInterval(40);
    connect(redrawTimer_, &QTimer::timeout, this, &PreviewPane::redrawRequested);

    auto *titleBar = new QWidget(this);
    titleBar_ = titleBar;
    titleBar->setFixedHeight(32);
    titleBar->setStyleSheet(QStringLiteral("background:#1e1e1e;color:#f5f5f5;border-bottom:1px solid #333;"));
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8, 0, 8, 0);
    auto *titleLabel = new QLabel(title, titleBar);
    titleLabel_ = titleLabel;
    titleLabel->setStyleSheet(QStringLiteral("font-weight:600;background:transparent;"));
    badge_ = new QLabel(badge, titleBar);
    badge_->setStyleSheet(QStringLiteral("color:#c8c8c8;background:transparent;"));
    zoomBadge_ = new QLabel(QStringLiteral("100%"), titleBar);
    zoomBadge_->setStyleSheet(QStringLiteral("color:#8fbceb;background:transparent;"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(badge_);
    titleLayout->addWidget(zoomBadge_);

    auto *stage = new QWidget(this);
    stage->setStyleSheet(QStringLiteral("background:#101010;"));
    stack_ = new QStackedLayout(stage);
    stack_->setContentsMargins(0, 0, 0, 0);
    stack_->setStackingMode(QStackedLayout::StackOne);

    placeholder_ = new QLabel(QStringLiteral("等待打开视频\n3FP 运行时未激活"), stage);
    placeholder_->setAlignment(Qt::AlignCenter);
    placeholder_->setStyleSheet(QStringLiteral("background:#101010;color:#9a9a9a;"));

    surface_ = new NativeVideoSurface(stage);
    surface_->setAttribute(Qt::WA_NativeWindow);
    surface_->setAttribute(Qt::WA_OpaquePaintEvent);
    surface_->setAutoFillBackground(false);
    surface_->setMouseTracking(true);
    surface_->setFocusPolicy(Qt::StrongFocus);
    surface_->setCursor(Qt::CrossCursor);
    surface_->installEventFilter(this);
    stack_->addWidget(placeholder_);
    stack_->addWidget(surface_);
    stack_->setCurrentWidget(placeholder_);

    pixel_ = new QLabel(stage);
    pixel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    pixel_->setStyleSheet(QStringLiteral("background:rgba(20,20,20,220);color:#f5f5f5;border:1px solid #555;padding:3px 6px;"));
    pixel_->setObjectName(QStringLiteral("pixelReadout"));
    pixel_->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar);
    layout->addWidget(stage, 1);
}

QWidget *PreviewPane::surface() const { return surface_; }
float PreviewPane::zoom() const { return zoom_; }
QPointF PreviewPane::pan() const { return QPointF(panX_, panY_); }
void PreviewPane::setChromeVisible(bool visible) { titleBar_->setVisible(visible); }
void PreviewPane::setVideoSize(const QSize &size) { videoSize_ = size; }

void PreviewPane::adoptView(float zoom, float panX, float panY)
{
    zoom_ = zoom;
    panX_ = panX;
    panY_ = panY;
    zoomBadge_->setText(QStringLiteral("%1%").arg(qRound(zoom_ * 100.0f)));
    surface_->setCursor(zoom_ > 1.0f ? Qt::OpenHandCursor : Qt::CrossCursor);
}

void PreviewPane::setActive(bool active)
{
    setStyleSheet(active ? QStringLiteral("#previewPane{border:2px solid #0067c0;}")
                         : QStringLiteral("#previewPane{border:1px solid #333;}") );
}

void PreviewPane::setSurfaceActive(bool active)
{
    stack_->setCurrentWidget(active ? surface_ : placeholder_);
}

void PreviewPane::setTitle(const QString &title) { titleLabel_->setText(title); }
void PreviewPane::setBadge(const QString &badge) { badge_->setText(badge); }
void PreviewPane::setPlaceholderText(const QString &text) { placeholder_->setText(text); }

void PreviewPane::setPixelText(const QString &text)
{
    pixel_->setText(text);
    pixel_->adjustSize();
    positionPixel();
    pixel_->setVisible(!text.isEmpty());
    pixel_->raise();
}

void PreviewPane::positionPixel()
{
    QWidget *stage = pixel_->parentWidget();
    QRect visible = stage->rect();
    for (QWidget *ancestor = stage->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        visible = visible.intersected(QRect(stage->mapFrom(ancestor, QPoint(0, 0)), ancestor->size()));
    pixel_->move(visible.left() + 8, std::max(visible.top(), visible.bottom() - pixel_->height() - 7));
}

bool PreviewPane::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != surface_)
        return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        applyWheel(wheel, wheel->position());
        return true;
    }
    if (event->type() == QEvent::Resize) {
        scheduleRedraw();
        return QWidget::eventFilter(watched, event);
    }
    if (event->type() == QEvent::MouseMove) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (dragging_) {
            const QPointF delta = mouse->position() - dragStart_;
            QSizeF fitted = surface_->size();
            if (!videoSize_.isEmpty()) fitted = QSizeF(videoSize_.scaled(surface_->size(), Qt::KeepAspectRatio));
            const double overflowX = std::max(1.0, fitted.width() * zoom_ - surface_->width());
            const double overflowY = std::max(1.0, fitted.height() * zoom_ - surface_->height());
            const float dx = static_cast<float>(delta.x() * 2.0 / overflowX);
            const float dy = static_cast<float>(delta.y() * 2.0 / overflowY);
            panX_ = std::clamp(static_cast<float>(dragPanStart_.x()) + dx, -1.0f, 1.0f);
            panY_ = std::clamp(static_cast<float>(dragPanStart_.y()) + dy, -1.0f, 1.0f);
            emit viewChanged(zoom_, panX_, panY_);
            return true;
        }
        emit pixelHovered(static_cast<int>(mouse->position().x()), static_cast<int>(mouse->position().y()));
    } else if (event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        emit activated();
        if (mouse->button() == Qt::LeftButton && zoom_ > 1.0f) {
            dragging_ = true;
            dragStart_ = mouse->position();
            dragPanStart_ = QPointF(panX_, panY_);
            surface_->setCursor(Qt::ClosedHandCursor);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton && dragging_) {
            dragging_ = false;
            surface_->setCursor(zoom_ > 1.0f ? Qt::OpenHandCursor : Qt::CrossCursor);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        dragging_ = false;
        updateZoom(1.0f, QPointF(surface_->width() / 2.0, surface_->height() / 2.0));
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void PreviewPane::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (pixel_->isVisible())
        positionPixel();
    scheduleRedraw();
}

void PreviewPane::wheelEvent(QWheelEvent *event)
{
    const QPointF surfacePosition = surface_->mapFromGlobal(event->globalPosition().toPoint());
    applyWheel(event, surfacePosition);
    event->accept();
}

void PreviewPane::applyWheel(QWheelEvent *event, const QPointF &surfacePosition)
{
    if (event->angleDelta().y() == 0)
        return;
    const float factor = event->angleDelta().y() > 0 ? 1.25f : 0.8f;
    updateZoom(zoom_ * factor, surfacePosition);
}

void PreviewPane::scheduleRedraw()
{
    redrawTimer_->start();
}

void PreviewPane::updateZoom(float next, const QPointF &anchor)
{
    next = std::clamp(next, 0.25f, 16.0f);
    const float ax = surface_->width() > 0 ? static_cast<float>(anchor.x() / surface_->width() * 2.0 - 1.0) : 0.0f;
    const float ay = surface_->height() > 0 ? static_cast<float>(anchor.y() / surface_->height() * 2.0 - 1.0) : 0.0f;
    const float delta = next > 0.0f ? (next - zoom_) / next : 0.0f;
    panX_ = std::clamp(panX_ + ax * delta, -1.0f, 1.0f);
    panY_ = std::clamp(panY_ + ay * delta, -1.0f, 1.0f);
    if (next == 1.0f)
        panX_ = panY_ = 0.0f;
    zoom_ = next;
    zoomBadge_->setText(QStringLiteral("%1%").arg(qRound(zoom_ * 100.0f)));
    surface_->setCursor(zoom_ > 1.0f ? Qt::OpenHandCursor : Qt::CrossCursor);
    emit viewChanged(zoom_, panX_, panY_);
}

}
