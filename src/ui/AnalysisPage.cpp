#include "ui/AnalysisPage.h"
#include "ui/ExportWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include <QCheckBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>
#include <utility>

namespace vsr {
namespace {
QString timeText(std::int64_t ticks)
{
    const auto ms = std::max<std::int64_t>(0, ticks / 10000);
    return QStringLiteral("%1:%2:%3.%4").arg(ms / 3600000, 2, 10, QLatin1Char('0'))
        .arg(ms / 60000 % 60, 2, 10, QLatin1Char('0'))
        .arg(ms / 1000 % 60, 2, 10, QLatin1Char('0')).arg(ms % 1000, 3, 10, QLatin1Char('0'));
}
}

AnalysisPage::AnalysisPage(ThreeFpApi &api, QWidget *parent) : QWidget(parent), api_(api)
{
    setAcceptDrops(true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);
    auto *header = new QHBoxLayout;
    auto *open = new QPushButton(QStringLiteral("打开视频…（最多 9 个）"), this);
    header->addWidget(open);
    connect(open, &QPushButton::clicked, this, [this] {
        openFiles(QFileDialog::getOpenFileNames(this, QStringLiteral("打开视频"), {},
            QStringLiteral("视频 (*.mkv *.mp4 *.mov *.avi *.webm *.m4v *.ts *.m2ts *.mts *.mpg *.mpeg *.wmv *.flv *.vob);;所有文件 (*)")));
    });
    mode_ = new QComboBox(this);
    mode_->setObjectName(QStringLiteral("analysisMode"));
    header->addWidget(mode_);
    auto *exportButton = new QPushButton(QStringLiteral("导出对比画布…"),this);
    exportButton->setObjectName(QStringLiteral("exportComparison"));
    header->addWidget(exportButton);
    connect(exportButton,&QPushButton::clicked,this,&AnalysisPage::exportComparison);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this] { setMode(mode_->currentData().toInt()); });
    header->addStretch();
    sync_ = new QCheckBox(QStringLiteral("同步缩放"), this); sync_->setChecked(true); header->addWidget(sync_);
    layout->addLayout(header);
    view_ = new MultiCompareView(this);
    layout->addWidget(view_, 1);
    connect(view_, &MultiCompareView::sourceSelected, this, &AnalysisPage::selectSource);
    connect(view_, &MultiCompareView::sourceRemoved, this, &AnalysisPage::removeVideo);
    for (int i=0;i<9;++i) panes_[i]=view_->pane(i);
    for (int row=0;row<4;++row) {
        rows_[row]=new QWidget(this); auto *line=new QHBoxLayout(rows_[row]); line->setContentsMargins(0,0,0,0); line->setSpacing(8);
        sources_[row]=new QComboBox(this); sources_[row]->setObjectName(QStringLiteral("analysisSource%1").arg(row)); sources_[row]->setMinimumWidth(120);
        sources_[row]->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        sources_[row]->setMinimumContentsLength(10);
        line->addWidget(sources_[row]);
        connect(sources_[row], &QComboBox::currentIndexChanged, this, [this,row](int video) { if(video>=0) selectSource(row,video); });
        times_[row]=new QLabel(this); times_[row]->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred); line->addWidget(times_[row]);
        sliders_[row]=new QSlider(Qt::Horizontal,this); sliders_[row]->setRange(0,100000); sliders_[row]->setMinimumWidth(160);
        sliders_[row]->setObjectName(QStringLiteral("analysisTimeline%1").arg(row));
        line->addWidget(sliders_[row],1);
        connect(sliders_[row], &QSlider::valueChanged, this,[this,row](int value) {
            const int i=slots_.value(row,-1); if(loaded(i)) seekGlobal(snapshot(i).duration100ns*value/100000-offsets_[i]);
        });
        for(int j=0;j<4;++j) {
            const QStringList labels{QStringLiteral("−1 帧"),QStringLiteral("+1 帧"),QStringLiteral("−1 秒"),QStringLiteral("+1 秒")};
            auto *button=new QPushButton(labels[j],this); line->addWidget(button);
            connect(button,&QPushButton::clicked,this,[this,row,j] { alignVideo(slots_.value(row,-1),j%2?1:-1,j>=2); });
        }
        layout->addWidget(rows_[row]); rows_[row]->hide();
    }
    controls_=new QWidget(this);
    auto *controls=new QHBoxLayout(controls_);
    controls->setContentsMargins(0,0,0,0);
    layoutChoice_=new QComboBox(this); layoutChoice_->setObjectName(QStringLiteral("analysisLayout"));
    layoutChoice_->setMinimumWidth(210);
    controls->addWidget(layoutChoice_);
    connect(layoutChoice_,&QComboBox::currentIndexChanged,this,[this] { refreshLayout(); });
    audio_=new QComboBox(this); audio_->setMinimumWidth(200); audio_->setMaximumWidth(300); audio_->setToolTip(QStringLiteral("音频选项：选择音频来源")); controls->addWidget(audio_);
    connect(audio_,&QComboBox::currentIndexChanged,this,[this] {
        pause();
        for(int i=0;i<videoCount_;++i) if(players_[i]) players_[i]->setMuted(audio_->currentData().toInt()!=i);
        seekGlobal(global_);
    });
    controls->addStretch();
    auto *previous=new QPushButton(QStringLiteral("◀ 帧"),this);
    play_=new QPushButton(QStringLiteral("播放"),this);
    auto *next=new QPushButton(QStringLiteral("帧 ▶"),this);
    controls->addWidget(previous); controls->addWidget(play_); controls->addWidget(next);
    connect(previous,&QPushButton::clicked,this,[this]{stepGlobal(-1);});
    connect(next,&QPushButton::clicked,this,[this]{stepGlobal(1);});
    connect(play_,&QPushButton::clicked,this,&AnalysisPage::togglePlayback);
    vrr_=new QCheckBox(QStringLiteral("VRR 低延迟"),this); pacing_=new QCheckBox(QStringLiteral("VRR Pacing"),this);
    vrr_->setProperty("vrrToggle", true); pacing_->setProperty("vrrToggle", true);
    controls->addWidget(vrr_); controls->addWidget(pacing_);
    connect(vrr_,&QCheckBox::toggled,this,[this](bool enabled){for(auto &p:players_)if(p)p->setVrrPresent(enabled);});
    connect(pacing_,&QCheckBox::toggled,this,[this](bool enabled){for(auto &p:players_)if(p)p->setVrrPacing(enabled);});
    controls->addStretch();
    auto *color = new QComboBox(this);
    color->setObjectName(QStringLiteral("colorProcessing"));
    color->setMinimumWidth(240);
    const QStringList colorNames{QStringLiteral("Nearest"),QStringLiteral("Bilinear"),QStringLiteral("Bicubic (Catmull-Rom)"),
        QStringLiteral("Lanczos 3"),QStringLiteral("Jinc 2"),QStringLiteral("Spline36"),QStringLiteral("Super-XBR（单阶段）"),
        QStringLiteral("Softcubic (B-spline)"),QStringLiteral("Mitchell-Netravali"),QStringLiteral("Bilateral"),QStringLiteral("亮度引导双边重建")};
    for(int i=0;i<colorNames.size();++i) color->addItem(QStringLiteral("颜色处理：%1").arg(colorNames[i]),i);
    color->setCurrentIndex(chromaAlgorithm_);
    color->setToolTip(color->currentText());
    color->view()->setMinimumWidth(320);
    connect(color,&QComboBox::currentIndexChanged,this,[this,color] {
        chromaAlgorithm_=color->currentData().toInt();
        color->setToolTip(color->currentText());
        for(auto &p:players_)if(p)p->setChromaAlgorithm(chromaAlgorithm_);
    });
    controls->addWidget(color);
    scaler_=new QComboBox(this); scaler_->setMaximumWidth(185);
    scaler_->addItems({QStringLiteral("放大：Nearest"),QStringLiteral("放大：Bilinear"),QStringLiteral("放大：Bicubic"),QStringLiteral("放大：Lanczos 3"),QStringLiteral("放大：Jinc 2"),QStringLiteral("放大：Spline36"),QStringLiteral("放大：Super-XBR（单阶段）")});
    controls->addWidget(scaler_);
    connect(scaler_,&QComboBox::currentIndexChanged,this,[this](int index){for(auto &p:players_)if(p)p->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(index),ThreeFpScalingAlgorithm::Lanczos3);});
    layout->addWidget(controls_);
    controls_->hide(); mode_->hide();
    status_=new QLabel(QStringLiteral("导入最多 9 个视频；每个视频独立保存对齐偏移。"),this); status_->setWordWrap(true); layout->addWidget(status_);
    auto *timer=new QTimer(this); connect(timer,&QTimer::timeout,this,&AnalysisPage::poll); timer->start(25);
    driftTime_.start();
}

void AnalysisPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    adjustTrackWidths();
}
void AnalysisPage::adjustTrackWidths()
{
    int desired = 160;
    for (int row = 0; row < std::min({4, trackCount_, videoCount_}); ++row)
        desired = std::max(desired, sources_[row]->fontMetrics().horizontalAdvance(sources_[row]->currentText()) + 42);
    int fixed = 160 + 7 * 8;
    if (rows_[0]) {
        fixed += times_[0]->sizeHint().width();
        for (auto *button : rows_[0]->findChildren<QPushButton *>()) fixed += button->sizeHint().width();
    }
    const int available = std::max(120, width() - 16 - fixed);
    for (auto *source : sources_) if(source) {
        source->setFixedWidth(std::clamp(desired,120,std::min(600,available)));
        source->setToolTip(source->currentText());
        int menuWidth=0;
        for(int i=0;i<source->count();++i) menuWidth=std::max(menuWidth,source->fontMetrics().horizontalAdvance(source->itemText(i))+42);
        source->view()->setMinimumWidth(std::min(900,menuWidth));
    }
}

void AnalysisPage::configurePlayer(int i)
{
    players_[i]=std::make_unique<ThreeFpPlayer>(api_,panes_[i]->surface());
    players_[i]->setChromaAlgorithm(chromaAlgorithm_);
    players_[i]->setMuted(audio_->currentData().toInt()!=i);
    players_[i]->setVrrPresent(vrr_->isChecked()); players_[i]->setVrrPacing(pacing_->isChecked());
    players_[i]->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(scaler_->currentIndex()),ThreeFpScalingAlgorithm::Lanczos3);
    connect(players_[i].get(),&ThreeFpPlayer::errorOccurred,this,[this](const QString &error){status_->setText(error);});
    connect(panes_[i],&PreviewPane::redrawRequested,players_[i].get(),&ThreeFpPlayer::redraw);
    connect(panes_[i],&PreviewPane::viewChanged,this,[this,pane=panes_[i]](float z,float x,float y){
        const int i = int(std::find(panes_.begin(), panes_.end(), pane) - panes_.begin());
        players_[i]->setView(z,x,y);
        if(sync_->isChecked() || view_->isWipe()) for(int j:visibleVideos()) if(j!=i && players_[j]) {panes_[j]->adoptView(z,x,y);players_[j]->setView(z,x,y);}
    });
    connect(panes_[i],&PreviewPane::pixelHovered,this,[this,pane=panes_[i]](int x,int y){
        const int i = int(std::find(panes_.begin(), panes_.end(), pane) - panes_.begin());
        ThreeFpPixelProbe sample{};
        if(!players_[i]->samplePixel(x,y,sample)){panes_[i]->setPixelText({});return;}
        const auto maxCode=(1u<<std::min(sample.outputBitDepth,16u))-1u;
        panes_[i]->setPixelText(QStringLiteral("(%1,%2) RGB %3 %4 %5").arg(x).arg(y).arg(qRound(sample.red*maxCode)).arg(qRound(sample.green*maxCode)).arg(qRound(sample.blue*maxCode)));
    });
}

