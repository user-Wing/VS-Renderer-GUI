#pragma once
#include <QImage>
#include <QString>
#include "color/ScreenshotColor.h"
class QScreen;
class QWidget;

namespace cap {
vsr::screenshot::DisplayEvidence displayEvidence(QScreen *screen);
QImage captureWgcScreen(QScreen *screen,QString *error); // Explicit backend probe; no persistent session.
bool placeCaptureWindow(QWidget *window,const QRect &nativeBounds);
QImage captureScreen(QScreen *screen, QString *error);
double sdrWhiteNits(const QImage &image);
QImage preview(const QImage &image);
QImage pngPixels(const QImage &image);
bool savePng(const QImage &image, const QString &path);
}
