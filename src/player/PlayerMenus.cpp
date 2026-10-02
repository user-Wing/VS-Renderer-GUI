#include "player/PlayerWindow.h"
#include "player/PlayerMenu.h"
#include "player/PlayerSubtitles.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "graph/PresetStore.h"
#include "ui/PreviewPane.h"
#include <QSettings>
#include <QMenu>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QFileDialog>
#include <QProcess>
#include <QPainter>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace vsr {
namespace {
QVariantMap subtitleStyle(QSettings &settings) {QVariantMap out;for(const auto &key:settings.allKeys())if(key.startsWith("subtitle/style/"))out.insert(key.mid(15),settings.value(key));return out;}
}
void PlayerWindow::selectSubtitle(int slot,int stream) {
    if(slot==0)primarySubtitle_=stream;else secondarySubtitle_=stream;
    settings_->setValue(slot==0?"subtitle/primary":"subtitle/secondary",stream);
    if(stream==-1) {subtitles_->load(slot,{},-1,{},{});return;}
    const auto external=slot==0?externalSubtitle_:externalSecondarySubtitle_;
    if(stream==-2 && !external.isEmpty()) {subtitles_->load(slot,external,-1,QFileInfo(external).suffix().toLower(),subtitleStyle(*settings_));return;}
    QString codec;for(const auto &entry:media_.value("streams").toArray()){const auto item=entry.toObject();if(item.value("index").toInt()==stream){codec=item.value("codec").toString();break;}}
    subtitles_->load(slot,mediaInput_,stream,codec,subtitleStyle(*settings_));
}
void PlayerWindow::attachSubtitle(const QString &path,int slot) {
    if(!deferred_.isEmpty()) {deferredSubtitle_=path;deferredSubtitleSlot_=slot;return;}
    if(!QFileInfo(path).isFile() || imageMode_)return;
    (slot==0?externalSubtitle_:externalSecondarySubtitle_)=QFileInfo(path).absoluteFilePath();
    selectSubtitle(slot,-2);message_->setText(tr("已挂载字幕：%1").arg(QFileInfo(path).fileName()));
}
void PlayerWindow::showContextMenu(const QPoint &position) {
    if(!imageMode_){const auto refreshed=QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();if(!refreshed.isEmpty())media_=refreshed;}
    auto *menu=new PlayerMenu(this);menu->setObjectName("playerContextMenu");menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(tr("打开文件…"),this,&PlayerWindow::chooseFiles);
    menu->addAction(tr("打开文件夹…"),this,&PlayerWindow::chooseFolder);
    menu->addAction(tr("打开链接…"),this,&PlayerWindow::chooseLink);
    auto *playlist=menu->addAction(tr("播放列表"));playlist->setCheckable(true);playlist->setChecked(playlistPinned_);connect(playlist,&QAction::toggled,this,&PlayerWindow::setPlaylistPinned);
    auto *wheel=PlayerMenu::add(menu,tr("鼠标滚轮"));auto *wheelGroup=new QActionGroup(wheel);for(const auto &mode:QStringList{"volume","zoom"}){auto *action=wheel->addAction(mode=="volume"?tr("音量调节"):imageMode_?tr("放大图片"):tr("放大视频"));action->setCheckable(true);action->setChecked((imageMode_?QString("zoom"):settings_->value("player/wheel","volume").toString())==mode);action->setEnabled(!imageMode_ || mode=="zoom");wheelGroup->addAction(action);connect(action,&QAction::triggered,this,[this,mode]{settings_->setValue("player/wheel",mode);});}
    menu->addSeparator();
    auto *presets=PlayerMenu::add(menu,tr("VapourSynth 预设"));
    const auto presetMenu=[&](const QString &title,const QString &directory) {auto *list=PlayerMenu::add(presets,title);const QDir dir(directory);for(const auto &name:dir.entryList({"*.vpy"},QDir::Files,QDir::Name)){if(directory.endsWith("/builtin") && name!="Anime.vpy" && name!="Realistic.vpy" && !name.startsWith("Anime-"))continue;auto label=name;if(directory.endsWith("/builtin")){if(name=="Anime.vpy")label=tr("Anime · 自动切换");else if(name.startsWith("Anime-")){const int stage=name.mid(6,1).toInt();if(stage>=0 && stage<6)label=tr("手动 · %1").arg(qualityNames().at(stage));}}auto *action=list->addAction(label,this,[this,path=dir.filePath(name)]{loadPreset(path);});action->setCheckable(true);action->setChecked(QFileInfo(preset_).absoluteFilePath()==QFileInfo(dir.filePath(name)).absoluteFilePath());}if(list->isEmpty()){auto *empty=list->addAction(tr("暂无预设"));empty->setEnabled(false);}return list;};
    presetMenu(tr("开发者内置"),QDir(PresetStore::directory()).filePath("builtin"));auto *user=presetMenu(tr("用户自定义"),PresetStore::directory());user->addSeparator();user->addAction(tr("加载 VPY 文件…"),this,[this]{const auto file=QFileDialog::getOpenFileName(this,tr("加载 VPY"),PresetStore::directory(),"VapourSynth (*.vpy)");if(!file.isEmpty())loadPreset(file);});
    presets->addAction(tr("停用预设 · 原画"),this,[this]{loadPreset({});});presets->setEnabled(!imageMode_);
    auto *audio=PlayerMenu::add(menu,tr("音频设置"));audio->setEnabled(!imageMode_ && !source_.isEmpty());
    auto *audioTracks=PlayerMenu::add(audio,tr("声音轨道"));auto *audioGroup=new QActionGroup(audioTracks);
    const auto selected=clock_->snapshot().selectedAudioStream;
    for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()!="audio")continue;const int index=stream.value("index").toInt();const auto tags=stream.value("tags").toObject();auto *action=audioTracks->addAction(QString("%1 · %2 · %3 %4 · %5 ch").arg(index).arg(stream.value("codec").toString(),tags.value("language").toString(),tags.value("title").toString()).arg(stream.value("channels").toInt()),this,[this,index]{selectAudio(index);});action->setCheckable(true);action->setChecked(externalAudio_.isEmpty() && selected==index);audioGroup->addAction(action);}
    if(!externalAudio_.isEmpty()){auto *mounted=audioTracks->addAction(tr("挂载：%1").arg(QFileInfo(externalAudio_).fileName()));mounted->setCheckable(true);mounted->setChecked(true);audioGroup->addAction(mounted);audioTracks->addAction(tr("卸载外部音频"),this,[this]{if(clock_->clearExternalAudio())externalAudio_.clear();});}
    audioTracks->addSeparator();audioTracks->addAction(tr("挂载音频文件…"),this,[this]{const auto path=QFileDialog::getOpenFileName(this,tr("挂载音频"),{},"Audio (*.mka *.aac *.ac3 *.dts *.eac3 *.flac *.m4a *.mp3 *.ogg *.opus *.wav *.wma)");if(!path.isEmpty())attachAudio(path);});
    auto *sync=PlayerMenu::add(audio,tr("声音同步"));sync->addAction(tr("复位"),this,[this]{audioDelay_=0;applyAudioEffects();});sync->addAction(tr("滞后 0.1s"),this,[this]{audioDelay_+=1000000;applyAudioEffects();});sync->addAction(tr("提前 0.1s"),this,[this]{audioDelay_-=1000000;applyAudioEffects();});auto *offset=sync->addAction(tr("当前偏移：%1 s").arg(audioDelay_/10000000.0,0,'f',1));offset->setEnabled(false);
    auto *muted=audio->addAction(tr("静音"));muted->setCheckable(true);muted->setChecked(mute_->isChecked());connect(muted,&QAction::toggled,mute_,&QPushButton::setChecked);
    audio->addAction(tr("均衡器…"),this,&PlayerWindow::showEqualizer);
    auto *subtitles=PlayerMenu::add(menu,tr("字幕设置"));
    const auto tracks=[&](const QString &title,int slot) {auto *list=PlayerMenu::add(subtitles,title);auto *group=new QActionGroup(list);group->setExclusive(true);auto *off=list->addAction(tr("无"),this,[this,slot]{selectSubtitle(slot,-1);});off->setCheckable(true);off->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==-1);group->addAction(off);
        const auto external=slot==0?externalSubtitle_:externalSecondarySubtitle_;
        if(!external.isEmpty()) {auto *mounted=list->addAction(tr("挂载：%1").arg(QFileInfo(external).fileName()),this,[this,slot]{selectSubtitle(slot,-2);});mounted->setCheckable(true);mounted->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==-2);group->addAction(mounted);}
        for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()!="subtitle")continue;const int index=stream.value("index").toInt();const auto tags=stream.value("tags").toObject();const auto label=QString("%1 · %2 · %3 %4").arg(index).arg(stream.value("codec").toString(),tags.value("language").toString(),tags.value("title").toString());auto *action=list->addAction(label,this,[this,slot,index]{selectSubtitle(slot,index);});action->setCheckable(true);action->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==index);group->addAction(action);}
    };
    tracks(tr("选择字幕 · 底部"),0);tracks(tr("选择次字幕 · 顶部"),1);
    auto *visible=subtitles->addAction(tr("显示字幕"));visible->setCheckable(true);visible->setChecked(subtitleVisible_);connect(visible,&QAction::toggled,this,[this](bool on){subtitleVisible_=on;settings_->setValue("subtitle/visible",on);});
    subtitles->addAction(tr("切换首 / 次字幕"),this,[this]{{const auto primary=primarySubtitle_;std::swap(externalSubtitle_,externalSecondarySubtitle_);selectSubtitle(0,secondarySubtitle_);selectSubtitle(1,primary);}});
    subtitles->addAction(tr("挂载字幕文件…"),this,[this]{const auto path=QFileDialog::getOpenFileName(this,tr("挂载字幕"),{},"Subtitles (*.ass *.ssa *.srt *.sup)");if(!path.isEmpty())attachSubtitle(path);});
    subtitles->addAction(tr("卸载挂载字幕，恢复内置选择"),this,[this]{externalSubtitle_.clear();externalSecondarySubtitle_.clear();selectSubtitle(0,-1);selectSubtitle(1,-1);})->setEnabled(!externalSubtitle_.isEmpty() || !externalSecondarySubtitle_.isEmpty());
    subtitles->addAction(tr("字幕样式设置…（SRT）"),this,&PlayerWindow::showSubtitleStyle);
    subtitles->setEnabled(!imageMode_);
    auto *capture=PlayerMenu::add(menu,tr("图像截取"));capture->addAction(imageMode_?tr("截取图片原始像素 PNG"):tr("截取当前源画面 · VS 处理后 PNG"),this,[this]{captureImage(true);})->setEnabled(!madvrMode() || imageMode_);capture->addAction(tr("截取实画面 · 含字幕及显示效果 PNG"),this,[this]{captureImage(false);});
    auto *scaling=PlayerMenu::add(menu,tr("缩放算法"));const QStringList algorithms{"Nearest","Bilinear","Cubic","Lanczos3","Jinc","Spline36","Super-XBR","D3D11 Native"};
    for(int side=0;side<2;++side){auto *list=PlayerMenu::add(scaling,side?tr("缩小"):tr("放大"));auto *group=new QActionGroup(list);const QString key=side?"render/downscale":"render/upscale";for(int n=0;n<algorithms.size();++n){auto *action=list->addAction(algorithms[n]);action->setCheckable(true);action->setChecked((fixedAnimeStage()>=4?(fixedAnimeStage()==4?4:7):settings_->value(key,4).toInt())==n);group->addAction(action);connect(action,&QAction::triggered,this,[this,key,n]{settings_->setValue(key,n);if(fixedAnimeStage()<0)qualityStage_=n==7?5:direct_?4:qualityStage_;suspendQualityCheck();if(n==7 && fixedAnimeStage()<0 && !profile().isEmpty() && !source_.isEmpty())refreshScript();else applyScaling();(direct_?clock_:output_)->redraw();});}list->setEnabled(!madvrMode() && fixedAnimeStage()<4);}
    auto *ring=scaling->addAction(tr("Anti-ringing · relaxed（仅 Jinc）"));ring->setCheckable(true);ring->setChecked(settings_->value("render/antiring",true).toBool());ring->setEnabled(!madvrMode() && fixedAnimeStage()!=5);connect(ring,&QAction::toggled,this,[this](bool enabled){settings_->setValue("render/antiring",enabled);(direct_?clock_:output_)->setAntiRinging(enabled);(direct_?clock_:output_)->redraw();});
    scaling->setEnabled(!imageMode_);
    menu->addSeparator();menu->addAction(isFullScreen()?tr("退出全屏 · Enter / Esc"):tr("全屏 · Enter"),this,&PlayerWindow::toggleFullscreen);menu->addAction(tr("设置…"),this,&PlayerWindow::showSettings);menu->popup(position);
}
void PlayerWindow::showSubtitleStyle() {
    QDialog dialog(this);dialog.setObjectName("playerSubtitleStyle");dialog.setWindowTitle(tr("字幕样式 · 仅 SRT"));dialog.resize(720,550);auto *outer=new QVBoxLayout(&dialog);outer->addWidget(new QLabel(tr("SRT 使用以下样式；ASS/SSA 保留原始高级样式与特效，SUP 保留位图。"),&dialog));auto *body=new QHBoxLayout;outer->addLayout(body);
    auto *fontGroup=new QGroupBox(tr("字体"),&dialog);auto *fontForm=new QFormLayout(fontGroup);body->addWidget(fontGroup);
    auto *font=new QFontComboBox(&dialog);font->setCurrentFont(QFont(settings_->value("subtitle/style/font","Segoe UI").toString()));fontForm->addRow(tr("默认字体"),font);
    QHash<QString,QSpinBox *> numbers;
    const auto number=[&](QFormLayout *form,const QString &key,const QString &label,int value,int min,int max){auto *box=new QSpinBox(&dialog);box->setRange(min,max);box->setValue(settings_->value("subtitle/style/"+key,value).toInt());form->addRow(label,box);numbers.insert(key,box);return box;};
    number(fontForm,"size",tr("字号（1080p 基准）"),36,8,200);number(fontForm,"spacing",tr("字间距"),0,-20,100);number(fontForm,"scaleX",tr("平面宽度 %"),100,10,300);number(fontForm,"scaleY",tr("平面高度 %"),100,10,300);
    auto *bold=new QCheckBox(tr("粗体"),&dialog);bold->setChecked(settings_->value("subtitle/style/bold",false).toBool());fontForm->addRow(bold);auto *italic=new QCheckBox(tr("斜体"),&dialog);italic->setChecked(settings_->value("subtitle/style/italic",false).toBool());fontForm->addRow(italic);
    auto *position=new QGroupBox(tr("位置与比例"),&dialog);auto *positionForm=new QFormLayout(position);body->addWidget(position);auto *alignment=new QComboBox(&dialog);alignment->addItem(tr("底部居中"),2);alignment->addItem(tr("底部靠左"),1);alignment->addItem(tr("底部靠右"),3);alignment->setCurrentIndex(qMax(0,alignment->findData(settings_->value("subtitle/style/alignment",2).toInt())));positionForm->addRow(tr("段落对齐"),alignment);
    number(positionForm,"left",tr("左侧间距"),40,0,500);number(positionForm,"right",tr("右侧间距"),40,0,500);number(positionForm,"margin",tr("底部 / 次字幕顶部间距"),48,0,500);
    auto *colors=new QGroupBox(tr("颜色、透明度、边框与阴影"),&dialog);auto *colorForm=new QFormLayout(colors);outer->addWidget(colors);QHash<QString,QPushButton *> choices;
    const QStringList keys{"color","outlineColor","shadowColor"};const QStringList names{tr("默认"),tr("边框"),tr("阴影")};
    for(int n=0;n<3;++n){auto *row=new QWidget(&dialog);auto *line=new QHBoxLayout(row);line->setContentsMargins(0,0,0,0);auto *pick=new QPushButton(&dialog);pick->setProperty("color",settings_->value("subtitle/style/"+keys[n],n?"#000000":"#ffffff"));pick->setText(pick->property("color").toString());line->addWidget(pick);choices.insert(keys[n],pick);auto *opacity=new QSpinBox(&dialog);opacity->setRange(0,100);opacity->setSuffix(" % 不透明");opacity->setValue(settings_->value("subtitle/style/"+keys[n]+"Opacity",100).toInt());line->addWidget(opacity);numbers.insert(keys[n]+"Opacity",opacity);colorForm->addRow(names[n],row);connect(pick,&QPushButton::clicked,&dialog,[pick,&dialog]{const auto color=QColorDialog::getColor(QColor(pick->property("color").toString()),&dialog);if(color.isValid()){pick->setProperty("color",color.name());pick->setText(color.name());}});}
    number(colorForm,"outline",tr("边框尺寸"),2,0,12);number(colorForm,"shadow",tr("阴影深度"),2,0,12);auto *box=new QCheckBox(tr("不透明背景框"),&dialog);box->setChecked(settings_->value("subtitle/style/box",false).toBool());colorForm->addRow(box);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);outer->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted){settings_->setValue("subtitle/style/font",font->currentFont().family());settings_->setValue("subtitle/style/bold",bold->isChecked());settings_->setValue("subtitle/style/italic",italic->isChecked());settings_->setValue("subtitle/style/alignment",alignment->currentData());settings_->setValue("subtitle/style/box",box->isChecked());for(auto it=numbers.begin();it!=numbers.end();++it)settings_->setValue("subtitle/style/"+it.key(),it.value()->value());for(auto it=choices.begin();it!=choices.end();++it)settings_->setValue("subtitle/style/"+it.key(),it.value()->property("color"));selectSubtitle(0,primarySubtitle_);selectSubtitle(1,secondarySubtitle_);settings_->sync();}
}
void PlayerWindow::captureImage(bool source) {
    if(source && !imageMode_ && direct_){setError(tr("原生直通时请使用实画面截图；VS 未生成独立源帧。"));return;}
    if(source && !imageMode_ && displayedFrame_.planes[0].isEmpty()) {setError(tr("还没有可截取的 VS 帧。"));return;}
    const QDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"));QDir().mkpath(dir.absolutePath());const auto name=QString("%1-%2-%3.png").arg(QFileInfo(source_).completeBaseName(),QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz"),source?"VS":"display");const QString file=dir.filePath(name);
    if(imageMode_){const auto image=source?pane_->image():pane_->surface()->grab().toImage();if(!image.isNull() && image.save(file,"PNG"))setError(tr("已保存无损 PNG：%1").arg(file));else setError(tr("截图保存失败。"));return;}
    if(!source) {auto image=madvrMode()&&lav_?lav_->capture():(direct_?clock_:output_)->capture();if(image.isNull()){setError(tr("当前渲染器未返回可截取画面。"));return;}if(infoVisible_){QPainter painter(&image);const auto ratio=pane_->surface()->devicePixelRatioF();painter.scale(ratio,ratio);info_->render(&painter,info_->pos());}if(image.save(file,"PNG"))setError(tr("已保存无损 PNG：%1").arg(file));else setError(tr("截图保存失败。"));return;}
    const auto frame=displayedFrame_;QString format;int bytes=1,subW=0,subH=0;const unsigned value=static_cast<unsigned>(frame.format);
    if(value<10){format=value==1?"gray":"gray16le";bytes=value==1?1:2;}
    else {const int depth=value%10;bytes=depth?2:1;const int bits=depth==0?8:depth==1?10:depth==2?12:16;
        if(value>=40)format=bits==8?"gbrp":QString("gbrp%1le").arg(bits);
        else {const auto sampling=value>=30?"444":value>=20?"422":"420";format=QString("yuv%1p").arg(sampling)+(bits==8?QString():QString::number(bits)+"le");subW=value<30?1:0;subH=value<20?1:0;}}
    auto *process=new QProcess(this);process->setObjectName("playerScreenshotProcess");connect(process,&QProcess::errorOccurred,this,[this,process](QProcess::ProcessError error){if(error==QProcess::FailedToStart){setError(tr("截图进程无法启动：%1").arg(process->errorString()));process->deleteLater();}});connect(process,&QProcess::finished,this,[this,process,file](int code){setError(code==0?tr("已保存 VS 源 PNG：%1").arg(file):tr("截图失败：%1").arg(QString::fromUtf8(process->readAllStandardError())));process->deleteLater();});
    QStringList arguments{"-v","error","-f","rawvideo","-pixel_format",format,"-video_size",QString("%1x%2").arg(frame.width).arg(frame.height)};
    if(value>=10 && value<40) {const QHash<unsigned,QString> matrices{{1,"bt709"},{5,"bt470bg"},{6,"smpte170m"},{9,"bt2020nc"},{10,"bt2020c"}};if(matrices.contains(frame.colorMatrix))arguments<<"-colorspace"<<matrices.value(frame.colorMatrix);if(frame.colorRange)arguments<<"-color_range"<<(frame.colorRange==2?"pc":"tv");}
    arguments<<"-i"<<"pipe:0"<<"-frames:v"<<"1"<<"-pix_fmt"<<"rgb48be"<<"-y"<<file;
    process->start(QDir(QCoreApplication::applicationDirPath()).filePath("runtime/ffmpeg/ffmpeg.exe"),arguments);
    for(int plane=0;plane<4;++plane)if(!frame.planes[plane].isEmpty()){const int width=plane>0&&value<40?(frame.width+(1<<subW)-1)>>subW:frame.width;const int height=plane>0&&value<40?(frame.height+(1<<subH)-1)>>subH:frame.height;for(int y=0;y<height;++y)process->write(frame.planes[plane].constData()+y*frame.strides[plane],width*bytes);}process->closeWriteChannel();
}
}