QList<int> AnalysisPage::visibleVideos() const { return slots_.mid(0,std::min(trackCount_,videoCount_)); }
int AnalysisPage::masterVideo() const { return slots_.isEmpty()?-1:slots_.first(); }
bool AnalysisPage::activeVideo(int video) const { return visibleVideos().contains(video) || audio_->currentData().toInt()==video; }
void AnalysisPage::selectSource(int slot,int video)
{
    if(slot<0 || slot>=slots_.size() || video<0 || video>=videoCount_) return;
    pause();
    const int other=slots_.indexOf(video);
    if(other>=0) slots_.swapItemsAt(slot,other);
    refreshLayout(); seekGlobal(global_);
}
void AnalysisPage::setMode(int tracks)
{
    if(tracks<2) return;
    pause(); trackCount_=tracks;
    { QSignalBlocker block(layoutChoice_); layoutChoice_->clear();
        if(tracks==2) {layoutChoice_->addItem(QStringLiteral("并排"),MultiCompareView::Pair);layoutChoice_->addItem(QStringLiteral("A/B 滑块"),MultiCompareView::Wipe);}
        else if(tracks==3) {
            layoutChoice_->addItem(QStringLiteral("常规：三路并排"),MultiCompareView::ThreeNormal);
            layoutChoice_->addItem(QStringLiteral("ABC 滑块：横向三段"),MultiCompareView::ThreeRow);
            layoutChoice_->addItem(QStringLiteral("ABC 滑块：纵向三段"),MultiCompareView::ThreeColumn);
            layoutChoice_->addItem(QStringLiteral("ABC 滑块：左半 + 右两区"),MultiCompareView::ThreeLeft);
            layoutChoice_->addItem(QStringLiteral("ABC 滑块：右半 + 左两区"),MultiCompareView::ThreeRight);
        }
        else if(tracks==4) {
            layoutChoice_->addItem(QStringLiteral("常规：四路 2×2"),MultiCompareView::Four);
            layoutChoice_->addItem(QStringLiteral("ABCD 滑块"),MultiCompareView::FourWipe);
        } else layoutChoice_->addItem(QStringLiteral("三列网格"),MultiCompareView::Grid);
    }
    refreshLayout(); seekGlobal(global_);
}
void AnalysisPage::refreshLayout()
{
    controls_->setVisible(videoCount_>0); mode_->setVisible(videoCount_>1);
    view_->setSlots(visibleVideos(),static_cast<MultiCompareView::Layout>(layoutChoice_->currentData().toInt()));
    if (view_->isWipe()) {
        if (sync_->isEnabled()) sync_->setProperty("previousValue", sync_->isChecked());
        sync_->setChecked(true);
        sync_->setEnabled(false);
        const int reference = masterVideo();
        if (reference >= 0) {
            const auto pan = panes_[reference]->pan();
            const float zoom = panes_[reference]->zoom();
            for (int i : visibleVideos()) {
                panes_[i]->adoptView(zoom, pan.x(), pan.y());
                if (players_[i]) players_[i]->setView(zoom, pan.x(), pan.y());
            }
        }
    } else if (!sync_->isEnabled()) {
        sync_->setEnabled(true);
        sync_->setChecked(sync_->property("previousValue").toBool());
    }
    for(int row=0;row<4;++row) {
        rows_[row]->setVisible(row<std::min(trackCount_,videoCount_));
        if(row<slots_.size()){QSignalBlocker block(sources_[row]);sources_[row]->setCurrentIndex(slots_[row]);}
    }
    adjustTrackWidths();
    QTimer::singleShot(40,this,[this]{for(int i:visibleVideos())if(players_[i])players_[i]->redraw();});
}

