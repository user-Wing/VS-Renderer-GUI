#include "player/PlayerDiscMenu.h"
#include <QLibrary>
#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QTimer>
#include <QWidget>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QJsonArray>
#include <windows.h>
#include <cstdarg>
#include <cstdio>
namespace vsr {
namespace {
constexpr wchar_t InputOwner[]=L"VSR.BD.InputOwner";
// VLC consumes input in its vout message loop before dispatching its WndProc.
// Forward it at GetMessage so Qt receives the same controls as normal video.
LRESULT CALLBACK discInput(int code,WPARAM removed,LPARAM message){
    if(code>=0 && removed==PM_REMOVE){auto *msg=reinterpret_cast<MSG *>(message);auto *owner=reinterpret_cast<QWidget *>(GetPropW(msg->hwnd,InputOwner));
        if(owner && msg->message==WM_RBUTTONDOWN)msg->message=WM_NULL;
        else if(owner && msg->message==WM_RBUTTONUP){QMetaObject::invokeMethod(owner,[owner]{const auto global=QCursor::pos();QContextMenuEvent event(QContextMenuEvent::Mouse,owner->mapFromGlobal(global),global);QCoreApplication::sendEvent(owner,&event);},Qt::QueuedConnection);msg->message=WM_NULL;}
        else if(owner && msg->message==WM_KEYDOWN){int key=0;switch(msg->wParam){case VK_RETURN:key=Qt::Key_Return;break;case VK_LEFT:key=Qt::Key_Left;break;case VK_RIGHT:key=Qt::Key_Right;break;case VK_UP:key=Qt::Key_Up;break;case VK_DOWN:key=Qt::Key_Down;break;case VK_SPACE:key=Qt::Key_Space;break;case VK_TAB:key=Qt::Key_Tab;break;case VK_ESCAPE:key=Qt::Key_Escape;break;case VK_F11:key=Qt::Key_F11;break;default:break;}if(key){QMetaObject::invokeMethod(owner,[owner,key]{QKeyEvent event(QEvent::KeyPress,key,Qt::NoModifier);QCoreApplication::sendEvent(owner,&event);},Qt::QueuedConnection);msg->message=WM_NULL;}}
    }
    return CallNextHookEx(nullptr,code,removed,message);
}
struct InputRoute{HWND parent;QHash<DWORD,HHOOK> *hooks;};
void routeDiscInput(HWND parent,QHash<DWORD,HHOOK> &hooks){
    InputRoute route{parent,&hooks};EnumChildWindows(parent,[](HWND child,LPARAM context)->BOOL{
        auto *route=reinterpret_cast<InputRoute *>(context);const auto thread=GetWindowThreadProcessId(child,nullptr);
        if(thread!=GetCurrentThreadId()){SetPropW(child,InputOwner,reinterpret_cast<HANDLE>(QWidget::find(reinterpret_cast<WId>(route->parent))));if(!route->hooks->contains(thread))if(const auto hook=SetWindowsHookExW(WH_GETMESSAGE,discInput,nullptr,thread))route->hooks->insert(thread,hook);}return TRUE;
    },reinterpret_cast<LPARAM>(&route));
}
}
struct PlayerDiscMenu::Impl {
    QTimer inputTimer;HWND window=nullptr;QHash<DWORD,HHOOK> inputHooks;QVector<QFuture<void>> retiring;
    int lastVolume=-1,lastMute=-1;QLibrary core,library;void *instance=nullptr,*player=nullptr,*media=nullptr;QString failure;QMutex mutex;
    struct Stats {int readBytes;float inputBitrate;int demuxBytes;float demuxBitrate;int corrupted,discontinuities,decodedVideo,decodedAudio,presented,lostVideo,playedAudio,lostAudio,sentPackets,sentBytes;float sendBitrate;};
    struct Description {int id;char *name;Description *next;};
    struct Chapter {qint64 offset,duration;char *name;};
#define VLC_FUNCTION(ret,name,args) using name##Type=ret(*)args; name##Type name=nullptr;
    VLC_FUNCTION(void*,libvlc_new,(int,const char* const*))
    VLC_FUNCTION(void,libvlc_release,(void*))
    VLC_FUNCTION(void*,libvlc_media_new_location,(void*,const char*))
    VLC_FUNCTION(void,libvlc_media_add_option,(void*,const char*))
    VLC_FUNCTION(void,libvlc_media_release,(void*))
    VLC_FUNCTION(void*,libvlc_media_player_new_from_media,(void*))
    VLC_FUNCTION(void,libvlc_media_player_release,(void*))
    VLC_FUNCTION(void,libvlc_media_player_set_hwnd,(void*,void*))
    VLC_FUNCTION(int,libvlc_media_player_play,(void*))
    VLC_FUNCTION(void,libvlc_media_player_stop,(void*))
    VLC_FUNCTION(void,libvlc_media_player_set_pause,(void*,int))
    VLC_FUNCTION(void,libvlc_media_player_navigate,(void*,unsigned))
    VLC_FUNCTION(void,libvlc_media_player_set_title,(void*,int))
    VLC_FUNCTION(int,libvlc_media_player_get_title,(void*))
    VLC_FUNCTION(int,libvlc_media_player_get_chapter,(void*))
    VLC_FUNCTION(int,libvlc_video_take_snapshot,(void*,unsigned,const char*,unsigned,unsigned))
    VLC_FUNCTION(int,libvlc_media_player_get_full_chapter_descriptions,(void*,int,Chapter***))
    VLC_FUNCTION(void,libvlc_chapter_descriptions_release,(Chapter**,unsigned))
    VLC_FUNCTION(unsigned,libvlc_video_get_size,(void*,unsigned,unsigned*,unsigned*))
    VLC_FUNCTION(float,libvlc_media_player_get_fps,(void*))
    VLC_FUNCTION(int,libvlc_audio_get_track,(void*))
    VLC_FUNCTION(int,libvlc_video_get_spu,(void*))
    VLC_FUNCTION(int,libvlc_media_player_get_state,(void*))
    VLC_FUNCTION(qint64,libvlc_media_player_get_time,(void*))
    VLC_FUNCTION(qint64,libvlc_media_player_get_length,(void*))
    VLC_FUNCTION(void,libvlc_media_player_set_time,(void*,qint64))
    VLC_FUNCTION(int,libvlc_media_player_set_rate,(void*,float))
    VLC_FUNCTION(int,libvlc_audio_set_volume,(void*,int))
    VLC_FUNCTION(void,libvlc_audio_set_mute,(void*,int))
    VLC_FUNCTION(Description*,libvlc_audio_get_track_description,(void*))
    VLC_FUNCTION(Description*,libvlc_video_get_spu_description,(void*))
    VLC_FUNCTION(void,libvlc_track_description_list_release,(Description*))
    VLC_FUNCTION(int,libvlc_audio_set_track,(void*,int))
    VLC_FUNCTION(int,libvlc_video_set_spu,(void*,int))
    VLC_FUNCTION(int,libvlc_media_get_stats,(void*,Stats*))
    VLC_FUNCTION(void,libvlc_video_set_key_input,(void*,unsigned))
    VLC_FUNCTION(void,libvlc_video_set_mouse_input,(void*,unsigned))
    using Log=void(*)(void*,int,const void*,const char*,va_list);
    VLC_FUNCTION(void,libvlc_log_set,(void*,Log,void*))
#undef VLC_FUNCTION
    bool load(){
        if(instance)return true;
        const auto root=QDir(QCoreApplication::applicationDirPath()).filePath("runtime/vlc");
        core.setFileName(root+"/libvlccore.dll");library.setFileName(root+"/libvlc.dll");
        core.setLoadHints(QLibrary::PreventUnloadHint);library.setLoadHints(QLibrary::PreventUnloadHint);
        if(!core.load() || !library.load()){failure=QStringLiteral("缺少 BD 菜单组件 runtime/vlc (libVLC 与 plugins)：")+library.errorString();return false;}
#define VLC_LOAD(name) name=reinterpret_cast<name##Type>(library.resolve(#name));if(!name){failure=QStringLiteral("BD 菜单组件缺少接口：")+QStringLiteral(#name);return false;}
        VLC_LOAD(libvlc_new) VLC_LOAD(libvlc_release) VLC_LOAD(libvlc_media_new_location)
        VLC_LOAD(libvlc_media_add_option) VLC_LOAD(libvlc_media_release) VLC_LOAD(libvlc_media_player_new_from_media)
        VLC_LOAD(libvlc_media_player_release) VLC_LOAD(libvlc_media_player_set_hwnd) VLC_LOAD(libvlc_media_player_play)
        VLC_LOAD(libvlc_media_player_stop) VLC_LOAD(libvlc_media_player_set_pause) VLC_LOAD(libvlc_media_player_navigate)
        VLC_LOAD(libvlc_media_player_set_title)
        VLC_LOAD(libvlc_media_player_get_title) VLC_LOAD(libvlc_media_player_get_full_chapter_descriptions) VLC_LOAD(libvlc_chapter_descriptions_release)
        VLC_LOAD(libvlc_media_player_get_chapter) VLC_LOAD(libvlc_video_take_snapshot)
        VLC_LOAD(libvlc_video_get_size) VLC_LOAD(libvlc_media_player_get_fps) VLC_LOAD(libvlc_audio_get_track) VLC_LOAD(libvlc_video_get_spu)
        VLC_LOAD(libvlc_media_player_get_state) VLC_LOAD(libvlc_media_player_get_time) VLC_LOAD(libvlc_media_player_get_length)
        VLC_LOAD(libvlc_media_player_set_time) VLC_LOAD(libvlc_media_player_set_rate) VLC_LOAD(libvlc_audio_set_volume) VLC_LOAD(libvlc_audio_set_mute)
        VLC_LOAD(libvlc_audio_get_track_description) VLC_LOAD(libvlc_video_get_spu_description) VLC_LOAD(libvlc_track_description_list_release)
        VLC_LOAD(libvlc_audio_set_track) VLC_LOAD(libvlc_video_set_spu) VLC_LOAD(libvlc_media_get_stats)
        VLC_LOAD(libvlc_video_set_key_input) VLC_LOAD(libvlc_video_set_mouse_input) VLC_LOAD(libvlc_log_set)
#undef VLC_LOAD
        const char *args[]={"--no-video-title-show","--no-interact","--avcodec-threads=0","--file-caching=300","--disc-caching=300","--no-osd","--mouse-hide-timeout=2147483647"};
        instance=libvlc_new(7,args);if(!instance){failure=QStringLiteral("无法初始化 BD 菜单组件，请检查 plugins 目录。");return false;}
        libvlc_log_set(instance,[](void *opaque,int level,const void*,const char *format,va_list args){char text[2048];vsnprintf(text,sizeof(text),format,args);if(level<3)return;auto *self=static_cast<Impl*>(opaque);QMutexLocker lock(&self->mutex);self->failure=QString::fromUtf8(text);},this);
        return true;
    }
};
PlayerDiscMenu::PlayerDiscMenu():impl_(std::make_unique<Impl>()){}
PlayerDiscMenu::~PlayerDiscMenu(){close();if(impl_->instance)impl_->libvlc_release(impl_->instance);}
bool PlayerDiscMenu::open(const QString &root,void *window){
    if(impl_->player || impl_->media)close();impl_->lastVolume=impl_->lastMute=-1;if(!impl_->load())return false;{QMutexLocker lock(&impl_->mutex);impl_->failure.clear();}
    const auto location=("bluray:///"+QDir::fromNativeSeparators(root)).toUtf8();auto *media=impl_->libvlc_media_new_location(impl_->instance,location.constData());if(!media)return false;
    impl_->libvlc_media_add_option(media,":bluray-menu");
    impl_->media=media;impl_->player=impl_->libvlc_media_player_new_from_media(media);if(!impl_->player)return false;
    impl_->libvlc_media_player_set_hwnd(impl_->player,window);
    impl_->window=static_cast<HWND>(window);QObject::connect(&impl_->inputTimer,&QTimer::timeout,&impl_->inputTimer,[this]{routeDiscInput(impl_->window,impl_->inputHooks);});impl_->inputTimer.start(100);
    // Qt handles keys and context menus; native mouse clicks still select BD buttons.
    impl_->libvlc_video_set_key_input(impl_->player,0);impl_->libvlc_video_set_mouse_input(impl_->player,1);
    if(impl_->libvlc_media_player_play(impl_->player)!=0){close();return false;}return true;
}
void PlayerDiscMenu::close(QWidget *retiredSurface){
    impl_->inputTimer.stop();impl_->inputTimer.disconnect();if(impl_->window){for(const auto hook:impl_->inputHooks)UnhookWindowsHookEx(hook);impl_->inputHooks.clear();EnumChildWindows(impl_->window,[](HWND child,LPARAM)->BOOL{RemovePropW(child,InputOwner);return TRUE;},0);impl_->window=nullptr;}
    auto *player=impl_->player,*media=impl_->media;impl_->player=impl_->media=nullptr;
    for(int i=impl_->retiring.size()-1;i>=0;--i)if(impl_->retiring[i].isFinished())impl_->retiring.removeAt(i);
    if(player || media){
        const auto stopped=QtConcurrent::run([this,player,media]{if(player){impl_->libvlc_media_player_stop(player);impl_->libvlc_media_player_release(player);}if(media)impl_->libvlc_media_release(media);});impl_->retiring.append(stopped);
        // Keep the previous HWND alive while its vout stops; the new media uses
        // another surface and starts immediately without waiting for teardown.
        if(retiredSurface){retiredSurface->hide();auto *watcher=new QFutureWatcher<void>(retiredSurface);QObject::connect(watcher,&QFutureWatcher<void>::finished,retiredSurface,&QObject::deleteLater);watcher->setFuture(stopped);return;}
    }
    // Shutdown still joins all retired players while processing HWND messages.
    for(const auto &stopped:impl_->retiring){if(stopped.isFinished())continue;QFutureWatcher<void> watcher;QEventLoop wait;QObject::connect(&watcher,&QFutureWatcher<void>::finished,&wait,&QEventLoop::quit);watcher.setFuture(stopped);if(!watcher.isFinished())wait.exec(QEventLoop::ExcludeUserInputEvents);}impl_->retiring.clear();
}
bool PlayerDiscMenu::active()const{return impl_->player!=nullptr;}
QString PlayerDiscMenu::error()const{QMutexLocker lock(&impl_->mutex);return impl_->failure;}
int PlayerDiscMenu::state()const{return active()?impl_->libvlc_media_player_get_state(impl_->player):0;}
void PlayerDiscMenu::navigate(unsigned command){if(active())impl_->libvlc_media_player_navigate(impl_->player,command);}
void PlayerDiscMenu::topMenu(){if(active())impl_->libvlc_media_player_set_title(impl_->player,0);}
void PlayerDiscMenu::pause(bool paused){if(active())impl_->libvlc_media_player_set_pause(impl_->player,paused);}
void PlayerDiscMenu::seek(qint64 ticks){if(active())impl_->libvlc_media_player_set_time(impl_->player,ticks/10000);}
void PlayerDiscMenu::rate(float value){if(active())impl_->libvlc_media_player_set_rate(impl_->player,value);}
void PlayerDiscMenu::volume(int value,bool muted){if(active()){if(impl_->lastVolume!=value){impl_->libvlc_audio_set_volume(impl_->player,value);impl_->lastVolume=value;}if(impl_->lastMute!=int(muted)){impl_->libvlc_audio_set_mute(impl_->player,muted);impl_->lastMute=int(muted);}}}
qint64 PlayerDiscMenu::position()const{return active()?qMax<qint64>(0,impl_->libvlc_media_player_get_time(impl_->player))*10000:0;}
qint64 PlayerDiscMenu::duration()const{return active()?qMax<qint64>(0,impl_->libvlc_media_player_get_length(impl_->player))*10000:0;}
QVector<QPair<int,QString>> PlayerDiscMenu::tracks(bool audio)const{QVector<QPair<int,QString>> out;if(!active())return out;auto *head=audio?impl_->libvlc_audio_get_track_description(impl_->player):impl_->libvlc_video_get_spu_description(impl_->player);for(auto *t=head;t;t=t->next)out.append({t->id,QString::fromUtf8(t->name)});impl_->libvlc_track_description_list_release(head);return out;}
void PlayerDiscMenu::selectTrack(bool audio,int id){if(active()){if(audio)impl_->libvlc_audio_set_track(impl_->player,id);else impl_->libvlc_video_set_spu(impl_->player,id);}}
quint64 PlayerDiscMenu::presentedFrames()const{Impl::Stats stats{};return active() && impl_->libvlc_media_get_stats(impl_->media,&stats)?qMax(0,stats.presented):0;}
QJsonObject PlayerDiscMenu::programme()const {
    if(!active())return {};const int title=impl_->libvlc_media_player_get_title(impl_->player);Impl::Chapter **chapters=nullptr;
    const int count=impl_->libvlc_media_player_get_full_chapter_descriptions(impl_->player,title,&chapters);QJsonArray starts;
    for(int i=0;i<count;++i)starts<<double(chapters[i]->offset*10000);if(chapters)impl_->libvlc_chapter_descriptions_release(chapters,qMax(0,count));
    unsigned width=0,height=0;impl_->libvlc_video_get_size(impl_->player,0,&width,&height);
    return {{"title",title},{"chapter",impl_->libvlc_media_player_get_chapter(impl_->player)},{"duration100ns",double(duration())},{"chapters",starts},{"width",int(width)},{"height",int(height)},{"fps",impl_->libvlc_media_player_get_fps(impl_->player)},{"audio",impl_->libvlc_audio_get_track(impl_->player)},{"subtitle",impl_->libvlc_video_get_spu(impl_->player)}};
}
bool PlayerDiscMenu::snapshot(const QString &path)const {return active() && impl_->libvlc_video_take_snapshot(impl_->player,0,path.toUtf8().constData(),0,0)==0;}
}
