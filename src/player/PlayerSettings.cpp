#include "player/PlayerWindow.h"
#include "player/PlayerChromePanel.h"
#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "ui/PreviewPane.h"
#include "player/PlayerLanguage.h"
#include "player/PlayerCache.h"
#include "player/PlayerAssociations.h"
#include "update/ComponentDownloads.h"
#include <QColorDialog>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QApplication>
#include <QTreeWidgetItemIterator>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollArea>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDialog>
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
bool PlayerWindow::madvrMode() const { return QFileInfo(source_).suffix().compare("mpls",Qt::CaseInsensitive)!=0 && !imageMode_ && !networkSource() && settings_->value("player/renderer","VS").toString()=="madVR"; }
void PlayerWindow::resetStatistics() { skippedFrames_=submittedFrames_=0; frameMilliseconds_=0;infoFpsTimer_.invalidate();infoCurrentFps_=0;if(interpolationWarning_)showInterpolationWarning(false);suspendQualityCheck(); }
void PlayerWindow::toggleFullscreen() {
    suspendQualityCheck();
    if (isFullScreen()) { controls_->show();fullscreenTitle_->hide();showNormal(); if(!normalGeometry_.isEmpty()) restoreGeometry(normalGeometry_);
        if(!playlistPinned_ && playlistExpansion_>0){resize(qMax(minimumWidth(),width()-playlistExpansion_),height());playlistExpansion_=0;}
        dockPlaylist(); }
    else { normalGeometry_=saveGeometry(); controls_->show();chromeIdle_.restart();showFullScreen();dockPlaylist();pane_->setFocus(); }
}
int PlayerWindow::prefetchCount() const {
    if(imageMode_)return 0;
    if(!settings_->value("performance/predecode",true).toBool()) return 0;
    // Compute utilization is not queue pressure: stopping lookahead at 50% GPU
    // serialized RIFE inference precisely when concurrent work was needed.
    if(usage_.ram>=settings_->value("performance/ram",50).toInt() || usage_.vram>=95) return 0;
    const auto headroom=usage_.totalMemoryMiB*(settings_->value("performance/ram",50).toInt()-usage_.ram)/100.0;
    const double frameMiB=std::max(1.0,double(clip_.width)*clip_.height*8/1048576);
    return std::clamp(static_cast<int>(headroom/frameMiB),0,std::clamp(settings_->value("performance/frames",8).toInt(),1,16));
}
void PlayerWindow::applySettings(bool reopen) {
    const bool resume=playing_ || autoPlay_;
    const qint64 at=!imageMode_ && ready_?position():-1;
    usage_=resources_.sample(reopen);
    const int cpu=std::clamp(settings_->value("performance/cpu",100).toInt(),1,100);
    const auto headroom=usage_.totalMemoryMiB*(std::clamp(settings_->value("performance/ram",50).toInt(),1,100)-usage_.ram)/100.0+usage_.memoryMiB;
    server_->setResourceLimits(interpolationStage()>=0?qMax(1,QThread::idealThreadCount()):qMax(1,QThread::idealThreadCount()*cpu/100),std::clamp(static_cast<int>(headroom),64,8192));
    applyScaling();
    applyColorSettings();
    auto *visible=direct_?clock_.get():output_.get();
    visible->setOutputFormat(settings_->value("decode/output").toString());
    clock_->setDecodeMode(settings_->value("decode/mode",2).toUInt());
    clock_->setSoftwarePreScale(settings_->value("decode/softwarePreScaleHeight",0).toInt());
    volume_->setValue(std::clamp(settings_->value("player/volume",100).toInt(),0,100));
    mute_->setChecked(settings_->value("player/muted",false).toBool());
    setRate(settings_->value("player/speed",1).toDouble());
    subtitleVisible_=settings_->value("subtitle/visible",true).toBool();
    applyAppearance();
    dockPlaylist();
    settings_->sync(); if(reopen && !(discMenu_ && discMenu_->active()) && !source_.isEmpty() && !imageMode_) {const auto path=source_;if(openFile(path)){autoPlay_=resume;settingsPosition_=at;}}
}
void PlayerWindow::applyAppearance() {
    language_->setLanguage(settings_->value("basic/language","zh_CN").toString());
    language_->updateWidgets(this);
#ifdef VSR_LITE_PLAYER
    pane_->setPlaceholderText(tr("VS Player\n拖入视频，或按 Ctrl+O 打开\nJinc / D3D11 · Tab 视频信息"));
#else
    pane_->setPlaceholderText(tr("VS Player\n拖入视频，或按 Ctrl+O 打开\nCtrl+P 加载 VPY 预设 · Tab 视频信息"));
#endif
    const QColor color(settings_->value("theme/background","#202124").toString());
    QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{settings_->value("theme/chineseFont","Microsoft YaHei UI").toString()});
    const auto latin=settings_->value("theme/latinFont","Comic Sans MS").toString();
    QFont font(latin);font.setPointSizeF(9.75);qApp->setFont(font);setFont(font);
    auto style=property("playerBaseStyle").toString();if(color.isValid())style.replace("#202124",color.name());
    auto family=latin;family.replace('\\',"\\\\").replace('"',"\\\"");setStyleSheet(style+QString("QWidget{font-family:\"%1\";font-size:9.75pt;}").arg(family));
    setWindowOpacity(std::clamp(settings_->value("theme/opacity",100).toInt(),10,100)/100.0);
    const auto chromeStyle=[](const QString &name){return QString("QWidget#%1,QWidget#%1 QWidget{background:transparent;color:#e8eaed;} QWidget#%1 QPushButton,QWidget#%1 QLineEdit,QWidget#%1 QDoubleSpinBox{background:rgba(48,50,56,90);border:1px solid #70747b;} QWidget#%1 QTreeWidget::item:selected{background:rgba(66,109,167,150);} QWidget#%1 QMenu{background:#292b30;}").arg(name);};
    controls_->setStyleSheet(chromeStyle("playerControls"));playlistPanel_->setStyleSheet(chromeStyle("playerPlaylistPanel"));fullscreenTitle_->setStyleSheet(chromeStyle("playerFullscreenTitle"));
    static_cast<PlayerChromePanel *>(controls_)->setTransparency(settings_->value("theme/bottomTransparency",50).toInt());
    static_cast<PlayerChromePanel *>(playlistPanel_)->setTransparency(settings_->value("theme/playlistTransparency",50).toInt());
    settings_->setValue("theme/bottomTransparency",std::clamp(settings_->value("theme/bottomTransparency",50).toInt(),0,100));settings_->setValue("theme/playlistTransparency",std::clamp(settings_->value("theme/playlistTransparency",50).toInt(),0,100));
    for(auto *dialog:findChildren<QDialog *>())dialog->setFont(font);
    for(QTreeWidgetItemIterator item(playlist_);*item;++item){auto itemFont=(*item)->font(0);itemFont.setFamilies(font.families());(*item)->setFont(0,itemFont);}
}
bool PlayerWindow::saveConfiguration(const QString &path) {
    settings_->sync(); if(QFileInfo(path).absoluteFilePath()==QFileInfo(settings_->fileName()).absoluteFilePath()) return settings_->status()==QSettings::NoError;
    QSettings output(path,QSettings::IniFormat);output.clear();for(const auto &key:settings_->allKeys()) output.setValue(key,settings_->value(key));output.sync();return output.status()==QSettings::NoError;
}
bool PlayerWindow::loadConfiguration(const QString &path) {
    if(!QFileInfo(path).isFile()) return false; QSettings input(path,QSettings::IniFormat);QVariantMap values;
    for(const auto &key:input.allKeys()) values.insert(key,input.value(key)); if(input.status()!=QSettings::NoError || values.isEmpty()) return false;
    settings_->clear(); for(auto it=values.begin();it!=values.end();++it) settings_->setValue(it.key(),it.value());
    preset_=settings_->value("player/preset").toString(); if(!QFileInfo::exists(preset_)) preset_.clear(); interpolationAuto_=interpolationStage()>=0 && settings_->value("player/interpolationAuto",false).toBool(); applySettings(true); return true;
}
void PlayerWindow::showSettings() {
    QDialog dialog(this); dialog.setObjectName("playerSettings");dialog.setWindowTitle(tr("VS Player 设置 · %1").arg(VSR_VERSION));dialog.resize(880,620);
    auto *outer=new QVBoxLayout(&dialog);auto *body=new QHBoxLayout;outer->addLayout(body,1);auto *categories=new QListWidget(&dialog);categories->addItems({tr("基本设置"),tr("主题设置"),tr("播放设置"),tr("性能设置"),tr("解码设置"),tr("渲染设置"),tr("缓存设置"),tr("文件关联"),tr("组件下载")});categories->setFixedWidth(145);body->addWidget(categories);
    categories->ensurePolished();int categoryWidth=145;for(int i=0;i<categories->count();++i)categoryWidth=qMax(categoryWidth,categories->fontMetrics().horizontalAdvance(categories->item(i)->text())+32);categories->setFixedWidth(categoryWidth);
    auto *stack=new QStackedWidget(&dialog);body->addWidget(stack,1);connect(categories,&QListWidget::currentRowChanged,stack,&QStackedWidget::setCurrentIndex);
    const auto page=[&] {auto *scroll=new QScrollArea(stack);scroll->setWidgetResizable(true);auto *widget=new QWidget(scroll);scroll->setWidget(widget);stack->addWidget(scroll);auto *form=new QFormLayout(widget);form->setVerticalSpacing(14);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);return form;};
    auto *basic=page();auto *volume=new QSpinBox(&dialog);volume->setRange(0,100);volume->setSuffix(" %");volume->setValue(volume_->value());basic->addRow(tr("默认音量"),volume);
    auto *subtitles=new QCheckBox(tr("显示挂载字幕"),&dialog);subtitles->setChecked(subtitleVisible_);basic->addRow(subtitles);
    auto *configInfo=new QLabel(tr("所有设置自动保存在程序旁 player.ini。\n可将全部设置导出为一个 INI，或载入已有配置。\nEnter / F11：无边框全屏；Esc：退出全屏。"),&dialog);configInfo->setWordWrap(true);basic->addRow(configInfo);
    auto *language=new QComboBox(&dialog);language->setObjectName("playerLanguage");language->addItem(tr("中文"),"zh_CN");language->addItem("English","en_US");language->setCurrentIndex(qMax(0,language->findData(settings_->value("basic/language","zh_CN"))));basic->addRow(tr("语言"),language);
    auto *autoplay=new QCheckBox(tr("打开文件自动播放；关闭则准备首帧并暂停"),&dialog);autoplay->setObjectName("playerAutoplay");autoplay->setChecked(settings_->value("basic/autoplay",true).toBool());basic->addRow(autoplay);
    auto *screenshotRow=new QWidget(&dialog);auto *screenshotLayout=new QHBoxLayout(screenshotRow);screenshotLayout->setContentsMargins(0,0,0,0);auto *screenshotPath=new QLineEdit(screenshotDirectory(),screenshotRow);screenshotPath->setObjectName("playerScreenshotPath");screenshotLayout->addWidget(screenshotPath,1);auto *screenshotBrowse=new QPushButton(tr("选择文件夹…"),screenshotRow);screenshotLayout->addWidget(screenshotBrowse);basic->addRow(tr("默认截图保存路径"),screenshotRow);connect(screenshotBrowse,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getExistingDirectory(&dialog,tr("选择截图保存文件夹"),screenshotPath->text());if(!path.isEmpty())screenshotPath->setText(path);});
    basic->addRow(new QLabel(tr("当前版本：%1").arg(VSR_VERSION),&dialog));
    auto *theme=page();auto *chineseFont=new QFontComboBox(&dialog);chineseFont->setObjectName("playerChineseFont");chineseFont->setCurrentFont(QFont(settings_->value("theme/chineseFont","Microsoft YaHei UI").toString()));theme->addRow(tr("中文字体"),chineseFont);
    auto *latinFont=new QFontComboBox(&dialog);latinFont->setObjectName("playerLatinFont");latinFont->setCurrentFont(QFont(settings_->value("theme/latinFont","Comic Sans MS").toString()));theme->addRow(tr("西文字体"),latinFont);
    auto *background=new QPushButton(settings_->value("theme/background","#202124").toString(),&dialog);background->setObjectName("playerBackground");theme->addRow(tr("背景色"),background);connect(background,&QPushButton::clicked,&dialog,[&]{const auto color=QColorDialog::getColor(QColor(background->text()),&dialog);if(color.isValid())background->setText(color.name());});
    auto *opacity=new QSpinBox(&dialog);opacity->setObjectName("playerOpacity");opacity->setRange(10,100);opacity->setSuffix(" %");opacity->setValue(settings_->value("theme/opacity",100).toInt());theme->addRow(tr("窗口不透明度"),opacity);
    const auto transparency=[&](const QString &key,const QString &name,const QString &label){auto *value=new QSpinBox(&dialog);value->setObjectName(name);value->setRange(0,100);value->setSuffix(" %");value->setValue(settings_->value(key,50).toInt());value->setToolTip(tr("0% 为不透明，100% 为透明；按钮和文字保持可读。"));theme->addRow(label,value);return value;};
    auto *bottomTransparency=transparency("theme/bottomTransparency","playerBottomTransparency",tr("底部透明度"));
    auto *playlistTransparency=transparency("theme/playlistTransparency","playerPlaylistTransparency",tr("右侧播放列表透明度"));
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
    auto *resizeFirst=new QCheckBox(tr("先 Jinc 到目标分辨率，再做增强(降低过大源的处理量)"),&dialog);resizeFirst->setChecked(settings_->value("performance/resizeBeforeEnhance",false).toBool());resizeFirst->setToolTip(tr("自动识别大于显示器分辨率的视频并降低分辨率。增强滤镜的分辨率越高，性能开销越大；小于目标分辨率的源保持原尺寸，不会先拉伸再增强。"));performance->addRow(resizeFirst);
    auto *resizeExplanation=new QLabel(resizeFirst->toolTip(),&dialog);resizeExplanation->setWordWrap(true);performance->addRow(resizeExplanation);
    auto *limitInfo=new QLabel(tr("占用阈值控制后台预解码；达到阈值便停止追加。CPU 同时限制 VS 工作线程，内存限制缓存目标。当前帧不强行中断，显存/缓存由插件分配，无法保证全机占用绝不超限。"),&dialog);limitInfo->setWordWrap(true);performance->addRow(limitInfo);
    auto *decode=page();auto *core=new QComboBox(&dialog);core->setObjectName("playerVideoDecoder");core->addItem("3FPlayer (FFF.Native.dll)","3FP");
