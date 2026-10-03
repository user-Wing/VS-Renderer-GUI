#pragma once
#include "image/ImageDocument.h"
#include <QStringList>

namespace vsr {
// Pixel layers are lazy, retaining a file-backed source until the document closes.
class ImagePsd final {
public:
    static std::unique_ptr<ImageDocument> load(const QString &path, QString *error = nullptr, QStringList *warnings = nullptr);
    static QString save(ImageDocument *document, const QString &path);
};
}
