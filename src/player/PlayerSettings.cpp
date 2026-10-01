#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "ui/PreviewPane.h"
#include "player/PlayerLanguage.h"
#include "player/PlayerCache.h"
#include "player/PlayerAssociations.h"
#include "update/PortableUpdater.h"
#include <QColorDialog>
#include <QFontComboBox>
#include <QLineEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollArea>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QThread>
#include <QSignalBlocker>
#include <algorithm>

namespace vsr {
bool PlayerWindow::madvrMode() const { return !networkSource() && settings_->value("player/renderer","VS").toString()=="madVR"; }
void PlayerWindow::resetStatistics() { skippedFrames_=submittedFrames_=0; frameMilliseconds_=0;suspendQualityCheck(); }
void PlayerWindow::toggleFullscreen() {
    suspendQualityCheck();
    if (isFullScreen()) { controls_->show();showNormal(); if(!normalGeometry_.isEmpty()) restoreGeometry(normalGeometry_);
        if(!playlistPinned_ && playlistExpansion_>0){resize(qMax(minimumWidth(),width()-playlistExpansion_),height());playlistExpansion_=0;}
        dockPlaylist(); }
    else { normalGeometry_=saveGeometry(); controls_->show();chromeIdle_.restart();showFullScreen();dockPlaylist();pane_->setFocus(); }
}
int PlayerWindow::prefetchCount() const {
    if(!settings_->value("performance/predecode",true).toBool()) return 0;
    if(usage_.cpu>=settings_->value("performance/cpu",50).toInt() || usage_.gpu>=settings_->value("performance/gpu",50).toInt() || usage_.ram>=settings_->value("performance/ram",50).toInt() || usage_.vram>=settings_->value("performance/vram",50).toInt()) return 0;
    const auto headroom=usage_.totalMemoryMiB*(settings_->value("performance/ram",50).toInt()-usage_.ram)/100.0;
    const double frameMiB=std::max(1.0,double(clip_.width)*clip_.height*8/1048576);
    return std::clamp(static_cast<int>(headroom/frameMiB),0,std::clamp(settings_->value("performance/frames",8).toInt(),1,16));
}
void PlayerWindow::applySettings(bool reopen) {
    usage_=resources_.sample();
    const int cpu=std::clamp(settings_->value("performance/cpu",50).toInt(),1,100);
    const auto headroom=usage_.totalMemoryMiB*(std::clamp(settings_->value("performance/ram",50).toInt(),1,100)-usage_.ram)/100.0+usage_.memoryMiB;
    server_->setResourceLimits(qMax(1,QThread::idealThreadCount()*cpu/100),std::clamp(static_cast<int>(headroom),64,8192));
    applyScaling();
    auto *visible=direct_?clock_.get():output_.get();
    visible->setOutputFormat(settings_->value("decode/output").toString());
    clock_->setDecodeMode(settings_->value("decode/mode",2).toUInt());
    volume_->setValue(std::clamp(settings_->value("player/volume",100).toInt(),0,100));
    mute_->setChecked(settings_->value("player/muted",false).toBool());
    setRate(settings_->value("player/speed",1).toDouble());
    subtitleVisible_=settings_->value("subtitle/visible",true).toBool();
    applyAppearance();
    settings_->sync(); if(reopen && !source_.isEmpty()) openFile(source_);
}
void PlayerWindow::applyAppearance() {
    language_->setLanguage(settings_->value("basic/language","zh_CN").toString());
    language_->updateWidgets(this);
    pane_->setPlaceholderText(tr("VS Player\n拖入视频，或按 Ctrl+O 打开\nCtrl+P 加载 VPY 预设 · Tab 视频信息"));
    QFont font;font.setFamilies({settings_->value("theme/latinFont","Segoe UI").toString(),settings_->value("theme/chineseFont","Microsoft YaHei UI").toString()});font.setPixelSize(13);setFont(font);
    const QColor color(settings_->value("theme/background","#202124").toString());
    if(color.isValid())setStyleSheet(property("playerBaseStyle").toString().replace("#202124",color.name()));
    setWindowOpacity(std::clamp(settings_->value("theme/opacity",100).toInt(),10,100)/100.0);
}
bool PlayerWindow::saveConfiguration(const QString &path) {
    settings_->sync(); if(QFileInfo(path).absoluteFilePath()==QFileInfo(settings_->fileName()).absoluteFilePath()) return settings_->status()==QSettings::NoError;
    QSettings output(path,QSettings::IniFormat);output.clear();for(const auto &key:settings_->allKeys()) output.setValue(key,settings_->value(key));output.sync();return output.status()==QSettings::NoError;
}
bool PlayerWindow::loadConfiguration(const QString &path) {
    if(!QFileInfo(path).isFile()) return false; QSettings input(path,QSettings::IniFormat);QVariantMap values;
    for(const auto &key:input.allKeys()) values.insert(key,input.value(key)); if(input.status()!=QSettings::NoError || values.isEmpty()) return false;
    settings_->clear(); for(auto it=values.begin();it!=values.end();++it) settings_->setValue(it.key(),it.value());
    preset_=settings_->value("player/preset").toString(); if(!QFileInfo::exists(preset_)) preset_.clear(); applySettings(true); return true;
}
void PlayerWindow::showSettings() {
    QDialog dialog(this); dialog.setObjectName("playerSettings");dialog.setWindowTitle(tr("VS Player 设置 · 1.0.2"));dialog.resize(880,620);
    auto *outer=new QVBoxLayout(&dialog);auto *body=new QHBoxLayout;outer->addLayout(body,1);auto *categories=new QListWidget(&dialog);categories->addItems({tr("基本设置"),tr("主题设置"),tr("播放设置"),tr("性能设置"),tr("解码设置"),tr("渲染设置"),tr("缓存设置"),tr("文件关联")});categories->setFixedWidth(145);body->addWidget(categories);
    auto *stack=new QStackedWidget(&dialog);body->addWidget(stack,1);connect(categories,&QListWidget::currentRowChanged,stack,&QStackedWidget::setCurrentIndex);
    const auto page=[&] {auto *scroll=new QScrollArea(stack);scroll->setWidgetResizable(true);auto *widget=new QWidget(scroll);scroll->setWidget(widget);stack->addWidget(scroll);auto *form=new QFormLayout(widget);form->setVerticalSpacing(14);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);return form;};
    auto *basic=page();auto *volume=new QSpinBox(&dialog);volume->setRange(0,100);volume->setSuffix(" %");volume->setValue(volume_->value());basic->addRow(tr("默认音量"),volume);
    auto *subtitles=new QCheckBox(tr("显示挂载字幕"),&dialog);subtitles->setChecked(subtitleVisible_);basic->addRow(subtitles);
    auto *configInfo=new QLabel(tr("所有设置自动保存在程序旁 player.ini。\n可将全部设置导出为一个 INI，或载入已有配置。\nEnter / F11：无边框全屏；Esc：退出全屏。"),&dialog);configInfo->setWordWrap(true);basic->addRow(configInfo);
    auto *language=new QComboBox(&dialog);language->setObjectName("playerLanguage");language->addItem(tr("中文"),"zh_CN");language->addItem("English","en_US");language->setCurrentIndex(qMax(0,language->findData(settings_->value("basic/language","zh_CN"))));basic->addRow(tr("语言"),language);
    auto *autoplay=new QCheckBox(tr("打开文件自动播放；关闭则准备首帧并暂停"),&dialog);autoplay->setObjectName("playerAutoplay");autoplay->setChecked(settings_->value("basic/autoplay",true).toBool());basic->addRow(autoplay);
    basic->addRow(new QLabel(tr("当前版本：%1").arg(VSR_VERSION),&dialog));
    auto *updates=new QPushButton(tr("检查更新"),&dialog);updates->setObjectName("playerCheckUpdates");basic->addRow(updates);connect(updates,&QPushButton::clicked,&dialog,[&]{PortableUpdater updater(&dialog);updater.check();updater.exec();});
    auto *theme=page();auto *chineseFont=new QFontComboBox(&dialog);chineseFont->setObjectName("playerChineseFont");chineseFont->setCurrentFont(QFont(settings_->value("theme/chineseFont","Microsoft YaHei UI").toString()));theme->addRow(tr("中文字体"),chineseFont);
    auto *latinFont=new QFontComboBox(&dialog);latinFont->setObjectName("playerLatinFont");latinFont->setCurrentFont(QFont(settings_->value("theme/latinFont","Segoe UI").toString()));theme->addRow(tr("西文字体"),latinFont);
    auto *background=new QPushButton(settings_->value("theme/background","#202124").toString(),&dialog);background->setObjectName("playerBackground");theme->addRow(tr("背景色"),background);connect(background,&QPushButton::clicked,&dialog,[&]{const auto color=QColorDialog::getColor(QColor(background->text()),&dialog);if(color.isValid())background->setText(color.name());});
    auto *opacity=new QSpinBox(&dialog);opacity->setObjectName("playerOpacity");opacity->setRange(10,100);opacity->setSuffix(" %");opacity->setValue(settings_->value("theme/opacity",100).toInt());theme->addRow(tr("窗口不透明度"),opacity);
    auto *playback=page();auto *remember=new QCheckBox(tr("记忆视频 / 音频播放位置"),&dialog);remember->setObjectName("playerRemember");remember->setChecked(settings_->value("playback/remember",true).toBool());playback->addRow(remember);
    auto *multithread=new QCheckBox(tr("后台多线程打开文件与源索引"),&dialog);multithread->setObjectName("playerMultithread");multithread->setChecked(settings_->value("playback/multithread",true).toBool());playback->addRow(multithread);
    auto *arrows=new QComboBox(&dialog);arrows->setObjectName("playerArrowMode");arrows->addItem(tr("1 秒"),"seconds");arrows->addItem(tr("1 帧"),"frame");arrows->addItem(tr("关键帧"),"keyframe");arrows->setCurrentIndex(qMax(0,arrows->findData(settings_->value("playback/arrows","seconds"))));playback->addRow(tr("左右方向键"),arrows);
    auto *ctrl=new QSpinBox(&dialog);ctrl->setObjectName("playerCtrlSeconds");ctrl->setRange(1,3600);ctrl->setSuffix(" s");ctrl->setValue(settings_->value("playback/ctrlSeconds",10).toInt());playback->addRow("Ctrl + ← / →",ctrl);
    auto *ctrlAlt=new QSpinBox(&dialog);ctrlAlt->setObjectName("playerCtrlAltSeconds");ctrlAlt->setRange(1,3600);ctrlAlt->setSuffix(" s");ctrlAlt->setValue(settings_->value("playback/ctrlAltSeconds",60).toInt());playback->addRow("Ctrl + Alt + ← / →",ctrlAlt);
    auto *openingInfo=new QLabel(tr("位置保存为媒体时间，倍帧 VPY 按输出帧率重新定位。关闭多线程选项时 VS 源使用单解码线程；3FP 的播放工作线程仍保留，界面不承担解码。"),&dialog);openingInfo->setWordWrap(true);playback->addRow(openingInfo);
    auto *performance=page();auto *preset=new QComboBox(&dialog);preset->addItems({tr("常规 · 全部 50%"),tr("极致 · 全部 100%"),tr("自定义")});performance->addRow(tr("性能预设"),preset);
    auto *predecode=new QCheckBox(tr("有资源盈余时提前处理后续帧"),&dialog);predecode->setChecked(settings_->value("performance/predecode",true).toBool());performance->addRow(predecode);
    QList<QSpinBox*> limits;const QStringList keys{"cpu","gpu","ram","vram"};const QStringList labels{tr("CPU 最大占用"),tr("GPU 最大占用"),tr("内存最大占用"),tr("进程显存预算最大占用")};
    for(int n=0;n<4;++n) {auto *limit=new QSpinBox(&dialog);limit->setRange(1,100);limit->setSuffix(" %");limit->setValue(std::clamp(settings_->value("performance/"+keys[n],50).toInt(),1,100));performance->addRow(labels[n],limit);limits<<limit;}
    bool normal=true,extreme=true;for(auto *limit:limits){normal&=limit->value()==50;extreme&=limit->value()==100;}preset->setCurrentIndex(normal?0:extreme?1:2);
    connect(preset,&QComboBox::currentIndexChanged,&dialog,[limits](int n){if(n<2)for(auto *limit:limits)limit->setValue(n?100:50);});
    for(auto *limit:limits)connect(limit,&QSpinBox::valueChanged,&dialog,[limits,preset]{bool normal=true,extreme=true;for(auto *box:limits){normal&=box->value()==50;extreme&=box->value()==100;}QSignalBlocker blocker(preset);preset->setCurrentIndex(normal?0:extreme?1:2);});
    auto *frames=new QSpinBox(&dialog);frames->setRange(1,16);frames->setValue(settings_->value("performance/frames",8).toInt());performance->addRow(tr("最多提前处理帧数"),frames);
    auto *resizeFirst=new QCheckBox(tr("先 Jinc 到目标分辨率，再做增强（降低过大源的处理量）"),&dialog);resizeFirst->setChecked(settings_->value("performance/resizeBeforeEnhance",false).toBool());performance->addRow(resizeFirst);
    auto *limitInfo=new QLabel(tr("占用阈值控制后台预解码；达到阈值便停止追加。CPU 同时限制 VS 工作线程，内存限制缓存目标。当前帧不强行中断，显存/缓存由插件分配，无法保证全机占用绝不超限。"),&dialog);limitInfo->setWordWrap(true);performance->addRow(limitInfo);
    auto *decode=page();auto *core=new QComboBox(&dialog);core->setObjectName("playerVideoDecoder");core->addItem("3FPlayer (FFF.Native.dll)","3FP");core->addItem("LAV Video Decoder (LAVVideo.ax)","LAV");core->setCurrentIndex(qMax(0,core->findData(settings_->value("decode/video",settings_->value("player/core","3FP")))));decode->addRow(tr("视频解码器"),core);
    auto *audioCore=new QComboBox(&dialog);audioCore->setObjectName("playerAudioDecoder");audioCore->addItem("3FPlayer (FFF.Native.dll)","3FP");audioCore->addItem("LAV Audio Decoder (LAVAudio.ax)","LAV");audioCore->setCurrentIndex(qMax(0,audioCore->findData(settings_->value("decode/audio",settings_->value("player/core","3FP")))));decode->addRow(tr("音频解码器"),audioCore);
    auto *lavAudioConfig=new QPushButton(tr("打开 LAV Audio 配置…"),&dialog);decode->addRow(lavAudioConfig);connect(lavAudioConfig,&QPushButton::clicked,&dialog,[this,&dialog]{if(lav_)lav_->showAudioSettings(reinterpret_cast<void *>(dialog.winId()));else{LavPlayback config;config.showAudioSettings(reinterpret_cast<void *>(dialog.winId()));}});
    auto *lavConfig=new QPushButton(tr("打开 LAV Video 配置…"),&dialog);decode->addRow(lavConfig);
    connect(lavConfig,&QPushButton::clicked,&dialog,[this,&dialog]{if(lav_)lav_->showVideoSettings(reinterpret_cast<void *>(dialog.winId()));else {LavPlayback config;config.showVideoSettings(reinterpret_cast<void *>(dialog.winId()));}});
    auto *hardware=new QComboBox(&dialog);hardware->addItem(tr("3FPlayer · D3D11 硬件解码（不可用时软件回退）"),2);hardware->addItem(tr("3FPlayer · FFmpeg CPU 软件解码"),1);hardware->setCurrentIndex(settings_->value("decode/mode",2).toInt()==2?0:1);decode->addRow(tr("3FP 解码方式"),hardware);
    auto *output=new QComboBox(&dialog);output->setObjectName("playerOutputFormat");output->addItem(tr("保持 VS 输出（默认）"),QString());
    const QStringList formats{"yuv420p","yuv420p10le","yuv420p16le","yuv422p","yuv422p10le","yuv422p16le","yuv444p","yuv444p10le","yuv444p16le","nv12","p010le","p016le","p210le","p216le","uyvy422","yuyv422","yvyu422","ayuv64le","y210le","y216le","xv30le","xv48le","rgb24","bgr24","rgb0","bgra","rgb565le","rgb555le","rgb48le","rgba64le"};
    for(const auto &format:formats)output->addItem(format,format);output->setCurrentIndex(qMax(0,output->findData(settings_->value("decode/output").toString())));decode->addRow(tr("3FP 输出像素格式"),output);
    auto *decodeInfo=new QLabel(tr("VS 源解码仍由 VPY 定义；LAV 视频选项用于本地文件播放时钟图。音频可独立使用 LAVAudio.ax。网络固定 3FP；内置原生视频路径使用 3FP。倍速音频使用 3FP / atempo。madVR 视频固定 LAV。"),&dialog);decodeInfo->setWordWrap(true);decode->addRow(decodeInfo);
    auto *render=page();auto *renderer=new QComboBox(&dialog);renderer->addItems({"VS","madVR"});renderer->setCurrentText(settings_->value("player/renderer","VS").toString());render->addRow(tr("视频渲染器"),renderer);
    auto *antiring=new QCheckBox(tr("Anti-ringing · relaxed（仅 Jinc，强度 0.5）"),&dialog);antiring->setChecked(settings_->value("render/antiring",true).toBool());render->addRow(antiring);
    auto *renderInfo=new QLabel(tr("VS：VPY 实时滤镜及原生 D3D11 呈现。\nmadVR：直接加载随附 madVR64.ax，处理设置交给 madVR；VS 预设不生效。\n默认放大/缩小采用 Jinc，同尺寸跳过缩放。\n抗振铃采用开放的局部范围约束，不宣称复刻 madVR 专有实现。"),&dialog);renderInfo->setWordWrap(true);render->addRow(renderInfo);
    auto *cache=page();auto *cacheMode=new QComboBox(&dialog);cacheMode->setObjectName("playerCacheMode");cacheMode->addItem(tr("软件目录（默认）"),"default");cacheMode->addItem(tr("自定义目录"),"custom");cacheMode->setCurrentIndex(settings_->value("cache/path").toString().isEmpty()?0:1);cache->addRow(tr("索引缓存位置"),cacheMode);
    auto *cachePath=new QLineEdit(playerCacheDirectory(*settings_),&dialog);cachePath->setObjectName("playerCachePath");cache->addRow(tr("存储路径"),cachePath);auto *browse=new QPushButton(tr("浏览…"),&dialog);cache->addRow(browse);connect(browse,&QPushButton::clicked,&dialog,[&]{const auto directory=QFileDialog::getExistingDirectory(&dialog,tr("选择索引缓存目录"),cachePath->text());if(!directory.isEmpty()){cachePath->setText(directory);cacheMode->setCurrentIndex(1);}});
    auto *cacheSize=new QLabel(&dialog);cacheSize->setObjectName("playerCacheSize");cache->addRow(tr("当前缓存大小"),cacheSize);
    const auto updateCache=[&]{const auto directory=cacheMode->currentIndex()==0?QDir(QCoreApplication::applicationDirPath()).filePath("cache/indexes"):cachePath->text();cacheSize->setText(QString::number(playerCacheBytes(directory)/1048576.0,'f',2)+" MiB");cachePath->setEnabled(cacheMode->currentIndex()==1);};
    connect(cacheMode,&QComboBox::currentIndexChanged,&dialog,[&]{if(cacheMode->currentIndex()==0)cachePath->setText(QDir(QCoreApplication::applicationDirPath()).filePath("cache/indexes"));updateCache();});connect(cachePath,&QLineEdit::textChanged,&dialog,updateCache);updateCache();
    auto *clearCache=new QPushButton(tr("清除视频寻帧索引缓存（ffindex / lwi）"),&dialog);clearCache->setObjectName("playerClearCache");cache->addRow(clearCache);connect(clearCache,&QPushButton::clicked,&dialog,[&]{const bool ok=clearPlayerIndexes(cachePath->text());updateCache();setError(ok?tr("寻帧索引缓存已清除，视频文件保持不变。"):tr("部分索引缓存正在使用或无法删除。"));});
    auto *cacheInfo=new QLabel(tr("索引记录关键帧和寻帧位置，避免重复扫描本地媒体；不缓存视频画面。网络直通不建立 VS 索引。路径、大小、修改时间变化会使用新索引。清除只处理该目录中的 ffindex / lwi 文件。"),&dialog);cacheInfo->setWordWrap(true);cache->addRow(cacheInfo);
    auto *associations=page();auto *associationFormats=new QListWidget(&dialog);associationFormats->setObjectName("playerAssociationFormats");const auto videoFormats=playerVideoExtensions(),audioFormats=playerAudioExtensions();const auto selected=settings_->value("associations/extensions").toStringList();for(const auto &extension:videoFormats+audioFormats){auto *item=new QListWidgetItem('.'+extension,associationFormats);item->setData(Qt::UserRole,extension);item->setCheckState(selected.contains(extension)?Qt::Checked:Qt::Unchecked);}associations->addRow(associationFormats);
    auto *choices=new QWidget(&dialog);auto *choiceRow=new QHBoxLayout(choices);choiceRow->setContentsMargins(0,0,0,0);const QStringList selections{tr("全选视频"),tr("全选音频"),tr("全选所有"),tr("取消所有")};for(int n=0;n<4;++n){auto *button=new QPushButton(selections[n],&dialog);choiceRow->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[=]{for(int row=0;row<associationFormats->count();++row){auto *item=associationFormats->item(row);const auto extension=item->data(Qt::UserRole).toString();item->setCheckState(n==2 || (n==0 && videoFormats.contains(extension)) || (n==1 && audioFormats.contains(extension))?Qt::Checked:Qt::Unchecked);}});}associations->addRow(choices);
    auto *registerButton=new QPushButton(tr("注册所选格式到当前用户"),&dialog);registerButton->setObjectName("playerRegisterAssociations");associations->addRow(registerButton);connect(registerButton,&QPushButton::clicked,&dialog,[&]{QStringList extensions;for(int n=0;n<associationFormats->count();++n)if(associationFormats->item(n)->checkState()==Qt::Checked)extensions<<associationFormats->item(n)->data(Qt::UserRole).toString();if(registerPlayerAssociations(extensions,QDir(QCoreApplication::applicationDirPath()).filePath("vs-player.exe"))){settings_->setValue("associations/extensions",extensions);setError(tr("格式已注册；请在 Windows 默认应用中选择 VS Player。"));}else setError(tr("文件关联注册失败。"));});
    auto *defaultsButton=new QPushButton(tr("打开 Windows 默认应用…"),&dialog);associations->addRow(defaultsButton);connect(defaultsButton,&QPushButton::clicked,&dialog,[]{QDesktopServices::openUrl(QUrl("ms-settings:defaultapps"));});
    auto *associationInfo=new QLabel(tr("注册到当前用户，无需管理员权限。Windows 最终默认程序由用户选择；取消选择会移除 VS Player 的打开方式入口，不改写其他程序的默认关联。"),&dialog);associationInfo->setWordWrap(true);associations->addRow(associationInfo);
    categories->setCurrentRow(0);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);auto *save=buttons->addButton(tr("导出配置…"),QDialogButtonBox::ActionRole);auto *load=buttons->addButton(tr("加载配置…"),QDialogButtonBox::ActionRole);outer->addWidget(buttons);
    const auto store=[&]{settings_->setValue("basic/language",language->currentData());settings_->setValue("basic/autoplay",autoplay->isChecked());settings_->setValue("theme/chineseFont",chineseFont->currentFont().family());settings_->setValue("theme/latinFont",latinFont->currentFont().family());settings_->setValue("theme/background",background->text());settings_->setValue("theme/opacity",opacity->value());settings_->setValue("playback/remember",remember->isChecked());settings_->setValue("playback/multithread",multithread->isChecked());settings_->setValue("playback/arrows",arrows->currentData());settings_->setValue("playback/ctrlSeconds",ctrl->value());settings_->setValue("playback/ctrlAltSeconds",ctrlAlt->value());settings_->setValue("cache/path",cacheMode->currentIndex()==0?QString():QFileInfo(cachePath->text()).absoluteFilePath());settings_->setValue("player/volume",volume->value());settings_->setValue("subtitle/visible",subtitles->isChecked());settings_->setValue("performance/predecode",predecode->isChecked());settings_->setValue("performance/frames",frames->value());settings_->setValue("performance/resizeBeforeEnhance",resizeFirst->isChecked());for(int n=0;n<4;++n)settings_->setValue("performance/"+keys[n],limits[n]->value());settings_->setValue("player/core",core->currentData());settings_->setValue("decode/video",core->currentData());settings_->setValue("decode/audio",audioCore->currentData());settings_->setValue("decode/mode",hardware->currentData());settings_->setValue("decode/output",output->currentData());settings_->setValue("player/renderer",renderer->currentText());settings_->setValue("render/antiring",antiring->isChecked());QStringList extensions;for(int n=0;n<associationFormats->count();++n)if(associationFormats->item(n)->checkState()==Qt::Checked)extensions<<associationFormats->item(n)->data(Qt::UserRole).toString();settings_->setValue("associations/extensions",extensions);};
    connect(save,&QPushButton::clicked,&dialog,[&]{const auto file=QFileDialog::getSaveFileName(&dialog,tr("保存完整配置"),"vs-player.ini","INI (*.ini)");if(!file.isEmpty()){store();if(!saveConfiguration(file))setError(tr("配置保存失败。"));}});
    connect(load,&QPushButton::clicked,&dialog,[&]{const auto file=QFileDialog::getOpenFileName(&dialog,tr("加载完整配置"),{},"INI (*.ini)");if(!file.isEmpty()){if(loadConfiguration(file))dialog.reject();else setError(tr("配置文件无效。"));}});
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    language_->updateWidgets(&dialog);
    if(dialog.exec()==QDialog::Accepted){store();applySettings(true);}
}
}
