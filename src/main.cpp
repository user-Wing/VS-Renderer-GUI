#include "runtime/PortableQtPlugins.h"
#include "runtime/PortableLayout.h"
#include "app/MainWindow.h"
#include "app/Style.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QSettings>
#include <QCommandLineParser>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QTimer>

namespace {
constexpr auto exportPipeName = "VSRenderer.ExportCommand";
constexpr qint64 maxExportPacket = 65536;

void handleExportConnections(QLocalServer &server, vsr::MainWindow &window)
{
    while (auto *socket = server.nextPendingConnection()) {
        socket->setReadBufferSize(maxExportPacket);
        QTimer::singleShot(3000, socket, &QLocalSocket::abort);
        QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        const auto receive = [socket, &window] {
            if (!socket->canReadLine()) {
                if (socket->bytesAvailable() >= maxExportPacket) socket->abort();
                return;
            }
            const auto packet = socket->readLine();
            const auto command = QJsonDocument::fromJson(packet).object().value("command").toString();
            if (command.trimmed().isEmpty()) {
                socket->write("ERROR: invalid export command\n");
            } else {
                window.receiveExportCommand(command);
                socket->write("OK\n");
            }
            socket->disconnectFromServer();
        };
        QObject::connect(socket, &QLocalSocket::readyRead, socket, receive);
        receive();
    }
}
}

int main(int argc, char *argv[])
{
    vsr::registerPortableQtPlugins();
    QApplication app(argc, argv);
    vsr::migratePortableLayout();
    QApplication::setApplicationName(QStringLiteral("VS Renderer"));
    QApplication::setOrganizationName(QStringLiteral("VSRenderer"));
    QApplication::setApplicationVersion(QStringLiteral(VSR_VERSION));
    if (!QFileInfo::exists(vsr::configFile("renderer.ini"))) {
        QSettings legacy;
        auto target = vsr::rendererSettings();
        for (const auto &key : legacy.allKeys()) target.setValue(key, legacy.value(key));
    }
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/renderer.ico")));

    QFont font(QStringLiteral("Comic Sans MS"));
    font.setPixelSize(13);
    QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{"Microsoft YaHei UI"});
    app.setFont(font);
    app.setStyleSheet(vsr::applicationStyleSheet(vsr::rendererSettings().value("appearance/dark",false).toBool()));

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
    const QDir profile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    profile.mkpath(".");
    QLockFile exportPipeLock(profile.filePath("export-command.lock"));
    exportPipeLock.setStaleLockTime(0);
    QLocalServer exportServer;
    exportServer.setSocketOptions(QLocalServer::UserAccessOption);
    QObject::connect(&exportServer, &QLocalServer::newConnection, &window,
                     [&] { handleExportConnections(exportServer, window); });
    // Windows can route the same pipe name to multiple listeners.
    QTimer exportPipeRetry;
    exportPipeRetry.setInterval(1000);
    const auto listenForExport = [&] {
        if (!exportPipeLock.tryLock()) return;
        if (exportServer.listen(exportPipeName)) exportPipeRetry.stop();
        else exportPipeLock.unlock();
    };
    QObject::connect(&exportPipeRetry, &QTimer::timeout, &window, listenForExport);
    exportPipeRetry.start();
    listenForExport();
    if(parser.isSet("source"))QTimer::singleShot(0,&window,[&window,path=parser.value("source"),a,b]{window.openPlayerSource(path,a,b);});
    return app.exec();
}
