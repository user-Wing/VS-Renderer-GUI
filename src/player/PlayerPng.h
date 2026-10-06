#pragma once
#include <QImage>
#include <QJsonObject>
#include <atomic>
namespace vsr {
// Fast, lossless RGBA8/non-interlaced path. Unsupported PNGs remain with Qt.
QImage decodeRgbaPng(const QString &path,const std::atomic<quint64> &generation,quint64 expected);
// RGBA16 PNG, adaptive filtering and libdeflate's highest lossless level.
// HDR additionally retains the original RGBA float32 samples and their encoding.
QString writeScreenshotPng(const QString &path,const QImage &image,QJsonObject metadata);
}
