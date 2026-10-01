#include "player/PlayerWindow.h"
#include <QApplication>
#include <QFont>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("VS Player"); app.setOrganizationName("VSRenderer"); app.setApplicationVersion(VSR_VERSION);
    QFont font("Segoe UI"); font.setPixelSize(13); app.setFont(font);
    vsr::PlayerWindow window; window.show();
    const auto arguments = app.arguments();
    for (int i = 1; i < arguments.size(); ++i)
        if (arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.loadPreset(arguments[i]);
        else window.openFile(arguments[i]);
    return app.exec();
}
