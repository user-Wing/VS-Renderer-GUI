#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace vsr {

inline constexpr auto playerStartupRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr auto playerStartupStateKey = "HKEY_CURRENT_USER\\Software\\VSPlayer\\Startup";
#ifdef VSR_LITE_PLAYER
inline constexpr auto playerStartupName = "VS Player Single";
#else
inline constexpr auto playerStartupName = "VS Player Full";
#endif

inline QString playerStartupExecutable() {
#ifdef VSR_LITE_PLAYER
    return qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH");
#else
    return QCoreApplication::applicationFilePath();
#endif
}

inline QString playerStartupCommand(const QString &exe) {
    return QStringLiteral("\"%1\" --standby").arg(QDir::toNativeSeparators(exe));
}

inline bool rememberPlayerStartupChoice(QSettings &state) {
    state.setValue(playerStartupName, true);
    state.sync();
    return state.status() == QSettings::NoError;
}

inline void initializePlayerLoginStartup() {
    const auto exe = playerStartupExecutable();
    const auto command = playerStartupCommand(exe);
    if (!QFileInfo(exe).isAbsolute() || !QFileInfo(exe).isFile() || command.size() >= 260) return;
    QSettings state(playerStartupStateKey, QSettings::NativeFormat);
    if (state.contains(playerStartupName)) return;
    QSettings entries(playerStartupRunKey, QSettings::NativeFormat);
    if (!entries.contains(playerStartupName)) {
        entries.setValue(playerStartupName, command);
        entries.sync();
    }
    if (entries.status() == QSettings::NoError) rememberPlayerStartupChoice(state);
}

}
