#include "player/PlayerWindow.h"
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
        QSlider::mouseMoveEvent(event); const auto duration = property("duration").toLongLong();
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
    const QVariantMap defaults{{"basic/language","zh_CN"},{"basic/autoplay",true},{"theme/chineseFont","Microsoft YaHei UI"},{"theme/latinFont","Segoe UI"},{"theme/background","#202124"},{"theme/opacity",100},{"playback/remember",true},{"playback/multithread",true},{"playback/arrows","seconds"},{"playback/ctrlSeconds",10},{"playback/ctrlAltSeconds",60},{"cache/path",QString()},{"decode/video",settings_->value("player/core","3FP")},{"decode/audio",settings_->value("player/core","3FP")},{"player/renderer","VS"},{"player/animeStage",0},{"player/speed",1.0},{"player/volume",100},{"player/muted",false},{"performance/predecode",true},{"performance/cpu",50},{"performance/gpu",50},{"performance/ram",50},{"performance/vram",50},{"performance/frames",8},{"performance/resizeBeforeEnhance",false},{"player/wheel","volume"},{"decode/mode",2},{"decode/output",QString()},{"render/upscale",4},{"render/downscale",4},{"render/antiring",true},{"subtitle/visible",true},{"subtitle/style/font","Segoe UI"},{"subtitle/style/size",36},{"subtitle/style/color","#ffffff"},{"subtitle/style/outlineColor","#000000"},{"subtitle/style/shadowColor","#000000"},{"subtitle/style/outline",2},{"subtitle/style/shadow",2},{"subtitle/style/alignment",2},{"subtitle/style/margin",48},{"subtitle/style/left",40},{"subtitle/style/right",40},{"subtitle/style/scaleX",100},{"subtitle/style/scaleY",100},{"subtitle/style/spacing",0}};
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
    server_ = std::make_unique<VapourSynthFrameServer>();
    network_=std::make_unique<PlayerNetworkInput>();
    connect(network_.get(),&PlayerNetworkInput::ready,this,[this](const QString &url){mediaInput_=url;openMedia();});
    connect(network_.get(),&PlayerNetworkInput::errorOccurred,this,[this](const QString &message){autoPlay_=false;setError(message);});
    subtitles_ = std::make_unique<PlayerSubtitles>();
    connect(subtitles_.get(), &PlayerSubtitles::errorOccurred, this, &PlayerWindow::setError);
    connect(subtitles_.get(), &PlayerSubtitles::imageReady, this, [this](const QImage &image) {
        if (madvrMode()) { if(lav_) lav_->setSubtitle(image); } else (direct_?clock_:output_)->setSubtitle(image);
    });
    pane_->surface()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pane_->surface(), &QWidget::customContextMenuRequested, this, [this](const QPoint &at) { showContextMenu(pane_->surface()->mapToGlobal(at)); });
    info_ = new QLabel(pane_->surface()); info_->setAttribute(Qt::WA_NativeWindow); info_->setTextInteractionFlags(Qt::NoTextInteraction); info_->setTextFormat(Qt::PlainText); info_->setWordWrap(true); info_->setStyleSheet("background:#17191e;color:#f4f4f4;padding:12px;font-family:Consolas;"); info_->hide();
    dragHint_=new QLabel(pane_->surface());dragHint_->setObjectName("playerDropHint");dragHint_->setAttribute(Qt::WA_NativeWindow);dragHint_->setAttribute(Qt::WA_TransparentForMouseEvents);dragHint_->setStyleSheet("background:#1d334d;color:#ffffff;padding:16px;border:1px solid #6da8ec;border-radius:8px;");dragHint_->hide();
    resetZoom_=new QPushButton(tr("还原画面"),pane_->surface());resetZoom_->setObjectName("playerResetZoom");resetZoom_->setAttribute(Qt::WA_NativeWindow);resetZoom_->move(8,8);resetZoom_->hide();
    resetZoom_->setToolTip(tr("恢复默认适配比例，并清除拖动偏移"));
    connect(resetZoom_,&QPushButton::clicked,this,[this]{pane_->adoptView(1,0,0);emit pane_->viewChanged(1,0,0);});
    timeline_ = new ChapterTimeline(this); timeline_->setObjectName("playerTimeline");
    controls_=new QWidget(central);controls_->setObjectName("playerControls");auto *transport=new QVBoxLayout(controls_);transport->setContentsMargins(0,0,0,0);transport->setSpacing(4);layout->addWidget(controls_);buildTransport(transport);
    message_ = new QLabel(controls_); message_->setObjectName("playerStatus"); message_->setContentsMargins(8, 0, 8, 0); message_->setText(tr("就绪")); transport->addWidget(message_);
    buildPlaylist(central);
    imageLoader_=std::make_unique<PlayerImage>();
    connect(imageLoader_.get(),&PlayerImage::loaded,this,[this](const QImage &image){
        if(!imageMode_)return;
        clip_.width=image.width();clip_.height=image.height();ready_=true;
        pane_->setImage(image);pane_->surface()->setFocus();
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
        message_->setText(preset_.isEmpty() ? tr("VS 原画播放") : tr("VS 预设：%1").arg(QFileInfo(preset_).fileName()));
        if (autoPlay_ && (lav_ || clock_->snapshot().state == ThreeFpState::Ready || clock_->snapshot().state == ThreeFpState::Paused)) { togglePlayback(); if (playing_) autoPlay_ = false; }
    });
    connect(pane_, &PreviewPane::viewChanged, this, [this](float z, float x, float y) { resetZoom_->setVisible(z!=1 || x!=0 || y!=0);if(resetZoom_->isVisible())resetZoom_->raise();if(imageMode_){pane_->adoptView(z,x,y);updateInfo();return;}suspendQualityCheck();if(!madvrMode()) (direct_?clock_:output_)->setView(z, x, y); });
    connect(pane_, &PreviewPane::redrawRequested, this, [this] { if(imageMode_){pane_->surface()->update();return;}if(madvrMode()) { if(lav_) { const auto size=pane_->surface()->size()*pane_->surface()->devicePixelRatioF();lav_->resizeVideo(size.width(),size.height()); } } else (direct_?clock_:output_)->redraw();
        if(ready_)suspendQualityCheck();if(!profile().isEmpty() && interpolationStage()<0 && ready_ && !networkSource())profileResize_->start(); });
    ensureProfiles();
    profileResize_=new QTimer(this);profileResize_->setSingleShot(true);profileResize_->setInterval(350);
    connect(profileResize_,&QTimer::timeout,this,[this]{if(!source_.isEmpty() && !profile().isEmpty() && !media_.isEmpty() && profileTarget()!=profileSize_) {resumeAt_=position();autoPlay_=playing_ || autoPlay_;refreshScript();}});
    applySettings(false);
    qApp->installEventFilter(this);
    timer_ = new QTimer(this); timer_->setInterval(10); connect(timer_, &QTimer::timeout, this, &PlayerWindow::updateState); timer_->start();
}
PlayerWindow::~PlayerWindow() { qApp->removeEventFilter(this); timer_->stop();savePosition(); settings_->sync(); network_->cancel(); subtitles_.reset(); lav_.reset(); clock_->stop(); server_.reset(); network_.reset(); output_.reset(); clock_.reset(); }
bool PlayerWindow::openFile(const QString &input) {
    const QUrl url(input);const bool remote=url.scheme()=="http" || url.scheme()=="https";
    const QString path=url.isLocalFile()?url.toLocalFile():remote?url.toString(QUrl::FullyEncoded):input;
    if(!remote && QFileInfo(path).isDir())return openFolder(path);
    if ((remote && (url.host().isEmpty() || !url.isValid())) || (!remote && !QFileInfo(path).isFile())) { setError(tr("文件或链接无效：%1").arg(path)); return false; }
    const auto suffix=QFileInfo(path).suffix().toLower();
    if (QStringList{"ass","ssa","srt","sup"}.contains(suffix)) { attachSubtitle(path); return true; }
    if (path.endsWith(".vpy", Qt::CaseInsensitive)) { loadPreset(path); return true; }
    const bool image=!remote && PlayerImage::supports(path);
    if (!image && !remote && !madvrMode() && server_->initializing()) { deferred_ = path; deferredSubtitle_.clear();deferredAudio_.clear(); message_->setText(tr("正在初始化 VS，完成后自动打开。")); return true; }
    savePosition(); imageLoader_->cancel();deferred_.clear();deferredAudio_.clear();network_->cancel(); server_->unloadScript();
    imageMode_=image;pane_->setImage({});pane_->adoptView(1,0,0);resetZoom_->hide();
    for(auto *control:QList<QWidget *>{timeline_,play_,time_,frame_,rate_,speedButton_})control->setEnabled(!image);
    lav_.reset(); clock_->pause(); playing_ = false; autoPlay_ = settings_->value("basic/autoplay",true).toBool(); ready_ = pending_ = seekPending_ = rateApplied_ = false;
    if(interpolationAuto_)preset_=QDir(PresetStore::directory()).filePath("builtin/Interpolation-0-RIFE.vpy");
    manualFrame_ = -1;clip_={};positionRestored_=false;qualityStage_=initialQualityStage();resumeAt_=settingsPosition_=-1;profileSize_={};resetStatistics(); displayedFrame_={};
    const auto actual=remote?path:QFileInfo(path).absoluteFilePath();
    const auto mounted=actual==source_?externalSubtitle_:QString();
    externalSubtitle_.clear();externalSecondarySubtitle_.clear();externalAudio_.clear();pendingExternalAudio_.clear();matchedTracks_=false;audioDelay_=0; primarySubtitle_=secondarySubtitle_=-1;
    subtitles_->load(0,{},-1,{},{});subtitles_->load(1,{},-1,{},{});
    source_ = actual; media_ = {}; setWindowTitle("VS Player — " + (remote?url.fileName():QFileInfo(path).fileName()));
    if(!mounted.isEmpty())attachSubtitle(mounted);
    if (image) {
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
    if(image){clock_->stop();output_->stop();profileResize_->stop();autoPlay_=false;playing_=false;play_->setText(QStringLiteral("▶"));pane_->setSurfaceActive(false);pane_->setPlaceholderText(tr("正在解码图片…"));timeline_->setValue(0);timeline_->setProperty("chapters",QJsonArray());timeline_->setProperty("duration",0);time_->setText("00:00:00.000");frame_->setText("0");duration_->setText("/ —");message_->setText(tr("正在解码图片…"));imageLoader_->open(source_);return true;}
    return openMedia();
}
bool PlayerWindow::openMedia() {
    setDirectMode(networkSource() || (!madvrMode() && (!profile().isEmpty() || playerAudioExtensions().contains(QFileInfo(source_).suffix().toLower()))));
    if (!clock_->openFile(mediaInput_)) { setError(clock_->lastError()); return false; }
    lavVideo_ = !networkSource() && (madvrMode() || (!direct_ && settings_->value("decode/video",settings_->value("player/core","3FP")).toString()=="LAV"));
    lavAudio_ = !networkSource() && settings_->value("decode/audio",settings_->value("player/core","3FP")).toString()=="LAV" && speed_==1;
    useLav_=lavVideo_ || lavAudio_;
    if (useLav_) {
        lav_ = std::make_unique<LavPlayback>();
        if (!lav_->open(mediaInput_,madvrMode()?reinterpret_cast<void *>(pane_->surface()->winId()):nullptr,lavVideo_,lavAudio_)) { setError(lav_->error()); lav_.reset(); autoPlay_ = false; return false; }
    }
    clock_->setMuted(lavAudio_ || mute_->isChecked()); clock_->setVolume(volume_->value() / 100.0f);
    if (lav_) lav_->volume(volume_->value() / 100.0f, !lavAudio_ || mute_->isChecked());
    if(madvrMode() && !networkSource()) {
        server_->unloadScript();ready_=true; output_->stop(); pane_->setSurfaceActive(true);
        const auto size=pane_->surface()->size()*pane_->surface()->devicePixelRatioF();lav_->resizeVideo(size.width(),size.height());
        if(speed_!=1) { lav_->setRate(speed_); lav_->volume(0,true);clock_->setMuted(mute_->isChecked()); }
        rendererBadge_->setText("madVR");message_->setText(tr("madshi video renderer · LAV DirectShow 输入"));
    } else { rendererBadge_->setText(networkSource()?"3FP":"VS"); if(direct_) {ready_=true;pane_->setSurfaceActive(true);} else refreshScript(); } return true;
}
void PlayerWindow::refreshScript() {
    if (imageMode_ || source_.isEmpty() || mediaInput_.isEmpty() || (madvrMode() && !networkSource())) return;
    const bool resume=autoPlay_ || playing_;
    if(resumeAt_<0 && ready_)resumeAt_=position();
    resetStatistics();
    clock_->pause(); if (lav_) lav_->pause(); playing_ = false; ready_ = pending_ = false;
    const auto mode=profile();
    bool high=false;for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()=="video")high=stream.value("width").toInt()>=3840 || stream.value("height").toInt()>=2160;}
    bool audioOnly=playerAudioExtensions().contains(QFileInfo(source_).suffix().toLower());
    if(!media_.value("streams").toArray().isEmpty()){audioOnly=true;for(const auto &entry:media_.value("streams").toArray())if(entry.toObject().value("type").toString()=="video")audioOnly=false;}
    const bool native=networkSource() || audioOnly || mode=="Realistic" || (mode=="Anime" && ((high && fixedAnimeStage()<0) || qualityStage_>=4));
    if(native && qualityStage_<4)qualityStage_=4;
    setDirectMode(native);
    applyScaling();
    profileSize_=profileTarget();autoPlay_=resume;
    if(direct_) {server_->unloadScript();ready_=true;lastFrame_=-1;applyScaling();message_->setText(networkSource()?tr("网络视频 · 3FPlayer 直通，VS 预设不生效"):(qualityStage_>=5 || (settings_->value("render/upscale",4).toInt()==7 && settings_->value("render/downscale",4).toInt()==7))?tr("D3D11 原生直通（不使用 Jinc）"):tr("%1 · Jinc 原生直通（无增强）").arg(mode));return;}
    output_->resetVideoOutput(); output_->setMuted(true);
    const auto vsSource=mediaInput_;
    auto result = preset_.isEmpty() ? VpyScriptBuilder::build(vsSource, SourceFilter::Ffms2, FilterGraph()) : PresetStore::load(preset_, vsSource);
    if (!result.errors.isEmpty()) { setError(result.errors.join('\n')); autoPlay_ = false; return; }
    result.script.replace(QRegularExpression("(?<![A-Za-z_])(?:vs\\.)?core\\.ffms2\\.Source\\("),"_vsr_player_ffms(");
    result.script.replace(QRegularExpression("(?<![A-Za-z_])(?:vs\\.)?core\\.lsmas\\.LWLibavSource\\("),"_vsr_player_lsmas(");
    const auto ffindex=playerIndexPath(*settings_,vsSource,"ffindex"),lwi=playerIndexPath(*settings_,vsSource,"lwi");
    const auto literal=[](QString text){return "'"+text.replace('\\',"/").replace("'","\\'")+"'";};
    result.script.prepend(QString("import vapoursynth as _vsr_vs\ndef _vsr_player_ffms(*args, **options):\n    options['cache'] = %1\n    options['cachefile'] = %2\n    options.setdefault('threads', %5)\n    try:\n        return _vsr_vs.core.ffms2.Source(*args, **options)\n    except _vsr_vs.Error as error:\n        if 'Source: No video track found' not in str(error): raise\n        if 'track' in options: options['stream_index'] = options.pop('track')\n        return _vsr_player_lsmas(*args, **options)\ndef _vsr_player_lsmas(*args, **options):\n    options.pop('cachedir', None)\n    options['cache'] = %3\n    options['cachefile'] = %4\n    options.setdefault('threads', %5)\n    return _vsr_vs.core.lsmas.LWLibavSource(*args, **options)\n")
        .arg(ffindex.isEmpty()?"False":"True",literal(ffindex),lwi.isEmpty()?"0":"1",literal(lwi),settings_->value("playback/multithread",true).toBool()?"0":"1"));
    if(!mode.isEmpty() && interpolationStage()<0)result.script.prepend(QString("_vsr_target_width = %1\n_vsr_target_height = %2\n_vsr_quality_stage = %3\n_vsr_resize_before_enhance = %4\n").arg(profileSize_.width()).arg(profileSize_.height()).arg(qualityStage_).arg(settings_->value("performance/resizeBeforeEnhance",false).toBool()?"True":"False"));
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
    preset_ = path; qualityStage_=initialQualityStage();settings_->setValue("player/preset",path); if (!source_.isEmpty() && !imageMode_) refreshScript();
}
void PlayerWindow::togglePlayback() {
    if (imageMode_ || !ready_ || source_.isEmpty()) return;
    suspendQualityCheck();
    if (!playing_) manualFrame_ = -1;
    const bool ok = playing_ ? (lav_ ? lav_->pause() : clock_->pause()) : (lav_ ? lav_->play() : clock_->play());
    if(lav_) {if(playing_)clock_->pause();else clock_->play();}
    if (ok) { playing_ = !playing_; play_->setText(playing_ ? QStringLiteral("Ⅱ") : QStringLiteral("▶")); }
}
qint64 PlayerWindow::position() const { if (!playing_ && manualFrame_ >= 0 && clip_.fpsNumerator > 0) return static_cast<qint64>(std::llround(manualFrame_ * 10000000.0 * clip_.fpsDenominator / clip_.fpsNumerator)); return lav_ ? lav_->position() : clock_->snapshot().position100ns; }
ThreeFpSnapshot PlayerWindow::snapshot() const { return clock_->snapshot(); }
ThreeFpSnapshot PlayerWindow::outputSnapshot() const { return direct_?clock_->snapshot():output_->snapshot(); }
void PlayerWindow::seekTime(qint64 time) {
    if(imageMode_)return;
    manualFrame_ = -1;resumeAt_=-1;suspendQualityCheck();
    lastFrame_=-1;
    const auto snap = clock_->snapshot(); time = std::clamp<qint64>(time, 0, std::max<qint64>(0, snap.duration100ns));
    if (lav_) { lav_->seek(time); clock_->seek(time); requested_ = lastFrame_ = -1; }
    else { generation_ = snap.timelineGeneration; seekPending_ = clock_->seek(time); }
}
void PlayerWindow::seekFrame(qint64 frame) {
    if (clip_.fpsNumerator <= 0 || !ready_) return;
    if (playing_) togglePlayback();
    const auto bounded = std::clamp<qint64>(frame, 0, clip_.totalFrames - 1);
    seekTime(static_cast<qint64>(std::llround(bounded * 10000000.0 * clip_.fpsDenominator / clip_.fpsNumerator)));
    if(direct_)clock_->seekFrame(bounded);
    manualFrame_ = static_cast<int>(bounded);
}
void PlayerWindow::requestFrame(int frame) {
    if (direct_ || !ready_ || pending_ || frame < 0 || frame == lastFrame_) return;
    requested_ = frame; pending_ = true; frameTimer_.restart(); server_->requestFrame(frame, playing_ ? prefetchCount() : 0);
}
double PlayerWindow::normalizedRate(double rate) { return std::round(std::clamp(rate, 0.1, 16.0) * 20) / 20; }
void PlayerWindow::setRate(double rate) {
    suspendQualityCheck();
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
void PlayerWindow::nextFile(int direction) { const int next = fileIndex_ + direction; if (next >= 0 && next < files_.size()) openFile(files_[next]); }
void PlayerWindow::setError(const QString &message) { message_->setText(message); message_->setToolTip(message); }
void PlayerWindow::updateState() {
    if(imageMode_){if(infoVisible_ && ++infoTick_>=50){infoTick_=0;updateInfo();}return;}
    const auto snap = clock_->snapshot();
    if(!pendingExternalAudio_.isEmpty() && (snap.state==ThreeFpState::Ready || snap.state==ThreeFpState::Playing || snap.state==ThreeFpState::Paused)){const auto path=pendingExternalAudio_;pendingExternalAudio_.clear();attachAudio(path);}
    if(direct_ && resumeAt_>=0 && (snap.state==ThreeFpState::Ready || snap.state==ThreeFpState::Paused || snap.state==ThreeFpState::Playing)) {clock_->seek(resumeAt_);resumeAt_=-1;}
    if (snap.state == ThreeFpState::Ready && !rateApplied_) { rateApplied_ = true; clock_->setPlaybackRate(speed_); }
    if (autoPlay_ && (madvrMode() || direct_ || lastFrame_ >= 0) && (lav_ || snap.state == ThreeFpState::Ready || snap.state == ThreeFpState::Paused)) { togglePlayback(); if (playing_) autoPlay_ = false; }
    if (!positionRestored_ && (snap.state == ThreeFpState::Ready || snap.state == ThreeFpState::Paused || snap.state == ThreeFpState::Playing)) {
        media_ = QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();
        applyAudioEffects();
        timeline_->setProperty("chapters", media_.value("chapters").toArray());
        if(madvrMode() || direct_) { for(const auto &entry:media_.value("streams").toArray()) {const auto stream=entry.toObject();if(stream.value("type").toString()=="video") {clip_.width=stream.value("width").toInt();clip_.height=stream.value("height").toInt();clip_.fpsNumerator=stream.value("averageFrameRateNumerator").toInt();clip_.fpsDenominator=stream.value("averageFrameRateDenominator").toInt();clip_.totalFrames=frameAtPosition100ns(snap.duration100ns,INT_MAX,clip_.fpsNumerator,clip_.fpsDenominator)+1;pane_->setVideoSize(QSize(clip_.width,clip_.height));break;}} }
        if(!positionRestored_) {positionRestored_=true;
            if(settingsPosition_>=0){resumeAt_=settingsPosition_;settingsPosition_=-1;}
            else if(settings_->value("playback/remember",true).toBool()) {const auto key=QString::fromLatin1(QCryptographicHash::hash(source_.toUtf8(),QCryptographicHash::Sha256).toHex());const auto stored=settings_->value("positions/"+key,0).toLongLong();if(stored>0 && stored<snap.duration100ns-10000000)resumeAt_=stored;}
        }
        if(resumeAt_>=0 && ready_ && !direct_ && profile().isEmpty()){const auto stored=resumeAt_;resumeAt_=-1;seekTime(stored);}
        if(!madvrMode() && (!profile().isEmpty() || networkSource()))refreshScript();
        else if(!media_.value("streams").toArray().isEmpty()) {bool video=false;for(const auto &entry:media_.value("streams").toArray())video|=entry.toObject().value("type").toString()=="video";if(!video){setDirectMode(true);ready_=true;server_->unloadScript();}}
        matchExternalTracks();
        if(externalSubtitle_.isEmpty()) for(const auto &entry:media_.value("streams").toArray()) {const auto stream=entry.toObject();if(stream.value("type").toString()=="subtitle") { selectSubtitle(0,stream.value("index").toInt());break;}}
    }
    if (seekPending_ && snap.timelineGeneration != generation_) { seekPending_ = false; requested_ = lastFrame_ = -1;
        if (lavKeyPending_ && lav_) { lavKeyPending_ = false; lav_->seek(snap.position100ns); } }
    const auto at = position(); const auto duration = snap.duration100ns;
    timeline_->setProperty("duration", duration);
    if (!time_->hasFocus()) time_->setText(timeText(at)); duration_->setText("/ " + timeText(duration));
    if (!frame_->hasFocus()) frame_->setText(QString::number(frameAtPosition100ns(at, clip_.totalFrames, clip_.fpsNumerator, clip_.fpsDenominator)));
    if (!timeline_->isSliderDown() && duration > 0) { QSignalBlocker blocker(timeline_); timeline_->setValue(static_cast<int>(at * 100000 / duration)); }
    if (!madvrMode() && !seekPending_ && ready_) requestFrame(frameAtPosition100ns(at, clip_.totalFrames, clip_.fpsNumerator, clip_.fpsDenominator));
    if (playing_ && (snap.state == ThreeFpState::Ended || (lav_ && duration > 0 && at >= duration - 10000))) { playing_ = false; if (lav_) lav_->pause(); clock_->pause(); play_->setText(QStringLiteral("▶")); }
    if ((madvrMode() || direct_) && ready_) lastFrame_=frameAtPosition100ns(at,clip_.totalFrames,clip_.fpsNumerator,clip_.fpsDenominator);
    if(++subtitleTick_>=3 && !source_.isEmpty()) {subtitleTick_=0;subtitles_->render(at,pane_->surface()->size()*pane_->surface()->devicePixelRatioF(),QSize(clip_.width,clip_.height),subtitleVisible_);}
    updateProfile();
    if (++infoTick_ >= 50) { infoTick_ = 0; updateInfo(); }
    if(!positionTimer_.isValid() || positionTimer_.elapsed()>=5000){positionTimer_.restart();savePosition();}
}
bool PlayerWindow::eventFilter(QObject *object, QEvent *event) {
    if(event->type()==QEvent::Wheel)if(auto *widget=qobject_cast<QWidget *>(object);widget && (widget==pane_ || pane_->isAncestorOf(widget)) && !imageMode_ && settings_->value("player/wheel","volume").toString()=="volume") {const int delta=static_cast<QWheelEvent *>(event)->angleDelta().y();if(delta)volume_->setValue(std::clamp(volume_->value()+(delta>0?5:-5),0,100));return true;}
    if(event->type()==QEvent::ContextMenu)if(auto *widget=qobject_cast<QWidget *>(object);widget && (widget==pane_ || pane_->isAncestorOf(widget))) {showContextMenu(static_cast<QContextMenuEvent *>(event)->globalPos());return true;}
    if (event->type() == QEvent::KeyPress && isActiveWindow()) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab && !source_.isEmpty()) { infoVisible_ = !infoVisible_; updateInfo(); info_->setVisible(infoVisible_); info_->raise(); return true; }
        const bool editing = qobject_cast<QLineEdit *>(object) || qobject_cast<QDoubleSpinBox *>(object) || qobject_cast<QLineEdit *>(qApp->focusWidget()) || qobject_cast<QDoubleSpinBox *>(qApp->focusWidget());
        if(!editing && (key->key()==Qt::Key_Left || key->key()==Qt::Key_Right)) {
            const int direction=key->key()==Qt::Key_Left?-1:1;
            if(imageMode_){nextFile(direction);return true;}
            if(key->modifiers() & Qt::ControlModifier)seekTime(position()+qint64(settings_->value(key->modifiers() & Qt::AltModifier?"playback/ctrlAltSeconds":"playback/ctrlSeconds",key->modifiers() & Qt::AltModifier?60:10).toInt())*10000000*direction);
            else {const auto mode=settings_->value("playback/arrows","seconds").toString();if(mode=="frame")seekFrame(lastFrame_+direction);else if(mode=="keyframe"){suspendQualityCheck();manualFrame_=-1;if(lav_){clock_->seek(position());lavKeyPending_=true;}generation_=clock_->snapshot().timelineGeneration;seekPending_=clock_->stepKeyframe(direction);}else seekTime(position()+direction*10000000);}
            return true;
        }
        if (!editing && key->key() == Qt::Key_Space) { togglePlayback(); return true; }
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
    if(QStringList{"ass","ssa","srt","sup","vtt"}.contains(suffix))message_->setText(area.y()<pane_->surface()->height()*.4?tr("作为次字幕加载（顶部）"):tr("作为主字幕加载（底部）"));
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
