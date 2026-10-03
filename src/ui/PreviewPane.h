#pragma once

#include <QPointF>
#include <QWidget>
#include <QImage>
#include <QTransform>

class QLabel;
class QStackedLayout;
class QTimer;
class QWheelEvent;

namespace vsr {

class PreviewPane final : public QWidget {
    Q_OBJECT
public:
    PreviewPane(const QString &title, const QString &badge, QWidget *parent = nullptr);
    QWidget *surface() const;
    float zoom() const;
    QPointF pan() const;
    void setChromeVisible(bool visible);
    void setVideoSize(const QSize &size);
    void setImage(const QImage &image);
    QImage image() const;
    void setImageTransform(const QTransform &transform);
    QTransform imageTransform() const;
    QSize imageDisplaySize() const;
    void adoptView(float zoom, float panX, float panY);
    void setActive(bool active);
    void setSurfaceActive(bool active);
    void setTitle(const QString &title);
    void setBadge(const QString &badge);
    void setPlaceholderText(const QString &text);
    void setPixelText(const QString &text);

signals:
    void viewChanged(float zoom, float panX, float panY);
    void pixelHovered(int x, int y);
    void redrawRequested();
    void activated();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void positionPixel();
    void applyWheel(QWheelEvent *event, const QPointF &surfacePosition);
    void scheduleRedraw();
    void updateZoom(float next, const QPointF &anchor);

    QWidget *titleBar_ = nullptr;
    QLabel *titleLabel_ = nullptr;
    QLabel *badge_ = nullptr;
    QLabel *zoomBadge_ = nullptr;
    QLabel *placeholder_ = nullptr;
    QLabel *pixel_ = nullptr;
    QWidget *surface_ = nullptr;
    QStackedLayout *stack_ = nullptr;
    QTimer *redrawTimer_ = nullptr;
    float zoom_ = 1.0f;
    float panX_ = 0.0f;
    float panY_ = 0.0f;
    bool dragging_ = false;
    QPointF dragStart_;
    QPointF dragPanStart_;
    QSize videoSize_;
};

}
