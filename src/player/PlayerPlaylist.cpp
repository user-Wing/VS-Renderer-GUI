#include "player/PlayerWindow.h"
#include "player/PlayerChromePanel.h"
#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/LavPlayback.h"
#include "player/PlayerSubtitles.h"
#include "ui/PreviewPane.h"
#include "player/PlayerImage.h"
#include "player/PlayerImageTools.h"
#include "bluray/BlurayCatalog.h"
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QProcess>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QJsonArray>
#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QVBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QHeaderView>
#include <QSettings>
#include <QScreen>
#include <QCollator>
#include <QSet>
#include "player/PlayerAssociations.h"

namespace vsr {
namespace { struct DiscList {QStringList paths;QHash<QString,QString> labels,groups;QHash<QString,QJsonObject> programmes;QString root,error;}; }
void PlayerWindow::chooseFiles() {
    const auto paths=QFileDialog::getOpenFileNames(this,tr("打开视频或图片"));
    if(paths.isEmpty())return;playlistDirectory_.clear();files_=paths;openFile(paths.first());
}
void PlayerWindow::chooseFolder() {
    const auto path=QFileDialog::getExistingDirectory(this,tr("打开文件夹"));if(!path.isEmpty())openFolder(path);
}
void PlayerWindow::chooseBluRay() {const auto path=QFileDialog::getExistingDirectory(this,tr("打开 BD 文件夹用于播放"));if(!path.isEmpty())chooseBluRayMode(path);}
void PlayerWindow::chooseBluRayMode(const QString &path) {
    QDialog dialog(this);dialog.setObjectName("playerBluRayMode");dialog.setWindowTitle(tr("BD 播放方式"));
    auto *layout=new QVBoxLayout(&dialog);layout->addWidget(new QLabel(tr("播放模式"),&dialog));
    auto *mode=new QComboBox(&dialog);mode->addItems({tr("1 - 节目列表模式"),tr("2 - 光盘菜单交互模式")});layout->addWidget(mode);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.setMinimumWidth(qMax(560,dialog.fontMetrics().horizontalAdvance(dialog.windowTitle())+240));dialog.resize(dialog.minimumWidth(),dialog.sizeHint().height());
    if(dialog.exec()==QDialog::Accepted)openBluRay(path,mode->currentIndex()==1);
}
void PlayerWindow::applyBlurayMetadata() {
    if(!externalChapters_.isEmpty())media_.insert("chapters",externalChapters_);
    if(blurayMetadata_.isEmpty())return;
    media_.insert("chapters",externalChapters_.isEmpty()?blurayMetadata_.value("chapters"):QJsonValue(externalChapters_));
    QJsonArray streams;const auto languages=blurayMetadata_.value("languages").toObject();
    for(const auto &value:media_.value("streams").toArray()){auto stream=value.toObject();const auto lang=languages.value(QString::number(stream.value("streamId").toInt())).toString();if(!lang.isEmpty())stream.insert("language",lang);streams<<stream;}
    media_.insert("streams",streams);
}
bool PlayerWindow::openBluRay(const QString &path, bool menus) {
    if(QFileInfo(path).isFile()){
        auto image=QDir::toNativeSeparators(path);image.replace("'","''");const auto script=QString("$ErrorActionPreference='Stop'; Mount-DiskImage -ImagePath '%1' -PassThru | Get-Volume | Where-Object DriveLetter | ForEach-Object { $_.DriveLetter + ':\\' }").arg(image);
        auto *mount=new QProcess(this);connect(mount,&QProcess::finished,this,[this,mount,menus](int code){const auto root=QString::fromUtf8(mount->readAllStandardOutput()).trimmed();if(code==0 && QFileInfo(root).isDir())openBluRay(root,menus);else setError(tr("装载 BD 镜像失败：%1").arg(QString::fromUtf8(mount->readAllStandardError())));mount->deleteLater();});
        connect(mount,&QProcess::errorOccurred,this,[this,mount]{setError(mount->errorString());mount->deleteLater();});mount->start("powershell.exe",{"-NoProfile","-NonInteractive","-EncodedCommand",QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(script.utf16()),script.size()*2).toBase64())});return true;
    }
    message_->setText(tr("正在读取 BD 播放列表…"));auto *scan=new QFutureWatcher<DiscList>(this);
    connect(scan,&QFutureWatcher<DiscList>::finished,this,[this,scan,menus]{const auto list=scan->result();scan->deleteLater();if(!list.error.isEmpty()){setError(list.error);return;}
        playlistDirectory_.clear();files_=list.paths;blurayLabels_=list.labels;blurayGroups_=list.groups;blurayProgrammes_=list.programmes;discRoot_=list.root;
        if(menus?openDiscMenu(discRoot_):openFile(files_.first())){setPlaylistPinned(true);updatePlaylist();}
    });scan->setFuture(QtConcurrent::run([path]{
        DiscList list;const auto result=BlurayCatalog::scan(path);QVector<BlurayTitle> titles;for(const auto &title:result.titles)if(title.suggested && title.warning.isEmpty())titles<<title;
        for(const auto &title:result.titles)if(!title.suggested && title.warning.isEmpty())titles<<title;
        if(titles.isEmpty()){list.error=tr("没有可播放的 BD 节目；请检查视频/CLPI 是否完整。%1").arg(result.warnings.join('\n'));return list;}
        list.root=QFileInfo(titles.first().disc).absolutePath();QSet<QString> referenced;int extra=1,episode=1;
        for(const auto &title:result.titles)for(const auto &clip:title.clips)referenced.insert(QDir(title.disc).filePath("STREAM/"+clip+".m2ts"));
        const auto prepared=QtConcurrent::blockingMapped<QList<QPair<QString,QString>>>(titles,[](const BlurayTitle &title){QString error;const auto path=BlurayCatalog::prepare(title,&error);return qMakePair(path,error);});
        for(int i=0;i<titles.size();++i){const auto &title=titles[i];const auto playlist=prepared[i].first;if(playlist.isEmpty()){list.error=prepared[i].second;return list;}list.paths<<playlist;list.programmes.insert(playlist,BlurayCatalog::metadata(playlist));
            const bool compilation=!title.suggested && std::any_of(titles.begin(),titles.end(),[&](const auto &candidate){return candidate.suggested && candidate.playlist==title.playlist;});
            list.labels.insert(playlist,title.suggested?tr("第 %1 集 · %2").arg(episode++).arg(title.label.section(" · 候选第",0,0)):compilation?tr("整卷连续播放 · %1 · %2").arg(QFileInfo(title.playlist).fileName(),BlurayCatalog::time(title.ticks)):tr("花絮 / 其它节目 %1 · %2 · %3").arg(extra++).arg(QFileInfo(title.playlist).fileName(),BlurayCatalog::time(title.ticks)));list.groups.insert(playlist,title.suggested?"episode":"extra");}
        QStringList audio;QDirIterator it(path,QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
        while(it.hasNext()){const auto file=it.next();const auto suffix=QFileInfo(file).suffix().toLower();if(playerAudioExtensions().contains(suffix))audio<<file;else if(suffix=="m2ts" && !referenced.contains(file)){list.paths<<file;const auto issue=BlurayCatalog::clipPlaybackIssue(file);list.labels.insert(file,issue.isEmpty()?tr("独立片段 · %1").arg(QFileInfo(file).fileName()):tr("菜单图形 · %1 (使用菜单模式)").arg(QFileInfo(file).fileName()));list.groups.insert(file,issue.isEmpty()?"extra":"menu");}}
        QCollator order;order.setNumericMode(true);order.setCaseSensitivity(Qt::CaseInsensitive);std::sort(audio.begin(),audio.end(),[&](const auto &a,const auto &b){return order.compare(a,b)<0;});
        for(const auto &file:audio){list.paths<<file;list.labels.insert(file,QFileInfo(file).fileName());list.groups.insert(file,"audio");}return list;
    }));return true;
}
bool PlayerWindow::openDiscMenu(const QString &root, bool menus) {
    QString error;const auto input=menus?BlurayCatalog::prepareMenu(root,&error):root;if(input.isEmpty()){setError(error);return false;}
    pendingMediaOpen_.clear();
    discProgrammeKey_.clear();discProgrammeSettling_.invalidate();seekUiTarget_=queuedSeek_=-1;seekPending_=false;
    savePosition();if(discMenu_ && discMenu_->active()){discMenu_->close(discSurface_);discSurface_=nullptr;}clock_->stop();output_->stop();server_->unloadScript();lav_.reset();subtitles_->load(0,{},-1,{},{});subtitles_->load(1,{},-1,{},{});
    pane_->setImage({});pane_->setSurfaceActive(true);
    if(!discMenu_)discMenu_=std::make_unique<PlayerDiscMenu>();
    if(!discSurface_){discSurface_=new QWidget(pane_->surface());discSurface_->setObjectName("playerDiscSurface");discSurface_->setAttribute(Qt::WA_NativeWindow);discSurface_->setFocusPolicy(Qt::StrongFocus);}
    imageMode_=audioMode_=false;imageTools_->hide();lyricLabel_->hide();audioSubtitle_->hide();info_->hide();infoVisible_=false;autoPlay_=false;pending_=false;profileStartup_.invalidate();
    discSurface_->setGeometry(pane_->surface()->rect());discSurface_->show();discSurface_->raise();
    if(!discMenu_->open(input,reinterpret_cast<void *>(discSurface_->winId()))){discSurface_->hide();setError(discMenu_->error());return false;}
    discMenuNavigation_=menus;discMenu_->volume(volume_->value(),mute_->isChecked());discMenu_->rate(speed_);
    if(menus){source_=root;fileIndex_=-1;}ready_=playing_=true;play_->setText("Ⅱ");rendererBadge_->setText("libVLC");decoderBadge_->setText("BD-Auto");decoderBadge_->setEnabled(false);decoderBadge_->setToolTip(tr("BD 菜单/短节目自动解码 (libVLC)"));message_->setText(menus?tr("BD 菜单交互 · 鼠标点击或方向键选择，Enter 确认；节目列表可切回 3FP"):tr("BD 短节目 · libVLC (保留 MPLS 顺序和裁切边界)"));discSurface_->setFocus();return true;
}

