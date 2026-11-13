#include "app/MainWindow.h"
#include <QInputDialog>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>
#include <climits>
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"

namespace vsr {
qint64 MainWindow::groupFrameBoundary(const FilterTimeGroup &g) const {
    if(g.startFrame>=0)return g.startFrame;
    if(sourceFpsNumerator_<=0||sourceFpsDenominator_<=0)return 0;
    return (g.startSeconds*sourceFpsNumerator_+sourceFpsDenominator_-1)/sourceFpsDenominator_;
}

QList<FilterTimeGroup> MainWindow::currentTimeGroups() const {
    auto groups=timeGroups_;if(timeGroupIndex_>=0 && timeGroupIndex_<groups.size()){
        auto &group=groups[timeGroupIndex_];
        if(PresetStore::metadata(group.graph,SourceFilter::Ffms2,{},{}).value("nodes")!=PresetStore::metadata(graph_,SourceFilter::Ffms2,{},{}).value("nodes"))group.script.clear();
        group.graph=graph_;
    }return groups;
}
void MainWindow::replaceTimeGroups(const QList<FilterTimeGroup> &groups){
    timeGroups_=groups;if(timeGroups_.isEmpty())timeGroups_.append({QStringLiteral("全片"),0,{}});
    refreshTimeGroups(0);
}
void MainWindow::refreshTimeGroups(int selected){
    QTimer::singleShot(0,this,&MainWindow::checkpointProject);
    timeGroupIndex_=qBound(0,selected,int(timeGroups_.size())-1);graph_=timeGroups_[timeGroupIndex_].graph;
    const QSignalBlocker listBlock(timeGroupList_),startBlock(groupStart_),nameBlock(groupName_),frameBlock(groupFrame_);
    timeGroupList_->clear();
    for(int i=0;i<timeGroups_.size();++i){const auto &g=timeGroups_[i];const auto begin=g.startFrame>=0?QString("#%1 帧").arg(g.startFrame):QString("%1 秒").arg(g.startSeconds);timeGroupList_->addItem(g.name+" · "+begin+(g.script.isEmpty()?QString():QStringLiteral(" · VPY")));}
    timeGroupList_->setCurrentRow(timeGroupIndex_);groupName_->setText(timeGroups_[timeGroupIndex_].name);
    groupStart_->setRange(0,8640000);
    groupStart_->setValue(int(timeGroups_[timeGroupIndex_].startSeconds));groupStart_->setEnabled(timeGroupIndex_>0);
    if(timeGroups_[timeGroupIndex_].startFrame>=0&&sourceFpsNumerator_>0)groupStart_->setToolTip(QStringLiteral("精确起点：%1 秒；下方帧号为准，秒输入用于整秒跳转。").arg(double(timeGroups_[timeGroupIndex_].startFrame)*sourceFpsDenominator_/sourceFpsNumerator_,0,'f',6));else groupStart_->setToolTip({});
    const bool fpsKnown=sourceFpsNumerator_>0 && sourceFpsDenominator_>0;
    const qint64 minimum=timeGroupIndex_?groupFrameBoundary(timeGroups_[timeGroupIndex_-1])+1:0;
    const qint64 maximum=timeGroupIndex_+1<timeGroups_.size()?groupFrameBoundary(timeGroups_[timeGroupIndex_+1])-1:sourceTotalFrames_>0?sourceTotalFrames_-1:INT_MAX;
    groupFrame_->setRange(int(qMin<qint64>(minimum,INT_MAX)),int(qBound<qint64>(minimum,maximum,INT_MAX)));groupFrame_->setValue(int(groupFrameBoundary(timeGroups_[timeGroupIndex_])));groupFrame_->setEnabled(timeGroupIndex_>0&&fpsKnown);
    refreshPipeline(graph_.nodes().isEmpty()?-1:0);
}
QWidget *MainWindow::buildTimeGroups(){
    auto *panel=new QWidget(this);panel->setObjectName("rendererTimeGroups");auto *layout=new QVBoxLayout(panel);layout->setContentsMargins(8,8,8,8);
    layout->addWidget(new QLabel(QStringLiteral("当前时间组 · CFR源帧边界（帧号从0开始）"),panel));
    timeGroupList_=new QListWidget(panel);timeGroupList_->setObjectName("rendererTimeGroupList");timeGroupList_->setContextMenuPolicy(Qt::CustomContextMenu);layout->addWidget(timeGroupList_,1);
    groupName_=new QLineEdit(panel);groupName_->setObjectName("rendererTimeGroupName");layout->addWidget(groupName_);
    groupStart_=new QSpinBox(panel);groupStart_->setObjectName("rendererTimeGroupStart");groupStart_->setSuffix(QStringLiteral(" 秒开始"));layout->addWidget(groupStart_);
    groupFrame_=new QSpinBox(panel);groupFrame_->setObjectName("rendererTimeGroupFrame");groupFrame_->setRange(0,INT_MAX);groupFrame_->setSuffix(QStringLiteral(" 帧开始（源帧，0基）"));layout->addWidget(groupFrame_);
    auto *hint=new QLabel(QStringLiteral("精确帧通过源帧号实现"),panel);hint->setObjectName("rendererFrameBoundaryHint");layout->addWidget(hint);
    auto *row=new QHBoxLayout;auto *add=new QPushButton(QStringLiteral("新建时间组"),panel);auto *remove=new QPushButton(QStringLiteral("删除"),panel);row->addWidget(add);row->addWidget(remove);layout->addLayout(row);
    connect(timeGroupList_,&QListWidget::currentRowChanged,this,[this](int index){if(index<0)return;timeGroups_=currentTimeGroups();refreshTimeGroups(index);});
    connect(groupName_,&QLineEdit::editingFinished,this,[this]{timeGroups_=currentTimeGroups();timeGroups_[timeGroupIndex_].name=groupName_->text().trimmed();refreshTimeGroups(timeGroupIndex_);activePreset_.clear();});
    connect(groupStart_,&QSpinBox::valueChanged,this,[this](int value){
        timeGroups_=currentTimeGroups();auto candidate=timeGroups_[timeGroupIndex_];candidate.startSeconds=value;candidate.startFrame=-1;
        if(sourceFpsNumerator_>0&&sourceFpsDenominator_>0){candidate.startFrame=groupFrameBoundary(candidate);if((timeGroupIndex_&&candidate.startFrame<=groupFrameBoundary(timeGroups_[timeGroupIndex_-1]))||(timeGroupIndex_+1<timeGroups_.size()&&candidate.startFrame>=groupFrameBoundary(timeGroups_[timeGroupIndex_+1]))){setStatus(QStringLiteral("起点与相邻时间组重叠。"),true);refreshTimeGroups(timeGroupIndex_);return;}}
        timeGroups_[timeGroupIndex_]=candidate;refreshTimeGroups(timeGroupIndex_);activePreset_.clear();});
    connect(groupFrame_,&QSpinBox::valueChanged,this,[this](int value){if(sourceFpsNumerator_<=0||sourceFpsDenominator_<=0)return;timeGroups_=currentTimeGroups();auto &g=timeGroups_[timeGroupIndex_];g.startFrame=value;g.startSeconds=qint64(value)*sourceFpsDenominator_/sourceFpsNumerator_;activePreset_.clear();refreshTimeGroups(timeGroupIndex_);});
    connect(add,&QPushButton::clicked,this,[this]{
        timeGroups_=currentTimeGroups();const int proposed=sourcePlayer_?qMax(1,int((sourcePlayer_->snapshot().position100ns+5000000)/10000000)):1;
        bool accepted=false;const int second=QInputDialog::getInt(this,QStringLiteral("新建时间组"),QStringLiteral("开始秒数（此前保留原时间组，后面使用新滤镜链）"),proposed,1,8640000,1,&accepted);if(!accepted)return;
        FilterTimeGroup added{QStringLiteral("时间组 %1").arg(timeGroups_.size()+1),second,graph_,timeGroups_[timeGroupIndex_].script};
        const bool known=sourceFpsNumerator_>0&&sourceFpsDenominator_>0;if(known)added.startFrame=groupFrameBoundary(added);
        int index=0;for(const auto &group:timeGroups_){const auto at=known?groupFrameBoundary(group):group.startSeconds;const auto next=known?added.startFrame:second;if(at==next){setStatus(QStringLiteral("该起点已有时间组。"),true);return;}if(at<next)++index;}
        timeGroups_.insert(index,added);activePreset_.clear();refreshTimeGroups(index);
    });
    auto *addFrame=new QPushButton(QStringLiteral("按帧新建…"),panel);addFrame->setObjectName("rendererAddFrameGroup");layout->addWidget(addFrame);
    connect(addFrame,&QPushButton::clicked,this,[this]{
        if(sourceFpsNumerator_<=0||sourceFpsDenominator_<=0){setStatus(QStringLiteral("请先打开CFR源并等待媒体信息就绪。"),true);return;}
        timeGroups_=currentTimeGroups();bool ok=false;const int frame=QInputDialog::getInt(this,QStringLiteral("按源帧建立时间组"),QStringLiteral("源帧号（从0开始；新时间组包含此帧）"),qMax(1,int(sourcePlayer_->snapshot().frameIndex)),1,sourceTotalFrames_>1?sourceTotalFrames_-1:INT_MAX,1,&ok);if(!ok)return;
        int index=0;for(const auto &g:timeGroups_){if(groupFrameBoundary(g)==frame){setStatus(QStringLiteral("该帧已有时间组。"),true);return;}if(groupFrameBoundary(g)<frame)++index;}
        timeGroups_.insert(index,{QStringLiteral("时间组 %1").arg(timeGroups_.size()+1),qint64(frame)*sourceFpsDenominator_/sourceFpsNumerator_,graph_,timeGroups_[timeGroupIndex_].script,frame});activePreset_.clear();refreshTimeGroups(index);
    });
    connect(remove,&QPushButton::clicked,this,[this]{if(timeGroups_.size()==1)return;timeGroups_=currentTimeGroups();timeGroups_.removeAt(timeGroupIndex_);timeGroups_.first().startSeconds=0;timeGroups_.first().startFrame=0;activePreset_.clear();refreshTimeGroups(qMin(timeGroupIndex_,int(timeGroups_.size())-1));});
    connect(timeGroupList_,&QListWidget::customContextMenuRequested,this,[this](const QPoint &point){
        const int row=timeGroupList_->indexAt(point).row();if(row<0)return;timeGroupList_->setCurrentRow(row);
        QMenu menu(this);auto *copy=menu.addAction(QStringLiteral("复制本时间组所有滤镜"));auto *paste=menu.addAction(QStringLiteral("粘贴所有滤镜到本时间组"));paste->setEnabled(hasCopiedFilters_);
        const auto action=menu.exec(timeGroupList_->viewport()->mapToGlobal(point));if(action==copy){copiedFilters_=graph_;copiedGroupScript_=currentTimeGroups()[timeGroupIndex_].script;hasCopiedFilters_=true;}else if(action==paste){graph_=copiedFilters_;timeGroups_[timeGroupIndex_].graph=graph_;timeGroups_[timeGroupIndex_].script=copiedGroupScript_;activePreset_.clear();refreshPipeline(graph_.nodes().isEmpty()?-1:0);}
    });
    // Parameter editor is constructed by the adjacent column afterwards.
    const QSignalBlocker block(timeGroupList_);timeGroupList_->addItem(QStringLiteral("全片 · 0–片尾 秒"));timeGroupList_->setCurrentRow(0);groupStart_->setRange(0,0);groupStart_->setEnabled(false);groupName_->setText(QStringLiteral("全片"));return panel;
}
}
