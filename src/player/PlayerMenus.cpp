#include "player/PlayerWindow.h"
#include "player/PlayerMenu.h"
#include "player/PlayerDiscMenu.h"
#include "player/PlayerTracks.h"
#include "player/PlayerSubtitles.h"
#include "player/PlayerMediaMatching.h"
#include "player/PlayerPng.h"
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
#include <QColorSpace>
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
#include <QSlider>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <QFutureWatcher>
#include <QtConcurrentRun>

namespace vsr {
namespace {
QVariantMap subtitleStyle(QSettings &settings) {QVariantMap out;for(const auto &key:settings.allKeys())if(key.startsWith("subtitle/style/"))out.insert(key.mid(15),settings.value(key));return out;}
}
void PlayerWindow::selectSubtitle(int slot,int stream) {
    if(slot==0)primarySubtitle_=stream;else secondarySubtitle_=stream;
    settings_->setValue(slot==0?"subtitle/primary":"subtitle/secondary",stream);
    if(stream==-1) {subtitles_->load(slot,{},-1,{},{});return;}
    const auto external=slot==0?externalSubtitle_:externalSecondarySubtitle_;
    if(stream==-2 && !external.isEmpty()) {QString codec=QFileInfo(external).suffix().toLower();for(const auto &entry:externalSubtitleTracks_[slot])if(entry.toObject().value("index").toInt()==externalSubtitleIndex_[slot])codec=entry.toObject().value("codec_name").toString();subtitles_->load(slot,external,externalSubtitleIndex_[slot],codec,subtitleStyle(*settings_));return;}
    QString codec;for(const auto &entry:media_.value("streams").toArray()){const auto item=entry.toObject();if(item.value("index").toInt()==stream){codec=item.value("codec").toString();break;}}
    subtitles_->load(slot,mediaInput_,stream,codec,subtitleStyle(*settings_));
}
void PlayerWindow::attachSubtitle(const QString &path,int slot) {
    if(!deferred_.isEmpty()) {deferredSubtitle_=path;deferredSubtitleSlot_=slot;return;}
    if(!QFileInfo(path).isFile() || imageMode_)return;
    (slot==0?externalSubtitle_:externalSecondarySubtitle_)=QFileInfo(path).absoluteFilePath();
    externalSubtitleTracks_[slot]={};externalSubtitleIndex_[slot]=-1;
    if(QFileInfo(path).suffix().compare("mks",Qt::CaseInsensitive)==0){const auto source=source_;auto *watcher=new QFutureWatcher<QJsonObject>(this);connect(watcher,&QFutureWatcher<QJsonObject>::finished,this,[this,watcher,path,slot,source]{const auto probe=watcher->result();watcher->deleteLater();if(source_!=source || (slot==0?externalSubtitle_:externalSecondarySubtitle_)!=QFileInfo(path).absoluteFilePath())return;
        for(const auto &entry:probe.value("streams").toArray())if(entry.toObject().value("codec_type").toString()=="subtitle")externalSubtitleTracks_[slot]<<entry;
        if(externalSubtitleTracks_[slot].isEmpty()){setError(tr("MKS 中未找到字幕轨。"));return;}externalSubtitleIndex_[slot]=externalSubtitleTracks_[slot].first().toObject().value("index").toInt();
        externalChapters_={};for(const auto &value:probe.value("chapters").toArray()){const auto c=value.toObject();externalChapters_<<QJsonObject{{"start100ns",double(qRound64(c.value("start_time").toString().toDouble()*10000000))},{"title",c.value("tags").toObject().value("title").toString()}};}applyBlurayMetadata();timeline_->setProperty("chapters",media_.value("chapters").toArray());selectSubtitle(slot,-2);message_->setText(tr("已挂载 MKS：%1 · %2 轨 · %3 章节").arg(QFileInfo(path).fileName()).arg(externalSubtitleTracks_[slot].size()).arg(externalChapters_.size()));
    });watcher->setFuture(QtConcurrent::run([path]{return PlayerAudioMetadata::probe(path);}));return;}
    selectSubtitle(slot,-2);message_->setText(tr("已挂载字幕：%1").arg(QFileInfo(path).fileName()));
}
void PlayerWindow::showContextMenu(const QPoint &position) {
    if(!imageMode_){const auto refreshed=QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();if(!refreshed.isEmpty())media_=refreshed;}
    applyBlurayMetadata();
    auto *menu=new PlayerMenu(this);menu->setObjectName("playerContextMenu");menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(tr("打开文件…"),this,&PlayerWindow::chooseFiles);
    menu->addAction(tr("打开文件夹…"),this,&PlayerWindow::chooseFolder);
    menu->addAction(tr("打开 BD 文件夹…"),this,&PlayerWindow::chooseBluRay);
    menu->addAction(tr("打开 BD 光盘镜像…"),this,[this]{const auto path=QFileDialog::getOpenFileName(this,tr("打开 BD 光盘镜像用于播放"),{},"Disc image (*.iso *.img *.vhd *.vhdx)");if(!path.isEmpty())openBluRay(path);});
    menu->addAction(tr("打开链接…"),this,&PlayerWindow::chooseLink);
#ifdef VSR_LITE_PLAYER
    if(!discRoot_.isEmpty()) {
        auto *titles=PlayerMenu::add(menu,tr("BD 原生节目菜单"));
        for(const auto &path:files_) {
            const auto name=blurayLabels_.value(path,QFileInfo(path).fileName());
            titles->addAction(name,this,[this,path]{openFile(path);});
        }
    }
#else
    if(!discRoot_.isEmpty())menu->addAction(tr("BD 主菜单 / 弹出菜单"),this,[this]{if(discMenu_ && discMenu_->active())discMenu_->topMenu();else openDiscMenu(discRoot_);});
#endif
    if(discMenu_ && discMenu_->active()){menu->addAction(tr("切回节目列表模式 (3FP)"),this,[this]{if(!files_.isEmpty())openFile(files_.first());});
        for(bool audio:{true,false}){auto *tracks=PlayerMenu::add(menu,audio?tr("音轨"):tr("字幕"));for(const auto &track:discMenu_->tracks(audio))tracks->addAction(track.second,this,[this,audio,id=track.first]{discMenu_->selectTrack(audio,id);});}
        menu->addAction(tr("设置…"),this,&PlayerWindow::showSettings);menu->addAction(tr("全屏 / 退出全屏"),this,&PlayerWindow::toggleFullscreen);menu->popup(position);return;
    }
    auto *playlist=menu->addAction(tr("播放列表"));playlist->setCheckable(true);playlist->setChecked(playlistPinned_);connect(playlist,&QAction::toggled,this,&PlayerWindow::setPlaylistPinned);
    auto *wheel=PlayerMenu::add(menu,tr("鼠标滚轮"));auto *wheelGroup=new QActionGroup(wheel);for(const auto &mode:QStringList{"volume","zoom"}){auto *action=wheel->addAction(mode=="volume"?tr("音量调节"):imageMode_?tr("放大图片"):tr("放大视频"));action->setCheckable(true);action->setChecked((imageMode_?QString("zoom"):settings_->value("player/wheel","volume").toString())==mode);action->setEnabled(!imageMode_ || mode=="zoom");wheelGroup->addAction(action);connect(action,&QAction::triggered,this,[this,mode]{settings_->setValue("player/wheel",mode);});}
    menu->addSeparator();
#ifdef VSR_LITE_PLAYER
    auto *passThrough=PlayerMenu::add(menu,tr("缩放模式"));
    passThrough->addAction(tr("Jinc"),this,[this]{loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-4-Jinc.vpy"));});
    passThrough->addAction(tr("D3D11 原生"),this,[this]{loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));});
#else
    auto *presets=PlayerMenu::add(menu,tr("VapourSynth 预设"));
    const auto presetMenu=[&](const QString &title,const QString &directory) {auto *list=PlayerMenu::add(presets,title);const QDir dir(directory);auto names=dir.entryList({"*.vpy"},QDir::Files,QDir::Name);if(directory.endsWith("/builtin")){names.removeAll("Anime.vpy");names.removeAll("Realistic.vpy");names.prepend("Realistic.vpy");names.prepend("Anime.vpy");}for(const auto &name:names){if(directory.endsWith("/builtin") && name!="Anime.vpy" && name!="Realistic.vpy" && !name.startsWith("Anime-"))continue;auto label=name;if(directory.endsWith("/builtin")){if(name=="Anime.vpy")label=tr("Anime · 自动切换");else if(name.startsWith("Anime-")){const int stage=name.mid(6,1).toInt();if(stage>=0 && stage<6)label=tr("手动 · %1").arg(qualityNames().at(stage));}}auto *action=list->addAction(label,this,[this,path=dir.filePath(name)]{loadPreset(path);});action->setCheckable(true);action->setChecked(QFileInfo(preset_).absoluteFilePath()==QFileInfo(dir.filePath(name)).absoluteFilePath());}if(list->isEmpty()){auto *empty=list->addAction(tr("暂无预设"));empty->setEnabled(false);}return list;};
    presetMenu(tr("开发者内置"),QDir(PresetStore::directory()).filePath("builtin"));auto *user=presetMenu(tr("用户自定义"),PresetStore::directory());user->addSeparator();user->addAction(tr("加载 VPY 文件…"),this,[this]{const auto file=QFileDialog::getOpenFileName(this,tr("加载 VPY"),PresetStore::directory(),"VapourSynth (*.vpy)");if(!file.isEmpty())loadPreset(file);});
    presets->addAction(tr("停用渲染 · Jinc 直通"),this,[this]{loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-4-Jinc.vpy"));});
    presets->addAction(tr("停用渲染 · D3D11 直通"),this,[this]{loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));});presets->setEnabled(!imageMode_);
#endif
#ifndef VSR_LITE_PLAYER
    auto *processing=PlayerMenu::add(menu,tr("图像处理"));auto *interpolation=PlayerMenu::add(processing,tr("补帧"));
    auto *interpolationGroup=new QActionGroup(interpolation);
    const auto interpolationChoice=[&](const QString &name,int stage,bool automatic,int renderer=0){auto *action=interpolation->addAction(name,this,[this,stage,automatic,renderer]{setInterpolation(stage,automatic,renderer);});action->setCheckable(true);action->setChecked(automatic?(interpolationAuto_ && interpolationRenderer()==renderer):(stage<0?interpolationStage()<0:!interpolationAuto_ && interpolationStage()==stage%4 && interpolationRenderer()==stage/4));interpolationGroup->addAction(action);};
    interpolationChoice(tr("自动补帧 · Jinc"),std::clamp(settings_->value("player/interpolationStart",0).toInt(),0,3),true,0);interpolationChoice(tr("自动补帧 · D3D11"),std::clamp(settings_->value("player/interpolationStart",0).toInt(),0,3),true,1);interpolation->addSeparator();
    for(int stage=0;stage<8;++stage)interpolationChoice(interpolationNames().at(stage%4)+(stage<4?tr(" · Jinc"):tr(" · D3D11")),stage,false);
    interpolation->addSeparator();interpolationChoice(tr("关闭"),-1,false);
    bool hasVideo=false;for(const auto &entry:media_.value("streams").toArray())if(entry.toObject().value("type").toString()=="video")hasVideo=true;
    processing->setEnabled(!imageMode_ && !madvrMode() && !networkSource() && (source_.isEmpty() || hasVideo));
#endif
    auto *audio=PlayerMenu::add(menu,tr("音频设置"));audio->setEnabled(!imageMode_ && !source_.isEmpty());
    auto *audioTracks=PlayerMenu::add(audio,tr("声音轨道"));auto *audioGroup=new QActionGroup(audioTracks);
    const auto selected=clock_->snapshot().selectedAudioStream;
    int audioOrdinal=0;
    for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()!="audio")continue;const int index=stream.value("index").toInt();auto *action=audioTracks->addAction(playerTrackLabel(stream,audioOrdinal++),this,[this,index]{selectAudio(index);});action->setToolTip(QString("%1 · %2 ch · stream 0:%3").arg(stream.value("codec").toString()).arg(stream.value("channels").toInt()).arg(index));action->setCheckable(true);action->setChecked(externalAudio_.isEmpty() && selected==index);audioGroup->addAction(action);}
    if(!externalAudio_.isEmpty()){auto *mounted=audioTracks->addAction(tr("挂载：%1").arg(QFileInfo(externalAudio_).fileName()));mounted->setCheckable(true);mounted->setChecked(true);audioGroup->addAction(mounted);audioTracks->addAction(tr("卸载外部音频"),this,[this]{if(clock_->clearExternalAudio())externalAudio_.clear();});}
    audioTracks->addSeparator();audioTracks->addAction(tr("挂载音频文件…"),this,[this]{const auto path=QFileDialog::getOpenFileName(this,tr("挂载音频"),{},"Audio (*.mka *.aac *.ac3 *.dts *.eac3 *.flac *.m4a *.mp3 *.ogg *.opus *.wav *.wma)");if(!path.isEmpty())attachAudio(path);})->setEnabled(!audioMode_);
    auto *sync=PlayerMenu::add(audio,tr("声音同步"));sync->addAction(tr("复位"),this,[this]{audioDelay_=0;applyAudioEffects();});sync->addAction(tr("滞后 0.1s"),this,[this]{audioDelay_+=1000000;applyAudioEffects();});sync->addAction(tr("提前 0.1s"),this,[this]{audioDelay_-=1000000;applyAudioEffects();});auto *offset=sync->addAction(tr("当前偏移：%1 s").arg(audioDelay_/10000000.0,0,'f',1));offset->setEnabled(false);
    auto *muted=audio->addAction(tr("静音"));muted->setCheckable(true);muted->setChecked(mute_->isChecked());connect(muted,&QAction::toggled,mute_,&QPushButton::setChecked);
    audio->addAction(tr("均衡器…"),this,&PlayerWindow::showEqualizer);
    auto *subtitles=PlayerMenu::add(menu,tr("字幕设置"));
    const auto matchedSubtitles=networkSource() || imageMode_ || audioMode_ || source_.isEmpty()?QStringList{}:playerMatchingSubtitles(source_);
    const auto tracks=[&](const QString &title,int slot) {auto *list=PlayerMenu::add(subtitles,title);auto *group=new QActionGroup(list);group->setExclusive(true);auto *off=list->addAction(tr("无"),this,[this,slot]{selectSubtitle(slot,-1);});off->setCheckable(true);off->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==-1);group->addAction(off);
        const auto external=slot==0?externalSubtitle_:externalSecondarySubtitle_;
        if(!external.isEmpty()) {auto *mounted=list->addAction(tr("挂载：%1").arg(QFileInfo(external).fileName()),this,[this,slot]{selectSubtitle(slot,-2);});mounted->setCheckable(true);mounted->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==-2);group->addAction(mounted);}
        for(const auto &path:matchedSubtitles)if(path!=external){auto *action=list->addAction(QFileInfo(path).fileName(),this,[this,path,slot]{attachSubtitle(path,slot);});action->setToolTip(path);action->setCheckable(true);group->addAction(action);}
        for(const auto &entry:externalSubtitleTracks_[slot]){const auto track=entry.toObject();const int index=track.value("index").toInt();auto *action=list->addAction(tr("MKS 轨 %1 · %2 · %3").arg(index+1).arg(track.value("codec_name").toString(),track.value("tags").toObject().value("language").toString()),this,[this,slot,index]{externalSubtitleIndex_[slot]=index;selectSubtitle(slot,-2);});action->setCheckable(true);action->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==-2 && externalSubtitleIndex_[slot]==index);group->addAction(action);}
        int ordinal=0;
        for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()!="subtitle")continue;const int index=stream.value("index").toInt();auto *action=list->addAction(playerTrackLabel(stream,ordinal++),this,[this,slot,index]{selectSubtitle(slot,index);});action->setToolTip(QString("%1 · stream 0:%2").arg(stream.value("codec").toString()).arg(index));action->setCheckable(true);action->setChecked((slot==0?primarySubtitle_:secondarySubtitle_)==index);group->addAction(action);}
    };
    tracks(tr("选择字幕 · 底部"),0);tracks(tr("选择次字幕 · 顶部"),1);
    auto *visible=subtitles->addAction(tr("显示字幕"));visible->setCheckable(true);visible->setChecked(subtitleVisible_);connect(visible,&QAction::toggled,this,[this](bool on){subtitleVisible_=on;settings_->setValue("subtitle/visible",on);});
    subtitles->addAction(tr("切换首 / 次字幕"),this,[this]{{const auto primary=primarySubtitle_;std::swap(externalSubtitle_,externalSecondarySubtitle_);std::swap(externalSubtitleTracks_[0],externalSubtitleTracks_[1]);std::swap(externalSubtitleIndex_[0],externalSubtitleIndex_[1]);selectSubtitle(0,secondarySubtitle_);selectSubtitle(1,primary);}});
    subtitles->addAction(tr("挂载字幕文件…"),this,[this]{const auto path=QFileDialog::getOpenFileName(this,tr("挂载字幕"),{},"Subtitles (*.ass *.ssa *.srt *.sup *.mks)");if(!path.isEmpty())attachSubtitle(path);});
    subtitles->addAction(tr("卸载挂载字幕，恢复内置选择"),this,[this]{externalSubtitle_.clear();externalSecondarySubtitle_.clear();externalSubtitleTracks_[0]={};externalSubtitleTracks_[1]={};externalSubtitleIndex_[0]=externalSubtitleIndex_[1]=-1;externalChapters_={};media_=QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();applyBlurayMetadata();timeline_->setProperty("chapters",media_.value("chapters").toArray());audioSubtitle_->hide();selectSubtitle(0,-1);selectSubtitle(1,-1);})->setEnabled(!externalSubtitle_.isEmpty() || !externalSecondarySubtitle_.isEmpty());
    subtitles->addAction(tr("字幕样式设置…(SRT)"),this,&PlayerWindow::showSubtitleStyle);
    subtitles->setEnabled(!imageMode_);
    if(!media_.value("chapters").toArray().isEmpty()){auto *chapters=PlayerMenu::add(menu,tr("章节"));for(const auto &value:media_.value("chapters").toArray()){const auto chapter=value.toObject();const auto at=qint64(chapter.value("start100ns").toDouble());chapters->addAction(chapter.value("title").toString()+QString(" · %1 s").arg(at/10000000.,0,'f',3),this,[this,at]{seekTime(at);});}}
    auto *capture=PlayerMenu::add(menu,tr("图像截取"));capture->addAction(imageMode_?tr("截取图片原始像素 · PNG 16-bit/通道"):direct_?tr("截取当前源画面 · PNG 16-bit/通道"):tr("截取当前源画面 · VS 处理后 PNG 16-bit/通道"),this,[this]{captureImage(true);})->setEnabled(!madvrMode() || imageMode_);capture->addAction(tr("截取实画面 · 含字幕及显示效果 PNG 16-bit/通道"),this,[this]{captureImage(false);});
    auto *captureTo=PlayerMenu::add(menu,tr("截图到指定路径…"));for(bool source:{true,false})captureTo->addAction(source?tr("源画面 · 选择文件夹保存…"):tr("实画面 · 选择文件夹保存…"),this,[this,source]{const auto directory=QFileDialog::getExistingDirectory(this,tr("选择截图保存文件夹"),screenshotDirectory());if(!directory.isEmpty())captureImage(source,directory);})->setEnabled(!source || !madvrMode() || imageMode_);
    auto *scaling=PlayerMenu::add(menu,tr("缩放算法"));const QStringList algorithms{"Nearest","Bilinear","Cubic","Lanczos3","Jinc","Spline36","Super-XBR","D3D11 Native","Lanczos4(4 taps)"};
    for(int side=0;side<2;++side){auto *list=PlayerMenu::add(scaling,side?tr("缩小"):tr("放大"));auto *group=new QActionGroup(list);const QString key=side?"render/downscale":"render/upscale";for(int n=0;n<algorithms.size();++n){
#ifdef VSR_LITE_PLAYER
        if(n != 4 && n != 7) continue;
#endif
        auto *action=list->addAction(algorithms[n]);action->setCheckable(true);action->setChecked((interpolationStage()>=0?(interpolationRenderer()?7:4):fixedAnimeStage()>=4?(fixedAnimeStage()==4?4:7):settings_->value(key,4).toInt())==n);group->addAction(action);connect(action,&QAction::triggered,this,[this,key,n]{settings_->setValue(key,n);if(fixedAnimeStage()<0)qualityStage_=n==7?5:direct_?4:qualityStage_;suspendQualityCheck();if(n==7 && fixedAnimeStage()<0 && !profile().isEmpty() && !source_.isEmpty())refreshScript();else applyScaling();(direct_?clock_:output_)->redraw();});}list->setEnabled(!madvrMode() && fixedAnimeStage()<4 && interpolationStage()<0);}
    auto *ring=scaling->addAction(tr("Anti-ringing · relaxed(仅 Jinc)"));ring->setCheckable(true);ring->setChecked(settings_->value("render/antiring",true).toBool());ring->setEnabled(!madvrMode() && fixedAnimeStage()!=5);connect(ring,&QAction::toggled,this,[this](bool enabled){settings_->setValue("render/antiring",enabled);(direct_?clock_:output_)->setAntiRinging(enabled);(direct_?clock_:output_)->redraw();});
    scaling->setEnabled(!imageMode_);
    menu->addSeparator();menu->addAction(isFullScreen()?tr("退出全屏 · Enter / Esc"):tr("全屏 · Enter"),this,&PlayerWindow::toggleFullscreen);menu->addAction(tr("设置…"),this,&PlayerWindow::showSettings);menu->popup(position);
}
void PlayerWindow::showSubtitleStyle() {
    QDialog dialog(this);dialog.setObjectName("playerSubtitleStyle");dialog.setWindowTitle(tr("字幕样式 · 仅 SRT"));dialog.resize(720,550);auto *outer=new QVBoxLayout(&dialog);outer->addWidget(new QLabel(tr("SRT 使用以下样式；ASS/SSA 保留原始高级样式与特效，SUP 保留位图。"),&dialog));auto *body=new QHBoxLayout;outer->addLayout(body);
    auto *fontGroup=new QGroupBox(tr("字体"),&dialog);auto *fontForm=new QFormLayout(fontGroup);body->addWidget(fontGroup);
    auto *font=new QFontComboBox(&dialog);font->setCurrentFont(QFont(settings_->value("subtitle/style/font","Comic Sans MS").toString()));fontForm->addRow(tr("默认字体"),font);
    QHash<QString,QSpinBox *> numbers;
    const auto number=[&](QFormLayout *form,const QString &key,const QString &label,int value,int min,int max){auto *box=new QSpinBox(&dialog);box->setRange(min,max);box->setValue(settings_->value("subtitle/style/"+key,value).toInt());form->addRow(label,box);numbers.insert(key,box);return box;};
    number(fontForm,"size",tr("字号(1080p 基准)"),36,8,200);number(fontForm,"spacing",tr("字间距"),0,-20,100);number(fontForm,"scaleX",tr("平面宽度 %"),100,10,300);number(fontForm,"scaleY",tr("平面高度 %"),100,10,300);
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
void PlayerWindow::saveScreenshot(const QString &path,const QImage &image,const QJsonObject &metadata){
    auto *watcher=new QFutureWatcher<QString>(this);connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,path,metadata]{const auto error=watcher->result();watcher->deleteLater();screenshotPending_=false;setError(error.isEmpty()?(metadata.value("hdr").toBool()?tr("已保存高精度 PNG、HDR 浮点数据及色彩信息：%1"):tr("已保存高精度无损 PNG：%1")).arg(path):tr("截图失败：%1").arg(error));});
    watcher->setFuture(QtConcurrent::run([path,image,metadata]{return writeScreenshotPng(path,image,metadata);}));
}
QString PlayerWindow::screenshotDirectory() const {const auto path=settings_->value("screenshot/path").toString().trimmed();return path.isEmpty()?QDir(QCoreApplication::applicationDirPath()).filePath("screenshots"):QFileInfo(path).absoluteFilePath();}
void PlayerWindow::captureImage(bool source,const QString &directory) {
    if(screenshotPending_){setError(tr("正在保存截图，请稍候。"));return;}
    if(source && !imageMode_ && !direct_ && displayedFrame_.planes[0].isEmpty()) {setError(tr("还没有可截取的 VS 帧。"));return;}
    const QDir dir(directory.isEmpty()?screenshotDirectory():directory);QDir().mkpath(dir.absolutePath());const auto name=QString("%1-%2-%3.png").arg(QFileInfo(source_).completeBaseName(),QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz"),source?(imageMode_ || direct_?"source":"VS"):"display");const QString file=dir.filePath(name);
    QJsonObject metadata{{"sourceFile",source_},{"stage",source?"source":"display"},{"position100ns",double(position())},{"hdr",false},{"encoding","encoded-RGB"}};
    if(imageMode_){const auto image=source?pane_->image():pane_->captureImage();if(image.isNull()){setError(tr("没有可截取画面。"));return;}const auto transfer=image.colorSpace().transferFunction();const auto format=image.format();const bool floating=format==QImage::Format_RGBA32FPx4 || format==QImage::Format_RGBA16FPx4 || format==QImage::Format_RGBX32FPx4 || format==QImage::Format_RGBX16FPx4;metadata["hdr"]=floating || transfer==QColorSpace::TransferFunction::St2084 || transfer==QColorSpace::TransferFunction::Hlg;if(transfer==QColorSpace::TransferFunction::Linear)metadata["encoding"]="linear-RGB";metadata["inputBitDepthPerChannel"]=image.pixelFormat().bitsPerPixel()/std::max(1,int(image.pixelFormat().channelCount()));screenshotPending_=true;saveScreenshot(file,image,metadata);return;}
    if(!source){auto *renderer=direct_?clock_.get():output_.get();const auto status=renderer->colorStatus();auto image=madvrMode()&&lav_?lav_->capture():renderer->captureFloat();if(image.isNull()){setError(tr("当前渲染器未返回可截取画面。"));return;}
        metadata["hdr"]=!madvrMode() && status.outputHdr;metadata["inputBitDepthPerChannel"]=madvrMode()?8:int(status.outputBits);metadata["renderer"]=madvrMode()?"madVR 8-bit capture interface":QString::fromUtf8(status.engine);metadata["qtInfoOverlay"]=infoVisible_;metadata["readbackBitDepth"]=image.text("readbackBitDepth").toInt();metadata["outputHdr"]=bool(status.outputHdr);metadata["tone"]=int(status.tone);metadata["gamut"]=int(status.gamut);metadata["dither"]=int(status.dither);metadata["targetPeakNits"]=status.targetPeak;metadata["iccProfile"]=QString::fromUtf8(status.profile);metadata["lutActive"]=bool(status.lutActive);
        if(status.outputHdr && !madvrMode()){metadata["encoding"]="scRGB-linear";metadata["linearUnitNits"]=80;image.setColorSpace(QColorSpace(QColorSpace::SRgbLinear));}
        else if(status.activeEngine==1 && status.iccState==1){QFile profile(QString::fromUtf8(status.profile));if(profile.open(QIODevice::ReadOnly))image.setColorSpace(QColorSpace::fromIccProfile(profile.readAll()));}
        else{metadata["pngPrimaries"]=1;metadata["pngTransfer"]=status.activeEngine==1 || madvrMode()?13:1;}
        if(infoVisible_){QPainter painter(&image);const auto ratio=pane_->surface()->devicePixelRatioF();painter.scale(ratio,ratio);info_->render(&painter,info_->pos());}
        screenshotPending_=true;saveScreenshot(file,image,metadata);return;}
    const auto frame=displayedFrame_;QString format;int bytes=1,subW=0,subH=0;const unsigned value=static_cast<unsigned>(frame.format);
    if(value<10){format=value==1?"gray":"gray16le";bytes=value==1?1:2;}
    else {const int depth=value%10;bytes=depth?2:1;const int bits=depth==0?8:depth==1?10:depth==2?12:16;
        if(value>=40)format=bits==8?"gbrp":QString("gbrp%1le").arg(bits);
        else {const auto sampling=value>=30?"444":value>=20?"422":"420";format=QString("yuv%1p").arg(sampling)+(bits==8?QString():QString::number(bits)+"le");subW=value<30?1:0;subH=value<20?1:0;}}
    const auto status=clock_->colorStatus();const auto snap=clock_->snapshot();const int width=direct_?int(snap.videoWidth):frame.width,height=direct_?int(snap.videoHeight):frame.height;
    if(width<=0 || height<=0){setError(tr("还没有可截取的源帧。"));return;}
    QJsonObject stream;if(direct_)for(const auto &entry:QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object().value("streams").toArray())if(entry.toObject().value("index").toInt()==snap.selectedVideoStream){stream=entry.toObject();break;}
    const int primaries=direct_?(status.sourceKind?int(status.sourcePrimaries):stream.value("colorPrimaries").toInt(2)):int(frame.colorPrimaries),transfer=direct_?(status.sourceKind?int(status.sourceTransfer):stream.value("colorTransfer").toInt(2)):int(frame.colorTransfer);
    metadata["hdr"]=transfer==16 || transfer==18;metadata["pngPrimaries"]=primaries;metadata["pngTransfer"]=transfer;metadata["inputBitDepthPerChannel"]=direct_?(status.sourceKind?int(status.sourceBits):stream.value("bitDepth").toInt()):value<10?(value==1?8:16):(value%10==0?8:value%10==1?10:value%10==2?12:16);
    const int matrix=direct_?(status.sourceKind?int(status.sourceMatrix):stream.value("colorSpace").toInt(2)):int(frame.colorMatrix);
    metadata["sourceMatrix"]=matrix;metadata["sourceRange"]=direct_?(status.sourceKind?int(status.sourceRange):stream.value("colorRange").toInt()):int(frame.colorRange);metadata["frameIndex"]=double(direct_?snap.frameIndex:frame.frameIndex);
    QString conversion="zscale=matrix=gbr:range=full";
    if(matrix==2 || (!direct_ && value<40 && matrix==0)){const auto fallback=direct_ && stream.value("colorModel").toString()=="RGB"?0:primaries==9 || transfer==16 || transfer==18?9:width>=1280?1:6;conversion+=QString(":matrixin=%1").arg(fallback);metadata["conversionMatrixFallback"]=fallback;}
    else if(direct_)conversion+=QString(":matrixin=%1").arg(matrix);
    auto *process=new QProcess(this);process->setObjectName("playerScreenshotProcess");connect(process,&QProcess::errorOccurred,this,[this,process](QProcess::ProcessError error){if(error==QProcess::FailedToStart){screenshotPending_=false;setError(tr("截图进程无法启动：%1").arg(process->errorString()));process->deleteLater();}});
    connect(process,&QProcess::finished,this,[this,process,file,metadata,width,height](int code){const auto data=process->readAllStandardOutput();const auto error=QString::fromUtf8(process->readAllStandardError());process->deleteLater();if(code!=0 || data.size()!=qint64(width)*height*12){screenshotPending_=false;setError(tr("截图失败：%1").arg(error));return;}
        QImage image(width,height,QImage::Format_RGBA32FPx4);const auto *planes=reinterpret_cast<const float *>(data.constData());const qsizetype samples=qsizetype(width)*height;for(int y=0;y<height;++y){auto *row=reinterpret_cast<float *>(image.scanLine(y));for(int x=0;x<width;++x){const qsizetype at=qsizetype(y)*width+x;row[x*4]=planes[samples*2+at];row[x*4+1]=planes[at];row[x*4+2]=planes[samples+at];row[x*4+3]=1;}}
        saveScreenshot(file,image,metadata);
    });
    QStringList arguments{"-v","error","-f","rawvideo","-pixel_format",format,"-video_size",QString("%1x%2").arg(frame.width).arg(frame.height)};
    if(value>=10 && value<40) {const QHash<unsigned,QString> matrices{{1,"bt709"},{5,"bt470bg"},{6,"smpte170m"},{9,"bt2020nc"},{10,"bt2020c"}};if(matrices.contains(frame.colorMatrix))arguments<<"-colorspace"<<matrices.value(frame.colorMatrix);if(frame.colorRange)arguments<<"-color_range"<<(frame.colorRange==2?"pc":"tv");}
    if(direct_){arguments={"-v","error","-ss",QString::number(std::max<qint64>(0,snap.position100ns)/10000000.0,'f',7)};if(mediaInput_.endsWith(".ffconcat",Qt::CaseInsensitive))arguments<<"-safe"<<"0";arguments<<"-i"<<(mediaInput_.isEmpty()?source_:mediaInput_)<<"-map"<<QString("0:%1").arg(snap.selectedVideoStream);}
    else arguments<<"-i"<<"pipe:0";
    arguments<<"-frames:v"<<"1"<<"-vf"<<conversion+",format=gbrpf32le"<<"-pix_fmt"<<"gbrpf32le"<<"-f"<<"rawvideo"<<"pipe:1";
    screenshotPending_=true;
    process->start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),arguments);
    if(direct_)return;
    for(int plane=0;plane<4;++plane)if(!frame.planes[plane].isEmpty()){const int width=plane>0&&value<40?(frame.width+(1<<subW)-1)>>subW:frame.width;const int height=plane>0&&value<40?(frame.height+(1<<subH)-1)>>subH:frame.height;for(int y=0;y<height;++y)process->write(frame.planes[plane].constData()+y*frame.strides[plane],width*bytes);}process->closeWriteChannel();
}
}