bool PlayerWindow::handoffDiscProgramme() {
    if(!discMenuNavigation_ || !discMenu_ || !discMenu_->active())return false;
    const auto programme=discMenu_->programme();const qint64 length=programme.value("duration100ns").toDouble();
    if(programme.value("title").toInt()<=0 || length<=0 || discMenu_->state()!=3)return false;
    const auto chapters=programme.value("chapters").toArray();QString match;
    const auto key=QString("%1/%2/%3").arg(programme.value("title").toInt()).arg(programme.value("chapter").toInt()).arg(length);
    if(discProgrammeKey_!=key){discProgrammeKey_=key;discProgrammeSettling_.restart();return false;}
    if(discProgrammeSettling_.elapsed()<250)return false;
    for(auto it=blurayProgrammes_.cbegin();it!=blurayProgrammes_.cend();++it){
        if(qAbs(qint64(it.value().value("duration100ns").toDouble())-length)>20000)continue;
        const auto expected=it.value().value("chapters").toArray();if(expected.size()!=chapters.size())continue;
        bool same=true;for(int i=0;i<expected.size();++i)if(qAbs(qint64(expected[i].toObject().value("start100ns").toDouble())-qint64(chapters[i].toDouble()))>20000){same=false;break;}
        if(!same)continue;if(!match.isEmpty() && match!=it.key())return false;match=it.key();
    }
    if(match.isEmpty())return false;
    const int chapter=programme.value("chapter").toInt();const auto at=qMax(discMenu_->position(),chapter>=0 && chapter<chapters.size()?qint64(chapters[chapter].toDouble()):qint64(0));const bool showInfo=infoVisible_;const int audio=programme.value("audio").toInt(),subtitle=programme.value("subtitle").toInt();
    if(!openFile(match))return false;settingsPosition_=at;autoPlay_=true;infoVisible_=showInfo;
    setProperty("discSelectedAudioPid",audio);setProperty("discSelectedSubtitlePid",subtitle);return true;
}

