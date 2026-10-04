#pragma once
#include <QObject>
#include <QImage>
#include <QColorSpace>
#include <QUndoStack>
#include <QUuid>
#include <memory>
#include <functional>
#include <array>

namespace vsr {
enum class ImagePrecision { UInt8, UInt16, Float32 };
enum class ImageBlendMode { Normal, Multiply, Screen, Overlay, Darken, Lighten, ColorDodge, ColorBurn, LinearDodge, LinearBurn, SoftLight, HardLight, Difference, Exclusion, Subtract, Divide };
enum class ImageBlendIfChannel { Gray, Red, Green, Blue };
enum class ImageSamplingQuality { Nearest, Bilinear };
using ImageBlendRange = std::array<float,4>; // Black start/full, white full/end; normalized 0..1.
struct ImageLayerInfo {
    QUuid id;
    QUuid parentId;
    QString name;
    QPoint offset;
    QSize extent;
    bool group = false, locked = false, passThrough = false, clipping = false;
    bool visible = true, maskEnabled = true;
    uchar maskDefault = 255;
    float opacity = 1;
    float fillOpacity = 1;
    quint8 channels = 7; // R=1, G=2, B=4; enabled for blending.
    ImageBlendIfChannel blendIfChannel = ImageBlendIfChannel::Gray;
    std::array<ImageBlendRange,4> blendIfSource{{{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}}};
    std::array<ImageBlendRange,4> blendIfBackdrop{{{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}}};
    ImageBlendMode blend = ImageBlendMode::Normal;
};

// Document coordinates for composite/selection; layer-local coordinates for pixels/masks.
// All calls belong to the owning thread. Worker tasks receive detached region images.
class ImageDocument final : public QObject {
    Q_OBJECT
public:
    static constexpr int TileSize = 256;
    explicit ImageDocument(QSize size, ImagePrecision precision, QObject *parent = nullptr);
    ~ImageDocument() override;
    QSize size() const;
    ImagePrecision precision() const;
    QImage::Format pixelFormat() const;
    QColorSpace colorSpace() const;
    void setColorSpace(const QColorSpace &space);
    void setMetadata(const QString &key, const QString &value);
    QString metadata(const QString &key) const;
    QList<ImageLayerInfo> layers() const; // Bottom to top.
    QUuid addLayer(const QString &name, const QImage &image = {}, QPoint offset = {});
    using RegionReader = std::function<QImage(const QRect &)>;
    using PreviewReader = std::function<QImage(const QRect &, QSize)>;
    using QualityPreviewReader = std::function<QImage(const QRect &, QSize, ImageSamplingQuality)>;
    QUuid addLazyLayer(const QString &name, QSize size, QPoint offset, RegionReader reader, PreviewReader preview = {}, QualityPreviewReader qualityPreview = {});
    void setLazyMask(const QUuid &id, RegionReader reader, PreviewReader preview, QRect bounds = {}, QualityPreviewReader qualityPreview = {});
    QRect layerBounds(const QUuid &id) const;
    QList<QRect> layerRegions(const QUuid &id) const;
    QUuid duplicateLayer(const QUuid &id);
    bool transformLayer(const QUuid &id, const QRect &targetDocumentBounds, QString *error = nullptr);
    bool flipLayer(const QUuid &id, bool horizontal, QString *error = nullptr);
    bool rotateLayer(const QUuid &id, int quarterTurns, QString *error = nullptr);
    QUuid mergeDown(const QUuid &id, QString *error = nullptr);
    QUuid createGroup(const QString &name, const QUuid &parentId = {});
    QUuid groupLayer(const QUuid &id, const QString &name, QString *error = nullptr);
    QUuid mergeVisible(QString *error = nullptr);
    QUuid flatten(QString *error = nullptr);
    void resizeCanvas(QSize size, QPoint offset = {});
    void crop(const QRect &rect);
    void convertPrecision(ImagePrecision precision);
    std::unique_ptr<ImageDocument> snapshot() const;
    void removeLayer(const QUuid &id);
    void updateLayer(const ImageLayerInfo &info, const QString &label = {});
    void moveLayer(const QUuid &id, int index);
    QImage readRegion(const QUuid &id, const QRect &localRegion) const;
    QImage readOriginalRegion(const QUuid &id, const QRect &localRegion) const;
    void writeRegion(const QUuid &id, QPoint localOrigin, const QImage &pixels, const QString &label = {});
    QImage maskRegion(const QUuid &id, const QRect &localRegion) const; // Grayscale16, absent=white.
    QImage maskPreview(const QUuid &id, const QRect &localRegion, QSize outputSize, ImageSamplingQuality quality = ImageSamplingQuality::Nearest) const;
    QRect maskBounds(const QUuid &id) const;
    void writeMask(const QUuid &id, QPoint localOrigin, const QImage &coverage, const QString &label = {});
    QImage selectionRegion(const QRect &documentRegion) const; // Grayscale16, absent selection=white.
    void writeSelection(QPoint origin, const QImage &coverage, const QString &label = {});
    void clearSelection();
    bool hasSelection() const;
    QRect selectionBounds() const;
    quint64 selectionId() const; // Stable content identity restored by undo/redo; zero means no selection.
    QImage composite(const QRect &documentRegion) const; // Native precision, no display tone mapping.
    QImage compositePreview(const QRect &documentRegion, QSize outputSize, ImageSamplingQuality quality = ImageSamplingQuality::Nearest) const; // Bounded native-precision preview.
    void beginEdit(const QString &label);
    void commitEdit();
    void cancelEdit();
    QUndoStack *history();
    quint64 revision() const;
    void setCacheBudget(qint64 bytes);
    qint64 residentBytes() const;
    qint64 storedBytes() const;
    QString storageError() const;
signals:
    void changed();
private:
    bool reframeLayer(const QUuid &id, const QRect &targetDocumentBounds, int quarterTurns, int flipAxis, QString *error);
    struct Impl;
    std::unique_ptr<Impl> d;
};
}
