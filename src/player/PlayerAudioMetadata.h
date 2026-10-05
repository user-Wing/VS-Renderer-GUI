#pragma once
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
namespace vsr {
struct PlayerLyricLine { qint64 start=0,end=0; QString text; };
struct PlayerAudioMetadata {
    bool audioOnly=false;
    QImage cover;
    QVector<PlayerLyricLine> lyrics;
    QString lyricSource,error;
    QJsonArray chapters;
    static PlayerAudioMetadata read(const QString &path);
    static QJsonObject probe(const QString &path);
    static QVector<PlayerLyricLine> parseLyrics(QString text,qint64 duration);
};
}