AnalysisPage::~AnalysisPage() = default;
ThreeFpSnapshot AnalysisPage::snapshot(int lane) const { return lane>=0 && lane<videoCount_ && players_[lane] ? players_[lane]->snapshot() : ThreeFpSnapshot{}; }
std::int64_t AnalysisPage::offset(int lane) const { return offsets_[lane]; }
bool AnalysisPage::busy() const { return std::any_of(pending_.begin(),pending_.end(),[](bool v){return v;}) || std::any_of(opening_.begin(),opening_.end(),[](bool v){return v;}); }
bool AnalysisPage::loaded(int lane) const
{
    if(lane<0 || lane>=videoCount_ || !players_[lane]) return false;
    const auto state = snapshot(lane).state;
    return !paths_[lane].isEmpty() && !opening_[lane] &&
        (state == ThreeFpState::Ready || state == ThreeFpState::Paused || state == ThreeFpState::Playing || state == ThreeFpState::Ended);
}

bool AnalysisPage::openVideo(int lane,const QString &path)
{
    if(lane<0 || lane>=9 || !QFileInfo(path).isFile()) return false;
    if(!players_[lane]) configurePlayer(lane);
    if(!players_[lane]->openFile(path)) return false;
    paths_[lane]=path; offsets_[lane]=0; opening_[lane]=true; pending_[lane]=false;
    panes_[lane]->setSurfaceActive(true); panes_[lane]->setBadge(QStringLiteral("加载中…"));
    operationTime_.restart(); return true;
}
void AnalysisPage::removeVideo(int video)
{
    if (video < 0 || video >= videoCount_) return;
    pause();
    queuedSeek_.reset();
    aligning_ = -1;
    globalStep_ = false;
    const int audio = audio_->currentData().toInt();
    disconnect(panes_[video], nullptr, this, nullptr);
    players_[video].reset();
    view_->removeSource(video);
    for (int i = video; i < videoCount_ - 1; ++i) {
        players_[i] = std::move(players_[i + 1]);
        paths_[i] = std::move(paths_[i + 1]);
        offsets_[i] = offsets_[i + 1];
        opening_[i] = opening_[i + 1];
        pending_[i] = pending_[i + 1];
        generations_[i] = generations_[i + 1];
        presents_[i] = presents_[i + 1];
    }
    --videoCount_;
    paths_[videoCount_].clear();
    offsets_[videoCount_] = 0;
    opening_[videoCount_] = pending_[videoCount_] = false;
    for (int i = 0; i < 9; ++i) panes_[i] = view_->pane(i);
    slots_.removeAll(video);
    for (int &id : slots_) if (id > video) --id;
    { QSignalBlocker block(audio_);
      audio_->setCurrentIndex(audio_->findData(audio == video ? -1 : audio > video ? audio - 1 : audio)); }
    openFiles({});
}
void AnalysisPage::openFiles(const QStringList &paths)
{
    pause();
    for(const auto &path:paths) {
        if(videoCount_>=9) {status_->setText(QStringLiteral("最多加载 9 个视频。"));break;}
        if(!QFileInfo(path).isFile()) continue;
        const int i=videoCount_++;
        if(openVideo(i,path)) slots_.append(i); else --videoCount_;
    }
    QStringList names;
    for(int i=0;i<videoCount_;++i) names.append(QStringLiteral("%1 · %2").arg(i+1).arg(QFileInfo(paths_[i]).fileName()));
    for(int i=0;i<videoCount_;++i) panes_[i]->setTitle(QStringLiteral("视频 %1").arg(i+1));
    view_->setSources(names);
    for(auto *source:sources_){QSignalBlocker block(source);source->clear();source->addItems(names);}
    { QSignalBlocker block(audio_);const int selected=audio_->currentData().toInt(); audio_->clear();audio_->addItem(QStringLiteral("音频：静音"),-1);
      for(int i=0;i<videoCount_;++i) audio_->addItem(QStringLiteral("音频：%1").arg(names[i]),i);
      audio_->setCurrentIndex(audio_->findData(selected)); }
    for(int i=0;i<videoCount_;++i) players_[i]->setMuted(audio_->currentData().toInt()!=i);
    { QSignalBlocker block(mode_);mode_->clear();mode_->addItem(QStringLiteral("AB"),2);
      if(videoCount_>=3)mode_->addItem(QStringLiteral("ABC"),3);
      if(videoCount_>=4)mode_->addItem(QStringLiteral("ABCD"),4);
      if(videoCount_>4)mode_->addItem(QStringLiteral("%1 路网格").arg(videoCount_),videoCount_);
      mode_->setCurrentIndex(mode_->count()-1); }
    setMode(std::max(2,videoCount_));
}

