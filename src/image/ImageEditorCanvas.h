#pragma once
#include "image/ImageEditorTools.h"
#include "image/ImageHdr.h"
#include <QWidget>
#include <QPolygonF>
#include <QHash>
class QThread;
class QPainter;

namespace vsr {
class ImageHdrSurface;
class ImageEditorCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit ImageEditorCanvas(ImageEditorTools *tools,QWidget *parent = nullptr);
    ~ImageEditorCanvas() override;
    void setTool(ImageEditorTool tool);
    ImageEditorTool tool() const { return tool_; }
    QPointF documentPoint(QPointF point) const;
    QPointF screenPoint(QPointF point) const;
    double zoom() const { return zoom_; }
    void fitToWindow();
    void actualSize();
    void zoomBy(double factor,QPointF anchor = {});
    void invalidateDocumentPreview();
    void cancelGesture();
    void startFreeTransform();
    bool freeTransformActive() const { return freeTransform_; }
    QRectF selectedLayerBounds() const;
    void setSelectionMode(ImageSelectionMode mode) { defaultSelectionMode_=mode; }
    void setPreviewChannel(int channel);
    void setHdrPreviewSettings(const ImageHdrPreviewSettings &settings);
    ImageHdrPreviewSettings hdrPreviewSettings() const { return hdrSettings_; }
    bool setNativeHdrEnabled(bool enabled);
    bool nativeHdrActive() const;
    QString nativeHdrStatus() const;
    bool previewBusy() const { return previewThread_ || refreshPending_ || refreshAgain_; }
    void setMaskPreview(bool enabled);
    void setCropAspect(double ratio);
    void setShowGrid(bool value) { showGrid_=value;update(); }
    QPainterPath vectorPath() const { return vectorPath_; }
    void fillVectorPath();
    void strokeVectorPath();
    void selectVectorPath();
signals:
    void colorPicked(const QColor &color);
    void viewChanged();
    void toolChanged(ImageEditorTool tool);
    void cursorPositionChanged(QPointF position,bool visible);
    void rulerMeasured(double distance,double angle);
    void sliceCreated(const QRect &region);
    void previewReady();
    void previewError(const QString &error);
    void selectionContextMenuRequested(QPoint globalPosition);
protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void leaveEvent(QEvent *) override;
private:
    QPainterPath gesturePath() const;
    void finishSelection();
    void refreshPreview();
    QRect visibleDocumentRegion() const;
    QRect bufferedRegion(const QRect &visible) const;
    QSize previewOutput(const QRect &region) const;
    bool previewCovers(const QRect &visible) const;
    void updatePreviewTarget();
    void rememberColorSelection(const QPainterPath &prior);
    void quickSelect(QPointF point);
    void paintGuides(QPainter &painter);
    int layerHandle(QPointF point) const;
    void updateLayerTransform(QPointF point,bool proportional);
    void finishLayerTransform();
    void prepareMovePreview();
    ImageEditorTools *tools_;
    ImageEditorTool tool_ = ImageEditorTool::Hand;
    double zoom_ = 1;
    QPointF origin_,anchor_,current_,lastScreen_;
    QPolygonF polygon_;
    QImage preview_,nativePreview_;
    QImage overview_,overviewNative_,moveBackdrop_,movePixels_;
    QRect moveRegion_;
    QPointF moveDelta_;
    QThread *previewThread_ = nullptr;
    ImageHdrSurface *hdrSurface_;
    bool refreshAgain_ = false;
    QRectF previewTarget_;
    QRect previewRegion_;
    quint64 previewRevision_ = 0;
    quint64 previewInvalidation_ = 0,previewGeneration_ = 0;
    ImageHdrPreviewSettings previewSettings_;
    QUuid previewLayer_;
    bool previewMask_ = false;
    QPainterPath selectionOutline_;
    QHash<quint64,QPainterPath> selectionOutlines_;
    ImageSelectionMode selectionMode_ = ImageSelectionMode::Replace;
    ImageSelectionMode defaultSelectionMode_ = ImageSelectionMode::Replace;
    ImageHdrPreviewSettings hdrSettings_;
    QPainterPath vectorPath_;
    QPointF rulerStart_,rulerEnd_;
    QRect slice_;
    QRectF cropRect_,cropAnchorRect_;
    int cropHandle_=0;
    double cropAspect_ = 0;
    QRectF transformRect_,transformAnchorRect_;
    int transformHandle_ = -1;
    bool freeTransform_ = false;
    bool maskPreview_ = false;
    bool showGrid_=false,rulerVisible_=false;
    bool dragging_ = false,panning_ = false,fit_ = true,refreshPending_ = false;
};
}
