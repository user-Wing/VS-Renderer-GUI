#include "player/PlayerCache.h"
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFile>
namespace vsr {
QString playerCacheDirectory(const QSettings &settings) {
    const auto custom=settings.value("cache/path").toString().trimmed();
    return QDir::cleanPath(custom.isEmpty()?QDir(QCoreApplication::applicationDirPath()).filePath("cache/indexes"):QFileInfo(custom).absoluteFilePath());
}
QString playerIndexPath(const QSettings &settings,const QString &source,const QString &extension) {
    const QFileInfo info(source);
    const auto identity=info.absoluteFilePath().toUtf8()+'\n'+QByteArray::number(info.size())+'\n'+QByteArray::number(info.lastModified().toMSecsSinceEpoch());
    const auto directory=playerCacheDirectory(settings);if(!QDir().mkpath(directory))return {};
    return QDir(directory).filePath(QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex())+'.'+extension);
}
qint64 playerCacheBytes(const QString &directory) {
    qint64 bytes=0;for(const auto &file:QDir(directory).entryInfoList({"*.ffindex","*.lwi"},QDir::Files|QDir::NoSymLinks))bytes+=file.size();return bytes;
}
bool clearPlayerIndexes(const QString &directory) {
    bool ok=true;for(const auto &file:QDir(directory).entryInfoList({"*.ffindex","*.lwi"},QDir::Files|QDir::NoSymLinks))ok=QFile::remove(file.absoluteFilePath())&&ok;return ok;
}
}
