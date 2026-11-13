// Included once by MainWindow.cpp: project persistence is independent of playback.
#include "app/RenderRoute.h"
#include "graph/PresetStore.h"
#include "ui/CompareView.h"
#include <QAction>
#include <QCryptographicHash>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMessageBox>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace vsr {
QJsonObject MainWindow::projectSnapshot() const {
    QJsonObject root{{"schemaVersion",3},{"sourcePath",sourcePath_->text()},{"sourceFilter",sourceFilter_->currentData().toInt()},{"editingRoute",routeIndex_},{"editingTimeGroup",timeGroupIndex_}};
    QJsonArray routes;for(int i=0;i<routes_.size();++i){const auto &r=routes_[i];auto row=PresetStore::metadata(i==routeIndex_?currentTimeGroups():r.groups,static_cast<SourceFilter>(sourceFilter_->currentData().toInt()),{},{});row["visible"]=r.visible;row["preset"]=i==routeIndex_?activePreset_:r.preset;const auto preset=row["preset"].toString();if(!preset.isEmpty()){QFile file(preset);if(file.open(QIODevice::ReadOnly)&&file.size()<=8*1024*1024)row["presetScript"]=QString::fromUtf8(file.readAll());}routes.append(row);}root["routes"]=routes;return root;
}
bool MainWindow::applyProjectSnapshot(const QJsonObject &root){
    const auto error=PresetStore::compatibilityError(root);if(!error.isEmpty()){QMessageBox::warning(this,QStringLiteral("请更新 VS Renderer"),error);return false;}
    auto entries=root.value("routes").toArray();if(entries.isEmpty())entries.append(root);if(entries.size()>3)return false;
    QList<RouteState> prepared;
    for(int i=0;i<entries.size();++i){const auto e=entries[i].toObject();RouteState r;r.groups=PresetStore::timeGroups(e);r.preset=e.value("preset").toString();
        if(e.contains("presetScript")){const auto bytes=e.value("presetScript").toString().toUtf8();const auto folder=QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("VSRenderer/project-scripts");QDir().mkpath(folder);const auto filePath=QDir(folder).filePath(QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())+".vpy");QSaveFile file(filePath);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){setStatus(QStringLiteral("项目内嵌VPY恢复失败；当前项目保持不变。"),true);return false;}r.preset=filePath;}
        r.visible=e.value("visible").toBool(true);prepared.append(r);
    }
    projectApplying_=true;additionalRoutes_.clear();while(routes_.size()>1){compareView_->removeProcessedPane(routes_.size()-1);routes_.removeLast();}routes_=prepared;for(int i=1;i<routes_.size();++i)createAdditionalRoute();
    routeIndex_=qBound(0,root.value("editingRoute").toInt(),int(routes_.size())-1);activePreset_=routes_[routeIndex_].preset;replaceTimeGroups(routes_[routeIndex_].groups);refreshTimeGroups(qBound(0,root.value("editingTimeGroup").toInt(),int(timeGroups_.size())-1));
    for(int i=0;i<routes_.size();++i)compareView_->setRouteVisible(i,routes_[i].visible);updateRouteChoices();
    sourceFilter_->setCurrentIndex(qMax(0,sourceFilter_->findData(root.value("sourceFilter").toInt())));
    const auto source=root.value("sourcePath").toString();if(source!=sourcePath_->text()){if(QFileInfo::exists(source))loadSource(source);else sourcePath_->setText(source);}
    refreshPipeline(graph_.nodes().isEmpty()?-1:0);projectApplying_=false;return true;
}
bool MainWindow::saveProjectTo(const QString &path, QString *error){
    if(error)error->clear();
    const auto data=projectSnapshot();const auto bytes=QJsonDocument(data).toJson(QJsonDocument::Indented);
    if(QFileInfo::exists(path)){const auto backup=path+".bak";QFile old(path);QSaveFile copy(backup);if(!old.open(QIODevice::ReadOnly)||!copy.open(QIODevice::WriteOnly)||copy.write(old.readAll())!=old.size()||!copy.commit()){if(error)*error=QStringLiteral("备份旧项目失败，原文件未修改。");setStatus(QStringLiteral("备份旧项目失败，原文件未修改。"),true);return false;}}
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){if(error)*error=file.errorString();setStatus(QStringLiteral("项目保存失败，原文件保持不变：%1").arg(file.errorString()),true);return false;}
    projectPath_=path;savedProject_=data;setWindowModified(false);QFile::remove(recoveryPath_);setStatus(QStringLiteral("项目已保存：%1").arg(path));return true;
}
void MainWindow::saveProject(){auto path=projectPath_;if(path.isEmpty())path=QFileDialog::getSaveFileName(this,QStringLiteral("保存 VS Renderer 项目"),{},QStringLiteral("VS Renderer Project (*.vsr.json)"));if(!path.isEmpty())saveProjectTo(path);}
bool MainWindow::confirmProjectReplacement(){
    checkpointProject();if(!isWindowModified())return true;
    const auto answer=QMessageBox::warning(this,tr("项目尚未保存"),tr("保存当前处理路、滤镜和帧时间组后继续？"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(answer==QMessageBox::Cancel)return false;if(answer==QMessageBox::Save){saveProject();checkpointProject();return !isWindowModified();}return true;
}
void MainWindow::openProject(){
    const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("打开 VS Renderer 项目"),{},QStringLiteral("VS Renderer Project (*.vsr.json)"));if(path.isEmpty())return;
    if(!confirmProjectReplacement())return;
    QString error;if(!openProjectFrom(path,&error))setStatus(error,true);
}
bool MainWindow::openProjectFrom(const QString &path, QString *error){
    if(error)error->clear();
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>16*1024*1024){if(error)*error=QStringLiteral("项目不可读或过大。");return false;}
    QJsonParseError parse;const auto document=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError||!document.isObject()){if(error)*error=QStringLiteral("项目JSON无效。");return false;}
    const auto root=document.object();const int version=root.value("schemaVersion").toInt();
    if(version<1||version>3||(!root.value("routes").isArray()&&!root.value("nodes").isArray()&&!root.value("timeGroups").isArray())){if(error)*error=QStringLiteral("不是有效的 VS Renderer 项目。");return false;}
    const auto compatibility=PresetStore::compatibilityError(root);if(!compatibility.isEmpty()){if(error)*error=compatibility;return false;}
    if(!applyProjectSnapshot(root)){if(error)*error=QStringLiteral("项目恢复失败。");return false;}
    projectPath_=path;savedProject_=projectSnapshot();projectHistory_={savedProject_};projectHistoryIndex_=0;setWindowModified(false);QFile::remove(recoveryPath_);return true;
}

