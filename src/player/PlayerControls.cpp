#include "player/PlayerShortcuts.h"
#include "player/PlayerWindow.h"
#include "player/PlayerDiscMenu.h"
#include "player/PlayerChromePanel.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <QMenu>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QJsonArray>
#include "player/PlayerTracks.h"

namespace vsr {
namespace {
class RateSpinBox final : public QDoubleSpinBox {
public:
    explicit RateSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {}
protected:
    QValidator::State validate(QString &text, int &) const override {
        QString number = text; number.remove(QChar(0x00d7)); bool ok;
        const double value = number.trimmed().toDouble(&ok);
        return ok && std::isfinite(value) ? QValidator::Acceptable : QValidator::Intermediate;
    }
    double valueFromText(const QString &text) const override {
        QString number = text; number.remove(QChar(0x00d7)); bool ok;
        const double parsed = number.trimmed().toDouble(&ok);
        return ok && std::isfinite(parsed) ? PlayerWindow::normalizedRate(parsed) : value();
    }
};
}
void PlayerWindow::buildTransport(QVBoxLayout *layout) {
    auto *transport = transportGrid_ = new QGridLayout;transport->setContentsMargins(8,0,8,0);transport->setHorizontalSpacing(8);transport->setVerticalSpacing(4);transport->setColumnStretch(0,1);layout->addLayout(transport);
    auto *slider = timeline_;slider->setProperty("minimalControl",true);transport->addWidget(slider,0,0);
    mute_ = new QPushButton(this);mute_->setObjectName("playerMute");mute_->setCheckable(true);mute_->setFixedWidth(34);mute_->setIconSize(QSize(20,20));
    volume_ = new QSlider(Qt::Horizontal, this); volume_->setObjectName("playerVolume"); volume_->setRange(0, 200); volume_->setValue(100); volume_->setFixedWidth(130); volume_->setToolTip(tr("音量"));transport->addWidget(volume_,0,1);
    auto *volumePercent=new QLineEdit("100%",this);volumePercent->setFixedWidth(54);volumePercent->setAlignment(Qt::AlignRight);volumePercent->setObjectName("playerVolumePercent");volumePercent->setMinimumWidth(volumePercent->fontMetrics().horizontalAdvance("100%"));
    connect(volumePercent,&QLineEdit::editingFinished,this,[this,volumePercent]{bool ok=false;auto text=volumePercent->text();text.remove('%');const int value=text.toInt(&ok);if(ok)volume_->setValue(std::clamp(value,0,200));volumePercent->setText(QString::number(volume_->value())+"%");});
    const auto updateVolumeUi=[this,volumePercent]{volumePercent->setText(QString::number(volume_->value())+"%");mute_->setIcon(QIcon(mute_->isChecked()?":/ui/microphone-muted.svg":":/ui/microphone.svg"));mute_->setToolTip(mute_->isChecked()?tr("取消静音"):tr("静音"));mute_->setAccessibleName(mute_->toolTip());};updateVolumeUi();
    const auto volume = [this,updateVolumeUi] {updateVolumeUi();clock_->setVolume(volume_->value() / 100.0f); clock_->setMuted(lavAudio_ || mute_->isChecked()); if (lav_) lav_->volume(volume_->value() / 100.0f,!lavAudio_ || mute_->isChecked());settings_->setValue("player/volume",volume_->value());settings_->setValue("player/muted",mute_->isChecked()); };
    connect(volume_, &QSlider::valueChanged, this, volume); connect(mute_, &QPushButton::toggled, this, volume);
    equalizerButton_=new QPushButton("EQ",this);equalizerButton_->setObjectName("playerEqualizerButton");equalizerButton_->setToolTip(tr("均衡器"));equalizerButton_->setFixedWidth(34);auto *audioFooter=new QWidget(controls_);audioFooter->setObjectName("playerVolumeFooter");audioFooter->setProperty("minimalControl",true);auto *audioRow=new QHBoxLayout(audioFooter);audioRow->setContentsMargins(0,0,0,0);audioRow->setSpacing(4);audioRow->addWidget(mute_);audioRow->addWidget(equalizerButton_);audioRow->addWidget(volumePercent);transport->addWidget(audioFooter,1,1);connect(equalizerButton_,&QPushButton::clicked,this,&PlayerWindow::showEqualizer);
    timelinePreview_=new QTimer(this);timelinePreview_->setInterval(16);
    connect(timelinePreview_,&QTimer::timeout,this,[this]{
        if(!timelineDragging_ || timelineTarget_<0){timelinePreview_->stop();return;}
        const auto snap=clock_->snapshot();
        const auto rendered=outputSnapshot();
        if(timelineWaiting_ && (snap.timelineGeneration==generation_ || rendered.presentedVideoFrames<=timelinePresented_))return;
        const auto target=timelineTarget_;timelineTarget_=-1;timelinePresented_=rendered.presentedVideoFrames;
        seekTime(target);timelineSeekTarget_=target;timelineWaiting_=seekPending_;
    });
    connect(slider,&QSlider::sliderPressed,this,[this]{
        timelineDragging_=ready_ && ((discMenu_ && discMenu_->active()) || (!madvrMode() && clock_->snapshot().selectedVideoStream>=0));
        timelineResume_=timelineDragging_ && playing_;timelineWaiting_=false;timelineSeekTarget_=-1;
        if(timelineResume_)togglePlayback();
    });
    connect(slider, &QSlider::valueChanged, this, [this, slider](int value) {
        const auto target=(discMenu_ && discMenu_->active()?discMenu_->duration():clock_->snapshot().duration100ns) * value / 100000;
        if(slider->isSliderDown() && timelineDragging_){
            // Disc seeks can cross many clips; commit only the final drag position.
            if((discMenu_ && discMenu_->active()) || !blurayMetadata_.isEmpty() || blurayGroups_.contains(source_))return;
            timelineTarget_=target;if(!timelinePreview_->isActive())timelinePreview_->start();
        }
        else if(!slider->isSliderDown())seekTime(target);
    });
    connect(slider, &QSlider::sliderReleased, this, [this, slider] {
        timelinePreview_->stop();timelineDragging_=false;timelineTarget_=-1;timelineWaiting_=false;
        const auto target=(discMenu_ && discMenu_->active()?discMenu_->duration():clock_->snapshot().duration100ns) * slider->sliderPosition() / 100000;
        if(target!=timelineSeekTarget_)seekTime(target);timelineSeekTarget_=-1;
        if(timelineResume_ && !playing_){if(discMenu_ && discMenu_->active())togglePlayback();else autoPlay_=true;}timelineResume_=false;
    });
    auto *row = new QHBoxLayout; row->setContentsMargins(0, 0, 0, 0); row->setSpacing(4); transport->addLayout(row,1,0);
    const auto button = [this, row](const QString &text, const QString &tooltip, auto action) {
        auto *b = new QPushButton(text, this); b->setToolTip(tooltip); b->setFixedWidth(34); row->addWidget(b);b->setProperty("minimalControl",text=="▶" || text=="−1s" || text=="+1s"); connect(b, &QPushButton::clicked, this, action); return b;
    };
    play_ = button(QStringLiteral("▶"), tr("播放 / 暂停 · Space"), [this] { togglePlayback(); }); play_->setObjectName("playerPlay");
    button("|‹", tr("退一帧"), [this] { stepFrame(-1); })->setObjectName("playerPreviousFrame");
    button("›|", tr("进一帧"), [this] { stepFrame(1); })->setObjectName("playerNextFrame");
    button("−1s", tr("退一秒"), [this] { QKeyEvent key(QEvent::KeyPress,Qt::Key_Left,Qt::NoModifier);handleShortcut(&key); })->setObjectName("playerBackwardSecond");
    button("+1s", tr("进一秒"), [this] { QKeyEvent key(QEvent::KeyPress,Qt::Key_Right,Qt::NoModifier);handleShortcut(&key); })->setObjectName("playerForwardSecond");
    const auto keyframe = [this](int direction) {
        suspendQualityCheck();
        manualFrame_ = -1;
        if (playing_) togglePlayback();
        if (lav_) { clock_->seek(position()); lavKeyPending_ = true; }
        generation_ = clock_->snapshot().timelineGeneration; seekPending_ = clock_->stepKeyframe(direction);
    };
    button("K‹", tr("上一个关键帧"), [=] { keyframe(-1); })->setObjectName("playerPreviousKeyframe");
    button("›K", tr("下一个关键帧"), [=] { keyframe(1); })->setObjectName("playerNextKeyframe");
    button("|◀", tr("上一个视频"), [this] { nextFile(-1); })->setObjectName("playerPreviousFile");
    button("▶|", tr("下一个视频"), [this] { nextFile(1); })->setObjectName("playerNextFile");
    time_ = new QLineEdit("00:00:00.000", this); time_->setObjectName("playerPosition"); time_->setFixedWidth(110); time_->setToolTip(tr("输入 HH:MM:SS.mmm 或秒数，Enter 精确跳转")); row->addWidget(time_);time_->setProperty("minimalControl",true);
    connect(time_, &QLineEdit::returnPressed, this, [this] {
        const auto parts = time_->text().split(':'); bool ok = !parts.isEmpty() && parts.size() <= 3; double seconds = 0;
        for (const auto &part : parts) { bool valid; double value = part.toDouble(&valid); ok = ok && valid && value >= 0; seconds = seconds * 60 + value; }
        if (ok && std::isfinite(seconds) && seconds < 1e9) { seekTime(static_cast<qint64>(std::llround(seconds * 10000000))); time_->clearFocus(); } else setError(tr("时间格式无效。"));
    });
    duration_ = new QLabel("/ 00:00:00.000", this);duration_->setObjectName("playerDuration"); row->addWidget(duration_);
    frame_ = new QLineEdit("0", this); frame_->setObjectName("playerFrame"); frame_->setFixedWidth(65); frame_->setToolTip(tr("精确输出帧号，从 0 开始；Enter 跳转")); row->addWidget(frame_);
    connect(frame_, &QLineEdit::textEdited, this, [this] { frameEditPending_=true; });
    connect(frame_, &QLineEdit::editingFinished, this, [this] { commitFrameEdit(); });
    rate_ = new RateSpinBox(this); rate_->setObjectName("playerSpeed"); rate_->setRange(0.1, 16); rate_->setDecimals(2); rate_->setSingleStep(.05); rate_->setValue(1); rate_->setSuffix("×"); rate_->setButtonSymbols(QAbstractSpinBox::NoButtons); rate_->setKeyboardTracking(false); rate_->setFixedWidth(65); row->addWidget(rate_);
    rate_->setStyleSheet("QDoubleSpinBox{background:rgba(48,50,56,180);border:1px solid #70747b;} QDoubleSpinBox QLineEdit{background:transparent;border:none;}");
    connect(rate_, &QDoubleSpinBox::valueChanged, this, &PlayerWindow::setRate);
    for(auto *widget:QList<QWidget*>{mute_,volume_,volumePercent,rate_})widget->setProperty("minimalControl",true);
    rate_->setReadOnly(true); rate_->setFocusPolicy(Qt::NoFocus);
    rate_->setCursor(Qt::PointingHandCursor); rate_->setToolTip(tr("调整倍速"));
    const auto badge = [this, row](const QString &text) { auto *label = new QLabel(text, this); label->setProperty("playerBadge",true);label->setFixedHeight(28); row->addWidget(label); return label; };
    videoBadge_ = badge("—");videoBadge_->setObjectName("playerVideoCodec"); audioBadge_ = badge("—");audioBadge_->setObjectName("playerAudioCodec");
    subtitleBadge_=badge("Sub"); subtitleBadge_->setObjectName("playerSubtitleTrack");
    for (auto *label : {videoBadge_,audioBadge_,subtitleBadge_}) label->setCursor(Qt::PointingHandCursor);
    decoderBadge_ = new QPushButton("3FP-HW", this); decoderBadge_->setObjectName("playerDecoder");decoderBadge_->setFixedHeight(28); decoderBadge_->setToolTip(tr("切换 3FP 硬件 / 软件解码")); row->addWidget(decoderBadge_);
    connect(decoderBadge_, &QPushButton::clicked, this, [this] {
        if (lavVideo_) { showSettings(); return; }
        const auto snap = clock_->snapshot();
        const unsigned mode = settings_->value("decode/mode",2).toUInt()==2 ? 1 : 2;
        const auto at = position();
        const bool resume = playing_ || autoPlay_;
        if (!clock_->setDecodeMode(mode)) { setError(clock_->lastError()); return; }
        settings_->setValue("decode/mode",mode);
        if (source_.isEmpty() || imageMode_) return;
        reopeningDecoder_=true; reopenAudio_=snap.selectedAudioStream; reopenVideo_=snap.selectedVideoStream;
        seekPending_=false; seekUiTarget_=queuedSeek_=resumeAt_=-1; manualFrame_=-1;
        positionRestored_=false; settingsPosition_=at; autoPlay_=resume; playing_=false; rateApplied_=false;
        if (!openMedia()) reopeningDecoder_=false;
    });
    hdrBadge_ = badge("SDR");hdrBadge_->setObjectName("playerHdr"); rendererBadge_ = badge("VS");rendererBadge_->setObjectName("playerRenderer");algorithmBadge_=badge("Jinc");algorithmBadge_->setObjectName("playerAlgorithm");algorithmBadge_->setToolTip(tr("算法"));interpolationBadge_=badge({});interpolationBadge_->setObjectName("playerInterpolationAlgorithm");interpolationBadge_->setToolTip(tr("补帧算法"));interpolationBadge_->hide();row->addStretch();
}
void PlayerWindow::updateTransportWidth() {
    const auto bindings=playerShortcuts(*settings_);
    for (int i=0;i<playerShortcutCount(*settings_,bindings.size());++i) {
        const auto binding=bindings.value(i);
        const auto prefix=shortcutPrefix(i);
        const auto key=settings_->value(prefix+"key",binding.key).toString();
        if (key!="Left" && key!="Right") continue;
        if (!settings_->value(prefix+"enabled",true).toBool()) continue;
        const auto action=settings_->value(prefix+"action",binding.action).toString();
        const auto value=settings_->value(prefix+"value",binding.value).toDouble();
        auto *button=controls_->findChild<QPushButton*>(key=="Left"?"playerBackwardSecond":"playerForwardSecond");
        if(button) button->setText(action=="seek" ? QString("%1%2s").arg(value<0?"−":"+").arg(std::abs(value)) : key=="Left"?"‹":"›");
    }
    if (!time_->hasFocus() && minimalMode_) time_->setText(time_->text().section('.',0,0));
    fitTransportControls();
}

void PlayerWindow::fitTransportControls() {
    const int available=settings_->value("experimental/oversize",false).toBool()
        ? qMin(width(),screen()->availableGeometry().width()) : width();
    time_->setFixedWidth(minimalMode_ ? 76 : 110);
    const auto controlWidth=[](QWidget *widget) {
        return qBound(widget->minimumWidth(),widget->sizeHint().width(),widget->maximumWidth());
    };
    const auto *row=transportGrid_->itemAtPosition(1,0)->layout();
    const auto margins=transportGrid_->contentsMargins();
    const auto *footer=transportGrid_->itemAtPosition(1,1)->widget();
    int room=available-margins.left()-margins.right()-transportGrid_->horizontalSpacing()
        -qMax(volume_->width(),footer->sizeHint().width());
    const auto button=[this](const char *name){return controls_->findChild<QPushButton*>(name);};
    for(QWidget *widget:QList<QWidget*>{play_,button("playerBackwardSecond"),button("playerForwardSecond"),time_,rate_})
        room-=controlWidth(widget)+row->spacing();
    const auto fit=[&](QList<QWidget*> widgets,int threshold) {
        int cost=0;
        for(auto *widget:widgets)cost+=controlWidth(widget)+row->spacing();
        const bool show=!minimalMode_ && available>=threshold && cost<=room;
        for(auto *widget:widgets)widget->setVisible(show);
        // Reserve higher-priority widths so controls stay hidden as the window narrows.
        room-=cost;
    };
    fit({duration_},730);
    fit({frame_},810);
    fit({button("playerPreviousFrame"),button("playerNextFrame")},670);
    QList<QWidget*> badges{videoBadge_,audioBadge_,subtitleBadge_,decoderBadge_,hdrBadge_,rendererBadge_};
    if(!algorithmBadge_->text().isEmpty() && algorithmBadge_->text()!="HLSL")badges<<algorithmBadge_;
    else algorithmBadge_->hide();
    if(!interpolationBadge_->text().isEmpty())badges<<interpolationBadge_;
    else interpolationBadge_->hide();
    fit(badges,1040);
    fit({button("playerPreviousKeyframe"),button("playerNextKeyframe")},870);
    fit({button("playerPreviousFile"),button("playerNextFile")},870);
}

namespace {
class TrackMenu final : public QMenu {
public:
    using QMenu::QMenu;
    std::function<void(QAction*,bool)> choose;
    void keyPressEvent(QKeyEvent *event) override {
        const int digit=event->key()-Qt::Key_0;
        if (digit>=0 && digit<=9 && (event->modifiers()==Qt::NoModifier || event->modifiers()==Qt::ControlModifier)) {
            for(auto *item:actions()) if(item->property("digit").toInt()==digit) {
                choose(item,event->modifiers()==Qt::ControlModifier);close();break;
            }
            event->accept();return;
        }
        QMenu::keyPressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if(event->button()==Qt::RightButton) {
            if(auto *item=actionAt(event->position().toPoint())) {choose(item,true);close();}
        } else QMenu::mouseReleaseEvent(event);
    }
};
}
void PlayerWindow::showTrackPopup(const QString &type,bool secondaryDefault) {
    auto *anchor = type=="video" ? videoBadge_ : type=="audio" ? audioBadge_ : subtitleBadge_;
    const auto name="playerTrackMenu-"+type;
    if(trackPopup_ && trackPopup_->isVisible()) {
        const bool same=trackPopup_->objectName()==name;
        trackPopup_->close();
        if(same)return;
    }
    auto *menu = new TrackMenu(this);
    menu->setObjectName(name);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->choose=[this,type,secondaryDefault](QAction *action,bool secondary){
        const int index=action->data().toInt();
        if(type=="subtitle"){
            const int slot=(secondary || secondaryDefault)?1:0;
            const auto file=action->property("externalPath").toString();
            if(!file.isEmpty())attachSubtitle(file,slot);
            else selectSubtitle(slot,index);
        }
        else if(!secondary && type=="video") clock_->selectVideo(index);
        else if(!secondary && index>=0) selectAudio(index);
        else if(!secondary && !externalAudio_.isEmpty()) attachAudio(externalAudio_);
    };
    int number=0;
    const auto add=[&](const QString &label,int index,int digit,const QString &externalPath={}){
        auto *item=menu->addAction(digit<=9 ? QString("%1  %2").arg(digit).arg(label) : label);
        item->setData(index);item->setProperty("digit",digit);item->setProperty("externalPath",externalPath);
        connect(item,&QAction::triggered,menu,[menu,item]{menu->choose(item,false);});
    };
    if(type=="subtitle") add(tr("关闭字幕"),-1,0);
    for(const auto &value:media_.value("streams").toArray()) {
        const auto stream=value.toObject();
        if(stream.value("type").toString()!=type) continue;
        ++number; add(playerTrackLabel(stream,number),stream.value("index").toInt(),number);
    }
    const auto external=type=="audio"?externalAudio_:type=="subtitle"?externalSubtitle_:QString();
    if(!external.isEmpty()) add(QFileInfo(external).fileName(),-2,++number,external);
    if(type=="subtitle" && !externalSecondarySubtitle_.isEmpty() && externalSecondarySubtitle_!=external)
        add(QFileInfo(externalSecondarySubtitle_).fileName(),-2,++number,externalSecondarySubtitle_);
    if(menu->isEmpty()) {delete menu;return;}
    trackPopup_=menu;
    QWidget *position=anchor->isVisible() ? static_cast<QWidget*>(anchor) : rate_;
    menu->popup(position->mapToGlobal(QPoint(0,-menu->sizeHint().height())));
}

void PlayerWindow::showSpeedPopup() {
    if(speedPopup_ && speedPopup_->isVisible()) {speedPopup_->close();return;}
    if(equalizerPopup_)equalizerPopup_->close();
    auto *popup = new PlayerChromePanel(this);popup->setWindowFlags(Qt::Popup);popup->setTransparency(100-property("playerMenuOpacity").toInt());popup->setAttribute(Qt::WA_DeleteOnClose); popup->setObjectName("playerSpeedPopup");speedPopup_=popup; auto *layout = new QVBoxLayout(popup);
    popup->setAttribute(Qt::WA_TranslucentBackground);popup->setStyleSheet("QWidget#playerSpeedPopup{background:transparent;} QWidget#playerSpeedPopup QPushButton{background:transparent;} QWidget#playerSpeedPopup QPushButton:checked{background:rgba(80,127,189,230);}");
    auto *grid = new QGridLayout; layout->addLayout(grid); const QList<double> rates{.25,.5,.75,.9,1,1.1,1.25,1.5,1.75,2,2.5,16};
    for (int i = 0; i < rates.size(); ++i) { auto *button = new QPushButton(QString::number(rates[i], 'f', 2), popup); button->setMinimumSize(75, 32);button->setProperty("speedValue",rates[i]); button->setCheckable(true); button->setChecked(std::abs(speed_ - rates[i]) < .001); grid->addWidget(button, i / 3, i % 3); connect(button, &QPushButton::clicked, popup, [this, popup, value = rates[i]] { setRate(value);for(auto *b:popup->findChildren<QPushButton *>())if(b->property("speedValue").isValid())b->setChecked(std::abs(speed_-b->property("speedValue").toDouble())<.001); }); }
    auto *bottom = new QHBoxLayout; layout->addLayout(bottom);
    const QList<double> deltas{-.5,-.05,0,.05,.5}; const QStringList labels{"≪","‹","1.0","›","≫"};
    auto *editor = new RateSpinBox(popup);
    editor->setRange(.1,16); editor->setDecimals(2); editor->setValue(speed_);
    editor->setButtonSymbols(QAbstractSpinBox::NoButtons); editor->setKeyboardTracking(false);
    connect(editor,&QDoubleSpinBox::valueChanged,this,&PlayerWindow::setRate);
    connect(rate_,&QDoubleSpinBox::valueChanged,editor,&QDoubleSpinBox::setValue);
    for (int i = 0; i < deltas.size(); ++i) {
        if (i==2) {bottom->addWidget(editor);continue;}
        auto *b = new QPushButton(labels[i], popup); bottom->addWidget(b); connect(b, &QPushButton::clicked, popup, [this, popup, delta = deltas[i]] { setRate(delta == 0 ? 1 : speed_ + delta);for(auto *b:popup->findChildren<QPushButton *>())if(b->property("speedValue").isValid())b->setChecked(std::abs(speed_-b->property("speedValue").toDouble())<.001); }); }
    popup->adjustSize(); popup->move(rate_->mapToGlobal(QPoint(0, -popup->height()))); popup->show();
}
}