void PlayerWindow::chooseLink() {
    const auto url=QInputDialog::getText(this,tr("打开链接"),tr("HTTP / HTTPS 视频直链或 file:/// 本地文件链接"));if(!url.trimmed().isEmpty())openFile(url.trimmed());
}
void PlayerWindow::buildPlaylist(QWidget *parent) {
    playlistPanel_=new PlayerChromePanel(parent);playlistPanel_->setObjectName("playerPlaylistPanel");playlistPanel_->setAttribute(Qt::WA_NativeWindow);
    playlistPanel_->setStyleSheet("background:#292b30;border:1px solid #50535a;");auto *layout=new QVBoxLayout(playlistPanel_);
    auto *title=new QHBoxLayout;title->addWidget(new QLabel(tr("播放列表"),playlistPanel_),1);auto *menu=new QPushButton(tr("BD 菜单"),playlistPanel_);menu->setObjectName("playerOpenDiscMenu");title->addWidget(menu);connect(menu,&QPushButton::clicked,this,[this]{if(!discRoot_.isEmpty())openDiscMenu(discRoot_);});auto *close=new QPushButton("×",playlistPanel_);close->setObjectName("playerPlaylistClose");close->setFixedWidth(30);title->addWidget(close);layout->addLayout(title);connect(close,&QPushButton::clicked,this,[this]{setPlaylistPinned(false);playlistDismissed_=true;playlistPanel_->hide();});
    playlistWidth_=std::clamp(settings_->value("playlist/width",320).toInt(),180,800);
    connect(videoSplit_,&QSplitter::splitterMoved,this,[this]{if(playlistPinned_ && !isFullScreen()){playlistWidth_=playlistPanel_->width();settings_->setValue("playlist/width",playlistWidth_);}});
    playlist_=new QTreeWidget(playlistPanel_);playlist_->setObjectName("playerPlaylist");playlist_->setHeaderHidden(true);playlist_->header()->setSectionResizeMode(QHeaderView::Stretch);playlist_->setTextElideMode(Qt::ElideMiddle);playlist_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);playlist_->setToolTip(tr("双击文件播放；展开子目录查看文件"));layout->addWidget(playlist_);
    connect(playlist_,&QTreeWidget::itemExpanded,this,[this](QTreeWidgetItem *item){if(item->data(0,Qt::UserRole+1).toBool() && item->childCount()==0)populateFolder(item,item->data(0,Qt::UserRole).toString());});
    connect(playlist_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *item){if(!item->data(0,Qt::UserRole+1).toBool())openFile(item->data(0,Qt::UserRole).toString());});
    playlistPanel_->hide();
}
void PlayerWindow::populateFolder(QTreeWidgetItem *parent,const QString &path) {
    for(const auto &entry:QDir(path).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDir::DirsFirst|QDir::Name)) {
        if(!entry.isDir() && !playerVideoExtensions().contains(entry.suffix().toLower()) && !playerAudioExtensions().contains(entry.suffix().toLower()) && !PlayerImage::supports(entry.absoluteFilePath()))continue;
        auto *item=parent?new QTreeWidgetItem(parent):new QTreeWidgetItem(playlist_);
        item->setText(0,entry.fileName());item->setToolTip(0,entry.absoluteFilePath());item->setData(0,Qt::UserRole,entry.absoluteFilePath());item->setData(0,Qt::UserRole+1,entry.isDir());
        if(entry.absoluteFilePath()==source_) {QFont font=item->font(0);font.setBold(true);item->setFont(0,font);playlist_->setCurrentItem(item);}
        if(entry.isDir())item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
}
bool PlayerWindow::openFolder(const QString &path) {
    if(QFileInfo(QDir(path).filePath("BDMV/PLAYLIST")).isDir() || QFileInfo(QDir(path).filePath("PLAYLIST")).isDir())return openBluRay(path);
    if(!QFileInfo(path).isDir())return false;playlistDirectory_=QFileInfo(path).absoluteFilePath();files_.clear();
    QDirIterator iterator(playlistDirectory_,QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
    const auto mediaExtensions=playerVideoExtensions()+playerAudioExtensions();
    while(iterator.hasNext()){const auto file=iterator.next();if(mediaExtensions.contains(QFileInfo(file).suffix().toLower()) || PlayerImage::supports(file))files_<<file;}
    files_.sort(Qt::CaseInsensitive);updatePlaylist();playlistPanel_->show();playlistPanel_->raise();
    if(files_.isEmpty()){setError(tr("目录已加入列表，未找到可播放媒体。"));return true;}
    return openFile(files_.first());
}
void PlayerWindow::updatePlaylist() {
    if(playlist_->property("directory").toString()!=playlistDirectory_ || playlistDirectory_.isEmpty()) {
        playlist_->clear();playlist_->setProperty("directory",playlistDirectory_);
        if(!playlistDirectory_.isEmpty())populateFolder(nullptr,playlistDirectory_);
        else {QTreeWidgetItem *audio=nullptr;bool separated=false;for(const auto &file:files_) {
            const auto group=blurayGroups_.value(file);if(!separated && (group=="extra" || group=="menu" || group=="audio")){auto *separator=new QTreeWidgetItem(playlist_);separator->setText(0,"────────────────────");separator->setFlags(Qt::NoItemFlags);separated=true;}
            if(group=="audio" && !audio){audio=new QTreeWidgetItem(playlist_);audio->setText(0,tr("音频"));audio->setData(0,Qt::UserRole+1,true);audio->setExpanded(false);}
            auto *item=group=="audio"?new QTreeWidgetItem(audio):new QTreeWidgetItem(playlist_);const QUrl url(file);item->setText(0,blurayLabels_.contains(file)?blurayLabels_.value(file):url.scheme()=="http" || url.scheme()=="https"?url.fileName():QFileInfo(file).fileName());item->setData(0,Qt::UserRole,file);item->setToolTip(0,group=="menu"?file+"\n"+BlurayCatalog::clipPlaybackIssue(file):file);if(group=="menu")item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }}
    }
    for(QTreeWidgetItemIterator it(playlist_);*it;++it) {const bool current=(*it)->data(0,Qt::UserRole).toString()==source_;QFont font=(*it)->font(0);font.setBold(current);(*it)->setFont(0,font);if(current)playlist_->setCurrentItem(*it);}
}
void PlayerWindow::setPlaylistPinned(bool pinned) {
    if(playlistPinned_==pinned)return;playlistPinned_=pinned;playlistDismissed_=false;
    if(!isFullScreen()){
        if(pinned){const int old=width();resize(qMin(width()+playlistWidth_,screen()->availableGeometry().width()),height());playlistExpansion_=width()-old;}
        else {resize(qMax(minimumWidth(),width()-playlistExpansion_),height());playlistExpansion_=0;}
    }
    dockPlaylist();
}
void PlayerWindow::dockPlaylist() {
    layoutChrome();
    if(playlistPinned_ && !isFullScreen()){playlistPanel_->setWindowFlags(Qt::Widget);videoSplit_->addWidget(playlistPanel_);playlistPanel_->setMinimumWidth(180);videoSplit_->setSizes({qMax(1,videoSplit_->width()-playlistWidth_),playlistWidth_});playlistPanel_->show();}
    else {playlistPanel_->hide();playlistPanel_->setParent(centralWidget(),Qt::Tool|Qt::FramelessWindowHint);playlistPanel_->setAttribute(Qt::WA_TranslucentBackground);playlistPanel_->setMinimumWidth(0);}
}
void PlayerWindow::layoutChrome() {
    const bool overlay=isFullScreen() || settings_->value("render/stableViewport",true).toBool() || settings_->value("theme/bottomTransparency",50).toInt()>0 || settings_->value("theme/playlistTransparency",50).toInt()>0;
    auto *layout=qobject_cast<QVBoxLayout *>(centralWidget()->layout());
    if(overlay!=overlayChrome_){overlayChrome_=overlay;if(overlay){layout->removeWidget(videoSplit_);layout->removeWidget(controls_);layout->removeWidget(imageTools_);controls_->setWindowFlags(Qt::Tool|Qt::FramelessWindowHint);controls_->setAttribute(Qt::WA_TranslucentBackground);}
        else {controls_->setWindowFlags(Qt::Widget);layout->insertWidget(0,imageTools_);layout->addWidget(videoSplit_,1);layout->addWidget(controls_);}}
    if(overlay){videoSplit_->setGeometry(centralWidget()->rect());const int h=controls_->sizeHint().height();controlsTop_=centralWidget()->height()-h;controls_->setGeometry(QRect(centralWidget()->mapToGlobal(QPoint(0,controlsTop_)),QSize(centralWidget()->width(),h)));controls_->raise();if(imageTools_){imageTools_->setGeometry(0,0,centralWidget()->width(),imageTools_->sizeHint().height());imageTools_->raise();}}
}
void PlayerWindow::updateChrome() {
    layoutChrome();
    const auto *active=qApp->activeWindow();const bool activePlayer=isActiveWindow() || active==controls_ || active==playlistPanel_ || active==fullscreenTitle_;
    if(!isVisible() || isMinimized() || !activePlayer){controls_->hide();if(playlistPanel_->isWindow())playlistPanel_->hide();fullscreenTitle_->hide();return;}
    if(!isFullScreen())controls_->show();
    const auto pos=centralWidget()->mapFromGlobal(QCursor::pos());const bool inside=centralWidget()->rect().contains(pos);
    const bool moved=QCursor::pos()!=chromePointer_;chromePointer_=QCursor::pos();
    if(infoVisible_ && !imageMode_ && !(discMenu_ && discMenu_->active()) && settings_->value("info/audioDetailed",false).toBool())for(const auto &value:media_.value("streams").toArray()){const auto audio=value.toObject();if(audio.value("type").toString()=="audio" && audio.value("index").toInt()==clock_->snapshot().selectedAudioStream){updateAudioInfo(audio,clock_->snapshot().audioBitRate);break;}}
    if(playlistPanel_->isWindow()) {
        const int bottom=centralWidget()->height()-controls_->sizeHint().height();
        const int panelTop=isFullScreen()?fullscreenTitle_->sizeHint().height():0;
        const int panelWidth=qMin(playlistWidth_,centralWidget()->width()/2);playlistPanel_->setGeometry(QRect(centralWidget()->mapToGlobal(QPoint(centralWidget()->width()-panelWidth,panelTop)),QSize(panelWidth,bottom-panelTop)));
        if(!inside || pos.x()<centralWidget()->width()-panelWidth-12)playlistDismissed_=false;
        if(playlistPinned_ || (!playlistDismissed_ && inside && activePlayer && pos.x()>=centralWidget()->width()-panelWidth && pos.y()>=panelTop && pos.y()<bottom)) {playlistPanel_->show();playlistPanel_->raise();}
        else if(!playlistPinned_ && playlistPanel_->isVisible() && (!inside || pos.x()<centralWidget()->width()-panelWidth-12 || pos.y()<panelTop || pos.y()>=bottom))playlistPanel_->hide();
    }
    controls_->raise();
    if(!isFullScreen()){controls_->show();fullscreenTitle_->hide();return;}
    const bool bottom=inside && pos.y()>=controlsTop_-6;
    auto *focus=qApp->focusWidget();const bool editing=focus && controls_->isAncestorOf(focus) && (qobject_cast<QLineEdit *>(focus) || qobject_cast<QDoubleSpinBox *>(focus));
    if(qApp->activePopupWidget())return;
    if((bottom && moved) || editing) {controls_->show();chromeIdle_.restart();}
    else if(chromeIdle_.isValid() && chromeIdle_.elapsed()>=1000)controls_->hide();
    fullscreenTitleText_->setText(windowTitle());const int titleHeight=fullscreenTitle_->sizeHint().height();
    fullscreenTitle_->setGeometry(QRect(centralWidget()->mapToGlobal(QPoint()),QSize(centralWidget()->width(),titleHeight)));
    if(inside && pos.y()<=titleHeight){fullscreenTitle_->show();fullscreenTitle_->raise();titleIdle_.restart();}
    else if(!titleIdle_.isValid() || titleIdle_.elapsed()>=1000)fullscreenTitle_->hide();
}
}
