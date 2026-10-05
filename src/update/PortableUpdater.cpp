#include "update/PortableUpdater.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
#include <QVersionNumber>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace vsr {
namespace {
QString tool(const QString &name) { return QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/"+name); }
bool validVersion(const QString &v) { return QRegularExpression("^\\d+(?:\\.\\d+){1,3}$").match(v).hasMatch(); }
}
PortableUpdater::PortableUpdater(QWidget *parent) : QDialog(parent), network_(new QNetworkAccessManager(this)), process_(new QProcess(this)) {
    setObjectName("portableUpdater");setWindowTitle(tr("软件更新"));resize(570,220);
    auto *layout=new QVBoxLayout(this);status_=new QLabel(tr("当前版本：%1").arg(VSR_VERSION),this);status_->setWordWrap(true);layout->addWidget(status_);
    progress_=new QProgressBar(this);progress_->hide();layout->addWidget(progress_);
    auto *note=new QLabel(tr("完整更新 Renderer 和 Player；保留配置与自定义 VPY，旧目录作为备份。安装前请关闭另一个程序窗口。"),this);note->setWordWrap(true);layout->addWidget(note);
    action_=new QPushButton(tr("检查更新"),this);action_->setObjectName("updateAction");layout->addWidget(action_);
    connect(action_,&QPushButton::clicked,this,[this]{if(!payload_.isEmpty())install();else if(!selected_.version.isEmpty())prepare(selected_);else check();});
    auto *close=new QPushButton(tr("关闭"),this);layout->addWidget(close);connect(close,&QPushButton::clicked,this,&PortableUpdater::reject);
#ifdef Q_OS_WIN
    process_->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
PortableUpdater::~PortableUpdater() {
    cancelled_=true;if(reply_)reply_->abort();
    if(process_->state()!=QProcess::NotRunning){process_->kill();process_->waitForFinished(3000);}
}
void PortableUpdater::reject(){cancelled_=true;if(reply_)reply_->abort();if(process_->state()!=QProcess::NotRunning){process_->kill();process_->waitForFinished(3000);}QDialog::reject();}
bool PortableUpdater::newer(const QString &version,const QString &current) {
    return validVersion(version) && validVersion(current) && QVersionNumber::compare(QVersionNumber::fromString(version).normalized(),QVersionNumber::fromString(current).normalized())>0;
}
QList<PortableRelease> PortableUpdater::releases(const QJsonArray &files) {
    QList<PortableRelease> result;
    const QRegularExpression pattern("^VS-GUI/(\\d+(?:\\.\\d+){1,3})\\.7z$",QRegularExpression::CaseInsensitiveOption);
    for(const auto &value:files){const auto file=value.toObject();const auto path=file.value("Path").toString();const auto match=pattern.match(path);if(!match.hasMatch() || file.value("Type").toString()!="blob")continue;
        QUrl url("https://modelscope.cn/datasets/ARXChem/Software-List/resolve/"+file.value("Revision").toString("master")+"/"+path);
        result.append({match.captured(1),file.value("Sha256").toString().toLower(),url,static_cast<qint64>(file.value("Size").toDouble())});}
    return result;
}
void PortableUpdater::check() {
    if(busy_)return;busy_=true;cancelled_=false;selected_={};candidates_.clear();payload_.clear();action_->setEnabled(false);progress_->setRange(0,0);progress_->show();status_->setText(tr("正在检查 ModelScope 最新完整包…"));fetchPage(1);
}
void PortableUpdater::fetchPage(int page) {
    QUrl url(QString("https://modelscope.cn/api/v1/datasets/ARXChem/Software-List/repo/tree?Revision=master&Root=VS-GUI&Recursive=True&PageNumber=%1&PageSize=100").arg(page));
    QNetworkRequest request(url);request.setTransferTimeout(20000);reply_=network_->get(request);
    connect(reply_,&QNetworkReply::finished,this,[this,page]{auto *reply=reply_;reply_=nullptr;reply->deleteLater();if(cancelled_)return;
        if(reply->error()!=QNetworkReply::NoError){fail(reply->errorString());return;}
        QJsonParseError error;const auto body=QJsonDocument::fromJson(reply->readAll(),&error).object();
        if(error.error!=QJsonParseError::NoError || body.value("Code").toInt()!=200 || !body.value("Data").isObject() || !body.value("Data").toObject().value("Files").isArray()){fail(tr("更新目录响应无效。"));return;}
        const auto files=body.value("Data").toObject().value("Files").toArray();candidates_.append(releases(files));
        if(files.size()==100 && page<1000)fetchPage(page+1);else finishCheck();});
}
void PortableUpdater::finishCheck() {
    busy_=false;progress_->hide();action_->setEnabled(true);
    for(const auto &release:candidates_)if(newer(release.version,VSR_VERSION) && (selected_.version.isEmpty() || newer(release.version,selected_.version)))selected_=release;
    if(selected_.version.isEmpty()){status_->setText(tr("当前版本 %1 已是最新版本；不会降级安装旧包。").arg(VSR_VERSION));action_->setText(tr("检查更新"));return;}
    status_->setText(tr("发现版本 %1 · 完整包 %2 MiB").arg(selected_.version).arg(selected_.size/1048576.0,0,'f',1));action_->setText(tr("下载并验证完整更新"));
}
void PortableUpdater::fail(const QString &message) {
    busy_=false;archive_.reset();hash_.reset();progress_->hide();action_->setEnabled(true);status_->setText(tr("更新失败：%1").arg(message));emit failed(message);
}
void PortableUpdater::prepare(const PortableRelease &release) {
    if(busy_)return;
    if(!QRegularExpression("^[a-fA-F0-9]{64}$").match(release.sha256).hasMatch() || release.size<=0 || (release.url.scheme()!="https" && release.url.scheme()!="http")){fail(tr("更新包缺少有效大小或 SHA-256，停止安装。"));return;}
    for(const auto &name:QStringList{"aria2-next.exe","7z.exe","7z.dll","apply-update.ps1"})if(!QFileInfo::exists(tool(name))){fail(tr("缺少更新工具：%1").arg(name));return;}
    selected_=release;payload_.clear();cancelled_=false;work_=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/vs-gui-update-XXXXXX");if(!work_->isValid()){fail(tr("无法创建更新暂存目录。"));return;}
    busy_=true;action_->setEnabled(false);progress_->setRange(0,100);progress_->setValue(0);progress_->show();status_->setText(tr("正在下载版本 %1…").arg(release.version));
    process_->disconnect(this);
    connect(process_,&QProcess::readyReadStandardOutput,this,[this]{const auto text=QString::fromLocal8Bit(process_->readAllStandardOutput());const auto match=QRegularExpression("\\((\\d+)%\\)").match(text);if(match.hasMatch())progress_->setValue(match.captured(1).toInt());});
    connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart && !cancelled_)fail(process_->errorString());});
    connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exit){if(cancelled_)return;if(code || exit!=QProcess::NormalExit){fail(tr("完整包下载失败。"));return;}
        archive_=std::make_unique<QFile>(work_->filePath("update.7z"));if(!archive_->open(QIODevice::ReadOnly) || archive_->size()!=selected_.size){fail(tr("下载大小不匹配。"));return;}
        hash_=std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);status_->setText(tr("正在验证 SHA-256…"));progress_->setValue(0);verifyChunk();});
    process_->setWorkingDirectory(work_->path());
    process_->start(tool("aria2-next.exe"),{"--no-conf=true","--check-certificate=true","--max-connection-per-server=8","--split=8","--min-split-size=8M","--max-tries=3","--retry-wait=2","--connect-timeout=15","--timeout=30","--summary-interval=1","--console-log-level=warn","--enable-color=false","--file-allocation=none","--auto-file-renaming=false","--dir="+work_->path(),"--out=update.7z",release.url.toString(QUrl::FullyEncoded)});
}
void PortableUpdater::verifyChunk() {
    if(cancelled_ || !archive_)return;const auto bytes=archive_->read(8*1024*1024);
    if(bytes.isEmpty() && !archive_->atEnd()){fail(archive_->errorString());return;}hash_->addData(bytes);progress_->setValue(static_cast<int>(archive_->pos()*100/selected_.size));
    if(!archive_->atEnd()){QTimer::singleShot(0,this,&PortableUpdater::verifyChunk);return;}
    archive_.reset();const auto digest=QString::fromLatin1(hash_->result().toHex());hash_.reset();if(digest.compare(selected_.sha256,Qt::CaseInsensitive)!=0){fail(tr("SHA-256 不匹配，更新包未安装。"));return;}listArchive();
}
void PortableUpdater::prepareLocal(const PortableRelease &release,const QString &path) {
    if(busy_)return;
    if(!newer(release.version,VSR_VERSION)){fail(tr("本体只允许安装更高版本；下载缓存保留，不会降级。"));return;}
    if(!QRegularExpression("^[a-fA-F0-9]{64}$").match(release.sha256).hasMatch() || release.size<=0 || QFileInfo(path).size()!=release.size){fail(tr("缓存更新包大小或 SHA-256 无效"));return;}
    selected_=release;payload_.clear();cancelled_=false;work_=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/vs-gui-update-XXXXXX");if(!work_->isValid() || !QFile::copy(path,work_->filePath("update.7z"))){fail(tr("无法准备本地更新包"));return;}
    archive_=std::make_unique<QFile>(work_->filePath("update.7z"));if(!archive_->open(QIODevice::ReadOnly)){fail(archive_->errorString());return;}busy_=true;action_->setEnabled(false);progress_->setRange(0,100);progress_->show();hash_=std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);verifyChunk();
}
bool PortableUpdater::safeArchiveListing(const QString &listing) {
    const int separator=listing.indexOf("----------");if(separator<0)return false;
    QSet<QString> paths;bool found=false;
    for(const auto &line:listing.mid(separator).split('\n')) {
        if(line.startsWith("Symbolic Link =") || line.startsWith("Hard Link =") || line.startsWith("Alternate Stream ="))return false;
        if(!line.startsWith("Path = "))continue;auto path=line.mid(7);if(path.endsWith('\r'))path.chop(1);path.replace('\\','/');const auto parts=path.split('/');
        if(path.isEmpty() || path.startsWith('/') || path.contains(':') || parts.contains("..") || parts.contains(".") || parts.contains("") || paths.contains(path.toLower()))return false;
        for(const auto &part:parts){if(part.endsWith('.') || part.endsWith(' ') || QRegularExpression("^(con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\\.|$)",QRegularExpression::CaseInsensitiveOption).match(part).hasMatch())return false;}
        paths.insert(path.toLower());found=true;
    }
    return found;
}
void PortableUpdater::listArchive() {
    status_->setText(tr("正在检查更新包路径…"));progress_->setRange(0,0);process_->disconnect(this);
    connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart && !cancelled_)fail(process_->errorString());});
    connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){if(cancelled_)return;const auto listing=QString::fromUtf8(process_->readAllStandardOutput());if(code || !safeArchiveListing(listing)){fail(tr("更新包路径不安全或格式无效。"));return;}extractArchive();});
    process_->start(tool("7z.exe"),{"l","-slt","-sccUTF-8",work_->filePath("update.7z")});
}
void PortableUpdater::extractArchive() {
    status_->setText(tr("正在解压完整更新包…"));process_->disconnect(this);
    connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart && !cancelled_)fail(process_->errorString());});
    connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){if(cancelled_)return;if(code){fail(tr("更新包解压失败。"));return;}verifyPayload();});
    process_->start(tool("7z.exe"),{"x",work_->filePath("update.7z"),"-o"+work_->filePath("payload"),"-y","-bb0"});
}
QString PortableUpdater::payloadDirectory(const QString &directory) {
    const QDir folder(directory);QStringList candidates{folder.absolutePath()},valid;
    for(const auto &child:folder.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden))candidates.append(folder.filePath(child));
    for(const auto &candidate:candidates) {
        const QDir payload(candidate);bool complete=true;
        for(const auto &name:QStringList{"VSRenderer.exe","vs-player.exe","FFF.Native.dll","Qt6Core.dll","runtime/python/python.exe","runtime/tools/aria2-next.exe","runtime/tools/7z.exe","runtime/tools/7z.dll","runtime/tools/apply-update.ps1","languages/en_US.json","languages/zh_CN.json","release.json"})if(!QFileInfo(payload.filePath(name)).isFile()){complete=false;break;}
        if(complete)valid.append(payload.absolutePath());
    }
    return valid.size()==1?valid.first():QString();
}
void PortableUpdater::verifyPayload() {
    const auto directory=payloadDirectory(work_->filePath("payload"));
    if(directory.isEmpty()){fail(tr("更新包顶层或下一层必须包含唯一完整便携目录。"));return;}
    const QDir payload(directory);
    QFile manifest(payload.filePath("release.json"));if(!manifest.open(QIODevice::ReadOnly) || QJsonDocument::fromJson(manifest.readAll()).object().value("version").toString()!=selected_.version){fail(tr("更新包内部版本不匹配。"));return;}
    payload_=payload.path();busy_=false;progress_->hide();status_->setText(tr("版本 %1 已验证。安装将关闭本程序，完成后重新打开；旧目录保留备份。").arg(selected_.version));action_->setEnabled(true);action_->setText(tr("安装并重启"));emit prepared(payload_);
}
void PortableUpdater::install() {
    if(!newer(selected_.version,VSR_VERSION)){fail(tr("不会降级安装旧本体，下载缓存保留。"));return;}
    if(payload_.isEmpty() || busy_)return;const auto helper=work_->filePath("apply-update.ps1");if(!QFile::copy(tool("apply-update.ps1"),helper)){fail(tr("无法准备安装脚本。"));return;}
    QProcess installer;
#ifdef Q_OS_WIN
    installer.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    installer.setProgram("powershell.exe");installer.setArguments({"-NoProfile","-ExecutionPolicy","Bypass","-WindowStyle","Hidden","-File",helper,"-ParentPid",QString::number(QCoreApplication::applicationPid()),"-Source",payload_,"-Target",QCoreApplication::applicationDirPath(),"-Launch",QFileInfo(QCoreApplication::applicationFilePath()).fileName()});
    installer.setWorkingDirectory(work_->path());if(!installer.startDetached()){fail(tr("无法启动更新安装。"));return;}work_->setAutoRemove(false);qApp->closeAllWindows();qApp->quit();
}
}
