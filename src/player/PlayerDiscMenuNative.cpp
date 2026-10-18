#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpApi.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QWidget>

namespace vsr {
struct PlayerDiscMenu::Impl {
    ThreeFpApi api;
    void *handle=nullptr;
    bool pausePending=false,resumeRequested=false;
    qint64 seekTarget=-1;
    quint64 seekGeneration=0,seekPresents=0;
    QString failure;
    using Navigate=ThreeFpResult(*)(void*,int,int,int);
    using Status=ThreeFpResult(*)(void*,char*,uint32_t,uint32_t*);
    using Copy=ThreeFpResult(*)(void*,void*,uint32_t,uint32_t*,uint32_t*,uint32_t,uint32_t);
    Navigate navigate=nullptr;Status status=nullptr;Copy copy=nullptr;
    ThreeFpSnapshot snapshot() const {
        ThreeFpSnapshot s{};s.size=sizeof(s);s.version=8;
        if(handle)api.snapshot(handle,&s);return s;
    }
};
PlayerDiscMenu::PlayerDiscMenu() : impl_(std::make_unique<Impl>()) {}
PlayerDiscMenu::~PlayerDiscMenu() { close(); }
bool PlayerDiscMenu::open(const QString &root,void *window,bool) {
    close();auto &i=*impl_;
    if(!i.api.available()){i.failure=i.api.errorString();return false;}
    const auto dll=i.api.libraryPath();
    i.navigate=reinterpret_cast<Impl::Navigate>(QLibrary::resolve(dll,"FFF3FP_DiscNavigate"));
    i.status=reinterpret_cast<Impl::Status>(QLibrary::resolve(dll,"FFF3FP_GetDiscStatus"));
    i.copy=reinterpret_cast<Impl::Copy>(QLibrary::resolve(dll,"FFF3FP_CopyFrame"));
    if(!i.navigate || !i.status || !i.copy){i.failure=QStringLiteral("Native disc navigation API is unavailable.");return false;}
    ThreeFpConfiguration c{};c.size=sizeof(c);c.version=i.api.apiVersion();
    c.outputWindow=window;c.decodeMode=2;c.colorMode=2;c.sdrPeakNits=100;c.sdrPaperWhiteNits=203;
    if(i.api.create(&c,&i.handle)!=ThreeFpResult::Success){i.failure=QStringLiteral("Could not create native disc player.");return false;}
    i.api.setScalingAlgorithms(i.handle,ThreeFpScalingAlgorithm::D3D11Native,ThreeFpScalingAlgorithm::D3D11Native);
    if(i.api.open(i.handle,root.toUtf8().constData())!=ThreeFpResult::Success){i.failure=i.api.sessionError(i.handle);close();return false;}
    return true;
}
void PlayerDiscMenu::close(QWidget *retiredSurface) {
    impl_->pausePending=impl_->resumeRequested=false;impl_->seekTarget=-1;
    auto *handle=impl_->handle;impl_->handle=nullptr;
    if(retiredSurface)retiredSurface->hide();
    // Match ThreeFpPlayer: finish the native session before replacing its HWND.
    // Pumping Qt events here can re-enter openDiscMenu while it replaces the surface.
    if(handle){impl_->api.stop(handle);impl_->api.destroy(handle);}
    if(retiredSurface)retiredSurface->deleteLater();
}
bool PlayerDiscMenu::active() const { return impl_->handle!=nullptr; }
QString PlayerDiscMenu::error() const { const auto text=impl_->api.sessionError(impl_->handle);return text.isEmpty()?impl_->failure:text; }
int PlayerDiscMenu::state() const {
    auto s=impl_->snapshot();
    if(impl_->pausePending && s.state!=ThreeFpState::Playing)impl_->pausePending=false;
    if(impl_->resumeRequested && !impl_->pausePending){
        if(s.state==ThreeFpState::Playing)impl_->resumeRequested=false;
        else if((s.state==ThreeFpState::Ready || s.state==ThreeFpState::Paused || s.state==ThreeFpState::Ended) && impl_->api.play(impl_->handle)==ThreeFpResult::Success)impl_->resumeRequested=false;
    }
    if(s.state==ThreeFpState::Ready){impl_->api.play(impl_->handle);return 1;}
    if(s.state==ThreeFpState::Failed)return 7;
    if(s.state==ThreeFpState::Ended)return 6;
    return int(s.state);
}
void PlayerDiscMenu::navigate(unsigned command) {
    // Existing Qt controls use VLC's activate/up/down/left/right/popup ordering.
    const int commands[]{4,0,1,2,3,6};
    if(active() && command<6)impl_->navigate(impl_->handle,commands[command],0,0);
}
void PlayerDiscMenu::topMenu() { impl_->seekTarget=-1;if(active())impl_->navigate(impl_->handle,5,0,0); }
void PlayerDiscMenu::pointer(const QPoint &position,const QSize &surface,bool activate) {
    if(!active() || surface.isEmpty())return;
    const auto s=impl_->snapshot();const QSize video(s.videoWidth,s.videoHeight);if(video.isEmpty())return;
    const auto fitted=video.scaled(surface,Qt::KeepAspectRatio);
    const QRect area(QPoint((surface.width()-fitted.width())/2,(surface.height()-fitted.height())/2),fitted);
    if(!area.contains(position))return;
    const auto point=position-area.topLeft();
    impl_->navigate(impl_->handle,activate?9:8,point.x()*video.width()/fitted.width(),point.y()*video.height()/fitted.height());
}
void PlayerDiscMenu::pause(bool paused) { if(active()){if(paused){impl_->resumeRequested=false;impl_->pausePending=impl_->api.pause(impl_->handle)==ThreeFpResult::Success;}else{impl_->resumeRequested=true;state();}} }
void PlayerDiscMenu::seek(qint64 ticks) {
    if(!active())return;const auto s=impl_->snapshot();
    if(impl_->api.seek(impl_->handle,ticks)==ThreeFpResult::Success){impl_->seekTarget=qBound(qint64(0),ticks,s.duration100ns);impl_->seekGeneration=s.timelineGeneration;impl_->seekPresents=s.presentedVideoFrames;}
}
void PlayerDiscMenu::volume(int value,bool muted) { if(active())impl_->api.setVolume(impl_->handle,value/100.f,muted); }
void PlayerDiscMenu::rate(float value) { if(active())impl_->api.setPlaybackRate(impl_->handle,value); }
qint64 PlayerDiscMenu::position() const {
    const auto s=impl_->snapshot();
    if(impl_->seekTarget>=0){
        if(s.state==ThreeFpState::Failed || s.state==ThreeFpState::Ended ||
            (s.timelineGeneration!=impl_->seekGeneration && s.presentedVideoFrames>impl_->seekPresents && qAbs(s.position100ns-impl_->seekTarget)<10000000))impl_->seekTarget=-1;
        else return impl_->seekTarget;
    }
    return s.position100ns;
}
qint64 PlayerDiscMenu::duration() const { return impl_->snapshot().duration100ns; }
quint64 PlayerDiscMenu::presentedFrames() const { return impl_->snapshot().swapChainPresents; }
QVector<QPair<int,QString>> PlayerDiscMenu::tracks(bool audio) const {
    QVector<QPair<int,QString>> result;
    const auto media=QJsonDocument::fromJson(impl_->api.mediaInfo(impl_->handle).toUtf8()).object();
    int ordinal=0;
    for(const auto &v:media.value("streams").toArray()){
        const auto s=v.toObject();if(s.value("type").toString()!=(audio?"audio":"subtitle"))continue;
        result.append({ordinal++,s.value("codec").toString()+" "+s.value("language").toString()});
    }
    if(!audio)result.prepend({-1,QStringLiteral("关闭")});return result;
}
void PlayerDiscMenu::selectTrack(bool audio,int id) { if(active())impl_->navigate(impl_->handle,audio?12:13,id,0); }
QJsonObject PlayerDiscMenu::programme() const {
    if(!active())return {};QByteArray text(65536,'\0');uint32_t size=0;
    if(impl_->status(impl_->handle,text.data(),uint32_t(text.size()),&size)!=ThreeFpResult::Success)return {};
    auto result=QJsonDocument::fromJson(text.constData()).object();result.insert("duration100ns",double(duration()));
    result.insert("chapters",result.value("chapterStarts100ns"));result.insert("nativeDisc",true);return result;
}
bool PlayerDiscMenu::snapshot(const QString &path) const {
    if(!active())return false;uint32_t w=0,h=0;
    impl_->copy(impl_->handle,nullptr,0,&w,&h,1,0);
    if(!w || !h || quint64(w)*h>40000000)return false;
    QImage image(int(w),int(h),QImage::Format_ARGB32);
    return impl_->copy(impl_->handle,image.bits(),uint32_t(image.sizeInBytes()),&w,&h,1,0)==ThreeFpResult::Success && image.save(path,"PNG");
}
}
