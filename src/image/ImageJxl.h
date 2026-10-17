#pragma once
#include <QImage>
#include <QString>

namespace vsr {
// Lossless RGBA16, with native JPEG XL color encoding and embedded desktop white.
QString writeJxl(const QString &path,const QImage &image);
QImage readJxl(const QString &path,QString *error=nullptr);
}
