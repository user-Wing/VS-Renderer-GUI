#pragma once
#include <QStringList>
namespace vsr {
QStringList playerVideoExtensions();
QStringList playerAudioExtensions();
QStringList playerImageExtensions();
bool registerPlayerAssociations(const QStringList &extensions,const QString &executable,const QString &root="HKEY_CURRENT_USER\\Software");
}
