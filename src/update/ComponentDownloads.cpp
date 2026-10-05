#include "update/ComponentDownloads.h"
#include "update/PortableUpdater.h"
#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QVersionNumber>
#include <QtConcurrentRun>
#include <QtEndian>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace vsr {
namespace {
const QStringList ids{"VS-GUI","ffmpeg","mkvtoolnix","MadVR","LAV-Filters","aria2-next"};
QString title(const QString &id){return id=="VS-GUI"?QStringLiteral("软件本体"):id;}
QString tool(const QString &name){return QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/"+name);}
QJsonObject json(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?QJsonDocument::fromJson(f.readAll()).object():QJsonObject();}
QString digest(const QString &path){QFile f(path);QCryptographicHash hash(QCryptographicHash::Sha256);if(!f.open(QIODevice::ReadOnly))return {};while(!f.atEnd()){const auto bytes=f.read(8*1024*1024);if(bytes.isEmpty())return {};hash.addData(bytes);}return QString::fromLatin1(hash.result().toHex());}
bool x64(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return false;const auto dos=f.read(64);if(dos.size()!=64 || dos.left(2)!="MZ")return false;const auto offset=qFromLittleEndian<quint32>(dos.constData()+60);if(offset>quint64(f.size()-6) || !f.seek(offset))return false;const auto pe=f.read(6);return pe.left(4)==QByteArray("PE\0\0",4) && qFromLittleEndian<quint16>(pe.constData()+4)==0x8664;}
QString fileVersion(const QString &path){
#ifdef Q_OS_WIN
    const auto native=QDir::toNativeSeparators(path);const auto *name=reinterpret_cast<LPCWSTR>(native.utf16());DWORD ignored=0;const DWORD size=GetFileVersionInfoSizeW(name,&ignored);if(!size)return {};QByteArray data(size,'\0');if(!GetFileVersionInfoW(name,0,size,data.data()))return {};VS_FIXEDFILEINFO *version=nullptr;UINT length=0;if(!VerQueryValueW(data.data(),L"\\",reinterpret_cast<LPVOID *>(&version),&length) || length<sizeof(VS_FIXEDFILEINFO))return {};return QString("%1.%2.%3.%4").arg(HIWORD(version->dwFileVersionMS)).arg(LOWORD(version->dwFileVersionMS)).arg(HIWORD(version->dwFileVersionLS)).arg(LOWORD(version->dwFileVersionLS));
#else
    Q_UNUSED(path);return {};
#endif
}
}
ComponentDownloads::ComponentDownloads(QWidget *parent):QWidget(parent),network_(new QNetworkAccessManager(this)){
    setObjectName("componentDownloads");cache_=QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("components");QDir().mkpath(cache_);
    auto *layout=new QVBoxLayout(this);auto *bar=new QHBoxLayout;bar->addWidget(new QLabel(tr("Full 版 · 软件与组件下载"),this));bar->addStretch();automatic_=new QCheckBox(tr("自动下载最新版本"),this);automatic_->setObjectName("componentAutomatic");bar->addWidget(automatic_);auto *refreshButton=new QPushButton(tr("刷新版本"),this);bar->addWidget(refreshButton);auto *clean=new QPushButton(tr("清理本地缓存"),this);bar->addWidget(clean);layout->addLayout(bar);
    table_=new QTreeWidget(this);table_->setObjectName("componentVersions");table_->setHeaderLabels({tr("组件 / 可下载的程序包"),tr("版本"),tr("本地版本"),tr("大小")});table_->header()->setSectionResizeMode(0,QHeaderView::Stretch);table_->setColumnWidth(1,100);table_->setColumnWidth(2,105);table_->setColumnWidth(3,90);layout->addWidget(table_,1);
    for(const auto &id:ids){auto *group=new QTreeWidgetItem(table_);group->setText(0,title(id));group->setData(0,Qt::UserRole,id);groups_[id]=group;}
    auto *actions=new QHBoxLayout;auto *download=new QPushButton(tr("下载所选版本"),this);download->setObjectName("componentDownload");actions->addWidget(download);apply_=new QPushButton(tr("应用组件并重启"),this);apply_->setEnabled(false);actions->addWidget(apply_);softwareButton_=new QPushButton(tr("安装已下载的新版本本体"),this);softwareButton_->setEnabled(false);actions->addWidget(softwareButton_);actions->addStretch();layout->addLayout(actions);
    status_=new QLabel(tr("读取各组件目录中的 Windows 程序包；旧本体只下载，组件可以安装旧版本。"),this);status_->setObjectName("componentStatus");status_->setWordWrap(true);layout->addWidget(status_);
    connect(refreshButton,&QPushButton::clicked,this,&ComponentDownloads::refresh);connect(automatic_,&QCheckBox::toggled,this,[this](bool checked){if(checked)refresh();});
    connect(download,&QPushButton::clicked,this,[this]{if(downloading_)return;const auto *item=table_->currentItem();if(!item || item->parent()==nullptr)return;const auto id=item->parent()->data(0,Qt::UserRole).toString();const int index=item->data(0,Qt::UserRole).toInt();queue_={releases_[id].at(index)};downloadNext();});
    connect(clean,&QPushButton::clicked,this,[this]{if(downloading_){status_->setText(tr("下载/验证期间不能清理缓存。"));return;}int removed=0;for(const auto &name:QDir(cache_).entryList({"*.7z","*.zip","*.exe","*.partial","*.aria2"},QDir::Files))removed+=QFile::remove(QDir(cache_).filePath(name));status_->setText(tr("已清理 %1 个下载缓存文件；已安装组件保留。").arg(removed));software_={};softwareButton_->setEnabled(false);});
    connect(apply_,&QPushButton::clicked,this,&ComponentDownloads::installComponents);connect(softwareButton_,&QPushButton::clicked,this,&ComponentDownloads::installSoftware);
    process_.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
    process_.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(process_.errorString());});refresh();
}
ComponentDownloads::~ComponentDownloads(){if(process_.state()!=QProcess::NotRunning){process_.kill();process_.waitForFinished(3000);}}
QString ComponentDownloads::destination(const QString &id){if(id=="ffmpeg")return ".";if(id=="mkvtoolnix")return "runtime/mkvtoolnix";if(id=="MadVR")return "madVR09217";if(id=="LAV-Filters")return "LAVFilters64";if(id=="aria2-next")return "runtime/tools";return {};}
bool ComponentDownloads::newer(const QString &a,const QString &b){return !a.isEmpty() && (b.isEmpty() || QVersionNumber::compare(QVersionNumber::fromString(a).normalized(),QVersionNumber::fromString(b).normalized())>0);}
QList<ComponentRelease> ComponentDownloads::releases(const QString &id,const QJsonArray &files){
    QList<ComponentRelease> result;for(const auto &value:files){const auto f=value.toObject();const auto path=f.value("Path").toString();if(f.value("Type").toString()!="blob" || !path.startsWith(id+"/",Qt::CaseInsensitive))continue;
        const auto name=QFileInfo(path).fileName();if(!QStringList{"7z","zip","exe"}.contains(QFileInfo(name).suffix().toLower()) || QRegularExpression("source|arm64|aarch64|win32|x86(?![_-]64)|updater|setup|installer",QRegularExpression::CaseInsensitiveOption).match(name).hasMatch())continue;
        if(id!="VS-GUI" && !name.contains(id=="LAV-Filters"?"LAV":id,Qt::CaseInsensitive))continue;
        QString version;auto matches=QRegularExpression("(?<![a-zA-Z0-9])(\\d+(?:\\.\\d+){1,3})").globalMatch(QFileInfo(path).path()+"/"+QFileInfo(path).completeBaseName());while(matches.hasNext())version=matches.next().captured(1);
        if(id=="aria2-next"){const auto m=QRegularExpression("/v?(\\d+(?:\\.\\d+){1,3})/").match(path);if(m.hasMatch())version=m.captured(1);}
        if(id=="ffmpeg"){const auto m=QRegularExpression("20\\d{6}").match(name);if(m.hasMatch())version=m.captured(0);}
        if(id=="MadVR" && version.isEmpty()){const auto m=QRegularExpression("madVR(0)([0-9]{2})([0-9]{2})",QRegularExpression::CaseInsensitiveOption).match(name);if(m.hasMatch())version=m.captured(1)+"."+m.captured(2)+"."+m.captured(3);}
        if(version.isEmpty())continue;const auto sha=f.value("Sha256").toString().toLower();const auto size=qint64(f.value("Size").toDouble());if(size<=0 || !QRegularExpression("^[a-f0-9]{64}$").match(sha).hasMatch())continue;
        QUrl url("https://modelscope.cn/datasets/ARXChem/Software-List/resolve/"+f.value("Revision").toString("master")+"/"+path);result<<ComponentRelease{id,version,path,sha,url,size};
    }std::stable_sort(result.begin(),result.end(),[](const auto &a,const auto &b){return newer(a.version,b.version);});return result;
}
QString ComponentDownloads::localVersion(const QString &id)const{
    if(id=="VS-GUI")return VSR_VERSION;const auto app=QCoreApplication::applicationDirPath();const auto installed=json(QDir(app).filePath("components.json")).value(id).toString();
    QString file=id=="mkvtoolnix"?"runtime/mkvtoolnix/mkvmerge.exe":id=="ffmpeg"?"ffmpeg.exe":id=="aria2-next"?"runtime/tools/aria2-next.exe":id=="MadVR"?"madVR09217/madVR64.ax":"LAVFilters64/LAVVideo.ax";
    if(!QFileInfo(QDir(app).filePath(file)).isFile())return {};if(id=="MadVR" || id=="LAV-Filters")return fileVersion(QDir(app).filePath(file));if(id=="ffmpeg" && !installed.isEmpty())return installed;
    QProcess p;p.start(QDir(app).filePath(file),{id=="ffmpeg"?"-version":"--version"});if(!p.waitForFinished(1500))return {};const auto output=QString::fromUtf8(p.readAllStandardOutput());const auto match=QRegularExpression("\\d+(?:\\.\\d+){1,3}").match(output);return match.captured(0);
}
void ComponentDownloads::refresh(){if(pending_ || downloading_)return;releases_.clear();status_->setText(tr("正在读取 ModelScope 组件目录…"));pending_=ids.size();for(const auto &id:ids){auto *group=groups_[id];qDeleteAll(group->takeChildren());group->setText(2,localVersion(id));fetch(id,1);}}
void ComponentDownloads::fetch(const QString &id,int page){
    QUrl url("https://modelscope.cn/api/v1/datasets/ARXChem/Software-List/repo/tree");QUrlQuery query;query.addQueryItem("Revision","master");query.addQueryItem("Root",id);query.addQueryItem("Recursive","True");query.addQueryItem("PageNumber",QString::number(page));query.addQueryItem("PageSize","100");url.setQuery(query);QNetworkRequest request(url);request.setTransferTimeout(20000);auto *reply=network_->get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply,id,page]{const auto doc=QJsonDocument::fromJson(reply->readAll()).object();const auto data=doc.value("Data").toObject();const auto files=data.value("Files").toArray();const bool ok=reply->error()==QNetworkReply::NoError && doc.value("Code").toInt()==200 && data.value("Files").isArray();reply->deleteLater();
        if(ok){releases_[id].append(releases(id,files));if(files.size()==100 && page<1000){fetch(id,page+1);return;}}
        auto &found=releases_[id];std::stable_sort(found.begin(),found.end(),[](const auto &a,const auto &b){return newer(a.version,b.version);});auto *group=groups_[id];
        for(int i=0;i<found.size();++i){auto *row=new QTreeWidgetItem(group);row->setText(0,QFileInfo(found[i].path).fileName());row->setToolTip(0,found[i].path);row->setText(1,found[i].version);row->setText(3,QString::number(found[i].size/1048576.,'f',1)+" MiB");row->setData(0,Qt::UserRole,i);}group->setToolTip(0,ok?(found.isEmpty()?tr("没有可用 Windows 程序包；源码/说明/安装器不列入"):id):tr("目录不可用或请求失败，可稍后刷新"));if(--pending_==0)finishRefresh();
    });
}
void ComponentDownloads::finishRefresh(){status_->setText(tr("版本列表已刷新；展开组件选择版本。缺少程序包的分类可悬停查看原因。"));if(automatic_->isChecked()){for(const auto &id:ids)if(!releases_[id].isEmpty() && newer(releases_[id].first().version,localVersion(id)))queue_<<releases_[id].first();if(queue_.isEmpty())status_->setText(tr("本地版本不低于可下载的最新版本，无需下载。"));else downloadNext();}}
QString ComponentDownloads::cacheFile(const ComponentRelease &r)const{return QDir(cache_).filePath(r.id+"-"+r.sha256.left(12)+"-"+QFileInfo(r.path).fileName());}
void ComponentDownloads::fail(const QString &message){queue_.clear();downloading_=false;process_.disconnect(this);status_->setText(tr("组件下载失败：%1").arg(message));apply_->setEnabled(!plan_.isEmpty());}
void ComponentDownloads::downloadNext(){
    if(queue_.isEmpty()){downloading_=false;apply_->setEnabled(!plan_.isEmpty());status_->setText(tr("下载验证完成。缓存：%1；已暂存组件需应用并重启。").arg(cache_));return;}downloading_=true;selected_=queue_.takeFirst();archive_=cacheFile(selected_);status_->setText(tr("正在下载 %1 %2…").arg(title(selected_.id),selected_.version));apply_->setEnabled(false);
    if(QFileInfo(archive_).isFile()){verify();return;}process_.disconnect(this);connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(process_.errorString());});
    connect(&process_,&QProcess::finished,this,[this](int code,QProcess::ExitStatus state){if(code || state!=QProcess::NormalExit){fail(QString::fromLocal8Bit(process_.readAllStandardOutput()));return;}verify();});
    process_.start(tool("aria2-next.exe"),{"--no-conf=true","--check-certificate=true","--continue=true","--max-connection-per-server=4","--split=4","--max-tries=3","--retry-wait=2","--file-allocation=none","--auto-file-renaming=false","--allow-overwrite=true","--dir="+cache_,"--out="+QFileInfo(archive_+".partial").fileName(),selected_.url.toString(QUrl::FullyEncoded)});
}
void ComponentDownloads::verify(){
    const auto path=QFileInfo::exists(archive_)?archive_:archive_+".partial";if(QFileInfo(path).size()!=selected_.size){fail(tr("文件大小不匹配：%1").arg(path));return;}status_->setText(tr("正在校验 %1 SHA-256…").arg(title(selected_.id)));auto *watcher=new QFutureWatcher<QString>(this);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,path]{const auto hash=watcher->result();watcher->deleteLater();if(hash!=selected_.sha256){QFile::remove(path);fail(tr("SHA-256 不匹配，包未安装；请重新下载。"));return;}if(path!=archive_ && !QFile::rename(path,archive_)){fail(tr("缓存重命名失败"));return;}if(selected_.id=="VS-GUI"){if(!newer(selected_.version,VSR_VERSION)){QMessageBox::information(this,tr("旧版本本体仅下载"),tr("当前版本 %1，不会降级更新到 %2。包已经下载到：\n%3").arg(VSR_VERSION,selected_.version,archive_));}else {software_=selected_;softwareButton_->setEnabled(true);}finishDownload();}else unpack();});watcher->setFuture(QtConcurrent::run([path]{return digest(path);}));
}
void ComponentDownloads::unpack(){
    QTemporaryDir temp(QDir(cache_).filePath("staging-XXXXXX"));if(!temp.isValid()){fail(tr("不能创建解压目录"));return;}temp.setAutoRemove(false);staging_=temp.path();
    if(QFileInfo(archive_).suffix().compare("exe",Qt::CaseInsensitive)==0){if(!QFile::copy(archive_,QDir(staging_).filePath(selected_.id=="aria2-next"?"aria2-next.exe":QFileInfo(selected_.path).fileName()))){fail(tr("暂存 EXE 失败"));return;}extracted();return;}
    process_.disconnect(this);connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(process_.errorString());});
    connect(&process_,&QProcess::finished,this,[this](int code,QProcess::ExitStatus state){const auto listing=QString::fromUtf8(process_.readAllStandardOutput());if(code || state!=QProcess::NormalExit || !PortableUpdater::safeArchiveListing(listing)){fail(tr("压缩包路径不安全或读取失败"));return;}
        process_.disconnect(this);connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(process_.errorString());});connect(&process_,&QProcess::finished,this,[this](int result,QProcess::ExitStatus exit){if(result || exit!=QProcess::NormalExit){fail(tr("解压失败"));return;}extracted();});process_.start(tool("7z.exe"),{"x","-y","-o"+staging_,archive_});});process_.start(tool("7z.exe"),{"l","-slt","-sccUTF-8",archive_});
}
QString ComponentDownloads::payloadDirectory(const QString &id,const QString &root){
    const QString primary=id=="ffmpeg"?"ffmpeg.exe":id=="mkvtoolnix"?"mkvmerge.exe":id=="MadVR"?"madVR64.ax":id=="LAV-Filters"?"LAVVideo.ax":"aria2-next.exe";QStringList found;QDirIterator it(root,{primary},QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
    while(it.hasNext()){const auto file=it.next();if(!x64(file))continue;const auto folder=QFileInfo(file).absolutePath();bool complete=true;const auto needed=id=="LAV-Filters"?QStringList{"LAVAudio.ax","LAVSplitter.ax"}:id=="MadVR"?QStringList{"madHcCtrl.exe"}:QStringList{};for(const auto &name:needed)complete&=QFileInfo(QDir(folder).filePath(name)).isFile();if(complete)found<<folder;}return found.size()==1?found.first():QString();
}
void ComponentDownloads::extracted(){const auto payload=payloadDirectory(selected_.id,staging_);if(payload.isEmpty()){fail(tr("没有找到唯一完整的 64 位组件目录/主程序；包未安装：%1").arg(archive_));return;}for(int i=plan_.size()-1;i>=0;--i)if(plan_[i].toObject().value("id").toString()==selected_.id)plan_.removeAt(i);plan_<<QJsonObject{{"id",selected_.id},{"version",selected_.version},{"source",payload}};emit componentPrepared(selected_.id,selected_.version,payload);finishDownload();}
void ComponentDownloads::finishDownload(){downloadNext();}
void ComponentDownloads::installSoftware(){if(software_.id.isEmpty() || !newer(software_.version,VSR_VERSION) || downloading_)return;PortableUpdater updater(this);updater.prepareLocal({software_.version,software_.sha256,software_.url,software_.size},cacheFile(software_));updater.exec();}
void ComponentDownloads::installComponents(){
    if(plan_.isEmpty() || downloading_)return;if(QMessageBox::question(this,tr("应用组件"),tr("关闭另一程序窗口，然后退出本程序应用 %1 个组件并重启？配置和 VPY 保留。").arg(plan_.size()))!=QMessageBox::Yes)return;
    const auto planPath=QDir(cache_).filePath("install-plan.json");QSaveFile plan(planPath);if(!plan.open(QIODevice::WriteOnly) || plan.write(QJsonDocument(plan_).toJson())<0 || !plan.commit()){fail(tr("无法保存组件安装计划"));return;}
    QProcess helper;
#ifdef Q_OS_WIN
    helper.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    helper.setProgram("powershell.exe");helper.setArguments({"-NoProfile","-ExecutionPolicy","Bypass","-WindowStyle","Hidden","-File",tool("apply-components.ps1"),"-ParentPid",QString::number(QCoreApplication::applicationPid()),"-Plan",planPath,"-Target",QCoreApplication::applicationDirPath(),"-Launch",QFileInfo(QCoreApplication::applicationFilePath()).fileName()});if(!helper.startDetached()){fail(tr("无法启动组件安装"));return;}qApp->closeAllWindows();qApp->quit();
}
}