#ifndef VSR_LITE_PLAYER
    core->addItem("LAV Video Decoder (LAVVideo.ax)","LAV");
#endif
    core->setCurrentIndex(qMax(0,core->findData(settings_->value("decode/video",settings_->value("player/core","3FP")))));decode->addRow(tr("视频解码器"),core);
    auto *audioCore=new QComboBox(&dialog);audioCore->setObjectName("playerAudioDecoder");audioCore->addItem("3FPlayer (FFF.Native.dll)","3FP");
#ifndef VSR_LITE_PLAYER
    audioCore->addItem("LAV Audio Decoder (LAVAudio.ax)","LAV");
#endif
    audioCore->setCurrentIndex(qMax(0,audioCore->findData(settings_->value("decode/audio",settings_->value("player/core","3FP")))));decode->addRow(tr("音频解码器"),audioCore);
#ifndef VSR_LITE_PLAYER
    auto *lavAudioConfig=new QPushButton(tr("打开 LAV Audio 配置…"),&dialog);connect(lavAudioConfig,&QPushButton::clicked,&dialog,[this,&dialog]{if(lav_)lav_->showAudioSettings(reinterpret_cast<void *>(dialog.winId()));else{LavPlayback config;config.showAudioSettings(reinterpret_cast<void *>(dialog.winId()));}});
    auto *lavConfig=new QPushButton(tr("打开 LAV Video 配置…"),&dialog);
    connect(lavConfig,&QPushButton::clicked,&dialog,[this,&dialog]{if(lav_)lav_->showVideoSettings(reinterpret_cast<void *>(dialog.winId()));else {LavPlayback config;config.showVideoSettings(reinterpret_cast<void *>(dialog.winId()));}});
