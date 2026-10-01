#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "graph/PresetStore.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>
#include <cmath>

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
    auto *first = new QHBoxLayout; first->setContentsMargins(8, 0, 8, 0); first->setSpacing(8); layout->addLayout(first);
    auto *slider = timeline_;
    first->addWidget(slider, 1);
    mute_ = new QPushButton(QStringLiteral("♪"), this); mute_->setObjectName("playerMute"); mute_->setCheckable(true); mute_->setToolTip(tr("静音")); first->addWidget(mute_);
    volume_ = new QSlider(Qt::Horizontal, this); volume_->setObjectName("playerVolume"); volume_->setRange(0, 100); volume_->setValue(100); volume_->setFixedWidth(105); volume_->setToolTip(tr("音量")); first->addWidget(volume_);
    const auto volume = [this] { clock_->setVolume(volume_->value() / 100.0f); clock_->setMuted(lavAudio_ || mute_->isChecked()); if (lav_) lav_->volume(volume_->value() / 100.0f,!lavAudio_ || mute_->isChecked());settings_->setValue("player/volume",volume_->value());settings_->setValue("player/muted",mute_->isChecked()); };
    connect(volume_, &QSlider::valueChanged, this, volume); connect(mute_, &QPushButton::toggled, this, volume);
    connect(slider, &QSlider::valueChanged, this, [this, slider](int value) { if (!slider->isSliderDown()) seekTime(clock_->snapshot().duration100ns * value / 100000); });
    connect(slider, &QSlider::sliderReleased, this, [this, slider] { seekTime(clock_->snapshot().duration100ns * slider->value() / 100000); });
    auto *row = new QHBoxLayout; row->setContentsMargins(8, 0, 8, 0); row->setSpacing(4); layout->addLayout(row);
    const auto button = [this, row](const QString &text, const QString &tooltip, auto action) {
        auto *b = new QPushButton(text, this); b->setToolTip(tooltip); b->setFixedWidth(34); row->addWidget(b); connect(b, &QPushButton::clicked, this, action); return b;
    };
    play_ = button(QStringLiteral("▶"), tr("播放 / 暂停 · Space"), [this] { togglePlayback(); }); play_->setObjectName("playerPlay");
    button("|‹", tr("退一帧"), [this] { seekFrame(lastFrame_ - 1); });
    button("›|", tr("进一帧"), [this] { seekFrame(lastFrame_ + 1); });
    button("−1s", tr("退一秒"), [this] { seekTime(position() - 10000000); });
    button("+1s", tr("进一秒"), [this] { seekTime(position() + 10000000); });
    const auto keyframe = [this](int direction) {
        suspendQualityCheck();
        manualFrame_ = -1;
        if (playing_) togglePlayback();
        if (lav_) { clock_->seek(position()); lavKeyPending_ = true; }
        generation_ = clock_->snapshot().timelineGeneration; seekPending_ = clock_->stepKeyframe(direction);
    };
    button("K‹", tr("上一个关键帧"), [=] { keyframe(-1); });
    button("›K", tr("下一个关键帧"), [=] { keyframe(1); });
    button("|◀", tr("上一个视频"), [this] { nextFile(-1); });
    button("▶|", tr("下一个视频"), [this] { nextFile(1); });
    time_ = new QLineEdit("00:00:00.000", this); time_->setObjectName("playerPosition"); time_->setFixedWidth(110); time_->setToolTip(tr("输入 HH:MM:SS.mmm 或秒数，Enter 精确跳转")); row->addWidget(time_);
    connect(time_, &QLineEdit::returnPressed, this, [this] {
        const auto parts = time_->text().split(':'); bool ok = !parts.isEmpty() && parts.size() <= 3; double seconds = 0;
        for (const auto &part : parts) { bool valid; double value = part.toDouble(&valid); ok = ok && valid && value >= 0; seconds = seconds * 60 + value; }
        if (ok && std::isfinite(seconds) && seconds < 1e9) { seekTime(static_cast<qint64>(std::llround(seconds * 10000000))); time_->clearFocus(); } else setError(tr("时间格式无效。"));
    });
    duration_ = new QLabel("/ 00:00:00.000", this); row->addWidget(duration_);
    frame_ = new QLineEdit("0", this); frame_->setObjectName("playerFrame"); frame_->setFixedWidth(65); frame_->setToolTip(tr("精确输出帧号，从 0 开始；Enter 跳转")); row->addWidget(frame_);
    connect(frame_, &QLineEdit::returnPressed, this, [this] { bool ok; const auto n = frame_->text().toLongLong(&ok); if (ok) { seekFrame(n); frame_->clearFocus(); } });
    rate_ = new RateSpinBox(this); rate_->setObjectName("playerSpeed"); rate_->setRange(0.1, 16); rate_->setDecimals(2); rate_->setSingleStep(.05); rate_->setValue(1); rate_->setSuffix("×"); rate_->setButtonSymbols(QAbstractSpinBox::NoButtons); rate_->setKeyboardTracking(false); rate_->setFixedWidth(65); row->addWidget(rate_);
    connect(rate_, &QDoubleSpinBox::valueChanged, this, &PlayerWindow::setRate);
    speedButton_=button("▾", tr("倍速配置"), [this] { showSpeedPopup(); }); speedButton_->setObjectName("playerSpeedPopupButton");
    const auto badge = [this, row](const QString &text) { auto *label = new QLabel(text, this); label->setStyleSheet("background:#36383d;color:#c3c8d1;padding:3px;border-radius:2px;"); row->addWidget(label); return label; };
    videoBadge_ = badge("—"); audioBadge_ = badge("—");
    decoderBadge_ = new QPushButton("3FP-HW", this); decoderBadge_->setObjectName("playerDecoder"); decoderBadge_->setToolTip(tr("切换 3FP 硬件 / 软件解码")); row->addWidget(decoderBadge_);
    connect(decoderBadge_, &QPushButton::clicked, this, [this] { if (lavVideo_) { showSettings(); return; } const auto mode = clock_->snapshot().decodeMode == 2 ? 1u : 2u; if (clock_->setDecodeMode(mode)) {settings_->setValue("decode/mode",mode);if(!source_.isEmpty())openFile(source_);} });
    hdrBadge_ = badge("SDR"); rendererBadge_ = badge("VS"); row->addStretch();
    auto *menuButton = button("☰", tr("打开、预设与设置"), [] {});
    auto *menu = new QMenu(menuButton);
    menu->addAction(tr("打开视频…"), this, &PlayerWindow::chooseFiles);
    menu->addAction(tr("打开文件夹…"), this, &PlayerWindow::chooseFolder);
    menu->addAction(tr("打开链接…"), this, &PlayerWindow::chooseLink);
    menu->addAction(tr("加载 VPY…"), this, [this] { const auto path = QFileDialog::getOpenFileName(this, tr("加载 VPY"), PresetStore::directory(), "VapourSynth (*.vpy)"); if (!path.isEmpty()) loadPreset(path); });
    menu->addAction(tr("停用预设 · 原画播放"), this, [this] { loadPreset({}); });
    menu->addAction(tr("设置…"), this, &PlayerWindow::showSettings);
    menuButton->setMenu(menu);
}
void PlayerWindow::showSpeedPopup() {
    if(speedPopup_ && speedPopup_->isVisible()) {speedPopup_->close();return;}
    auto *popup = new QWidget(this, Qt::Popup); popup->setAttribute(Qt::WA_DeleteOnClose); popup->setObjectName("playerSpeedPopup");speedPopup_=popup; auto *layout = new QVBoxLayout(popup);
    auto *grid = new QGridLayout; layout->addLayout(grid); const QList<double> rates{.25,.5,.75,.9,1,1.1,1.25,1.5,1.75,2,2.5,16};
    for (int i = 0; i < rates.size(); ++i) { auto *button = new QPushButton(QString::number(rates[i], 'f', 2), popup); button->setMinimumSize(75, 32);button->setProperty("speedValue",rates[i]); button->setCheckable(true); button->setChecked(std::abs(speed_ - rates[i]) < .001); grid->addWidget(button, i / 3, i % 3); connect(button, &QPushButton::clicked, popup, [this, popup, value = rates[i]] { setRate(value);for(auto *b:popup->findChildren<QPushButton *>())if(b->property("speedValue").isValid())b->setChecked(std::abs(speed_-b->property("speedValue").toDouble())<.001); }); }
    auto *bottom = new QHBoxLayout; layout->addLayout(bottom);
    const QList<double> deltas{-.5,-.05,0,.05,.5}; const QStringList labels{"≪","‹","1.0","›","≫"};
    for (int i = 0; i < deltas.size(); ++i) { auto *b = new QPushButton(labels[i], popup); bottom->addWidget(b); connect(b, &QPushButton::clicked, popup, [this, popup, delta = deltas[i]] { setRate(delta == 0 ? 1 : speed_ + delta);for(auto *b:popup->findChildren<QPushButton *>())if(b->property("speedValue").isValid())b->setChecked(std::abs(speed_-b->property("speedValue").toDouble())<.001); }); }
    popup->adjustSize(); popup->move(rate_->mapToGlobal(QPoint(0, -popup->height()))); popup->show();
}
}
