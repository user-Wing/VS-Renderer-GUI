#include "player/PlayerWindow.h"
#include "ui/PreviewPane.h"
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

namespace vsr {
void PlayerWindow::chooseFiles() {
    const auto paths=QFileDialog::getOpenFileNames(this,tr("打开视频"));
    if(paths.isEmpty())return;playlistDirectory_.clear();files_=paths;openFile(paths.first());
}
void PlayerWindow::chooseFolder() {
    const auto path=QFileDialog::getExistingDirectory(this,tr("打开文件夹"));if(!path.isEmpty())openFolder(path);
}
void PlayerWindow::chooseLink() {
    const auto url=QInputDialog::getText(this,tr("打开链接"),tr("HTTP / HTTPS 视频直链或 file:/// 本地文件链接"));if(!url.trimmed().isEmpty())openFile(url.trimmed());
}
void PlayerWindow::buildPlaylist(QWidget *parent) {
    playlistPanel_=new QWidget(parent);playlistPanel_->setObjectName("playerPlaylistPanel");playlistPanel_->setAttribute(Qt::WA_NativeWindow);
    playlistPanel_->setStyleSheet("background:#292b30;border:1px solid #50535a;");auto *layout=new QVBoxLayout(playlistPanel_);
    auto *title=new QHBoxLayout;title->addWidget(new QLabel(tr("播放列表"),playlistPanel_),1);auto *close=new QPushButton("×",playlistPanel_);close->setObjectName("playerPlaylistClose");close->setFixedWidth(30);title->addWidget(close);layout->addLayout(title);connect(close,&QPushButton::clicked,this,[this]{setPlaylistPinned(false);playlistDismissed_=true;playlistPanel_->hide();});
    playlistWidth_=std::clamp(settings_->value("playlist/width",320).toInt(),180,800);
    connect(videoSplit_,&QSplitter::splitterMoved,this,[this]{if(playlistPinned_ && !isFullScreen()){playlistWidth_=playlistPanel_->width();settings_->setValue("playlist/width",playlistWidth_);}});
    playlist_=new QTreeWidget(playlistPanel_);playlist_->setObjectName("playerPlaylist");playlist_->setHeaderHidden(true);playlist_->header()->setSectionResizeMode(QHeaderView::Stretch);playlist_->setTextElideMode(Qt::ElideMiddle);playlist_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);playlist_->setToolTip(tr("双击文件播放；展开子目录查看文件"));layout->addWidget(playlist_);
    connect(playlist_,&QTreeWidget::itemExpanded,this,[this](QTreeWidgetItem *item){if(item->data(0,Qt::UserRole+1).toBool() && item->childCount()==0)populateFolder(item,item->data(0,Qt::UserRole).toString());});
    connect(playlist_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *item){if(!item->data(0,Qt::UserRole+1).toBool())openFile(item->data(0,Qt::UserRole).toString());});
    playlistPanel_->hide();
}
void PlayerWindow::populateFolder(QTreeWidgetItem *parent,const QString &path) {
    for(const auto &entry:QDir(path).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDir::DirsFirst|QDir::Name)) {
        auto *item=parent?new QTreeWidgetItem(parent):new QTreeWidgetItem(playlist_);
        item->setText(0,entry.fileName());item->setToolTip(0,entry.absoluteFilePath());item->setData(0,Qt::UserRole,entry.absoluteFilePath());item->setData(0,Qt::UserRole+1,entry.isDir());
        if(entry.absoluteFilePath()==source_) {QFont font=item->font(0);font.setBold(true);item->setFont(0,font);playlist_->setCurrentItem(item);}
        if(entry.isDir())item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
}
bool PlayerWindow::openFolder(const QString &path) {
    if(!QFileInfo(path).isDir())return false;playlistDirectory_=QFileInfo(path).absoluteFilePath();files_.clear();
    QDirIterator iterator(playlistDirectory_,{"*.mkv","*.mp4","*.mov","*.avi","*.webm","*.ts","*.m2ts","*.wmv","*.flv","*.mp3","*.flac","*.wav","*.m4a","*.ogg","*.opus"},QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
    while(iterator.hasNext())files_<<iterator.next();files_.sort(Qt::CaseInsensitive);updatePlaylist();playlistPanel_->show();playlistPanel_->raise();
    if(files_.isEmpty()){setError(tr("目录已加入列表，未找到可播放媒体。"));return true;}
    return openFile(files_.first());
}
void PlayerWindow::updatePlaylist() {
    if(playlist_->property("directory").toString()!=playlistDirectory_ || playlistDirectory_.isEmpty()) {
        playlist_->clear();playlist_->setProperty("directory",playlistDirectory_);
        if(!playlistDirectory_.isEmpty())populateFolder(nullptr,playlistDirectory_);
        else for(const auto &file:files_) {auto *item=new QTreeWidgetItem(playlist_);const QUrl url(file);item->setText(0,url.scheme()=="http" || url.scheme()=="https"?url.fileName():QFileInfo(file).fileName());item->setData(0,Qt::UserRole,file);item->setToolTip(0,file);}
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
    if(playlistPinned_ && !isFullScreen()){videoSplit_->addWidget(playlistPanel_);playlistPanel_->setMinimumWidth(180);videoSplit_->setSizes({qMax(1,videoSplit_->width()-playlistWidth_),playlistWidth_});playlistPanel_->show();}
    else {playlistPanel_->hide();playlistPanel_->setParent(centralWidget());playlistPanel_->setMinimumWidth(0);}
}
void PlayerWindow::updateChrome() {
    const auto pos=centralWidget()->mapFromGlobal(QCursor::pos());const bool inside=centralWidget()->rect().contains(pos);
    if(!playlistPinned_ || isFullScreen()) {
        const int panelWidth=qMin(playlistWidth_,centralWidget()->width()/2);playlistPanel_->setGeometry(centralWidget()->width()-panelWidth,0,panelWidth,pane_->height());
        if(!inside || pos.x()<playlistPanel_->x()-12)playlistDismissed_=false;
        if(!playlistDismissed_ && inside && isActiveWindow() && pos.x()>=playlistPanel_->x() && pos.y()<pane_->height()) {playlistPanel_->show();playlistPanel_->raise();}
        else if(playlistPanel_->isVisible() && (!inside || pos.x()<playlistPanel_->x()-12 || pos.y()>=pane_->height()))playlistPanel_->hide();
    }
    if(!isFullScreen()){controls_->show();return;}
    const bool bottom=inside && pos.y()>=centralWidget()->height()-(controls_->isVisible()?controls_->height()+6:24);
    auto *focus=qApp->focusWidget();const bool editing=focus && controls_->isAncestorOf(focus) && (qobject_cast<QLineEdit *>(focus) || qobject_cast<QDoubleSpinBox *>(focus));
    if(bottom || editing || qApp->activePopupWidget() || qApp->activeModalWidget()) {controls_->show();chromeIdle_.restart();}
    else if(chromeIdle_.isValid() && chromeIdle_.elapsed()>=2000)controls_->hide();
}
}
