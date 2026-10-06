#pragma once
#include <QString>
#include <QStringList>
namespace vsr {
double playerMediaMatchScore(const QString &video, const QString &track);
QStringList playerMatchingSubtitles(const QString &video);
}
