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
#include <QPainter>
#include <QColorSpace>
#include <QThread>
#include <memory>
#include <windows.h>

#include <algorithm>

namespace vsr {
namespace {

class NativeVideoSurface final : public QWidget {
public:
    QImage image;
    QTransform imageTransform;
    QList<QImage> levels;
    QList<QThread *> builders;
    quint64 imageGeneration = 0;
    float zoom = 1, panX = 0, panY = 0;
    explicit NativeVideoSurface(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
    }
    ~NativeVideoSurface() override {for(auto *thread:builders){thread->requestInterruption();thread->wait();delete thread;}}
    void buildLevels() {
        levels.clear();const auto generation=++imageGeneration;
        for(auto *thread:builders)thread->requestInterruption();
        if(qint64(image.width())*image.height()<8*1024*1024)return;
        MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);GlobalMemoryStatusEx(&memory);
        const quint64 budget=std::min<quint64>(512ull*1024*1024,memory.ullAvailPhys/8);
        const auto source=image;auto result=std::make_shared<QList<QImage>>();
        auto *thread=QThread::create([source,result,budget]{
            QImage level=source;quint64 used=0;
            for(int edge=8192;edge>=256;edge/=2){
                if(QThread::currentThread()->isInterruptionRequested())return;
                const auto size=source.size().scaled({edge,edge},Qt::KeepAspectRatio);
                if(size.width()>=level.width() || quint64(size.width())*size.height()*source.depth()/8+used>budget)continue;
                level=level.scaled(size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
                if(level.isNull())return;used+=level.sizeInBytes();result->append(level);
            }
        });builders.append(thread);
        connect(thread,&QThread::finished,this,[this,thread,result,generation]{builders.removeOne(thread);if(generation==imageGeneration){levels=*result;update();}thread->deleteLater();});thread->start();
    }
    QPaintEngine *paintEngine() const override { return image.isNull() ? nullptr : QWidget::paintEngine(); }
protected:
    void paintEvent(QPaintEvent *) override {
        if(image.isNull())return;
        QPainter painter(this);painter.fillRect(rect(),QColor("#101010"));
        const QRectF bounds=imageTransform.mapRect(QRectF(QPointF(),image.size()));
        const QSizeF fitted=bounds.size().scaled(size(),Qt::KeepAspectRatio)*zoom;
        const QPointF origin((width()-fitted.width())/2+panX*std::max(0.0,fitted.width()-width())/2,
                             (height()-fitted.height())/2+panY*std::max(0.0,fitted.height()-height())/2);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        // Clip before mapping into painter coordinates: huge zooms exceed raster's coordinate range.
        const QRectF destination=QRectF(origin,fitted).intersected(QRectF(rect()));
        if(destination.isEmpty())return;
        QTransform screen;
        screen.translate(origin.x(),origin.y());screen.scale(fitted.width()/bounds.width(),fitted.height()/bounds.height());screen.translate(-bounds.x(),-bounds.y());
        const auto mapping=imageTransform*screen;
        const QRectF source=mapping.inverted().mapRect(destination).intersected(QRectF(QPointF(),image.size()));
        // Draw only the visible source, with its local origin to avoid raster coordinate limits.
        painter.setWorldTransform(QTransform::fromTranslate(source.x(),source.y())*mapping);
        const QImage *sample=&image;
        const double density=std::max(fitted.width()/bounds.width(),fitted.height()/bounds.height())*devicePixelRatioF();
        for(const auto &level:levels)if(double(level.width())/image.width()>=density)sample=&level;
        const double sx=double(sample->width())/image.width(),sy=double(sample->height())/image.height();
        painter.drawImage(QRectF(QPointF(),source.size()),*sample,QRectF(source.x()*sx,source.y()*sy,source.width()*sx,source.height()*sy));
    }
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
void PreviewPane::setImage(const QImage &image) {
    auto *surface=static_cast<NativeVideoSurface *>(surface_);surface->image=image;surface->buildLevels();
    surface->imageTransform.reset();
    surface->setAttribute(Qt::WA_PaintOnScreen,image.isNull());
    surface->setAttribute(Qt::WA_NoSystemBackground,image.isNull());
    if(!image.isNull()){setVideoSize(image.size());setSurfaceActive(true);}
    adoptView(zoom_,panX_,panY_);surface->update();
}
QImage PreviewPane::image() const { return static_cast<NativeVideoSurface *>(surface_)->image; }
QImage PreviewPane::captureImage() const {
    const auto *view=static_cast<NativeVideoSurface *>(surface_);if(view->image.isNull())return {};
    const auto format=view->image.format();const bool floating=format==QImage::Format_RGBA32FPx4 || format==QImage::Format_RGBA16FPx4 || format==QImage::Format_RGBX32FPx4 || format==QImage::Format_RGBX16FPx4;
    const auto ratio=surface_->devicePixelRatioF();QImage result(surface_->size()*ratio,floating?QImage::Format_RGBA32FPx4:QImage::Format_RGBA64);result.setDevicePixelRatio(ratio);result.setColorSpace(view->image.colorSpace());
    QPainter painter(&result);painter.fillRect(QRectF(QPointF(),surface_->size()),QColor("#101010"));painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const auto bounds=view->imageTransform.mapRect(QRectF(QPointF(),view->image.size()));const auto fitted=bounds.size().scaled(surface_->size(),Qt::KeepAspectRatio)*view->zoom;
    const QPointF origin((surface_->width()-fitted.width())/2+view->panX*std::max(0.0,fitted.width()-surface_->width())/2,(surface_->height()-fitted.height())/2+view->panY*std::max(0.0,fitted.height()-surface_->height())/2);
    QTransform screen;screen.translate(origin.x(),origin.y());screen.scale(fitted.width()/bounds.width(),fitted.height()/bounds.height());screen.translate(-bounds.x(),-bounds.y());
    painter.setWorldTransform(view->imageTransform*screen);painter.drawImage(QPointF(),view->image);return result;
}
QTransform PreviewPane::imageTransform() const { return static_cast<NativeVideoSurface *>(surface_)->imageTransform; }
QSize PreviewPane::imageDisplaySize() const { return imageTransform().mapRect(QRectF(QPointF(),image().size())).size().toSize(); }
void PreviewPane::setImageTransform(const QTransform &transform) {
    static_cast<NativeVideoSurface *>(surface_)->imageTransform=transform;
    setVideoSize(imageDisplaySize());adoptView(1,0,0);emit viewChanged(1,0,0);surface_->update();
}

void PreviewPane::adoptView(float zoom, float panX, float panY)
{
    zoom_ = zoom;
    panX_ = panX;
    panY_ = panY;
    zoomBadge_->setText(QStringLiteral("%1%").arg(qRound(zoom_ * 100.0f)));
    surface_->setCursor(zoom_ > 1.0f ? Qt::OpenHandCursor : Qt::CrossCursor);
    auto *surface=static_cast<NativeVideoSurface *>(surface_);
    surface->zoom=zoom_;surface->panX=panX_;surface->panY=panY_;
    if(!surface->image.isNull())surface->update();
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
    if(!image().isNull() && event->modifiers().testFlag(Qt::ShiftModifier)){
        const auto fitted=imageDisplaySize().scaled(surface_->size(),Qt::KeepAspectRatio)*zoom_;
        const auto pixels=event->pixelDelta();const auto angles=event->angleDelta();
        double dx=pixels.isNull()?angles.x()/120.0*60:pixels.x();double dy=pixels.isNull()?angles.y()/120.0*60:pixels.y();
        if(event->modifiers().testFlag(Qt::ShiftModifier)){dx+=dy;dy=0;}
        const double spanX=std::max(0.0,double(fitted.width()-surface_->width())),spanY=std::max(0.0,double(fitted.height()-surface_->height()));
        if(spanX>0)panX_=std::clamp(float(panX_+dx*2/spanX),-1.f,1.f);
        if(spanY>0)panY_=std::clamp(float(panY_+dy*2/spanY),-1.f,1.f);
        adoptView(zoom_,panX_,panY_);emit viewChanged(zoom_,panX_,panY_);return;
    }
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
    next = std::clamp(next, 0.25f, image().isNull()?16.0f:65536.0f);
    const QSizeF fitted=videoSize_.isEmpty()?QSizeF(surface_->size()):QSizeF(videoSize_.scaled(surface_->size(),Qt::KeepAspectRatio));
    const auto anchoredPan=[&](double extent,double base,double at,float pan){
        const double oldSize=base*zoom_,newSize=base*next;
        const double oldOrigin=(extent-oldSize)/2+pan*std::max(0.,oldSize-extent)/2;
        const double newOrigin=at-(at-oldOrigin)*next/zoom_;
        const double overflow=std::max(0.,newSize-extent);
        return overflow>0?std::clamp(float((newOrigin-(extent-newSize)/2)*2/overflow),-1.f,1.f):0.f;
    };
    panX_=anchoredPan(surface_->width(),fitted.width(),anchor.x(),panX_);
    panY_=anchoredPan(surface_->height(),fitted.height(),anchor.y(),panY_);
    adoptView(next,panX_,panY_);
    emit viewChanged(zoom_, panX_, panY_);
}

}