void AnalysisPage::pause()
{
    if (playing_ && !busy()) {
        const int master = masterVideo();
        global_ = snapshot(master).position100ns - offsets_[master];
    }
    playing_ = false;
    resume_ = false;
    for (int i = 0; i < videoCount_; ++i) if (loaded(i)) players_[i]->pause();
    play_->setText(QStringLiteral("播放"));
}

void AnalysisPage::togglePlayback()
{
    if (playing_ || resume_) { pause(); return; }
    if (busy() || !loaded(masterVideo())) return;
    bool ok = true;
    for (int i = 0; i < videoCount_; ++i) if (loaded(i) && activeVideo(i)) ok = players_[i]->play() && ok;
    if (!ok) { pause(); return; }
    playing_ = true;
    driftTime_.restart();
    play_->setText(QStringLiteral("暂停"));
}

std::int64_t AnalysisPage::clampGlobal(std::int64_t position) const
{
    std::int64_t lower = std::numeric_limits<std::int64_t>::min();
    std::int64_t upper = std::numeric_limits<std::int64_t>::max();
    for (int i = 0; i < videoCount_; ++i) if (loaded(i) && activeVideo(i)) {
        lower = std::max(lower, -offsets_[i]);
        upper = std::min(upper, snapshot(i).duration100ns - offsets_[i]);
    }
    return std::clamp(position, lower, std::max(lower, upper));
}

