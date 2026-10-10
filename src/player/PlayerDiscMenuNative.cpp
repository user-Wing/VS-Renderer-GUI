#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpApi.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QWidget>

namespace vsr {
struct PlayerDiscMenu::Impl {
    ThreeFpApi api;
    void *handle=nullptr;
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
bool PlayerDiscMenu::open(const QString &root,void *window) {
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
    if(impl_->handle){impl_->api.stop(impl_->handle);impl_->api.destroy(impl_->handle);impl_->handle=nullptr;}
    if(retiredSurface)retiredSurface->deleteLater();
}
bool PlayerDiscMenu::active() const { return impl_->handle!=nullptr; }
QString PlayerDiscMenu::error() const { const auto text=impl_->api.sessionError(impl_->handle);return text.isEmpty()?impl_->failure:text; }
int PlayerDiscMenu::state() const {
    auto s=impl_->snapshot();
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
void PlayerDiscMenu::topMenu() { if(active())impl_->navigate(impl_->handle,5,0,0); }
void PlayerDiscMenu::pause(bool paused) { if(active()){if(paused)impl_->api.pause(impl_->handle);else impl_->api.play(impl_->handle);} }
void PlayerDiscMenu::seek(qint64 ticks) { if(active())impl_->api.seek(impl_->handle,ticks); }
void PlayerDiscMenu::volume(int value,bool muted) { if(active())impl_->api.setVolume(impl_->handle,value/100.f,muted); }
void PlayerDiscMenu::rate(float value) { if(active())impl_->api.setPlaybackRate(impl_->handle,value); }
qint64 PlayerDiscMenu::position() const { return impl_->snapshot().position100ns; }
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
    auto result=QJsonDocument::fromJson(text.constData()).object();result.insert("duration100ns",double(duration()));return result;
}
bool PlayerDiscMenu::snapshot(const QString &path) const {
    if(!active())return false;uint32_t w=0,h=0;
    impl_->copy(impl_->handle,nullptr,0,&w,&h,1,0);
    if(!w || !h || quint64(w)*h>40000000)return false;
    QImage image(int(w),int(h),QImage::Format_ARGB32);
    return impl_->copy(impl_->handle,image.bits(),uint32_t(image.sizeInBytes()),&w,&h,1,0)==ThreeFpResult::Success && image.save(path,"PNG");
}
}
