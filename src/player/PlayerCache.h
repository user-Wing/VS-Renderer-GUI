#pragma once
#include <QString>
class QSettings;
namespace vsr {
QString playerCacheDirectory(const QSettings &settings);
QString playerIndexPath(const QSettings &settings, const QString &source, const QString &extension);
qint64 playerCacheBytes(const QString &directory);
bool clearPlayerIndexes(const QString &directory);
}