#endif
    auto *hardware=new QComboBox(&dialog);hardware->addItem(tr("3FPlayer · D3D11 硬件解码(不可用时软件回退)"),2);hardware->addItem(tr("3FPlayer · FFmpeg CPU 软件解码"),1);hardware->setCurrentIndex(settings_->value("decode/mode",2).toInt()==2?0:1);decode->addRow(tr("3FP 解码方式"),hardware);
    auto *output=new QComboBox(&dialog);output->setObjectName("playerOutputFormat");
#ifdef VSR_LITE_PLAYER
    output->addItem(tr("原生输出(默认)"),QString());
#else
    output->addItem(tr("保持 VS 输出(默认)"),QString());
#endif
    auto *preScale=new QComboBox(&dialog);preScale->setObjectName("playerSoftwarePreScale");preScale->addItem(tr("关闭(保持完整源分辨率)"),0);preScale->addItem("2160p",2160);preScale->addItem("1080p",1080);preScale->addItem("720p",720);preScale->setCurrentIndex(qMax(0,preScale->findData(settings_->value("decode/softwarePreScaleHeight",0))));decode->addRow(tr("CPU 预缩放(实验)"),preScale);preScale->setToolTip(tr("仅作用于 CPU 软解：在上传前按比例缩小，保留位深和色度格式；降低传输/渲染开销，但牺牲空间细节。硬解与 VS 输出不受影响。"));
    const QStringList formats{"yuv420p","yuv420p10le","yuv420p16le","yuv422p","yuv422p10le","yuv422p16le","yuv444p","yuv444p10le","yuv444p16le","nv12","p010le","p016le","p210le","p216le","uyvy422","yuyv422","yvyu422","ayuv64le","y210le","y216le","xv30le","xv48le","rgb24","bgr24","rgb0","bgra","rgb565le","rgb555le","rgb48le","rgba64le"};
    for(const auto &format:formats)output->addItem(format,format);output->setCurrentIndex(qMax(0,output->findData(settings_->value("decode/output").toString())));decode->addRow(tr("3FP 输出像素格式"),output);
    auto *colorConfig=new QPushButton(tr("打开3FP 解码配置…"),&dialog);colorConfig->setObjectName("playerColorConfig");decode->addRow(colorConfig);connect(colorConfig,&QPushButton::clicked,this,&PlayerWindow::showColorSettings);
