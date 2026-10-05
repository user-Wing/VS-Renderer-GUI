#include "app/MainWindow.h"
#include "app/Style.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("VS Renderer"));
    QApplication::setOrganizationName(QStringLiteral("VSRenderer"));
    QApplication::setApplicationVersion(QStringLiteral(VSR_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/renderer.ico")));

    QFont font(QStringLiteral("Comic Sans MS"));
    font.setPixelSize(13);
    QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{"Microsoft YaHei UI"});
    app.setFont(font);
    app.setStyleSheet(vsr::applicationStyleSheet());

    vsr::MainWindow window;
    window.show();
    return app.exec();
}