void MainWindow::checkpointProject(){
    if(!projectProtectionReady_||projectApplying_)return;const auto current=projectSnapshot();
    // Selection of an editing route alone is not a content modification.
    const auto comparableProject=[](QJsonObject value){value.remove("editingRoute");value.remove("editingTimeGroup");QJsonArray routes;for(const auto &v:value["routes"].toArray()){auto r=v.toObject();if(r.contains("presetScript"))r.remove("preset");routes.append(r);}value["routes"]=routes;return value;};
    auto comparable=comparableProject(current),previous=comparableProject(projectHistory_.value(projectHistoryIndex_));
    if(comparable!=previous){while(projectHistory_.size()>projectHistoryIndex_+1)projectHistory_.removeLast();projectHistory_.append(current);if(projectHistory_.size()>64)projectHistory_.removeFirst();projectHistoryIndex_=projectHistory_.size()-1;}
    auto saved=comparableProject(savedProject_);const bool dirty=comparable!=saved;setWindowModified(dirty);
    if(dirty){QSaveFile f(recoveryPath_);const auto bytes=QJsonDocument(QJsonObject{{"project",current},{"path",projectPath_},{"pid",double(QCoreApplication::applicationPid())},{"updated",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}).toJson();if(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size())f.commit();}else QFile::remove(recoveryPath_);
}
void MainWindow::stepProjectHistory(int direction){checkpointProject();const int target=projectHistoryIndex_+direction;if(target<0||target>=projectHistory_.size())return;if(applyProjectSnapshot(projectHistory_[target])){projectHistoryIndex_=target;checkpointProject();}}
void MainWindow::initializeProjectProtection(){
    const auto override=qEnvironmentVariable("VSR_RECOVERY_ROOT");const auto base=override.isEmpty()?QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("VSRenderer/recovery"):override;QDir().mkpath(base);
    recoveryPath_=QDir(base).filePath("session-"+QString::number(QCoreApplication::applicationPid())+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json");
    savedProject_=projectSnapshot();projectHistory_={savedProject_};projectProtectionReady_=true;setWindowTitle(windowTitle()+"[*]");
    auto *poll=new QTimer(this);poll->setInterval(1200);connect(poll,&QTimer::timeout,this,&MainWindow::checkpointProject);poll->start();
    auto *save=new QAction(QStringLiteral("保存项目"),this);save->setShortcut(QKeySequence::Save);addAction(save);connect(save,&QAction::triggered,this,&MainWindow::saveProject);
    auto *saveAs=new QAction(QStringLiteral("项目另存为"),this);saveAs->setShortcut(QKeySequence::SaveAs);addAction(saveAs);connect(saveAs,&QAction::triggered,this,[this]{const auto p=QFileDialog::getSaveFileName(this,QStringLiteral("项目另存为"),projectPath_,"VS Renderer Project (*.vsr.json)");if(!p.isEmpty())saveProjectTo(p);});
    for(const auto &entry:QList<QPair<QKeySequence,int>>{{QKeySequence::Undo,-1},{QKeySequence::Redo,1}}){auto *a=new QAction(this);a->setShortcut(entry.first);addAction(a);connect(a,&QAction::triggered,this,[this,d=entry.second]{stepProjectHistory(d);});}
    QTimer::singleShot(0,this,[this,base]{
        if(!isVisible())return;const auto files=QDir(base).entryList({"session-*.json"},QDir::Files,QDir::Time);
        for(const auto &name:files){const auto path=QDir(base).filePath(name);if(path==recoveryPath_)continue;QFile f(path);if(!f.open(QIODevice::ReadOnly))continue;const auto o=QJsonDocument::fromJson(f.readAll()).object();f.close();
#ifdef Q_OS_WIN
            HANDLE process=OpenProcess(SYNCHRONIZE,FALSE,DWORD(o["pid"].toDouble()));if(process){const bool alive=WaitForSingleObject(process,0)==WAIT_TIMEOUT;CloseHandle(process);if(alive)continue;}
#endif
            const auto result=QMessageBox::question(this,QStringLiteral("恢复未保存项目"),QStringLiteral("发现恢复快照 %1。是否恢复？选择否只保留快照，不覆盖任何项目。").arg(o["updated"].toString()),QMessageBox::Yes|QMessageBox::No);if(result==QMessageBox::Yes&&applyProjectSnapshot(o["project"].toObject())){projectPath_=o["path"].toString();checkpointProject();}break;
        }
    });
}
}
