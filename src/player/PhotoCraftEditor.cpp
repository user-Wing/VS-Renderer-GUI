#include "player/PhotoCraftEditor.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QTimer>
#include <QUuid>

namespace vsr {
PhotoCraftEditor *PhotoCraftEditor::instance() {
    static auto *editor=new PhotoCraftEditor(QCoreApplication::instance());
    return editor;
}
QString PhotoCraftEditor::executablePath() {
    return QDir(QCoreApplication::applicationDirPath()).filePath("runtime/photocraft/photocraft.exe");
}
PhotoCraftEditor::PhotoCraftEditor(QObject *parent) : QObject(parent) {
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    startupTimer_.setSingleShot(true);startupTimer_.setInterval(30000);
    connect(&startupTimer_,&QTimer::timeout,this,[this]{starting_=false;pending_.clear();emit errorOccurred(tr("PhotoCraft 启动超时，请检查图形驱动后重试。"));});
    connect(&server_,&QLocalServer::newConnection,this,[this]{
        auto *connection=server_.nextPendingConnection();
        if(socket_){connection->disconnectFromServer();connection->deleteLater();return;}
        socket_=connection;starting_=false;startupTimer_.stop();
        connect(connection,&QLocalSocket::disconnected,this,[this,connection]{if(socket_==connection)socket_=nullptr;connection->deleteLater();});
        flush();
    });
}
void PhotoCraftEditor::flush() {
    if(!socket_ || socket_->state()!=QLocalSocket::ConnectedState)return;
    for(const auto &path:pending_)socket_->write(QJsonDocument(QJsonObject{{"open",path}}).toJson(QJsonDocument::Compact)+'\n');
    pending_.clear();socket_->flush();
}
void PhotoCraftEditor::open(const QString &path) {
    pending_.append(QFileInfo(path).absoluteFilePath());
    if(socket_ && socket_->state()==QLocalSocket::ConnectedState){flush();return;}
    if(starting_)return;
    const auto program=executablePath();
    if(!QFileInfo::exists(program)){pending_.clear();emit errorOccurred(tr("PhotoCraft 图像编辑器未安装：%1").arg(program));return;}
    if(!server_.isListening() && !server_.listen("vsp-photocraft-"+QUuid::createUuid().toString(QUuid::WithoutBraces))){pending_.clear();emit errorOccurred(server_.errorString());return;}
    QProcess process;
    process.setProgram(program);process.setArguments({"--vsp-pipe",server_.fullServerName()});
    process.setWorkingDirectory(QFileInfo(program).absolutePath());
    auto environment=QProcessEnvironment::systemEnvironment();
    environment.insert("PHOTOCRAFT_CONFIG_DIR",QFileInfo(program).absolutePath()+"/PhotoCraftData");
    environment.insert("VSP_IMAGE_IMPORTER",QDir(QCoreApplication::applicationDirPath()).filePath("vs-player.exe"));
    const QSettings settings(QDir(QCoreApplication::applicationDirPath()).filePath("player.ini"),QSettings::IniFormat);
    const auto locale=settings.value("basic/language","zh_CN").toString();
    environment.insert("LC_ALL",locale);environment.insert("PHOTOCRAFT_LOCALE",locale);
    process.setProcessEnvironment(environment);
    if(!process.startDetached()){pending_.clear();emit errorOccurred(tr("无法启动 PhotoCraft 图像编辑器。"));return;}
    starting_=true;
    startupTimer_.start();
}
}
