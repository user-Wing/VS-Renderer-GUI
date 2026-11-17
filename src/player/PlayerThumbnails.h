#pragma once
#include <QStringList>
namespace vsr {
QStringList thumbnailExtensions();
bool registerThumbnailProvider(const QStringList &extensions,const QString &library,const QString &root="HKEY_CURRENT_USER\\Software");
bool unregisterThumbnailProvider(const QString &root="HKEY_CURRENT_USER\\Software");
bool installPlayerThumbnails(const QStringList &extensions,QString *error);
}
