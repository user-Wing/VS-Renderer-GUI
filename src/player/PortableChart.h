#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
namespace vsr {
inline QString resolveParetoImage(QSettings &settings){
    const QDir directory(QFileInfo(settings.fileName()).absolutePath());
    auto name=settings.value("render/paretoImage").toString().trimmed();if(name.isEmpty())name="Pareto.avif";
    auto path=directory.absoluteFilePath(name);
    // Old absolute entries remain usable after moving the portable directory.
    const auto local=directory.filePath(QFileInfo(name).fileName());
    if(QFileInfo(local).isFile())path=local;
    if(!QFileInfo(path).isFile())path=QDir(QCoreApplication::applicationDirPath()).filePath(QFileInfo(name).fileName());
    if(!QFileInfo(path).isFile())return {};
    auto relative=directory.relativeFilePath(path);
    if(relative.startsWith("../")){
        // Single-player assets live in the temporary runtime on every launch.
        if(QFileInfo(path).absolutePath()==QCoreApplication::applicationDirPath())relative=QFileInfo(path).fileName();
        else relative=path;
    }
    settings.setValue("render/paretoImage",relative);settings.sync();return path;
}
}
