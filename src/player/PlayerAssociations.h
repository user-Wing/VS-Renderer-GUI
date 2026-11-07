#pragma once
#include <QStringList>
#include <QCoreApplication>
#include <QFileInfo>
namespace vsr {
inline QString playerAssociationExecutable(){const auto launcher=qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH");return launcher.isEmpty()?QCoreApplication::applicationFilePath():QFileInfo(launcher).absoluteFilePath();}
QStringList playerVideoExtensions();
QStringList playerAudioExtensions();
QStringList playerImageExtensions();
bool registerPlayerAssociations(const QStringList &extensions,const QString &executable,const QString &root="HKEY_CURRENT_USER\\Software");
}
