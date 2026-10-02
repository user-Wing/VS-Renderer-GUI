#include "app/MainWindow.h"
#include "app/Style.h"

#include <QApplication>
#include <QFont>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("VS Renderer"));
    QApplication::setOrganizationName(QStringLiteral("VSRenderer"));
    QApplication::setApplicationVersion(QStringLiteral(VSR_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/renderer.ico")));

    QFont font(QStringLiteral("Segoe UI Variable"));
    font.setPixelSize(13);
    app.setFont(font);
    app.setStyleSheet(vsr::applicationStyleSheet());

    vsr::MainWindow window;
    window.show();
    return app.exec();
}
