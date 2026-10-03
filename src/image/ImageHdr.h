#pragma once
#include "image/ImageDocument.h"
namespace vsr {
struct ImageHdrPreviewSettings {
    double exposure = 0;
    double whitePoint = 4;
    bool toneMap = false;
    int channel = -1; // -1 composite, 0/1/2 RGB, 3 alpha (display only).
};
class ImageHdr final {
public:
    // SDR display mapping only. The source is never modified or re-tagged as another working space.
    static QImage preview(const QImage &source, const ImageHdrPreviewSettings &settings = {});
    // Streamed 32-bit IEEE-float RGBA TIFF; unassociated alpha, ICC, negatives and >1 preserved.
    // Uses BigTIFF only when the classic 4 GiB offset limit would be exceeded.
    static bool exportFloatTiff(ImageDocument *document, const QString &path, QString *error = nullptr);
};
}
