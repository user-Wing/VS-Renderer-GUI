#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include <QSettings>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QFileInfo>
#include <array>

namespace vsr {
void PlayerWindow::useNativeAudio() {
    if(!lavAudio_)return;
    lavAudio_=false;if(lav_)lav_->volume(0,true);
    clock_->setMuted(mute_->isChecked());clock_->setVolume(volume_->value()/100.f);
    // Keep LAV's video clock and renderer if selected; native audio follows the same timeline.
    clock_->seek(position());if(playing_)clock_->play();
}
void PlayerWindow::selectAudio(int stream) {
    if(imageMode_ || source_.isEmpty())return;
    useNativeAudio();
    if(!externalAudio_.isEmpty() && !clock_->clearExternalAudio())return;
    if(clock_->selectAudio(stream)){externalAudio_.clear();applyAudioEffects();}
}
void PlayerWindow::attachAudio(const QString &path) {
    if(!deferred_.isEmpty()){deferredAudio_=path;return;}
    if(imageMode_ || source_.isEmpty() || !QFileInfo(path).isFile())return;
    const auto state=clock_->snapshot().state;
    if(state!=ThreeFpState::Ready && state!=ThreeFpState::Playing && state!=ThreeFpState::Paused && state!=ThreeFpState::Ended) {
        if(state==ThreeFpState::Idle || state==ThreeFpState::Opening)pendingExternalAudio_=path;return;
    }
    useNativeAudio();
    if(clock_->loadExternalAudio(path)) {externalAudio_=QFileInfo(path).absoluteFilePath();applyAudioEffects();message_->setText(tr("作为外部音频加载：%1").arg(QFileInfo(path).fileName()));}
}
void PlayerWindow::applyAudioEffects() {
    if(imageMode_)return;
    std::array<float,10> gains{};for(int i=0;i<10;++i)gains[i]=settings_->value(QString("audio/eq/%1").arg(i),0).toFloat();
    const bool enabled=settings_->value("audio/eq/enabled",false).toBool();
    const float wave=settings_->value("audio/wave",100).toFloat()/100;
    if(enabled || wave!=1 || audioDelay_!=0)useNativeAudio();
    clock_->setAudioEffects(enabled,gains.data(),wave,audioDelay_);
}
void PlayerWindow::showEqualizer() {
    if(equalizerWindow_){equalizerWindow_->show();equalizerWindow_->raise();equalizerWindow_->activateWindow();return;}
    auto *dialog=new QDialog(this);equalizerWindow_=dialog;dialog->setObjectName("playerEqualizer");dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(tr("均衡器"));dialog->resize(640,300);
    auto *layout=new QVBoxLayout(dialog);auto *header=new QHBoxLayout;layout->addLayout(header);
    auto *enabled=new QCheckBox(tr("均衡器"),dialog);enabled->setChecked(settings_->value("audio/eq/enabled",false).toBool());header->addWidget(enabled);
    auto *presets=new QComboBox(dialog);presets->addItems({tr("默认"),tr("低音增强"),tr("人声"),tr("高音增强")});header->addWidget(presets,1);
    auto *reset=new QPushButton(tr("复位"),dialog);header->addWidget(reset);
    auto *row=new QHBoxLayout;layout->addLayout(row);std::array<QSlider *,10> bands{};
    const QStringList labels{"60","170","310","600","1K","3K","6K","12K","14K","16K"};
    for(int i=0;i<10;++i){auto *column=new QVBoxLayout;row->addLayout(column);auto *label=new QLabel(labels[i],dialog);label->setAlignment(Qt::AlignCenter);column->addWidget(label);auto *slider=new QSlider(Qt::Vertical,dialog);bands[i]=slider;slider->setObjectName(QString("equalizerBand%1").arg(i));slider->setRange(-12,12);slider->setValue(settings_->value(QString("audio/eq/%1").arg(i),0).toInt());column->addWidget(slider,1,Qt::AlignHCenter);auto *value=new QLabel(QString::number(slider->value())+" dB",dialog);value->setAlignment(Qt::AlignCenter);column->addWidget(value);connect(slider,&QSlider::valueChanged,dialog,[this,i,value](int gain){value->setText(QString::number(gain)+" dB");settings_->setValue(QString("audio/eq/%1").arg(i),gain);applyAudioEffects();});}
    for(int n=0;n<2;++n){auto *column=new QVBoxLayout;row->addLayout(column);auto *label=new QLabel(n?"WAV":"MST",dialog);label->setAlignment(Qt::AlignCenter);column->addWidget(label);auto *slider=new QSlider(Qt::Vertical,dialog);slider->setRange(0,100);slider->setObjectName(n?"equalizerWave":"equalizerMaster");slider->setValue(n?settings_->value("audio/wave",100).toInt():volume_->value());column->addWidget(slider,1,Qt::AlignHCenter);if(n)connect(slider,&QSlider::valueChanged,dialog,[this](int v){settings_->setValue("audio/wave",v);applyAudioEffects();});else{connect(slider,&QSlider::valueChanged,volume_,&QSlider::setValue);connect(volume_,&QSlider::valueChanged,slider,&QSlider::setValue);}}
    connect(enabled,&QCheckBox::toggled,dialog,[this](bool on){settings_->setValue("audio/eq/enabled",on);applyAudioEffects();});
    connect(presets,&QComboBox::currentIndexChanged,dialog,[bands](int preset){const int values[4][10]{{0,0,0,0,0,0,0,0,0,0},{6,5,3,1,0,0,0,0,0,0},{-2,-1,0,2,4,3,1,0,-1,-2},{0,0,0,0,0,1,3,4,5,5}};for(int i=0;i<10;++i)bands[i]->setValue(values[preset][i]);});
    connect(reset,&QPushButton::clicked,dialog,[bands,presets]{presets->setCurrentIndex(0);for(auto *band:bands)band->setValue(0);});
    dialog->setStyleSheet("QSlider::groove:vertical{width:4px;background:#151515;}QSlider::sub-page:vertical{background:#151515;}QSlider::add-page:vertical{background:#2389ee;}QSlider::handle:vertical{height:12px;margin:0 -4px;background:#e8e8e8;border-radius:6px;}");
    dialog->show();
}
}
