#include "player/PlayerWindow.h"
#include "player/PlayerAssociations.h"
#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpPlayer.h"
#include <QMenu>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include <QSettings>

namespace vsr {
void PlayerWindow::setLoopPoint(bool out,qint64 time) {
    if(time<0)return;
    if(out){
        if(loopIn_<0 || time<=loopIn_){showStatus(tr("请先设置 A 入点，B 出点必须晚于 A。"));return;}
        loopOut_=time;loopEnabled_=true;
    }else {loopIn_=time;if(loopOut_<=time)loopOut_=-1;loopEnabled_=loopOut_>loopIn_;}
    showStatus(out?tr("已设置 B 出点，A–B 循环已开启。"):tr("已设置 A 入点。"));
}
bool PlayerWindow::updateLoop(qint64 time,bool ended) {
    if(!loopEnabled_ || loopIn_<0 || loopOut_<=loopIn_ || !playing_ || !ready_ ||
       seekPending_ || seekUiTarget_>=0 || timelineDragging_ || timelineWaiting_ || resumeAt_>=0)return false;
    if(time<loopOut_ && !ended)return false;
    togglePlayback();
    seekTime(loopIn_);autoPlay_=true;
    return true;
}
void PlayerWindow::addLoopMenu(QMenu *menu) {
    const bool available=ready_ && !imageMode_ && !source_.isEmpty() && clock_->snapshot().duration100ns>0 && !(discMenu_ && discMenu_->active());
    const qint64 at=seekUiTarget_>=0?seekUiTarget_:position();
    auto *loop=menu->addMenu(tr("A–B 循环"));loop->setEnabled(available);
    const auto label=[](qint64 time){return time<0?QStringLiteral("—"):QString::number(time/10000000.0,'f',3)+QStringLiteral(" s");};
    auto *a=loop->addAction(tr("将当前点设为 A 入点 (%1)").arg(label(loopIn_)),this,[this,at]{setLoopPoint(false,at);});a->setObjectName("playerLoopIn");
    auto *b=loop->addAction(tr("将当前点设为 B 出点 (%1)").arg(label(loopOut_)),this,[this,at]{setLoopPoint(true,at);});b->setObjectName("playerLoopOut");b->setEnabled(loopIn_>=0 && at>loopIn_);
    auto *enabled=loop->addAction(tr("启用 A–B 循环"));enabled->setObjectName("playerLoopEnabled");enabled->setCheckable(true);enabled->setChecked(loopEnabled_);enabled->setEnabled(loopIn_>=0 && loopOut_>loopIn_);connect(enabled,&QAction::toggled,this,[this](bool on){loopEnabled_=on;});
    auto *clear=loop->addAction(tr("清除 A/B 入点与出点"),this,[this]{loopIn_=loopOut_=-1;loopEnabled_=false;});clear->setObjectName("playerLoopClear");
    const bool localVideo=available && !audioMode_ && !networkSource() && QFileInfo(source_).isFile() && source_==mediaInput_;
    auto *open=menu->addAction(tr("在 VS Renderer 中打开当前视频…"),this,[this]{openInRenderer(false);});open->setObjectName("playerOpenRenderer");open->setEnabled(localVideo);
    auto *send=menu->addAction(tr("将 A–B 段送到 VS Renderer 时间组…"),this,[this]{openInRenderer(true);});send->setObjectName("playerLoopRenderer");send->setEnabled(localVideo && loopIn_>=0 && loopOut_>loopIn_);
}
void PlayerWindow::openInRenderer(bool loop) {
    if(loop && (loopIn_<0 || loopOut_<=loopIn_))return;
    auto executable=QDir(QFileInfo(playerAssociationExecutable()).absolutePath()).filePath("VSRenderer.exe");
    if(!QFileInfo(executable).isFile())executable=settings_->value("integration/renderer").toString();
    if(!QFileInfo(executable).isFile())executable=QFileDialog::getOpenFileName(this,tr("选择 VS Renderer 程序"),{},"VS Renderer (VSRenderer.exe)");
    if(executable.isEmpty())return;
    QStringList args{"--source",QFileInfo(source_).absoluteFilePath()};
    if(loop)args<<"--loop-in"<<QString::number(loopIn_)<<"--loop-out"<<QString::number(loopOut_);
    if(!QProcess::startDetached(executable,args,QFileInfo(executable).absolutePath())){setError(tr("无法启动 VS Renderer。"));return;}
    settings_->setValue("integration/renderer",executable);
    showStatus(tr("已在新的 VS Renderer 窗口打开，原有项目保持不变。"));
}
}