#ifndef VSR_LITE_PLAYER
    decode->addRow(lavConfig);decode->addRow(lavAudioConfig);
#endif
    auto *decodeInfo=new QLabel(tr("VS 源解码仍由 VPY 定义；LAV 视频选项用于本地文件播放时钟图。音频可独立使用 LAVAudio.ax。网络固定 3FP；内置原生视频路径使用 3FP。倍速音频使用 3FP / atempo。madVR 视频固定 LAV。"),&dialog);decodeInfo->setWordWrap(true);decode->addRow(decodeInfo);
    auto *render=page();auto *renderer=new QComboBox(&dialog);renderer->addItems({"VS","madVR"});renderer->setCurrentText(settings_->value("player/renderer","VS").toString());render->addRow(tr("视频渲染器"),renderer);
    auto *stableViewport=new QCheckBox(tr("划出播放列表/底部控制时不改变渲染分辨率"),&dialog);stableViewport->setObjectName("playerStableViewport");stableViewport->setChecked(settings_->value("render/stableViewport",true).toBool());stableViewport->setToolTip(tr("控制界面覆盖视频，保持视频视口和渲染分辨率，避免重新分配渲染资源及重建增强链。"));render->addRow(stableViewport);
    auto *animeStage=new QComboBox(&dialog);animeStage->setObjectName("playerAnimeStage");animeStage->addItems(qualityNames());animeStage->setCurrentIndex(std::clamp(settings_->value("player/animeStage",0).toInt(),0,5));render->addRow(tr("Anime 起始档位"),animeStage);
    auto *interpolationStart=new QComboBox(&dialog);interpolationStart->setObjectName("playerInterpolationStart");interpolationStart->addItems(interpolationNames().mid(0,4));interpolationStart->setCurrentIndex(std::clamp(settings_->value("player/interpolationStart",0).toInt(),0,3));render->addRow(tr("补帧起始档位"),interpolationStart);
    auto *animeInfo=new QLabel(tr("仅作用于自动 Anime 预设；持续丢帧超过 5% 按列表顺序降载。开发者内置的六个手动版本固定档位，不自动切换。no CNN 使用梯度 / DoG 线条处理，不执行神经网络。"),&dialog);animeInfo->setWordWrap(true);render->addRow(animeInfo);
    auto *antiring=new QCheckBox(tr("Anti-ringing · relaxed(仅 Jinc，强度 0.5)"),&dialog);antiring->setChecked(settings_->value("render/antiring",true).toBool());render->addRow(antiring);
    auto *renderInfo=new QLabel(tr("VS：VPY 实时滤镜及原生 D3D11 呈现。\nmadVR：直接加载随附 madVR64.ax，处理设置交给 madVR；VS 预设不生效。\n默认放大/缩小采用 Jinc，同尺寸跳过缩放。\n抗振铃采用开放的局部范围约束，不宣称复刻 madVR 专有实现。"),&dialog);renderInfo->setWordWrap(true);render->addRow(renderInfo);
