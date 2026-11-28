#pragma once
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QPluginLoader>
#include <qt_windows.h>

namespace vsr {
// Register the packaged Qt 6 factories before QApplication selects its platform.
inline void registerPortableQtPlugins() {
    wchar_t executable[32768];
    const auto length = GetModuleFileNameW(nullptr, executable, 32768);
    if (!length || length >= 32768) return;
    const QDir directory = QFileInfo(QString::fromWCharArray(executable, int(length))).absoluteDir();
    for (const char *name : {"qwindows", "qnetworklistmanager", "qmodernwindowsstyle", "qsvgicon",
                             "qgif", "qico", "qjpeg", "qsvg", "qtiff", "qwebp", "qavif", "qheif",
                             "qcertonlybackend", "qschannelbackend", "qopensslbackend"}) {
        const auto path = directory.filePath(QString::fromLatin1(name) + ".dll");
        if (!QFileInfo::exists(path)) continue;
        QPluginLoader plugin(path);
        if (!plugin.load()) continue;
        QLibrary library(path);
        const auto instance = reinterpret_cast<QtPluginInstanceFunction>(library.resolve("qt_plugin_instance"));
        const auto metadata = reinterpret_cast<QtPluginMetaDataFunction>(library.resolve("qt_plugin_query_metadata_v2"));
        if (instance && metadata) qRegisterStaticPluginFunction(QStaticPlugin(instance, metadata));
    }
}
}
