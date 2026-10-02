#include "player/PlayerWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include "player/PlayerAssociations.h"
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("VS Player"); app.setOrganizationName("VSRenderer"); app.setApplicationVersion(VSR_VERSION);
    app.setWindowIcon(QIcon(":/icons/player.ico"));
    QSettings associations("HKEY_CURRENT_USER\\Software\\VSPlayer\\Capabilities", QSettings::NativeFormat);
    associations.beginGroup("FileAssociations");
    QStringList extensions;
    for (const auto &key : associations.childKeys())
        if (key.startsWith('.') && associations.value(key).toString().startsWith("VSPlayer."))
            extensions << key.mid(1);
    associations.endGroup();
    const auto iconPath=QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+",-101";
    if (!extensions.isEmpty() && associations.value("ApplicationIcon").toString()!=iconPath)
        vsr::registerPlayerAssociations(extensions, QCoreApplication::applicationFilePath());
    QFont font("Segoe UI"); font.setPixelSize(13); app.setFont(font);
    vsr::PlayerWindow window; window.show();
    const auto arguments = app.arguments();
    for (int i = 1; i < arguments.size(); ++i)
        if (arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.loadPreset(arguments[i]);
        else window.openFile(arguments[i]);
    return app.exec();
}
