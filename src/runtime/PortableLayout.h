#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>

namespace vsr {
inline QString programRoot() {
    const auto launcher = qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH");
    return QDir::isAbsolutePath(launcher) ? QFileInfo(launcher).absolutePath() : QCoreApplication::applicationDirPath();
}
inline QString configFile(const QString &name) {
    return QDir(programRoot()).filePath("config/" + name);
}
inline QString playerConfigFile() {
    const auto overridePath = qEnvironmentVariable("VSR_PLAYER_CONFIG");
    return overridePath.isEmpty() ? configFile("player.ini") : overridePath;
}
// Merge old installs without replacing a setting or component already installed at its new path.
inline void migrateEntry(const QString &from, const QString &to) {
    QFileInfo source(from);
    if (!source.exists() || source.isSymLink() || QFileInfo(to).isSymLink()) return;
    if (!QDir().mkpath(QFileInfo(to).absolutePath())) return;
    if (!QFileInfo::exists(to) && QDir().rename(from, to)) return;
    if (source.isDir() && QFileInfo(to).isDir()) {
        const QDir old(from);
        for (const auto &entry : old.entryInfoList(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks))
            migrateEntry(entry.absoluteFilePath(), QDir(to).filePath(entry.fileName()));
        QDir().rmdir(from);
    }
}
inline void migratePortableLayout() {
    const QDir root(programRoot());
    root.mkpath("config");
    const QPair<QString,QString> moves[] = {
        {"player.ini","config/player.ini"},{"vs-cap.ini","config/cap.ini"},{"renderer.ini","config/renderer.ini"},
        {"themes","config/skin"},{"themes.ini","config/themes.ini"},{"languages","config/languages"},
        {"shaders/custom","config/shaders/custom"},{"luts","config/LUT"},{"preset-descriptions.json","config/preset-descriptions.json"},
        {"cache/indexes","config/cache"},{"LAVFilters64","tools/lav"},{"madVR09217","tools/madvr09217"}
    };
    for (const auto &move : moves) migrateEntry(root.filePath(move.first), root.filePath(move.second));
    const QDir presets(root.filePath("vpy"));
    for(const auto &name : presets.entryList({"*.vpy"},QDir::Files|QDir::NoSymLinks))
        migrateEntry(presets.filePath(name),presets.filePath("user/"+name));
    if (QFileInfo::exists(playerConfigFile())) {
        QSettings player(playerConfigFile(),QSettings::IniFormat);
        const QPair<QString,QString> paths[]={{"shaders/custom","config/shaders/custom"},{"luts","config/LUT"},
            {"runtime/ai-subtitles","tools/ai-subtitles"},{"cache/indexes","config/cache"}};
        for(const auto &key:{"render/customShader","render/shaderDirectory","ai/package","cache/path","player/preset"}) {
            const auto value=QDir::fromNativeSeparators(player.value(key).toString());
            if(value.isEmpty())continue;
            if(QString::fromLatin1(key)=="player/preset" && QFileInfo(value).dir()==presets && !QFileInfo::exists(value)) {
                const auto target=presets.filePath("user/"+QFileInfo(value).fileName());
                if(QFileInfo::exists(target))player.setValue(key,target);
            }
            for(const auto &path:paths){
                const auto old=root.filePath(path.first);
                if(value==old || value.startsWith(old+"/"))player.setValue(key,root.filePath(path.second)+value.mid(old.size()));
            }
        }
    }
    if (QFileInfo::exists(configFile("themes.ini"))) {
        QSettings theme(configFile("themes.ini"), QSettings::IniFormat);
        QString oldSkinFolder;
        for (const auto &key : theme.allKeys()) {
            const auto value = theme.value(key).toString();
            QString relative;
            if (value.startsWith("themes/")) relative=value.mid(7);
            else if (value.startsWith(root.filePath("themes/"))) relative=value.mid(root.filePath("themes/").size());
            if(relative.isEmpty())continue;
            theme.setValue(key,"skin/"+relative);
            oldSkinFolder=QFileInfo(configFile("skin/"+relative)).absolutePath();
        }
        if(!oldSkinFolder.isEmpty() && QFileInfo(oldSkinFolder).canonicalFilePath().startsWith(QFileInfo(configFile("skin")).absoluteFilePath()+"/",Qt::CaseInsensitive) && !QFileInfo::exists(oldSkinFolder+"/skin.ini")){
            QSettings pack(oldSkinFolder+"/skin.ini",QSettings::IniFormat);
            for(const auto &key:theme.allKeys()){
                auto value=theme.value(key);
                if(key.endsWith("/image") && !value.toString().isEmpty())
                    value=QDir(oldSkinFolder).relativeFilePath(QFileInfo(theme.fileName()).dir().absoluteFilePath(value.toString()));
                pack.setValue(key,value);
            }
        }
    }
}
inline QSettings rendererSettings() {
    if (QCoreApplication::applicationName() != "VS Renderer") return QSettings();
    return QSettings(configFile("renderer.ini"), QSettings::IniFormat);
}
}