void AnalysisPage::seekLane(int lane, std::int64_t target)
{
    const auto snap = snapshot(lane);
    generations_[lane] = snap.timelineGeneration;
    presents_[lane] = snap.presentedVideoFrames;
    pending_[lane] = players_[lane]->seek(std::clamp<std::int64_t>(target, 0, snap.duration100ns));
    operationTime_.restart();
}

void AnalysisPage::seekGlobal(std::int64_t position)
{
    if (!loaded(masterVideo())) return;
    if (busy()) { queuedSeek_ = position; return; }
    const bool resume = playing_ || resume_;
    pause();
    resume_ = resume;
    global_ = clampGlobal(position);
    for (int i = 0; i < videoCount_; ++i) if (loaded(i) && activeVideo(i)) seekLane(i, global_ + offsets_[i]);
}

void AnalysisPage::beginStep(int lane, int direction, bool global)
{
    if (busy() || !loaded(lane)) return;
    pause();
    const auto snap = snapshot(lane);
    if ((direction < 0 && snap.position100ns <= 0) || (direction > 0 && snap.position100ns >= snap.duration100ns)) return;
    generations_[lane] = snap.timelineGeneration;
    presents_[lane] = snap.presentedVideoFrames;
    pending_[lane] = players_[lane]->stepFrame(direction);
    if (pending_[lane]) { aligning_ = lane; globalStep_ = global; operationTime_.restart(); }
}

