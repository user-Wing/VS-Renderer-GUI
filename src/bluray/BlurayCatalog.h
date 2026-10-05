#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QStringList>
#include <QVector>

namespace vsr {
struct BlurayPart {
    QString clip;
    quint32 in = 0, out = 0;
    QByteArray data;
    bool multiAngle = false;
};
struct BlurayMark {
    int item = 0;
    quint32 time = 0;
    QByteArray data;
};
struct BlurayPlaylist {
    QString path;
    QByteArray header;
    QVector<BlurayPart> parts;
    QVector<BlurayMark> marks;
    bool subpaths = false;
};
struct BlurayTitle {
    QString disc, playlist, label;
    int firstItem = 0, itemCount = 0;
    qint64 ticks = 0;
    int chapters = 0;
    QStringList clips;
    bool suggested = false;
    QString warning;
};
struct BlurayScan {
    QString source;
    QVector<BlurayTitle> titles;
    QStringList warnings;
};
class BlurayCatalog {
public:
    static bool parse(const QByteArray &data, BlurayPlaylist *playlist, QString *error);
    static QByteArray slice(const BlurayPlaylist &playlist, int first, int count);
    static BlurayScan scan(const QString &path, const QJsonObject &profile = {});
    static QString prepare(const BlurayTitle &title, QString *error);
    static QJsonObject feedback(const BlurayScan &scan);
    static QJsonObject metadata(const QString &playlist);
    static QString mkvmerge();
    static QString time(qint64 ticks);
};
}
