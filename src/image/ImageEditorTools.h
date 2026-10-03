#pragma once
#include "image/ImageDocument.h"
#include <QColor>
#include <QPainterPath>
#include <QFont>
#include <algorithm>

namespace vsr {
enum class ImageEditorTool { Move, Rectangle, Ellipse, Lasso, Polygon, Brush, Eraser, Eyedropper, Hand,
    MagneticLasso, MagicWand, QuickSelection, ObjectSelection, Crop, Ruler, Heal, Clone, ColorReplace,
    Gradient, Fill, Blur, Sharpen, Dodge, Burn, Sponge, Path, Text, ShapeRectangle, ShapeEllipse,
    HistoryBrush, Slice, Zoom };
enum class ImageSelectionMode { Replace, Add, Subtract, Intersect };

class ImageEditorTools final : public QObject {
    Q_OBJECT
public:
    explicit ImageEditorTools(ImageDocument *document, QObject *parent = nullptr);
    ImageDocument *document() const { return document_; }
    void setLayer(const QUuid &id);
    QUuid layer() const { return layer_; }
    QUuid pickLayer(QPoint documentPoint) const;
    bool autoPickLayer(QPoint documentPoint);
    void setBrushColor(const QColor &color) { color_=color; }
    QColor brushColor() const { return color_; }
    void setBrushRadius(double radius);
    double brushRadius() const { return radius_; }
    void setBrushOpacity(double opacity);
    double brushOpacity() const { return opacity_; }
    void setTolerance(double value) { tolerance_=std::clamp(value,0.0,1.0); }
    double tolerance() const { return tolerance_; }
    void setGradientEndColor(const QColor &color) { endColor_=color; }
    QColor gradientEndColor() const { return endColor_; }
    void setText(const QString &text,const QFont &font) { text_=text;font_=font; }
    void setCloneSource(QPointF point);
    bool hasCloneSource() const { return sourceSet_; }
    bool captureHistorySource();
    void setEditingMask(bool value) { endStroke();editingMask_=value; }
    bool editingMask() const { return editingMask_; }
    bool beginStroke(QPointF point, ImageEditorTool tool);
    bool selectColor(QPoint seed, ImageSelectionMode mode, QRect limit = {});
    bool selectObject(QRect box, ImageSelectionMode mode);
    void beginSelectionStroke();
    void endSelectionStroke(bool cancel = false);
    QPointF magneticPoint(QPointF point) const;
    QPainterPath lastSelectionOutline() const { return lastSelectionOutline_; }
    bool fillSelection();
    bool floodFill(QPoint seed);
    bool fillGradient(QPointF start,QPointF end);
    bool fillPath(const QPainterPath &path,const QString &label = {});
    bool strokePath(const QPainterPath &path);
    bool createText(QPointF baseline);
    bool invertSelection();
    bool featherSelection(int radius);
    bool selectPath(const QPainterPath &path, ImageSelectionMode mode);
    bool beginStroke(QPointF point, bool erase);
    void continueStroke(QPointF point);
    void endStroke(bool cancel = false);
    bool beginMove(QPointF point);
    void continueMove(QPointF point);
    void endMove(bool cancel = false);
    QColor sample(QPoint point) const;
    // Flat PNG only: UInt8/UInt16 retained, Float32 and oversized staging rejected.
    static QString savePng(ImageDocument *document, const QString &path);
    static QString savePngRegion(ImageDocument *document,const QRect &region,const QString &path);
signals:
    void errorOccurred(const QString &error);
    void selectionEdited(const QPainterPath &path,ImageSelectionMode mode);
    void selectionInverted();
    void layerChanged(const QUuid &id);
private:
    void dab(QPointF point);
    void effectDab(QPointF point);
    void maskDab(QPointF point);
    void paintCoverage(const QRect &region,const QImage &coverage);
    void paintPath(const QPainterPath &path);
    bool applySelectionMask(const QRect &region,const QImage &mask,ImageSelectionMode mode);
    bool boundedCanvas(const QString &operation) const;
    ImageDocument *document_;
    QUuid layer_;
    QColor color_ = Qt::black;
    QColor endColor_ = Qt::white;
    QString text_ = QStringLiteral("Text");
    QFont font_ = QFont(QStringLiteral("Arial"),24);
    double radius_ = 12, opacity_ = 1;
    double tolerance_ = .12;
    ImageEditorTool strokeTool_ = ImageEditorTool::Brush;
    QPointF sourcePoint_,sourceDelta_;
    QImage historySource_,cloneSource_;
    QImage replaceTarget_;
    QPoint historyOrigin_,cloneOrigin_;
    bool sourceSet_ = false;
    bool selecting_ = false;
    bool editingMask_ = false;
    ImagePrecision cachedPrecision_ = ImagePrecision::UInt8;
    QPainterPath lastSelectionOutline_;
    QPointF previous_, anchor_;
    ImageLayerInfo movingLayer_;
    QList<ImageLayerInfo> movingLayers_;
    bool stroking_ = false, erasing_ = false, moving_ = false;
};
}
