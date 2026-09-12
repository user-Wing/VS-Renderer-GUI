#pragma once

#include <QPointF>
#include <QWidget>

class QLabel;
class QStackedLayout;

namespace vsr {

class PreviewPane final : public QWidget {
    Q_OBJECT
public:
    PreviewPane(const QString &title, const QString &badge, QWidget *parent = nullptr);
    QWidget *surface() const;
    float zoom() const;
    void adoptView(float zoom, float panX, float panY);
    void setActive(bool active);
    void setSurfaceActive(bool active);
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

private:
    void updateZoom(float next, const QPointF &anchor);

    QLabel *badge_ = nullptr;
    QLabel *placeholder_ = nullptr;
    QLabel *pixel_ = nullptr;
    QWidget *surface_ = nullptr;
    QStackedLayout *stack_ = nullptr;
    float zoom_ = 1.0f;
    float panX_ = 0.0f;
    float panY_ = 0.0f;
    bool dragging_ = false;
    QPointF dragStart_;
    QPointF dragPanStart_;
};

}
