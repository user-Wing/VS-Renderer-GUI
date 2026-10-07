#include "player/PlayerWindow.h"
#include "bluray/BlurayCatalog.h"
#include "player/PlayerInfoPanel.h"
#include "player/PlayerChromePanel.h"
#include "player/PlayerDiscMenu.h"
#include "player/PlayerTracks.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "backend/FrameTimeline.h"
#include "graph/PresetStore.h"
#include "ui/PreviewPane.h"
#include "player/PlayerSubtitles.h"
#include "player/PlayerNetworkInput.h"
#include "player/PlayerCache.h"
#include "player/PlayerLanguage.h"
#include "player/PlayerImage.h"
#include "player/PlayerAudioMetadata.h"
#include "player/PlayerImageTools.h"
#include "player/PlayerAssociations.h"
#include <QCryptographicHash>
#include <QSplitter>
#include <QWheelEvent>
#include <QRegularExpression>
#include <QScreen>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QMoveEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QUrl>
#include <QContextMenuEvent>
#include <QToolTip>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <climits>

namespace vsr {
namespace {
QString timeText(qint64 ticks) {
    const auto ms = std::max<qint64>(0, ticks / 10000);
    return QString("%1:%2:%3.%4").arg(ms / 3600000, 2, 10, QChar('0')).arg(ms / 60000 % 60, 2, 10, QChar('0')).arg(ms / 1000 % 60, 2, 10, QChar('0')).arg(ms % 1000, 3, 10, QChar('0'));
}

}
class ChapterTimeline final : public QSlider {
public:
    explicit ChapterTimeline(QWidget *parent) : QSlider(Qt::Horizontal, parent) { setRange(0, 100000); setMouseTracking(true); }

protected:
    int valueAt(const QPointF &point) const {
        QStyleOptionSlider option;initStyleOption(&option);const auto groove=style()->subControlRect(QStyle::CC_Slider,&option,QStyle::SC_SliderGroove,this);const auto handle=style()->subControlRect(QStyle::CC_Slider,&option,QStyle::SC_SliderHandle,this);
        return QStyle::sliderValueFromPosition(minimum(),maximum(),qRound(point.x())-groove.left()-handle.width()/2,qMax(1,groove.width()-handle.width()),option.upsideDown);
    }
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()!=Qt::LeftButton){QSlider::mousePressEvent(event);return;}setSliderDown(true);setValue(valueAt(event->position()));event->accept();
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if(event->button()!=Qt::LeftButton){QSlider::mouseReleaseEvent(event);return;}setValue(valueAt(event->position()));setSliderDown(false);event->accept();
    }
    void paintEvent(QPaintEvent *event) override {
        QSlider::paintEvent(event); const auto duration = property("duration").toLongLong();
        const auto chapters = property("chapters").toJsonArray(); if (duration <= 0) return;
        QStyleOptionSlider option; initStyleOption(&option);
        const auto groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        QPainter painter(this); painter.setPen(QPen(QColor("#8ab7ff"), 2));
        for (const auto &value : chapters) {
            const auto chapter = value.toObject();
            const int x = groove.left() + static_cast<int>(chapter.value("start100ns").toDouble() / duration * groove.width());
            painter.drawLine(x, groove.center().y() - 4, x, groove.center().y() + 4);
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if(isSliderDown() && (event->buttons() & Qt::LeftButton)){setValue(valueAt(event->position()));event->accept();}else QSlider::mouseMoveEvent(event); const auto duration = property("duration").toLongLong();
        const auto chapters = property("chapters").toJsonArray(); if (duration <= 0) return;
        for (const auto &value : chapters) { const auto chapter = value.toObject();
            if (std::abs(event->position().x() - chapter.value("start100ns").toDouble() / duration * width()) < 8)
                QToolTip::showText(event->globalPosition().toPoint(), chapter.value("title").toString(), this);
        }
    }
};
PlayerWindow::PlayerWindow() {
    settings_ = std::make_unique<QSettings>(QDir(QCoreApplication::applicationDirPath()).filePath("player.ini"), QSettings::IniFormat);
    language_=std::make_unique<PlayerLanguage>(settings_->value("basic/language","zh_CN").toString());
    if (!settings_->contains("player/core")) settings_->setValue("player/core", QSettings().value("player/core", "3FP"));
    if(settings_->value("theme/latinFont").toString()=="Segoe UI")settings_->setValue("theme/latinFont","Comic Sans MS");
    const QVariantMap defaults{{"basic/language","zh_CN"},{"basic/autoplay",true},{"theme/chineseFont","Microsoft YaHei UI"},{"theme/latinFont","Comic Sans MS"},{"theme/background","#202124"},{"theme/opacity",100},{"playback/remember",true},{"playback/multithread",true},{"playback/arrows","seconds"},{"playback/ctrlSeconds",10},{"playback/ctrlAltSeconds",60},{"cache/path",QString()},{"decode/video",settings_->value("player/core","3FP")},{"decode/audio",settings_->value("player/core","3FP")},{"player/renderer","VS"},{"player/animeStage",0},{"player/speed",1.0},{"player/volume",100},{"player/muted",false},{"performance/predecode",true},{"performance/cpu",100},{"performance/gpu",100},{"performance/ram",50},{"performance/vram",50},{"performance/frames",8},{"performance/resizeBeforeEnhance",false},{"player/wheel","volume"},{"decode/mode",2},{"decode/output",QString()},{"render/upscale",4},{"render/downscale",4},{"render/antiring",true},{"subtitle/visible",true},{"subtitle/style/font","Comic Sans MS"},{"subtitle/style/size",36},{"subtitle/style/color","#ffffff"},{"subtitle/style/outlineColor","#000000"},{"subtitle/style/shadowColor","#000000"},{"subtitle/style/outline",2},{"subtitle/style/shadow",2},{"subtitle/style/alignment",2},{"subtitle/style/margin",48},{"subtitle/style/left",40},{"subtitle/style/right",40},{"subtitle/style/scaleX",100},{"subtitle/style/scaleY",100},{"subtitle/style/spacing",0}};
    for(auto it=defaults.begin();it!=defaults.end();++it)if(!settings_->contains(it.key()))settings_->setValue(it.key(),it.value());
    preset_=settings_->value("player/preset").toString();
    if (!QFileInfo::exists(preset_)) preset_.clear();
    interpolationAuto_=interpolationStage()>=0 && settings_->value("player/interpolationAuto",false).toBool();
    subtitleVisible_=settings_->value("subtitle/visible",true).toBool();
    setWindowTitle("VS Player"); resize(1280, 760); setMinimumSize(1000, 500); setAcceptDrops(true);
    setStyleSheet(QStringLiteral(
        "QMainWindow,QDialog,QWidget{background:#202124;color:#e8eaed;}"
        "QPushButton,QComboBox,QLineEdit,QDoubleSpinBox{background:#303238;border:1px solid #50535a;border-radius:3px;padding:4px;}"
        "QPushButton:hover,QComboBox QAbstractItemView::item:hover{background:#426da7;}"
        "QPushButton:checked{background:#507fbd;} QSlider::groove:horizontal{height:4px;background:#555;}"
        "QSlider::sub-page:horizontal{background:#8ab4f8;} QSlider::handle:horizontal{background:#e5e7eb;width:10px;margin:-4px 0;border-radius:5px;}"
        "QMenu{background:#292b30;border:1px solid #555;} QMenu::item{padding:6px 36px 6px 12px;} QMenu::right-arrow{width:8px;height:8px;margin-right:8px;} QMenu::item:selected{background:#426da7;}"));
    setProperty("playerBaseStyle",styleSheet());
    auto *central = new QWidget(this); auto *layout = new QVBoxLayout(central); layout->setContentsMargins(0, 0, 0, 6); layout->setSpacing(4); setCentralWidget(central);
    pane_ = new PreviewPane("VS Player", "VS", central); pane_->setChromeVisible(false); pane_->setPlaceholderText(tr("VS Player\n拖入视频，或按 Ctrl+O 打开\nCtrl+P 加载 VPY 预设 · Tab 视频信息")); videoSplit_=new QSplitter(Qt::Horizontal,central);videoSplit_->setObjectName("playerVideoSplit");videoSplit_->addWidget(pane_);layout->addWidget(videoSplit_,1);
    hidden_ = new QWidget(central); hidden_->setAttribute(Qt::WA_NativeWindow); hidden_->resize(320, 180); hidden_->hide();
    clock_ = std::make_unique<ThreeFpPlayer>(api_, hidden_); output_ = std::make_unique<ThreeFpPlayer>(api_, pane_->surface()); output_->setMuted(true); clock_->setAutomaticHdr(true); output_->setAutomaticHdr(true);
    server_ = std::make_unique<VapourSynthFrameServer>(nullptr,false);
    network_=std::make_unique<PlayerNetworkInput>();
    connect(network_.get(),&PlayerNetworkInput::ready,this,[this](const QString &url){mediaInput_=url;openMedia();});
    connect(network_.get(),&PlayerNetworkInput::errorOccurred,this,[this](const QString &message){autoPlay_=false;setError(message);});
    subtitles_ = std::make_unique<PlayerSubtitles>();
    connect(subtitles_.get(), &PlayerSubtitles::errorOccurred, this, &PlayerWindow::setError);
    connect(subtitles_.get(), &PlayerSubtitles::imageReady, this, [this](const QImage &image) {
        if(audioMode_){if(!externalSubtitle_.isEmpty() || !externalSecondarySubtitle_.isEmpty()){audioSubtitle_->setGeometry(pane_->surface()->rect());auto pixmap=QPixmap::fromImage(image);pixmap.setDevicePixelRatio(pane_->surface()->devicePixelRatioF());audioSubtitle_->setPixmap(pixmap);audioSubtitle_->show();audioSubtitle_->raise();}return;}
        if (madvrMode()) { if(lav_) lav_->setSubtitle(image); } else (direct_?clock_:output_)->setSubtitle(image);
    });
    pane_->surface()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pane_->surface(), &QWidget::customContextMenuRequested, this, [this](const QPoint &at) { showContextMenu(pane_->surface()->mapToGlobal(at)); });
    info_ = new PlayerInfoPanel(pane_->surface());info_->hide();
    audioInfo_=new PlayerInfoPanel(pane_->surface());audioInfo_->setObjectName("playerAudioInfoPanel");audioInfo_->setRightSide(true);audioInfo_->hide();
    auto *audioMode=static_cast<PlayerInfoPanel *>(info_)->audioMode();audioMode->setCurrentIndex(settings_->value("info/audioDetailed",false).toBool()?1:0);
    connect(audioMode,&QComboBox::currentIndexChanged,this,[this](int index){settings_->setValue("info/audioDetailed",index==1);updateInfo();});
    fullscreenTitle_=new PlayerChromePanel(central);fullscreenTitle_->setObjectName("playerFullscreenTitle");
    auto *titleLayout=new QHBoxLayout(fullscreenTitle_);fullscreenTitleText_=new QLabel(fullscreenTitle_);titleLayout->addWidget(fullscreenTitleText_,1);
    auto *exitFullscreen=new QPushButton(tr("退出全屏"),fullscreenTitle_);exitFullscreen->setObjectName("playerExitFullscreen");titleLayout->addWidget(exitFullscreen);connect(exitFullscreen,&QPushButton::clicked,this,&PlayerWindow::toggleFullscreen);fullscreenTitle_->hide();
    lyricLabel_=new QLabel(pane_->surface());lyricLabel_->setObjectName("playerLyrics");lyricLabel_->setAttribute(Qt::WA_NativeWindow);lyricLabel_->setTextFormat(Qt::PlainText);lyricLabel_->setAlignment(Qt::AlignCenter);lyricLabel_->setWordWrap(true);lyricLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);lyricLabel_->setStyleSheet("background:rgba(0,0,0,160);color:white;padding:8px;font-size:20px;");lyricLabel_->hide();
    audioSubtitle_=new QLabel(pane_->surface());audioSubtitle_->setObjectName("playerAudioSubtitle");audioSubtitle_->setAttribute(Qt::WA_TransparentForMouseEvents);audioSubtitle_->setStyleSheet("background:transparent;");audioSubtitle_->hide();
    dragHint_=new QLabel(pane_->surface());dragHint_->setObjectName("playerDropHint");dragHint_->setAttribute(Qt::WA_NativeWindow);dragHint_->setAttribute(Qt::WA_TransparentForMouseEvents);dragHint_->setStyleSheet("background:#1d334d;color:#ffffff;padding:16px;border:1px solid #6da8ec;border-radius:8px;");dragHint_->hide();
    resetZoom_=new QPushButton(tr("还原画面"),pane_->surface());resetZoom_->setObjectName("playerResetZoom");resetZoom_->setAttribute(Qt::WA_NativeWindow);resetZoom_->move(8,8);resetZoom_->hide();
    interpolationWarning_=new QLabel(tr("补帧性能不足，建议关闭补帧。"),pane_->surface());interpolationWarning_->setObjectName("playerInterpolationWarning");interpolationWarning_->setAttribute(Qt::WA_NativeWindow);interpolationWarning_->setAttribute(Qt::WA_TransparentForMouseEvents);interpolationWarning_->setStyleSheet("background:#3d2f15;color:#ffd780;padding:8px;border:1px solid #b98d38;");interpolationWarning_->move(12,12);interpolationWarning_->hide();
    resetZoom_->setToolTip(tr("恢复默认适配比例，并清除拖动偏移"));
    connect(resetZoom_,&QPushButton::clicked,this,[this]{pane_->adoptView(1,0,0);emit pane_->viewChanged(1,0,0);});
    timeline_ = new ChapterTimeline(this); timeline_->setObjectName("playerTimeline");
    controls_=new PlayerChromePanel(central);controls_->setObjectName("playerControls");auto *transport=new QVBoxLayout(controls_);transport->setContentsMargins(0,0,0,0);transport->setSpacing(4);layout->addWidget(controls_);buildTransport(transport);
    message_ = new QLabel(controls_); message_->setObjectName("playerStatus"); message_->setContentsMargins(8, 0, 8, 0); message_->setText(tr("就绪")); transport->addWidget(message_);
    buildPlaylist(central);
    imageTools_=new PlayerImageTools(pane_,central);layout->insertWidget(0,imageTools_);
    connect(imageTools_,&PlayerImageTools::errorOccurred,this,&PlayerWindow::setError);
    connect(imageTools_,&PlayerImageTools::statusChanged,message_,&QLabel::setText);
    connect(imageTools_,&PlayerImageTools::imageRemoved,this,[this](const QString &path){
        if(source_!=path)return;
        const int index=files_.indexOf(path);files_.removeAll(path);
        if(!files_.isEmpty()){openFile(files_[std::clamp(index,0,int(files_.size())-1)]);return;}
        imageLoader_->cancel();source_.clear();imageMode_=ready_=false;pane_->setImage({});pane_->setSurfaceActive(false);pane_->setPlaceholderText(tr("图片已移入回收站"));imageTools_->setSource({});infoVisible_=false;info_->hide();resetZoom_->hide();setWindowTitle("VS Player");message_->setText(tr("图片已移入回收站"));updatePlaylist();
    });
    imageLoader_=std::make_unique<PlayerImage>();
    connect(imageLoader_.get(),&PlayerImage::loaded,this,[this](const QImage &image){
        if(!imageMode_)return;
        clip_.width=image.width();clip_.height=image.height();ready_=true;
        pane_->setImage(image);pane_->surface()->setFocus();
        imageTools_->setReady(true);
        rendererBadge_->setText("Image");videoBadge_->setText(QFileInfo(source_).suffix().toUpper());audioBadge_->setText("—");decoderBadge_->setText(QFileInfo(source_).suffix().compare("avif",Qt::CaseInsensitive)==0?"libavif / dav1d":"Image");hdrBadge_->setText("—");
        message_->setText(tr("图片：%1 × %2 · 左右键切图 · 滚轮缩放").arg(image.width()).arg(image.height()));
        updateInfo();
    });
    connect(imageLoader_.get(),&PlayerImage::failed,this,[this](const QString &error){if(imageMode_){const auto message=tr("图片解码失败：%1").arg(error);setError(message);pane_->setPlaceholderText(message);}});
    chromeTimer_=new QTimer(this);chromeTimer_->setInterval(100);connect(chromeTimer_,&QTimer::timeout,this,&PlayerWindow::updateChrome);chromeTimer_->start();
    connect(clock_.get(), &ThreeFpPlayer::errorOccurred, this, &PlayerWindow::setError);
    connect(output_.get(), &ThreeFpPlayer::errorOccurred, this, &PlayerWindow::setError);
    connect(server_.get(), &VapourSynthFrameServer::initialized, this, [this](bool ok) {
        if (!ok && !imageMode_ && !madvrMode() && !networkSource() && !direct_) { setError(server_->errorString()); return; }
          if (!deferred_.isEmpty()) { const auto path = deferred_; const auto subtitle=deferredSubtitle_;const int slot=deferredSubtitleSlot_;const auto audio=deferredAudio_; deferred_.clear(); deferredSubtitle_.clear();deferredAudio_.clear(); openFile(path); if(!subtitle.isEmpty())attachSubtitle(subtitle,slot);if(!audio.isEmpty())attachAudio(audio); }
          else if(ok && !direct_ && !source_.isEmpty())refreshScript();
    });
    connect(server_.get(), &VapourSynthFrameServer::errorOccurred, this, [this](const QString &error) { if(imageMode_ || madvrMode() || direct_) return; if(advanceInterpolation())return; ready_ = pending_ = false; autoPlay_ = false; clock_->pause(); if (lav_) lav_->pause(); playing_ = false; setError(error); });
    connect(server_.get(), &VapourSynthFrameServer::scriptLoaded, this, [this](const VapourSynthClipInfo &clip, const VapourSynthClipInfo &) {
        if(imageMode_ || madvrMode() || direct_) return;
        clip_ = clip; if(resumeAt_>=0){clock_->seek(resumeAt_);resumeAt_=-1;} ready_ = true; pending_ = false; requested_ = lastFrame_ = -1; pane_->setVideoSize(QSize(clip.width, clip.height)); pane_->setSurfaceActive(true); requestFrame(frameAtPosition100ns(position(), clip.totalFrames, clip.fpsNumerator, clip.fpsDenominator));
    });
    connect(server_.get(), &VapourSynthFrameServer::frameReady, this, [this](const VapourSynthFrame &frame) {
        pending_ = false;
        if (!ready_ || seekPending_ || frame.frameIndex != requested_) return;
        if(imageMode_ || madvrMode() || direct_) return;
        if (playing_ && lastFrame_ >= 0 && frame.frameIndex > lastFrame_ + 1) skippedFrames_ += frame.frameIndex - lastFrame_ - 1;
        const double milliseconds=frameTimer_.isValid()?frameTimer_.elapsed():0;
        frameMilliseconds_=submittedFrames_?frameMilliseconds_*.9+milliseconds*.1:milliseconds;
        ++submittedFrames_; displayedFrame_=frame;
        if (!output_->submitFrame(frame)) { setError(output_->lastError()); return; }
        lastFrame_ = static_cast<int>(frame.frameIndex);
        message_->clear();
        if (autoPlay_ && seekUiTarget_<0 && (lav_ || clock_->snapshot().state == ThreeFpState::Ready || clock_->snapshot().state == ThreeFpState::Paused)) { togglePlayback(); if (playing_) autoPlay_ = false; }
        const int target=frameAtPosition100ns(position(),clip_.totalFrames,clip_.fpsNumerator,clip_.fpsDenominator);
        if(playing_ && frame.frameIndex+1<clip_.totalFrames && frame.frameIndex<target)requestFrame(target);
    });
    connect(pane_, &PreviewPane::viewChanged, this, [this](float z, float x, float y) { resetZoom_->setVisible(z!=1 || x!=0 || y!=0);if(resetZoom_->isVisible())resetZoom_->raise();if(imageMode_){pane_->adoptView(z,x,y);updateInfo();return;}suspendQualityCheck();if(!madvrMode()) (direct_?clock_:output_)->setView(z, x, y); });
    connect(pane_, &PreviewPane::redrawRequested, this, [this] { if(imageMode_){pane_->surface()->update();return;}if(madvrMode()) { if(lav_) { const auto size=pane_->surface()->size()*pane_->surface()->devicePixelRatioF();lav_->resizeVideo(size.width(),size.height()); } } else (direct_?clock_:output_)->redraw();
        if(ready_)suspendQualityCheck();if(!direct_ && !profile().isEmpty() && interpolationStage()<0 && ready_ && !networkSource())profileResize_->start(); });
    ensureProfiles();
    profileResize_=new QTimer(this);profileResize_->setSingleShot(true);profileResize_->setInterval(350);
    connect(profileResize_,&QTimer::timeout,this,[this]{if(!direct_ && !source_.isEmpty() && !profile().isEmpty() && !media_.isEmpty() && profileTarget()!=profileSize_) {resumeAt_=position();autoPlay_=playing_ || autoPlay_;refreshScript();}});
    applySettings(false);
    qApp->installEventFilter(this);
    timer_ = new QTimer(this); timer_->setTimerType(Qt::PreciseTimer);timer_->setInterval(interpolationStage()>=0?4:10); connect(timer_, &QTimer::timeout, this, &PlayerWindow::updateState); timer_->start();
}
PlayerWindow::~PlayerWindow() { qApp->removeEventFilter(this); timer_->stop();timelinePreview_->stop();savePosition(); settings_->sync(); network_->cancel(); discMenu_.reset(); subtitles_.reset(); lav_.reset(); clock_->stop(); server_.reset(); network_.reset(); output_.reset(); clock_.reset(); }
void PlayerWindow::moveEvent(QMoveEvent *event){QMainWindow::moveEvent(event);if(chromeTimer_){layoutChrome();layoutInfoPanels();}}
bool PlayerWindow::openFile(const QString &input) {
    if(discMenu_ && discMenu_->active()){discMenu_->close(discSurface_);discSurface_=nullptr;}
    decoderBadge_->setEnabled(true);decoderBadge_->setToolTip(tr("切换 3FP 硬件 / 软件解码"));
    const QUrl url(input);const bool remote=url.scheme()=="http" || url.scheme()=="https";
    const QString path=url.isLocalFile()?url.toLocalFile():remote?url.toString(QUrl::FullyEncoded):input;
    if(!remote && QFileInfo(path).isDir())return openFolder(path);
    if ((remote && (url.host().isEmpty() || !url.isValid())) || (!remote && !QFileInfo(path).isFile())) { setError(tr("文件或链接无效：%1").arg(path)); return false; }
    const auto suffix=QFileInfo(path).suffix().toLower();
    if(suffix=="m2ts"){const auto issue=BlurayCatalog::clipPlaybackIssue(path);if(!issue.isEmpty()){setError(issue);return false;}}
    if(!remote && QStringList{"iso","img","vhd","vhdx"}.contains(suffix))return openBluRay(path);
    if(!remote && suffix=="bdmv")return openBluRay(QFileInfo(path).absolutePath());
    if (QStringList{"ass","ssa","srt","sup","mks"}.contains(suffix)) { attachSubtitle(path); return true; }
    if (path.endsWith(".vpy", Qt::CaseInsensitive)) { loadPreset(path); return true; }
    const bool image=!remote && PlayerImage::supports(path);
    if(clock_->snapshot().state==ThreeFpState::Opening){pendingMediaOpen_=path;return true;}
    pendingMediaOpen_.clear();
    timelinePreview_->stop();timelineTarget_=timelineSeekTarget_=-1;timelineDragging_=timelineResume_=timelineWaiting_=false;
    seekUiTarget_=queuedSeek_=-1;
    if (suffix!="mpls" && !playerAudioExtensions().contains(suffix) && !image && !remote && !madvrMode() && !preset_.isEmpty() && profile()!="Realistic" && fixedAnimeStage()<4 && server_->initializing()) { deferred_ = path; deferredSubtitle_.clear();deferredAudio_.clear(); message_->setText(tr("正在初始化 VS，完成后自动打开。")); return true; }
    savePosition(); imageLoader_->cancel();deferred_.clear();deferredAudio_.clear();network_->cancel(); server_->unloadScript();
    imageMode_=image;audioMode_=!image && playerAudioExtensions().contains(suffix);audioMetadataLoaded_=false;audioMetadata_={};setProperty("audioOnly",audioMode_);setProperty("lyricCount",0);lyricLabel_->clear();lyricLabel_->hide();pane_->setImage({});pane_->adoptView(1,0,0);resetZoom_->hide();
    imageTools_->setSource(image?QFileInfo(path).absoluteFilePath():QString());
    for(auto *control:QList<QWidget *>{timeline_,play_,time_,frame_,rate_,speedButton_})control->setEnabled(!image);
    lav_.reset(); clock_->pause(); playing_ = false; autoPlay_ = settings_->value("basic/autoplay",true).toBool(); ready_ = pending_ = seekPending_ = rateApplied_ = false;
    if(interpolationAuto_){const QStringList stages{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};preset_=QDir(PresetStore::directory()).filePath("builtin/"+stages[std::clamp(settings_->value("player/interpolationStart",0).toInt(),0,3)]);if(settings_->value("player/interpolationRenderer",0).toInt()==1)preset_.replace(".vpy","-D3D11.vpy");}
    frameEditPending_=false;frameJumpTarget_=-1;profileFallback_=false;profileStartup_.invalidate();
    manualFrame_ = -1;clip_={};positionRestored_=false;qualityStage_=initialQualityStage();resumeAt_=settingsPosition_=-1;profileSize_={};resetStatistics(); displayedFrame_={};
    const auto actual=remote?path:QFileInfo(path).absoluteFilePath();
    const auto mounted=actual==source_?externalSubtitle_:QString();
    externalSubtitle_.clear();externalSecondarySubtitle_.clear();externalAudio_.clear();pendingExternalAudio_.clear();matchedTracks_=false;audioDelay_=0; primarySubtitle_=secondarySubtitle_=-1;
    subtitles_->load(0,{},-1,{},{});subtitles_->load(1,{},-1,{},{});
    externalChapters_={};externalSubtitleTracks_[0]={};externalSubtitleTracks_[1]={};externalSubtitleIndex_[0]=externalSubtitleIndex_[1]=-1;audioSubtitle_->hide();
    source_ = actual; media_ = {}; setWindowTitle("VS Player — " + (remote?url.fileName():QFileInfo(path).fileName()));
    setProperty("discSelectedAudioPid",QVariant());setProperty("discSelectedSubtitlePid",QVariant());
    blurayMetadata_=suffix=="mpls"?BlurayCatalog::metadata(path):QJsonObject();
    if(blurayLabels_.contains(source_))setWindowTitle("VS Player — "+blurayLabels_.value(source_));
    if(!mounted.isEmpty())attachSubtitle(mounted);
    if (image && (playlistDirectory_.isEmpty() || !files_.contains(source_))) {
        files_.clear();playlistDirectory_.clear();const auto directory=QFileInfo(source_).absoluteDir();
        for(const auto &entry:directory.entryInfoList(QDir::Files,QDir::Name|QDir::IgnoreCase))if(PlayerImage::supports(entry.absoluteFilePath()))files_<<entry.absoluteFilePath();
    } else if (!files_.contains(source_)) {
        files_.clear();playlistDirectory_.clear(); const QDir directory = QFileInfo(source_).absoluteDir();
        if(!remote)for (const auto &file : directory.entryList({"*.mkv", "*.mp4", "*.mov", "*.avi", "*.webm", "*.ts", "*.m2ts", "*.wmv", "*.flv"}, QDir::Files, QDir::Name)) files_ << directory.filePath(file);
        if (!files_.contains(source_)) files_ << source_;
    }
    fileIndex_ = files_.indexOf(source_);
    updatePlaylist();
    if(remote) {mediaInput_.clear();clock_->stop();network_->open(url);message_->setText(tr("正在解析视频链接…"));return true;}
    mediaInput_=source_;
    if(suffix=="mpls"){
        if(QDir(QFileInfo(path).absolutePath()).dirName().compare("PLAYLIST",Qt::CaseInsensitive)!=0){setError(tr("MPLS 须位于 BDMV/PLAYLIST 中"));return false;}
        const auto root=QFileInfo(QFileInfo(path).absolutePath()).absolutePath();
        const auto playlists=QDir(QFileInfo(path).absolutePath()).entryList({"*.mpls"},QDir::Files);
        if(playlists.size()!=1)return openBluRay(root);
        QString error;mediaInput_=BlurayCatalog::playbackInput(path,&error);if(mediaInput_.isEmpty()){setError(error);return false;}
    }
    if(image){clock_->stop();output_->stop();profileResize_->stop();autoPlay_=false;playing_=false;play_->setText(QStringLiteral("▶"));pane_->setSurfaceActive(false);pane_->setPlaceholderText(tr("正在解码图片…"));timeline_->setValue(0);timeline_->setProperty("chapters",QJsonArray());timeline_->setProperty("duration",0);time_->setText("00:00:00.000");frame_->setText("0");duration_->setText("/ —");message_->setText(tr("正在解码图片…"));imageLoader_->open(source_);return true;}
    return openMedia();
}
bool PlayerWindow::openMedia() {
    setDirectMode(mediaInput_.startsWith("bluray:") || networkSource() || (!madvrMode() && (preset_.isEmpty() || (!profile().isEmpty() && interpolationStage()<0) || playerAudioExtensions().contains(QFileInfo(source_).suffix().toLower()))));
    clock_->setClockOnly(audioMode_ || (!direct_ && !madvrMode()));
    if (!clock_->openFile(mediaInput_)) { setError(clock_->lastError()); return false; }
    applyBlurayMetadata();
    lavVideo_ = !audioMode_ && !networkSource() && (madvrMode() || (!direct_ && settings_->value("decode/video",settings_->value("player/core","3FP")).toString()=="LAV"));
    lavAudio_ = QFileInfo(source_).suffix().compare("mpls",Qt::CaseInsensitive)!=0 && !networkSource() && settings_->value("decode/audio",settings_->value("player/core","3FP")).toString()=="LAV" && speed_==1;
    useLav_=lavVideo_ || lavAudio_;
    if (useLav_) {
        lav_ = std::make_unique<LavPlayback>();
        if (!lav_->open(mediaInput_,madvrMode()?reinterpret_cast<void *>(pane_->surface()->winId()):nullptr,lavVideo_,lavAudio_)) { setError(lav_->error()); lav_.reset(); autoPlay_ = false; return false; }
    }
    clock_->setMuted(lavAudio_ || mute_->isChecked()); clock_->setVolume(volume_->value() / 100.0f);
    if (lav_) lav_->volume(volume_->value() / 100.0f, !lavAudio_ || mute_->isChecked());
    if(madvrMode() && !audioMode_ && !networkSource()) {
        server_->unloadScript();ready_=true; output_->stop(); pane_->setSurfaceActive(true);
        const auto size=pane_->surface()->size()*pane_->surface()->devicePixelRatioF();lav_->resizeVideo(size.width(),size.height());
        if(speed_!=1) { lav_->setRate(speed_); lav_->volume(0,true);clock_->setMuted(mute_->isChecked()); }
        rendererBadge_->setText("madVR");message_->setText(tr("madshi video renderer · LAV DirectShow 输入"));
    } else { rendererBadge_->setText(direct_?"3FP":"VS"); if(direct_) {ready_=true;pane_->setSurfaceActive(true);if(preset_.isEmpty())message_->clear();} else refreshScript(); } return true;
}
void PlayerWindow::refreshScript() {
    if ((discMenu_ && discMenu_->active()) || imageMode_ || audioMode_ || source_.isEmpty() || mediaInput_.isEmpty() || (madvrMode() && !networkSource())) return;
    const bool resume=autoPlay_ || playing_;
    if(resumeAt_<0 && ready_)resumeAt_=position();
    resetStatistics();
    clock_->pause(); if (lav_) lav_->pause(); playing_ = false; ready_ = pending_ = false;
    const auto mode=profile();
    bool high=false;for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()=="video")high=stream.value("width").toInt()>=3840 || stream.value("height").toInt()>=2160;}
    bool audioOnly=playerAudioExtensions().contains(QFileInfo(source_).suffix().toLower());
    if(!audioMode_ && !media_.value("streams").toArray().isEmpty()){audioOnly=true;for(const auto &entry:media_.value("streams").toArray())if(entry.toObject().value("type").toString()=="video")audioOnly=false;}
    const bool native=mediaInput_.startsWith("bluray:") || networkSource() || audioOnly || preset_.isEmpty() || mode=="Realistic" || (mode=="Anime" && ((high && fixedAnimeStage()<0) || qualityStage_>=4));
    if(native && qualityStage_<4)qualityStage_=4;
    setDirectMode(native);
    applyScaling();
    profileSize_=profileTarget();autoPlay_=resume;
    if(direct_) {server_->unloadScript();ready_=true;lastFrame_=-1;applyScaling();message_->clear();return;}
    server_->initialize();
    if(server_->initializing()){message_->setText(tr("正在初始化 VS，完成后加载滤镜。"));return;}
    output_->resetVideoOutput(); output_->setMuted(true);
    const auto vsSource=mediaInput_;
    auto result = preset_.isEmpty() ? VpyScriptBuilder::build(vsSource, SourceFilter::Ffms2, FilterGraph()) : PresetStore::load(preset_, vsSource);
    if (!result.errors.isEmpty()) { setError(result.errors.join('\n')); autoPlay_ = false; return; }
    result.script.replace(QRegularExpression("(?<![A-Za-z_])(?:vs\\.)?core\\.ffms2\\.Source\\("),"_vsr_player_ffms(");
    result.script.replace(QRegularExpression("(?<![A-Za-z_])(?:vs\\.)?core\\.lsmas\\.LWLibavSource\\("),"_vsr_player_lsmas(");
    const auto ffindex=playerIndexPath(*settings_,vsSource,"ffindex"),lwi=playerIndexPath(*settings_,vsSource,"lwi");
    const auto literal=[](QString text){return "'"+text.replace('\\',"/").replace("'","\\'")+"'";};
    if(vsSource.endsWith(".ffconcat") && source_.endsWith(".mpls")){
        QFile file(source_);BlurayPlaylist playlist;QString error;
        if(!file.open(QIODevice::ReadOnly) || !BlurayCatalog::parse(file.readAll(),&playlist,&error)){setError(error);return;}
        QString parts;QHash<QString,double> starts;
        const auto bdmv=QFileInfo(QFileInfo(source_).absolutePath()).absolutePath();
        for(const auto &part:playlist.parts){
            const auto path=QDir(bdmv).filePath("STREAM/"+part.clip+".m2ts");
            if(!starts.contains(path)){
                const auto probe=PlayerAudioMetadata::probe(path);bool found=false;
                for(const auto &entry:probe.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("codec_type").toString()=="video"){starts.insert(path,stream.value("start_time").toString().toDouble());found=true;break;}}
                if(!found){setError(tr("无法读取 BD 片段视频时间：%1").arg(path));return;}
            }
            parts+=QString("(%1, %2, %3, %4, %5),\n").arg(literal(path),QString::number(part.in/45000.-starts.value(path),'f',9),QString::number((part.out-part.in)/45000.,'f',9),literal(playerIndexPath(*settings_,path,"ffindex")),literal(playerIndexPath(*settings_,path,"lwi")));
        }
        result.script.replace("_vsr_player_ffms(","_vsr_bd_ffms(");result.script.replace("_vsr_player_lsmas(","_vsr_bd_lsmas(");
        result.script.prepend(QString("def _vsr_bd_source(loader, args, options, lsmas):\n    source = args[0] if args else options.get('source')\n    if source != %1: return loader(*args, **options)\n    options = dict(options)\n    options.pop('source', None)\n    options.pop('cachedir', None)\n    clips = []\n    for path, start, duration, ffindex, lwi in [%2]:\n        opts = dict(options)\n        index = lwi if lsmas else ffindex\n        opts['cache'] = int(bool(index))\n        opts['cachefile'] = index\n        opts.setdefault('threads', %3)\n        clip = loader(path, *args[1:], **opts)\n        first = max(0, round(start * clip.fps_num / clip.fps_den))\n        count = round(duration * clip.fps_num / clip.fps_den)\n        clips.append(clip[first:first + count])\n    return _vsr_vs.core.std.Splice(clips)\ndef _vsr_bd_ffms(*args, **options):\n    return _vsr_bd_source(_vsr_vs.core.ffms2.Source, args, options, False)\ndef _vsr_bd_lsmas(*args, **options):\n    return _vsr_bd_source(_vsr_vs.core.lsmas.LWLibavSource, args, options, True)\n").arg(literal(vsSource),parts,settings_->value("playback/multithread",true).toBool()?"0":"1"));
    }
    result.script.prepend(QString("import vapoursynth as _vsr_vs\ndef _vsr_player_ffms(*args, **options):\n    options['cache'] = %1\n    options['cachefile'] = %2\n    options.setdefault('threads', %5)\n    try:\n        return _vsr_vs.core.ffms2.Source(*args, **options)\n    except _vsr_vs.Error as error:\n        if 'Source: No video track found' not in str(error): raise\n        if 'track' in options: options['stream_index'] = options.pop('track')\n        return _vsr_player_lsmas(*args, **options)\ndef _vsr_player_lsmas(*args, **options):\n    options.pop('cachedir', None)\n    options['cache'] = %3\n    options['cachefile'] = %4\n    options.setdefault('threads', %5)\n    return _vsr_vs.core.lsmas.LWLibavSource(*args, **options)\n")
        .arg(ffindex.isEmpty()?"False":"True",literal(ffindex),lwi.isEmpty()?"0":"1",literal(lwi),settings_->value("playback/multithread",true).toBool()?"0":"1"));
    if(!mode.isEmpty() && interpolationStage()<0)result.script.prepend(QString("_vsr_target_width = %1\n_vsr_target_height = %2\n_vsr_quality_stage = %3\n_vsr_resize_before_enhance = %4\n").arg(profileSize_.width()).arg(profileSize_.height()).arg(qualityStage_).arg(settings_->value("performance/resizeBeforeEnhance",false).toBool()?"True":"False"));
    if(mode=="Anime" && QFileInfo(source_).size()>1000000000 &&
        (ffindex.isEmpty() || !QFileInfo::exists(ffindex)) && (lwi.isEmpty() || !QFileInfo::exists(lwi))){
        profileFallback_=true;qualityStage_=4;refreshScript();
        message_->setText(tr("首次 VS 索引需要扫描大文件：已切换 Jinc 直通，避免机械盘全片读取。"));return;
    }
    requested_=lastFrame_=-1;profileStartup_.restart();
    server_->loadScript(result.script, preset_.isEmpty() ? QDir(QCoreApplication::applicationDirPath()).filePath("vpy/player.vpy") : preset_);
    message_->setText(preset_.isEmpty() ? tr("正在加载 VS 原画播放…") : tr("正在加载 VPY：%1").arg(QFileInfo(preset_).fileName()));
    if(QUrl(source_).scheme()=="http" || QUrl(source_).scheme()=="https")message_->setText(tr("正在读取网络视频并建立帧索引；首次索引需要扫描视频…"));
}
bool PlayerWindow::networkSource() const {const auto scheme=QUrl(source_).scheme();return scheme=="http" || scheme=="https";}
void PlayerWindow::savePosition() {
    if(imageMode_ || source_.isEmpty() || !ready_ || !positionRestored_ || (seekPending_ && clock_->snapshot().timelineGeneration==generation_) || resumeAt_>=0 || !settings_->value("playback/remember",true).toBool())return;
    const auto key=QString::fromLatin1(QCryptographicHash::hash(source_.toUtf8(),QCryptographicHash::Sha256).toHex());
    settings_->setValue("positions/"+key,clock_->snapshot().state==ThreeFpState::Ended?0:position());
}
void PlayerWindow::loadPreset(const QString &path) {
    if(madvrMode()) { const auto text=tr("工作在 madVR 模式下，VS 预设不生效。");setError(text);QToolTip::showText(pane_->mapToGlobal(QPoint(20,20)),text,pane_,QRect(),4000);return; }
    interpolationAuto_=false;settings_->setValue("player/interpolationAuto",false);
    profileFallback_=false;profileStartup_.invalidate();preset_ = path; qualityStage_=initialQualityStage();settings_->setValue("player/preset",path); if (!source_.isEmpty() && !imageMode_) refreshScript();
}
void PlayerWindow::togglePlayback() {
    if(discMenu_ && discMenu_->active()){playing_=!playing_;discMenu_->pause(!playing_);play_->setText(playing_?"Ⅱ":"▶");return;}
    if (imageMode_ || !ready_ || source_.isEmpty()) return;
    suspendQualityCheck();
    if (!playing_) manualFrame_ = -1;
    const bool ok = playing_ ? (lav_ ? lav_->pause() : clock_->pause()) : (lav_ ? lav_->play() : clock_->play());
    if(lav_) {if(playing_)clock_->pause();else clock_->play();}
    if (ok) { playing_ = !playing_; play_->setText(playing_ ? QStringLiteral("Ⅱ") : QStringLiteral("▶")); }
}
qint64 PlayerWindow::position() const { if(discMenu_ && discMenu_->active())return discMenu_->position(); if (!playing_ && manualFrame_ >= 0 && clip_.fpsNumerator > 0) return static_cast<qint64>(std::llround(manualFrame_ * 10000000.0 * clip_.fpsDenominator / clip_.fpsNumerator)); return lav_ ? lav_->position() : clock_->snapshot().position100ns; }
ThreeFpSnapshot PlayerWindow::snapshot() const { return clock_->snapshot(); }
ThreeFpSnapshot PlayerWindow::outputSnapshot() const { return direct_?clock_->snapshot():output_->snapshot(); }
void PlayerWindow::seekTime(qint64 time) {
    if(discMenu_ && discMenu_->active()){discMenu_->seek(time);return;}
    if(imageMode_)return;
    frameJumpTarget_=-1;
    if(!direct_ && !lav_ && playing_){togglePlayback();autoPlay_=true;}
    manualFrame_ = -1;resumeAt_=-1;suspendQualityCheck();
    lastFrame_=-1;
    const auto snap = clock_->snapshot(); time = std::clamp<qint64>(time, 0, std::max<qint64>(0, snap.duration100ns));
    seekUiTarget_=time;
    if(snap.duration100ns>0 && !timeline_->isSliderDown()){QSignalBlocker blocker(timeline_);timeline_->setValue(int(time*100000/snap.duration100ns));}
    if(seekPending_){queuedSeek_=time;return;}
    seekPresented_=outputSnapshot().presentedVideoFrames;
    if (lav_) { lav_->seek(time); clock_->seek(time); requested_ = lastFrame_ = -1; }
    else { generation_ = snap.timelineGeneration; seekPending_ = clock_->seek(time); }
}
void PlayerWindow::seekFrame(qint64 frame) {
    if (clip_.fpsNumerator <= 0 || !ready_) return;
    if (playing_) togglePlayback();
    const auto bounded = std::clamp<qint64>(frame, 0, clip_.totalFrames - 1);
    if(direct_){resumeAt_=-1;suspendQualityCheck();generation_=clock_->snapshot().timelineGeneration;seekPending_=clock_->seekFrame(bounded);}
    else seekTime(static_cast<qint64>(std::llround(bounded * 10000000.0 * clip_.fpsDenominator / clip_.fpsNumerator)));
    manualFrame_ = static_cast<int>(bounded);
}
bool PlayerWindow::commitFrameEdit() {
    if(!frameEditPending_)return false;
    bool ok=false;const auto frame=frame_->text().toLongLong(&ok);
    if(ok && ready_ && clip_.fpsNumerator>0){frameJumpPresents_=outputSnapshot().swapChainPresents;seekFrame(frame);frameJumpTarget_=manualFrame_;frameEditPending_=false;}
    else setError(tr("帧号无效或视频尚未就绪。"));
    return true;
}
void PlayerWindow::requestFrame(int frame) {
    if (direct_ || !ready_ || pending_ || frame < 0 || frame == lastFrame_) return;
    int ahead=(playing_ || autoPlay_)?prefetchCount():timelineDragging_?std::min(2,prefetchCount()):0;
    // MVTools reuses neighbouring motion fields; avoid abandoning a small
    // lookahead window, but resync once its latency budget is exceeded.
    if(playing_ && lastFrame_>=0 && interpolationStage()>=2 && ahead>0 && frame>lastFrame_ && frame-lastFrame_<=std::max(ahead,qRound(.15*clip_.fpsNumerator/qMax(1,clip_.fpsDenominator))))frame=lastFrame_+1;
    requested_ = frame; pending_ = true; frameTimer_.restart(); server_->requestFrame(frame, ahead, autoPlay_ || lastFrame_<0);
}
double PlayerWindow::normalizedRate(double rate) { return std::round(std::clamp(rate, 0.1, 16.0) * 20) / 20; }
void PlayerWindow::setRate(double rate) {
    suspendQualityCheck();
    if(discMenu_ && discMenu_->active())discMenu_->rate(normalizedRate(rate));
    speed_ = normalizedRate(rate); { QSignalBlocker blocker(rate_); rate_->setValue(speed_); }
    settings_->setValue("player/speed",speed_);
    if (source_.isEmpty() || imageMode_) return;
    if (madvrMode() && lav_) {
        const auto at=position(); if(!lav_->setRate(speed_)) {setError(lav_->error());return;}
        lav_->volume(volume_->value()/100.0f,speed_!=1 || mute_->isChecked());
        clock_->setMuted(speed_==1 || mute_->isChecked());clock_->seek(at);clock_->setPlaybackRate(speed_);
        if(playing_ && speed_!=1) clock_->play();else clock_->pause();return;
    }
    if (lav_ && speed_ != 1) {
        const auto at = position(); const bool resume = playing_; lav_.reset(); useLav_ = lavVideo_ = lavAudio_ = false;
        clock_->setMuted(mute_->isChecked()); clock_->seek(at); clock_->setPlaybackRate(speed_); if (resume) clock_->play();
        message_->setText(tr("不变调倍速使用 3FP / atempo；LAV 配置保留。"));
    } else if (!lav_) clock_->setPlaybackRate(speed_);
}
void PlayerWindow::nextFile(int direction) {
    int next=fileIndex_+direction;
    if(imageMode_)while(next>=0 && next<files_.size() && !PlayerImage::supports(files_[next]))next+=direction;
    if(imageMode_ && direction>0 && next>=files_.size() && !files_.isEmpty()){
        next=0;while(next<files_.size() && !PlayerImage::supports(files_[next]))++next;if(next>=files_.size())return;openFile(files_[next]);
        auto *notice=findChild<QLabel *>("playerImageWrapNotice");
        if(!notice){notice=new QLabel(pane_->surface());notice->setObjectName("playerImageWrapNotice");notice->setAttribute(Qt::WA_NativeWindow);notice->setAttribute(Qt::WA_TransparentForMouseEvents);notice->setStyleSheet("background:#292b30;color:white;padding:8px;");notice->move(12,12);}
        notice->move(12,imageTools_->isVisible()?imageTools_->height()+12:12);notice->setText(tr("已回到文件夹中第一张图"));notice->adjustSize();notice->show();notice->raise();QTimer::singleShot(2500,notice,&QWidget::hide);return;
    }
    while(next>=0 && next<files_.size() && blurayGroups_.value(files_[next])=="menu")next+=direction;
    if(next>=0 && next<files_.size())openFile(files_[next]);
}
void PlayerWindow::setError(const QString &message) { message_->setText(message); message_->setToolTip(message);message_->show(); }
void PlayerWindow::updateState() {
    if(!pendingMediaOpen_.isEmpty() && clock_->snapshot().state!=ThreeFpState::Opening){const auto path=pendingMediaOpen_;pendingMediaOpen_.clear();openFile(path);return;}
    if(discMenu_ && discMenu_->active()){
        if(handoffDiscProgramme())return;
        discSurface_->setGeometry(pane_->surface()->rect());const auto state=discMenu_->state();if(state==7){setError(tr("BD 播放失败：%1").arg(discMenu_->error()));playing_=false;play_->setText("▶");}
        if(!timelineDragging_){QSignalBlocker blocker(timeline_);const auto length=discMenu_->duration(),at=discMenu_->position();timeline_->setProperty("duration",length);timeline_->setValue(length>0?int(double(at)/length*100000):0);time_->setText(timeText(at));duration_->setText("/ "+timeText(length));}
        discMenu_->volume(volume_->value(),mute_->isChecked());if(infoVisible_ && ++infoTick_>=50){infoTick_=0;updateInfo();}return;
    }
    if(imageMode_){if(infoVisible_ && ++infoTick_>=50){infoTick_=0;updateInfo();}return;}
    if(!direct_ && profile()=="Anime" && lastFrame_<0 && profileStartup_.isValid() && profileStartup_.elapsed()>=8000){
        profileStartup_.invalidate();profileFallback_=true;qualityStage_=4;refreshScript();message_->setText(tr("VS 首帧等待超过 8 秒：已切换 Jinc 直通。"));
    }
    const auto snap = clock_->snapshot();
    if(!pendingExternalAudio_.isEmpty() && (snap.state==ThreeFpState::Ready || snap.state==ThreeFpState::Playing || snap.state==ThreeFpState::Paused)){const auto path=pendingExternalAudio_;pendingExternalAudio_.clear();attachAudio(path);}
    if(direct_ && resumeAt_>=0 && (snap.state==ThreeFpState::Ready || snap.state==ThreeFpState::Paused || snap.state==ThreeFpState::Playing)) {clock_->seek(resumeAt_);resumeAt_=-1;}
    if (snap.state == ThreeFpState::Ready && !rateApplied_) { rateApplied_ = true; clock_->setPlaybackRate(speed_); }
    if (!positionRestored_ && (snap.state == ThreeFpState::Ready || snap.state == ThreeFpState::Paused || snap.state == ThreeFpState::Playing)) {
        media_ = QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();
        applyBlurayMetadata();
        if(!networkSource())loadAudioMetadata();
        applyAudioEffects();
        timeline_->setProperty("chapters", media_.value("chapters").toArray());
        if(madvrMode() || direct_) { for(const auto &entry:media_.value("streams").toArray()) {const auto stream=entry.toObject();if(stream.value("type").toString()=="video") {clip_.width=stream.value("width").toInt();clip_.height=stream.value("height").toInt();clip_.fpsNumerator=stream.value("averageFrameRateNumerator").toInt();clip_.fpsDenominator=stream.value("averageFrameRateDenominator").toInt();clip_.totalFrames=frameAtPosition100ns(snap.duration100ns,INT_MAX,clip_.fpsNumerator,clip_.fpsDenominator)+1;pane_->setVideoSize(QSize(clip_.width,clip_.height));break;}} }
        if(!positionRestored_) {positionRestored_=true;
            if(settingsPosition_>=0){resumeAt_=settingsPosition_;settingsPosition_=-1;}
            else if(settings_->value("playback/remember",true).toBool()) {const auto key=QString::fromLatin1(QCryptographicHash::hash(source_.toUtf8(),QCryptographicHash::Sha256).toHex());const auto stored=settings_->value("positions/"+key,0).toLongLong();if(stored>0 && stored<snap.duration100ns-10000000)resumeAt_=stored;}
        }
        if(resumeAt_>=0 && ready_ && !direct_ && profile().isEmpty()){const auto stored=resumeAt_;resumeAt_=-1;seekTime(stored);}
        if(!audioMode_ && !madvrMode() && (!profile().isEmpty() || networkSource()))refreshScript();
        else if(!media_.value("streams").toArray().isEmpty()) {bool video=false;for(const auto &entry:media_.value("streams").toArray())video|=entry.toObject().value("type").toString()=="video";if(!video){setDirectMode(true);ready_=true;server_->unloadScript();}}
        const auto streams=media_.value("streams").toArray();int audio=playerDefaultTrack(streams,"audio"),subtitle=playerDefaultTrack(streams,"subtitle");
        for(const auto &value:streams){const auto stream=value.toObject();if(property("discSelectedAudioPid").isValid() && stream.value("type").toString()=="audio" && stream.value("streamId").toInt()==property("discSelectedAudioPid").toInt())audio=stream.value("index").toInt();if(property("discSelectedSubtitlePid").isValid() && stream.value("type").toString()=="subtitle" && stream.value("streamId").toInt()==property("discSelectedSubtitlePid").toInt())subtitle=stream.value("index").toInt();}
        if(property("discSelectedSubtitlePid").isValid() && property("discSelectedSubtitlePid").toInt()<0)subtitle=-1;
        setProperty("discSelectedAudioPid",QVariant());setProperty("discSelectedSubtitlePid",QVariant());
        if(audio>=0 && (audio!=snap.selectedAudioStream || lavAudio_) && externalAudio_.isEmpty())selectAudio(audio);
        matchExternalTracks();
        if(externalSubtitle_.isEmpty())selectSubtitle(0,subtitle);
    }
    if (autoPlay_ && !seekPending_ && seekUiTarget_<0 && (madvrMode() || direct_ || lastFrame_ >= 0) && (lav_ || snap.state == ThreeFpState::Ready || snap.state == ThreeFpState::Paused)) { togglePlayback(); if (playing_) autoPlay_ = false; }
    if (seekPending_ && snap.timelineGeneration != generation_ && (!direct_ || snap.selectedVideoStream<0 || (snap.presentedVideoFrames>seekPresented_ && snap.frameIndex>=0))) { seekPending_ = false; requested_ = lastFrame_ = -1;
        if(queuedSeek_>=0){const auto next=queuedSeek_;queuedSeek_=-1;seekTime(next);return;}
        if(direct_)seekUiTarget_=-1;
        if (lavKeyPending_ && lav_) { lavKeyPending_ = false; lav_->seek(snap.position100ns); } }
    if(!direct_ && !seekPending_ && seekUiTarget_>=0){const auto rendered=outputSnapshot();if(rendered.presentedVideoFrames>seekPresented_ && rendered.frameIndex==frameAtPosition100ns(seekUiTarget_,clip_.totalFrames,clip_.fpsNumerator,clip_.fpsDenominator))seekUiTarget_=-1;}
    const auto at = position(); const auto duration = snap.duration100ns;
    timeline_->setProperty("duration", duration);
    if (!time_->hasFocus()) time_->setText(timeText(seekUiTarget_>=0?seekUiTarget_:at)); duration_->setText("/ " + timeText(duration));
    frame_->setEnabled(!audioMode_);if (!frame_->hasFocus() && !frameEditPending_) frame_->setText(audioMode_?QStringLiteral("—"):QString::number(frameAtPosition100ns(at, clip_.totalFrames, clip_.fpsNumerator, clip_.fpsDenominator)));
    if (!timeline_->isSliderDown() && seekUiTarget_<0 && !seekPending_ && duration > 0) { QSignalBlocker blocker(timeline_); timeline_->setValue(static_cast<int>(at * 100000 / duration)); }
    if (!madvrMode() && !seekPending_ && ready_) {
        const int target=frameAtPosition100ns(at, clip_.totalFrames, clip_.fpsNumerator, clip_.fpsDenominator);
        requestFrame(target);
    }
    if(frameJumpTarget_>=0){const auto rendered=outputSnapshot();if(rendered.swapChainPresents>frameJumpPresents_ && rendered.frameIndex==frameJumpTarget_)frameJumpTarget_=-1;}
    if (playing_ && (snap.state == ThreeFpState::Ended || (lav_ && duration > 0 && at >= duration - 10000))) { playing_ = false; if (lav_) lav_->pause(); clock_->pause(); play_->setText(QStringLiteral("▶")); }
    if ((madvrMode() || direct_) && ready_) lastFrame_=frameAtPosition100ns(at,clip_.totalFrames,clip_.fpsNumerator,clip_.fpsDenominator);
    if(++subtitleTick_>=3 && !source_.isEmpty()) {subtitleTick_=0;if(audioMode_)updateAudioLyrics(at);if(!audioMode_ || !externalSubtitle_.isEmpty() || !externalSecondarySubtitle_.isEmpty())subtitles_->render(at,pane_->surface()->size()*pane_->surface()->devicePixelRatioF(),QSize(clip_.width,clip_.height),subtitleVisible_);else audioSubtitle_->hide();}
    updateProfile();
    if (++infoTick_ >= 50) { infoTick_ = 0; updateInfo(); }
    if(!positionTimer_.isValid() || positionTimer_.elapsed()>=5000){positionTimer_.restart();savePosition();}
}
bool PlayerWindow::eventFilter(QObject *object, QEvent *event) {
    if(event->type()==QEvent::MouseButtonPress)if(auto *widget=qobject_cast<QWidget *>(object);widget && (widget==controls_ || controls_->isAncestorOf(widget)))chromeIdle_.restart();
    if(event->type()==QEvent::MouseButtonPress && frameEditPending_ && isActiveWindow() && object!=frame_)commitFrameEdit();
    if(event->type()==QEvent::Wheel)if(auto *widget=qobject_cast<QWidget *>(object);widget && (widget==pane_ || pane_->isAncestorOf(widget)) && widget!=info_ && !info_->isAncestorOf(widget) && !imageMode_ && settings_->value("player/wheel","volume").toString()=="volume") {const int delta=static_cast<QWheelEvent *>(event)->angleDelta().y();if(delta)volume_->setValue(std::clamp(volume_->value()+(delta>0?5:-5),0,100));return true;}
    if(event->type()==QEvent::ContextMenu)if(auto *widget=qobject_cast<QWidget *>(object);widget && (widget==pane_ || pane_->isAncestorOf(widget))) {showContextMenu(static_cast<QContextMenuEvent *>(event)->globalPos());return true;}
    if (event->type() == QEvent::KeyPress && (isActiveWindow() || qApp->activeWindow()==controls_ || qApp->activeWindow()==playlistPanel_ || qApp->activeWindow()==fullscreenTitle_)) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab && !source_.isEmpty()) { infoVisible_ = !infoVisible_;infoFpsTimer_.invalidate(); updateInfo(); if(infoVisible_){info_->show();info_->raise();}else {static_cast<PlayerInfoPanel *>(info_)->hidePanel();audioInfo_->hidePanel();} return true; }
        const bool editing = qApp->activePopupWidget() || qobject_cast<QComboBox *>(object) || qobject_cast<QComboBox *>(qApp->focusWidget()) || qobject_cast<QLineEdit *>(object) || qobject_cast<QDoubleSpinBox *>(object) || qobject_cast<QLineEdit *>(qApp->focusWidget()) || qobject_cast<QDoubleSpinBox *>(qApp->focusWidget());
        if(!editing && discMenu_ && discMenu_->active() && discMenuNavigation_){int command=-1;switch(key->key()){case Qt::Key_Return:case Qt::Key_Enter:command=0;break;case Qt::Key_Up:command=1;break;case Qt::Key_Down:command=2;break;case Qt::Key_Left:command=3;break;case Qt::Key_Right:command=4;break;case Qt::Key_Menu:command=5;break;default:break;}if(command>=0){discMenu_->navigate(command);return true;}}
        if(!editing && (key->key()==Qt::Key_Left || key->key()==Qt::Key_Right)) {
            const int direction=key->key()==Qt::Key_Left?-1:1;
            if(imageMode_){nextFile(direction);return true;}
            if(key->modifiers() & Qt::ControlModifier)seekTime(position()+qint64(settings_->value(key->modifiers() & Qt::AltModifier?"playback/ctrlAltSeconds":"playback/ctrlSeconds",key->modifiers() & Qt::AltModifier?60:10).toInt())*10000000*direction);
            else {const auto mode=settings_->value("playback/arrows","seconds").toString();if(mode=="frame")seekFrame(lastFrame_+direction);else if(mode=="keyframe"){suspendQualityCheck();manualFrame_=-1;if(lav_){clock_->seek(position());lavKeyPending_=true;}generation_=clock_->snapshot().timelineGeneration;seekPending_=clock_->stepKeyframe(direction);}else seekTime(position()+direction*10000000);}
            return true;
        }
        if (!editing && key->key() == Qt::Key_Space) { togglePlayback(); return true; }
        if((key->key()==Qt::Key_Return || key->key()==Qt::Key_Enter) && frameEditPending_){commitFrameEdit();return true;}
        if((key->key()==Qt::Key_Return || key->key()==Qt::Key_Enter) && frameJumpTarget_>=0)return true;
        if ((!editing && (key->key()==Qt::Key_Return || key->key()==Qt::Key_Enter)) || key->key()==Qt::Key_F11) {toggleFullscreen();return true;}
        if(key->key()==Qt::Key_Escape && isFullScreen()) {toggleFullscreen();return true;}
        if (key->modifiers() & Qt::ControlModifier) {
            if (key->key() == Qt::Key_O) { chooseFiles(); return true; }
            if (key->key() == Qt::Key_P) { const auto file = QFileDialog::getOpenFileName(this, tr("加载 VPY"), PresetStore::directory(), "VapourSynth (*.vpy)"); if (!file.isEmpty()) loadPreset(file); return true; }
        }
    }
    if(event->type()==QEvent::MouseButtonPress && object==speedButton_ && speedPopup_ && speedPopup_->isVisible()) {speedPopup_->close();return true;}
    return QMainWindow::eventFilter(object, event);
}
void PlayerWindow::dragEnterEvent(QDragEnterEvent *event) {
    if(event->mimeData()->hasUrls()){dragStatus_=message_->text();event->acceptProposedAction();}
}
void PlayerWindow::dragMoveEvent(QDragMoveEvent *event) {
    if(!event->mimeData()->hasUrls())return;
    const auto urls=event->mimeData()->urls();if(urls.isEmpty())return;
    const auto suffix=QFileInfo(urls.first().toLocalFile()).suffix().toLower();
    const auto area=pane_->surface()->mapFrom(this,event->position().toPoint());
    if(QStringList{"ass","ssa","srt","sup","vtt"}.contains(suffix))message_->setText(area.y()<pane_->surface()->height()*.4?tr("作为次字幕加载(顶部)"):tr("作为主字幕加载(底部)"));
    else if(playerAudioExtensions().contains(suffix))message_->setText(!imageMode_ && clock_->snapshot().selectedVideoStream>=0 && area.y()<pane_->surface()->height()*.5?tr("作为外部音频加载"):tr("关闭当前视频，单独播放音频"));
    dragHint_->setText(message_->text());dragHint_->adjustSize();dragHint_->move(qMax(0,(pane_->surface()->width()-dragHint_->width())/2),qMax(0,(pane_->surface()->height()-dragHint_->height())/2));dragHint_->show();dragHint_->raise();
    event->acceptProposedAction();
}
void PlayerWindow::dragLeaveEvent(QDragLeaveEvent *event) {dragHint_->hide();message_->setText(dragStatus_);event->accept();}
void PlayerWindow::dropEvent(QDropEvent *event) {
    dragHint_->hide();QStringList videos,audio,mounted;
    const auto area=pane_->surface()->mapFrom(this,event->position().toPoint());
    for(const auto &url:event->mimeData()->urls())if(url.isLocalFile() || url.scheme()=="http" || url.scheme()=="https") {
        const auto path=url.isLocalFile()?url.toLocalFile():url.toString(QUrl::FullyEncoded),suffix=QFileInfo(path).suffix().toLower();
        if(suffix=="vpy")loadPreset(path);else if(QStringList{"ass","ssa","srt","sup","vtt"}.contains(suffix))mounted<<path;else if(playerAudioExtensions().contains(suffix))audio<<path;else videos<<path;
    }
    message_->setText(dragStatus_);
    if(!videos.isEmpty()){files_=videos;openFile(videos.first());}
    if(!audio.isEmpty()) {if(!imageMode_ && (!videos.isEmpty() || !deferred_.isEmpty() || clock_->snapshot().selectedVideoStream>=0) && area.y()<pane_->surface()->height()*.5)attachAudio(audio.first());else{files_=audio;openFile(audio.first());}}
    if(!mounted.isEmpty())attachSubtitle(mounted.first(),area.y()<pane_->surface()->height()*.4?1:0);
    event->acceptProposedAction();
}
}