#ifdef VSR_LITE_PLAYER
    renderer->clear();renderer->addItem("D3D11");
    core->setEnabled(false);audioCore->setEnabled(false);
    decodeInfo->setText(tr("3FP 原生解码与显示，支持 D3D11 硬解和 FFmpeg 软解。"));
    output->setItemText(0,tr("保持原生输出(默认)"));
    render->labelForField(animeStage)->setVisible(false);animeStage->hide();
    render->labelForField(interpolationStart)->setVisible(false);interpolationStart->hide();animeInfo->hide();
    resizeFirst->hide();resizeExplanation->hide();frames->hide();performance->labelForField(frames)->hide();
    limitInfo->setText(tr("原生解码器使用自身的有界缓冲队列。"));
    renderInfo->setText(tr("Jinc / D3D11 两档；默认 D3D11 原生直通。"));
    delete categories->takeItem(8);
#endif
    auto *cache=page();auto *cacheMode=new QComboBox(&dialog);cacheMode->setObjectName("playerCacheMode");cacheMode->addItem(tr("软件目录(默认)"),"default");cacheMode->addItem(tr("自定义目录"),"custom");cacheMode->setCurrentIndex(settings_->value("cache/path").toString().isEmpty()?0:1);cache->addRow(tr("索引缓存位置"),cacheMode);
    auto *cachePath=new QLineEdit(playerCacheDirectory(*settings_),&dialog);cachePath->setObjectName("playerCachePath");cache->addRow(tr("存储路径"),cachePath);auto *browse=new QPushButton(tr("浏览…"),&dialog);cache->addRow(browse);connect(browse,&QPushButton::clicked,&dialog,[&]{const auto directory=QFileDialog::getExistingDirectory(&dialog,tr("选择索引缓存目录"),cachePath->text());if(!directory.isEmpty()){cachePath->setText(directory);cacheMode->setCurrentIndex(1);}});
    auto *cacheSize=new QLabel(&dialog);cacheSize->setObjectName("playerCacheSize");cache->addRow(tr("当前缓存大小"),cacheSize);
    const auto updateCache=[&]{const auto directory=cacheMode->currentIndex()==0?QDir(QCoreApplication::applicationDirPath()).filePath("cache/indexes"):cachePath->text();cacheSize->setText(QString::number(playerCacheBytes(directory)/1048576.0,'f',2)+" MiB");cachePath->setEnabled(cacheMode->currentIndex()==1);};
    connect(cacheMode,&QComboBox::currentIndexChanged,&dialog,[&]{if(cacheMode->currentIndex()==0)cachePath->setText(QDir(QCoreApplication::applicationDirPath()).filePath("cache/indexes"));updateCache();});connect(cachePath,&QLineEdit::textChanged,&dialog,updateCache);updateCache();
    auto *clearCache=new QPushButton(tr("清除视频寻帧索引缓存(ffindex / lwi)"),&dialog);clearCache->setObjectName("playerClearCache");cache->addRow(clearCache);connect(clearCache,&QPushButton::clicked,&dialog,[&]{const bool ok=clearPlayerIndexes(cachePath->text());updateCache();setError(ok?tr("寻帧索引缓存已清除，视频文件保持不变。"):tr("部分索引缓存正在使用或无法删除。"));});
    auto *cacheInfo=new QLabel(tr("索引记录关键帧和寻帧位置，避免重复扫描本地媒体；不缓存视频画面。网络直通不建立 VS 索引。路径、大小、修改时间变化会使用新索引。清除只处理该目录中的 ffindex / lwi 文件。"),&dialog);cacheInfo->setWordWrap(true);cache->addRow(cacheInfo);
    auto *associations=page();auto *associationFormats=new QListWidget(&dialog);associationFormats->setObjectName("playerAssociationFormats");const auto videoFormats=playerVideoExtensions(),audioFormats=playerAudioExtensions(),imageFormats=playerImageExtensions();const auto selected=settings_->value("associations/extensions").toStringList();for(const auto &extension:videoFormats+audioFormats+imageFormats){auto *item=new QListWidgetItem('.'+extension,associationFormats);item->setData(Qt::UserRole,extension);item->setCheckState(selected.contains(extension)?Qt::Checked:Qt::Unchecked);}associations->addRow(associationFormats);
    auto *choices=new QWidget(&dialog);auto *choiceRow=new QHBoxLayout(choices);choiceRow->setContentsMargins(0,0,0,0);const QStringList selections{tr("全选视频"),tr("全选音频"),tr("全选图片"),tr("全选所有"),tr("取消所有")};for(int n=0;n<5;++n){auto *button=new QPushButton(selections[n],&dialog);if(n==2)button->setObjectName("playerSelectImages");choiceRow->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[=]{for(int row=0;row<associationFormats->count();++row){auto *item=associationFormats->item(row);const auto extension=item->data(Qt::UserRole).toString();item->setCheckState(n==3 || (n==0 && videoFormats.contains(extension)) || (n==1 && audioFormats.contains(extension)) || (n==2 && imageFormats.contains(extension))?Qt::Checked:Qt::Unchecked);}});}associations->addRow(choices);
    auto *registerButton=new QPushButton(tr("注册所选格式到当前用户"),&dialog);registerButton->setObjectName("playerRegisterAssociations");associations->addRow(registerButton);connect(registerButton,&QPushButton::clicked,&dialog,[&]{QStringList extensions;for(int n=0;n<associationFormats->count();++n)if(associationFormats->item(n)->checkState()==Qt::Checked)extensions<<associationFormats->item(n)->data(Qt::UserRole).toString();if(registerPlayerAssociations(extensions,QDir(QCoreApplication::applicationDirPath()).filePath("vs-player.exe"))){settings_->setValue("associations/extensions",extensions);setError(tr("格式已注册；请在 Windows 默认应用中选择 VS Player。"));}else setError(tr("文件关联注册失败。"));});
    auto *defaultsButton=new QPushButton(tr("打开 Windows 默认应用…"),&dialog);associations->addRow(defaultsButton);connect(defaultsButton,&QPushButton::clicked,&dialog,[]{QDesktopServices::openUrl(QUrl("ms-settings:defaultapps"));});
    auto *associationInfo=new QLabel(tr("注册到当前用户，无需管理员权限。Windows 最终默认程序由用户选择；取消选择会移除 VS Player 的打开方式入口，不改写其他程序的默认关联。"),&dialog);associationInfo->setWordWrap(true);associations->addRow(associationInfo);
