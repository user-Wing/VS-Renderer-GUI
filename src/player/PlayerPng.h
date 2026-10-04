#pragma once
#include <QImage>
#include <atomic>
namespace vsr {
// Fast, lossless RGBA8/non-interlaced path. Unsupported PNGs remain with Qt.
QImage decodeRgbaPng(const QString &path,const std::atomic<quint64> &generation,quint64 expected);
}
