#pragma once
#include "image/ImageDocument.h"
#include <QPointF>
#include <QVector>
#include <array>

namespace vsr {
enum class ImageAdjustmentKind { Exposure, BrightnessContrast, Levels, Curves, HueSaturation, ColorBalance, ChannelMixer, Invert, Grayscale, Threshold, Posterize, GaussianBlur, UnsharpMask, Median, EdgeDetect };
struct ImageAdjustmentSpec {
    ImageAdjustmentKind kind = ImageAdjustmentKind::Exposure;
    double exposure = 0, brightness = 0, contrast = 0;
    double gamma = 1, inputBlack = 0, inputWhite = 1, outputBlack = 0, outputWhite = 1;
    double hue = 0, saturation = 1, lightness = 0;
    double redGain = 1, greenGain = 1, blueGain = 1;
    double threshold = .5;
    int posterizeLevels = 8;
    int radius = 3;
    double amount = 1; // Unsharp mask amount; radius is the kernel support in pixels.
    std::array<QVector<QPointF>,4> curves; // Master, R, G, B; empty = identity. Linear extrapolation preserves HDR.
    std::array<std::array<double,3>,3> channelMixer{{{{1,0,0}},{{0,1,0}},{{0,0,1}}}};
};
class ImageAdjustments final {
public:
    static bool validate(const ImageAdjustmentSpec &spec, QString *error = nullptr);
    static QImage process(const QImage &source, const ImageAdjustmentSpec &spec);
    // Applies one undo transaction, weighted by the document selection. Alpha and precision are retained.
    static bool apply(ImageDocument *document, const QUuid &layer, const ImageAdjustmentSpec &spec, const QString &label, QString *error = nullptr);
    // Converts all pixel layers in bounded blocks. Does not apply a selection to a profile conversion.
    static bool convertColorSpace(ImageDocument *document, const QColorSpace &target, QString *error = nullptr);
    static std::array<QVector<quint64>,4> histogram(const QImage &source, int bins = 256);
};
}