#ifndef VSR_LITE_PLAYER
    stack->addWidget(new ComponentDownloads(stack));
#endif
    categories->setCurrentRow(0);
    auto *footer=new QHBoxLayout;auto *load=new QPushButton(tr("加载预设…"),&dialog);load->setObjectName("playerLoadSettings");auto *save=new QPushButton(tr("保存预设…"),&dialog);save->setObjectName("playerSaveSettings");footer->addWidget(load);footer->addWidget(save);footer->addStretch();
    auto *cancel=new QPushButton(tr("取消(&N)"),&dialog);cancel->setObjectName("playerCancelSettings");auto *ok=new QPushButton(tr("确定(&Y)"),&dialog);ok->setObjectName("playerConfirmSettings");ok->setDefault(true);auto *apply=new QPushButton(tr("应用(&A)"),&dialog);apply->setObjectName("playerApplySettings");footer->addWidget(cancel);footer->addWidget(ok);footer->addWidget(apply);outer->addLayout(footer);
    const auto store=[&]{settings_->setValue("decode/softwarePreScaleHeight",preScale->currentData());settings_->setValue("screenshot/path",screenshotPath->text().trimmed());settings_->setValue("basic/language",language->currentData());settings_->setValue("basic/autoplay",autoplay->isChecked());settings_->setValue("theme/chineseFont",chineseFont->currentFont().family());settings_->setValue("theme/latinFont",latinFont->currentFont().family());settings_->setValue("theme/background",background->text());settings_->setValue("theme/opacity",opacity->value());settings_->setValue("theme/bottomTransparency",bottomTransparency->value());settings_->setValue("theme/playlistTransparency",playlistTransparency->value());settings_->setValue("playback/remember",remember->isChecked());settings_->setValue("playback/multithread",multithread->isChecked());settings_->setValue("playback/arrows",arrows->currentData());settings_->setValue("playback/ctrlSeconds",ctrl->value());settings_->setValue("playback/ctrlAltSeconds",ctrlAlt->value());settings_->setValue("cache/path",cacheMode->currentIndex()==0?QString():QFileInfo(cachePath->text()).absoluteFilePath());settings_->setValue("player/volume",volume->value());settings_->setValue("subtitle/visible",subtitles->isChecked());settings_->setValue("performance/predecode",predecode->isChecked());settings_->setValue("performance/frames",frames->value());settings_->setValue("performance/resizeBeforeEnhance",resizeFirst->isChecked());for(int n=0;n<4;++n)settings_->setValue("performance/"+keys[n],limits[n]->value());settings_->setValue("player/core",core->currentData());settings_->setValue("decode/video",core->currentData());settings_->setValue("decode/audio",audioCore->currentData());settings_->setValue("decode/mode",hardware->currentData());settings_->setValue("decode/output",output->currentData());settings_->setValue("player/renderer",
#ifdef VSR_LITE_PLAYER
        "VS"
#else
        renderer->currentText()
#endif
    );settings_->setValue("render/antiring",antiring->isChecked());settings_->setValue("render/stableViewport",stableViewport->isChecked());settings_->setValue("player/animeStage",animeStage->currentIndex());settings_->setValue("player/interpolationStart",interpolationStart->currentIndex());QStringList extensions;for(int n=0;n<associationFormats->count();++n)if(associationFormats->item(n)->checkState()==Qt::Checked)extensions<<associationFormats->item(n)->data(Qt::UserRole).toString();settings_->setValue("associations/extensions",extensions);};
    connect(apply,&QPushButton::clicked,&dialog,[&]{store();applySettings(true);language_->updateWidgets(&dialog);});
    connect(save,&QPushButton::clicked,&dialog,[&]{const auto file=QFileDialog::getSaveFileName(&dialog,tr("保存完整配置"),"vs-player.ini","INI (*.ini)");if(!file.isEmpty()){store();if(!saveConfiguration(file))setError(tr("配置保存失败。"));}});
    connect(load,&QPushButton::clicked,&dialog,[&]{const auto file=QFileDialog::getOpenFileName(&dialog,tr("加载完整配置"),{},"INI (*.ini)");if(!file.isEmpty()){if(loadConfiguration(file))dialog.reject();else setError(tr("配置文件无效。"));}});
    connect(ok,&QPushButton::clicked,&dialog,&QDialog::accept);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
    language_->updateWidgets(&dialog);
    if(dialog.exec()==QDialog::Accepted){store();applySettings(true);}
}
}