void AnalysisPage::stepGlobal(int direction) { beginStep(masterVideo(), direction, true); }
void AnalysisPage::alignVideo(int lane, int direction, bool seconds)
{
    if (busy() || !loaded(lane)) return;
    if (!seconds) { beginStep(lane, direction, false); return; }
    pause();
    aligning_ = lane;
    globalStep_ = false;
    seekLane(lane, snapshot(lane).position100ns + direction * 10000000LL);
}

void AnalysisPage::poll()
{
    for (int i = 0; i < videoCount_; ++i) {
        const auto snap = snapshot(i);
        if (opening_[i] && snap.state == ThreeFpState::Ready) {
            opening_[i] = false;
            seekLane(i, global_ + offsets_[i]);
        }
        if ((opening_[i] || pending_[i]) && snap.state == ThreeFpState::Failed) {
            opening_[i] = pending_[i] = false;
            pause();
            status_->setText(players_[i]->lastError());
        }
        if (pending_[i] && snap.timelineGeneration != generations_[i] &&
            (snap.presentedVideoFrames != presents_[i] || snap.state == ThreeFpState::Ended ||
             snap.position100ns >= snap.duration100ns)) pending_[i] = false;
        if(snap.videoWidth>0) panes_[i]->setVideoSize(QSize(snap.videoWidth,snap.videoHeight));
        if(snap.presentedVideoFrames>0) panes_[i]->setBadge(QStringLiteral("%1×%2").arg(snap.videoWidth).arg(snap.videoHeight));
        const int row=slots_.indexOf(i);
        if(row>=0 && row<4) {
            if(!sliders_[row]->isSliderDown() && !pending_[i]) {QSignalBlocker block(sliders_[row]);sliders_[row]->setValue(snap.duration100ns>0?int(snap.position100ns*100000/snap.duration100ns):0);}
            times_[row]->setText(QStringLiteral("%1 / %2").arg(timeText(snap.position100ns),timeText(snap.duration100ns)));
            times_[row]->setToolTip(QStringLiteral("相对偏移：%1 秒").arg(offsets_[i]/10000000.0,0,'f',6));
        }
    }
    if (busy()) {
        if (operationTime_.isValid() && operationTime_.elapsed() > 15000) {
            pending_ = {}; opening_ = {}; aligning_ = -1; queuedSeek_.reset(); pause();
            status_->setText(QStringLiteral("加载或定位超时，请重新打开视频。"));
        }
        return;
    }
    if (aligning_ >= 0) {
        const int lane = std::exchange(aligning_, -1);
        if (globalStep_) {
            global_ = snapshot(lane).position100ns - offsets_[lane];
            seekGlobal(global_);
        } else {
            offsets_[lane] = snapshot(lane).position100ns - global_;
        }
        status_->setText(QStringLiteral("视频 %1 对齐偏移：%2 秒").arg(lane+1).arg(offsets_[lane]/10000000.0,0,'f',6));
        return;
    }
    if (queuedSeek_) { const auto target = *queuedSeek_; queuedSeek_.reset(); seekGlobal(target); return; }
    if (resume_) { resume_ = false; togglePlayback(); }
    if (!playing_) return;
    const int master = masterVideo();
    global_ = snapshot(master).position100ns - offsets_[master];
    for (int i = 0; i < videoCount_; ++i) if (loaded(i) && activeVideo(i) && snapshot(i).state == ThreeFpState::Ended) { pause(); return; }
    if(driftTime_.elapsed()>500) {
        for(int i=0;i<videoCount_;++i) if(i!=master && loaded(i) && activeVideo(i) &&
            std::abs(snapshot(i).position100ns-offsets_[i]-global_)>800000) seekLane(i,global_+offsets_[i]);
        driftTime_.restart();
    }
}

void AnalysisPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
void AnalysisPage::dropEvent(QDropEvent *event)
{
    QStringList paths;
    for (const auto &url : event->mimeData()->urls()) if (url.isLocalFile()) paths.append(url.toLocalFile());
    openFiles(paths);
    event->acceptProposedAction();
}
}
