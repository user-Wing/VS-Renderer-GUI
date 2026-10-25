#include "update/SinglePlayerUpdater.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVersionNumber>
#include <memory>

namespace vsr {
SinglePlayerUpdater::SinglePlayerUpdater(QWidget *parent):QObject(parent),window_(parent){}
void SinglePlayerUpdater::check(bool silent){
    const auto target=qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH");
    if(!QFileInfo(target).isAbsolute() || !QFileInfo(target).isFile())return;
    auto *network=new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://api.github.com/repos/user-Wing/VS-Renderer-GUI/releases/latest"));
    request.setRawHeader("User-Agent","VS-Player-Updater");request.setTransferTimeout(15000);
    auto *reply=network->get(request);
    connect(reply,&QNetworkReply::finished,this,[=,this]{
        reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError){if(!silent)QMessageBox::warning(window_,"VS Player",reply->errorString());return;}
        const auto release=QJsonDocument::fromJson(reply->readAll()).object();auto version=release.value("tag_name").toString();if(version.startsWith('v',Qt::CaseInsensitive))version.remove(0,1);
        if(QVersionNumber::compare(QVersionNumber::fromString(version),QVersionNumber::fromString(VSR_VERSION))<=0){if(!silent)QMessageBox::information(window_,"VS Player",tr("当前已是最新版本。"));return;}
        QJsonObject asset;
        for(const auto &item:release.value("assets").toArray())if(QRegularExpression("^VS-Player(?:[-_.]v?[0-9.]+)?\\.exe$",QRegularExpression::CaseInsensitiveOption).match(item.toObject().value("name").toString()).hasMatch()){asset=item.toObject();break;}
        const auto digest=asset.value("digest").toString();const qint64 size=asset.value("size").toInteger();const QUrl url(asset.value("browser_download_url").toString());
        if(!QRegularExpression("^sha256:[a-fA-F0-9]{64}$").match(digest).hasMatch() || size<1024*1024 || size>300*1024*1024 || url.scheme()!="https" || url.host()!="github.com"){
            if(!silent)QMessageBox::warning(window_,"VS Player",tr("此版本没有可验证的单文件播放器附件，无法自动更新。"));return;
        }
        if(QMessageBox::question(window_,"VS Player",tr("发现 %1。下载并仅替换当前单文件播放器，然后重启？").arg(version))!=QMessageBox::Yes)return;
        auto work=std::make_shared<QTemporaryDir>(QDir::tempPath()+"/vs-single-update-XXXXXX");if(!work->isValid())return;
        auto file=std::make_shared<QFile>(work->filePath("VS-Player.exe"));if(!file->open(QIODevice::WriteOnly))return;
        QNetworkRequest downloadRequest(url);downloadRequest.setTransferTimeout(30000);auto *download=network->get(downloadRequest);
        auto hash=std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);auto bytes=std::make_shared<qint64>(0);
        connect(download,&QNetworkReply::readyRead,this,[=]{const auto data=download->readAll();*bytes+=data.size();if(*bytes>size || file->write(data)!=data.size()){download->abort();return;}hash->addData(data);});
        connect(download,&QNetworkReply::finished,this,[=,this]{
            download->deleteLater();file->close();
            if(download->error()!=QNetworkReply::NoError || *bytes!=size || hash->result().toHex()!=digest.mid(7).toLatin1().toLower()){
                QMessageBox::warning(window_,"VS Player",tr("单文件下载或 SHA-256 校验失败，当前程序未修改。"));return;
            }
            QFile executable(file->fileName());if(!executable.open(QIODevice::ReadOnly) || executable.read(2)!="MZ"){QMessageBox::warning(window_,"VS Player",tr("下载文件不是 Windows 程序。"));return;}executable.close();
            QFile script(work->filePath("replace.ps1"));if(!script.open(QIODevice::WriteOnly))return;
            script.write(R"PS(param([string]$Target,[string]$Payload,[int]$PlayerProcess,[string]$Work)
$ErrorActionPreference='Stop'
Wait-Process -Id $PlayerProcess -Timeout 30 -ErrorAction SilentlyContinue
$suffix=[Guid]::NewGuid().ToString('N')
$backup=$Target+'.backup-'+$suffix
$incoming=$Target+'.incoming-'+$suffix
try {
    Copy-Item -LiteralPath $Payload -Destination $incoming
    Move-Item -LiteralPath $Target -Destination $backup
    try { Move-Item -LiteralPath $incoming -Destination $Target }
    catch { Move-Item -LiteralPath $backup -Destination $Target; throw }
    Start-Process -FilePath $Target -WorkingDirectory (Split-Path -LiteralPath $Target) -WindowStyle Hidden
    Remove-Item -LiteralPath $backup
    Remove-Item -LiteralPath $Payload
} catch { $_ | Out-File -LiteralPath (Join-Path $Work 'update-error.txt'); Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'VS Player 更新失败') }
)PS");script.close();
            const auto powershell=QStandardPaths::findExecutable("powershell.exe");
            if(QProcess::startDetached(powershell,{"-NoProfile","-NonInteractive","-WindowStyle","Hidden","-ExecutionPolicy","Bypass","-File",script.fileName(),target,file->fileName(),QString::number(QCoreApplication::applicationPid()),work->path()})){work->setAutoRemove(false);qApp->quit();}
            else QMessageBox::warning(window_,"VS Player",tr("无法启动单文件替换程序，当前程序未修改。"));
        });
    });
}
}
