#pragma once
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/FrameTimeline.h"
#include "graph/HlslPreview.h"
#include "ui/PreviewPane.h"
#include <QObject>
#include <memory>
namespace vsr {
// Additional routes share only the source media clock; decoded output and VS state are private.
class RenderRoute final : public QObject {
    PreviewPane *pane_;std::unique_ptr<ThreeFpPlayer> player_;std::unique_ptr<VapourSynthFrameServer> server_;
    VapourSynthClipInfo clip_;QString script_,path_,shader_,fallback_;QVariantMap shaderParameters_;bool ready_=false,pending_=false,visible_=true;int requested_=-1,last_=-1;quint64 generation_=0;
    bool retryFallback(){if(shader_.isEmpty() || fallback_.isEmpty())return false;const auto script=fallback_;load(script,path_);return true;}
public:
    RenderRoute(ThreeFpApi &api,PreviewPane *pane,QObject *parent):QObject(parent),pane_(pane),player_(std::make_unique<ThreeFpPlayer>(api,pane->surface())),server_(std::make_unique<VapourSynthFrameServer>()){
        connect(pane_,&PreviewPane::viewChanged,this,[this](float zoom,float x,float y){player_->setView(zoom,x,y);});connect(pane_,&PreviewPane::redrawRequested,player_.get(),&ThreeFpPlayer::redraw);
        player_->setMuted(true);player_->setScalingAlgorithms(ThreeFpScalingAlgorithm::Jinc2,ThreeFpScalingAlgorithm::Jinc2);
        connect(server_.get(),&VapourSynthFrameServer::initialized,this,[this](bool ok){if(ok && visible_ && !script_.isEmpty())server_->loadScript(script_,path_);});
        connect(server_.get(),&VapourSynthFrameServer::scriptLoaded,this,[this](const VapourSynthClipInfo &clip,const VapourSynthClipInfo &){clip_=clip;ready_=true;pending_=false;last_=-1;if(!shader_.isEmpty()){const auto size=hlslOutputSize(shaderParameters_,QSize(clip.width,clip.height));if(size.isEmpty() || !hlslAspectCompatible(shaderParameters_,QSize(clip.width,clip.height),size) || !player_->setCustomShader(shader_,size.width(),size.height())){ready_=false;if(!retryFallback()) pane_->setPlaceholderText(QStringLiteral("该路 HLSL 输出不兼容。"));return;}}pane_->setVideoSize(QSize(clip.width,clip.height));});
        connect(server_.get(),&VapourSynthFrameServer::frameReady,this,[this](const VapourSynthFrame &frame){if(!visible_ || !pending_ || frame.frameIndex!=requested_)return;pending_=false;if(!shader_.isEmpty() && (frame.colorTransfer==16 || frame.colorTransfer==18 || frame.colorMatrix==9 || frame.colorMatrix==10 || (frame.colorPrimaries!=1 && frame.colorPrimaries!=2)) && retryFallback())return;if(player_->submitFrame(frame)){last_=frame.frameIndex;pane_->setSurfaceActive(true);pane_->setBadge(QString("VS · 帧 %1").arg(last_));}else{if(retryFallback())return;pane_->setSurfaceActive(false);pane_->setPlaceholderText(player_->lastError());}});
        connect(server_.get(),&VapourSynthFrameServer::errorOccurred,this,[this](const QString &error){ready_=pending_=false;pane_->setSurfaceActive(false);pane_->setPlaceholderText(error);});
    }
    void setScalingAlgorithms(ThreeFpScalingAlgorithm upscale,ThreeFpScalingAlgorithm downscale){player_->setScalingAlgorithms(upscale,downscale);}
    void setView(float zoom,float x,float y){pane_->adoptView(zoom,x,y);player_->setView(zoom,x,y);}
    ThreeFpSnapshot snapshot() const {return player_->snapshot();}
    bool hasFrame() const {return ready_ && last_>=0;}
    void reset(const QString &hint){script_.clear();ready_=pending_=false;last_=requested_=-1;server_->unloadScript();player_->stop();pane_->setSurfaceActive(false);pane_->setPlaceholderText(hint);pane_->setBadge(QStringLiteral("VS · 待渲染"));}
    void load(const QString &script,const QString &path,const QString &shader={},const QVariantMap &parameters={},const QString &fallback={}){fallback_=fallback;script_=script;path_=path;shader_=shader;shaderParameters_=parameters;ready_=pending_=false;last_=-1;if(!visible_)return;player_->setCustomShader({});player_->resetVideoOutput();player_->setMuted(true);pane_->setSurfaceActive(false);pane_->setPlaceholderText(QStringLiteral("正在加载该路处理链…"));if(server_->available())server_->loadScript(script,path);else server_->initialize();}
    void setVisible(bool visible){if(visible_==visible)return;visible_=visible;if(!visible){ready_=pending_=false;server_->unloadScript();player_->stop();pane_->setSurfaceActive(false);}else if(!script_.isEmpty())load(script_,path_,shader_,shaderParameters_,fallback_);}
    void sync(const ThreeFpSnapshot &source){if(!visible_ || !ready_ || pending_)return;if(generation_!=source.timelineGeneration){generation_=source.timelineGeneration;last_=-1;}const int frame=int(frameAtPosition100ns(source.position100ns,clip_.totalFrames,clip_.fpsNumerator,clip_.fpsDenominator));if(frame==last_)return;requested_=frame;pending_=true;server_->requestFrame(frame,2,source.state==ThreeFpState::Playing);}
};
}
