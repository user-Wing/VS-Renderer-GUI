#include "app/MainWindow.h"
#include "app/Style.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QSettings>
#include <QCommandLineParser>
#include <QTimer>

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
    app.setStyleSheet(vsr::applicationStyleSheet(QSettings().value("appearance/dark",false).toBool()));

    QCommandLineParser parser;parser.addHelpOption();
    parser.addOption({"source","Open a local video in a new workspace.","path"});
    parser.addOption({"loop-in","A boundary in 100 ns units.","ticks"});
    parser.addOption({"loop-out","Exclusive B boundary in 100 ns units.","ticks"});
    parser.process(app);
    qint64 a=-1,b=-1;
    if(parser.isSet("loop-in") || parser.isSet("loop-out")){
        bool validA=false,validB=false;a=parser.value("loop-in").toLongLong(&validA);b=parser.value("loop-out").toLongLong(&validB);
        if(!parser.isSet("source") || !validA || !validB || a<0 || b<=a)parser.showHelp(2);
    }
    vsr::MainWindow window;
    window.show();
    if(parser.isSet("source"))QTimer::singleShot(0,&window,[&window,path=parser.value("source"),a,b]{window.openPlayerSource(path,a,b);});
    return app.exec();
}
