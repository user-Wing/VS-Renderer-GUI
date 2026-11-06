#include "app/MainWindow.h"
#include "app/RenderRoute.h"
#include "ui/CompareView.h"
#include "ui/ExportWindow.h"
#include "graph/PresetStore.h"
#include "graph/HlslPreview.h"
#include "render/ShaderPackage.h"
#include "backend/FrameTimeline.h"
#include <QComboBox>
#include <QCheckBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSignalBlocker>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <functional>
#include <algorithm>
namespace vsr {
void MainWindow::updateRouteChoices(){
    const QSignalBlocker blocker(routeChoice_);routeChoice_->clear();for(int i=0;i<routes_.size();++i)routeChoice_->addItem(QString("路 %1 · %2").arg(i+1).arg(routes_[i].visible?QStringLiteral("显示"):QStringLiteral("隐藏")));routeChoice_->setCurrentIndex(routeIndex_);addRouteButton_->setEnabled(routes_.size()<3);removeRouteButton_->setEnabled(routes_.size()>1);
}
bool MainWindow::addRoute(){
    if(routes_.size()>=3)return false;
    routes_[routeIndex_].groups=currentTimeGroups();routes_[routeIndex_].preset=activePreset_;
    routes_.append(RouteState{});auto *pane=compareView_->addProcessedPane();additionalRoutes_.push_back(std::make_unique<RenderRoute>(api_,pane,this));auto *changedRoute=additionalRoutes_.back().get();connect(pane,&PreviewPane::viewChanged,this,[this,changedRoute](float zoom,float x,float y){if(!syncView_->isChecked())return;sourcePane_->adoptView(zoom,x,y);sourcePlayer_->setView(zoom,x,y);processedPane_->adoptView(zoom,x,y);processedPlayer_->setView(zoom,x,y);for(auto &route:additionalRoutes_)if(route.get()!=changedRoute)route->setView(zoom,x,y);});
    if(syncView_->isChecked())additionalRoutes_.back()->setView(sourcePane_->zoom(),sourcePane_->pan().x(),sourcePane_->pan().y());
    selectRoute(routes_.size()-1);validateScript();return true;
}
void MainWindow::selectRoute(int index){
    if(index<0 || index>=routes_.size())return;
    routes_[routeIndex_].groups=currentTimeGroups();routes_[routeIndex_].preset=activePreset_;
    routeIndex_=index;activePreset_=routes_[index].preset;replaceTimeGroups(routes_[index].groups);updateRouteChoices();
}
void MainWindow::removeRoute(){
    if(routes_.size()<2)return;const int removed=routeIndex_;
    if(removed==0){additionalRoutes_.erase(additionalRoutes_.begin());compareView_->removeProcessedPane(1);}
    else {additionalRoutes_.erase(additionalRoutes_.begin()+removed-1);compareView_->removeProcessedPane(removed);}
    routes_.removeAt(removed);routeIndex_=qMin(removed,int(routes_.size())-1);activePreset_=routes_[routeIndex_].preset;replaceTimeGroups(routes_[routeIndex_].groups);
    for(int i=0;i<routes_.size();++i)compareView_->setRouteVisible(i,routes_[i].visible);updateRouteChoices();
    if(removed==0){const auto editing=routeIndex_;selectRoute(0);validateScript();selectRoute(editing);}
}
void MainWindow::renderAdditionalRoute(int index){
    if(index<1 || index>=routes_.size() || sourcePath_->text().isEmpty())return;
    auto result=currentScript();QString shader;QVariantMap parameters;auto groups=currentTimeGroups();
    bool compatible=activePreset_.isEmpty() && groups.size()==1 && groups.first().script.isEmpty();
    if(!activePreset_.isEmpty()){QFile file(activePreset_);FilterGraph graph;SourceFilter filter=SourceFilter::Ffms2;compatible=file.open(QIODevice::ReadOnly) && PresetStore::verifiedGraph(QString::fromUtf8(file.readAll()),&graph,&filter);if(compatible)groups={{QStringLiteral("全片"),0,graph}};}
    if(compatible){const auto plan=hlslPreviewPlan(groups.first().graph);if(!plan.shaderPath.isEmpty() && !sourcePlayer_->snapshot().isHdrSource){const auto package=ShaderPackage::prepare(plan.shaderPath);if(!package.packagePath.isEmpty()){shader=package.packagePath;parameters=plan.parameters;groups.first().graph=plan.graph;result=VpyScriptBuilder::buildGroups(sourcePath_->text(),static_cast<SourceFilter>(sourceFilter_->currentData().toInt()),groups);}}}
    if(!result.errors.isEmpty()){setStatus(result.errors.join('\n'),true);return;}
    const auto directory=QStandardPaths::writableLocation(QStandardPaths::CacheLocation);QDir().mkpath(directory);
    const auto path=QDir(directory).filePath(QString("preview-route-%1.vpy").arg(index+1));QSaveFile file(path);if(!file.open(QIODevice::WriteOnly) || file.write(result.script.toUtf8())<0 || !file.commit()){setStatus(QStringLiteral("该路脚本写入失败。"),true);return;}
    additionalRoutes_[index-1]->load(result.script,path,shader,parameters);additionalRoutes_[index-1]->setVisible(routes_[index].visible);
}
void MainWindow::scanWorkingFolder(const QString &input){
    const auto folder=input.isEmpty()?QFileDialog::getExistingDirectory(this,QStringLiteral("打开工作文件夹")):input;if(folder.isEmpty())return;
    const int depth=std::clamp(QSettings().value("source/scanDepth",3).toInt(),0,16);auto *watcher=new QFutureWatcher<QStringList>(this);setStatus(QStringLiteral("正在扫描工作文件夹…"));
    connect(watcher,&QFutureWatcher<QStringList>::finished,this,[this,watcher]{const auto files=watcher->result();watcher->deleteLater();const QSignalBlocker blocker(sourceChoice_);sourceChoice_->clear();for(const auto &file:files)sourceChoice_->addItem(QFileInfo(file).fileName(),file);if(!files.isEmpty())loadSource(files.first());else setStatus(QStringLiteral("工作文件夹中未找到视频。"));});
    watcher->setFuture(QtConcurrent::run([folder,depth]{QStringList files;const QStringList suffixes{"mkv","mp4","mov","avi","webm","ts","m2ts","wmv","flv","m4v","mpg","mpeg","vob"};std::function<void(const QString&,int)> scan=[&](const QString &path,int level){for(const auto &entry:QDir(path).entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDir::DirsFirst|QDir::Name)){if(entry.isDir()){if(level<depth)scan(entry.absoluteFilePath(),level+1);}else if(suffixes.contains(entry.suffix().toLower()))files<<entry.absoluteFilePath();}};scan(folder,0);return files;}));
}
}
