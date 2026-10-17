#include "app/MainWindow.h"
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"

namespace vsr {
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
    timeGroupIndex_=qBound(0,selected,int(timeGroups_.size())-1);graph_=timeGroups_[timeGroupIndex_].graph;
    const QSignalBlocker listBlock(timeGroupList_),startBlock(groupStart_),nameBlock(groupName_);
    timeGroupList_->clear();
    for(int i=0;i<timeGroups_.size();++i)timeGroupList_->addItem(QString("%1 · %2–%3 秒").arg(timeGroups_[i].name+(timeGroups_[i].script.isEmpty()?QString():QStringLiteral(" · VPY"))).arg(timeGroups_[i].startSeconds).arg(i+1<timeGroups_.size()?QString::number(timeGroups_[i+1].startSeconds):QStringLiteral("片尾")));
    timeGroupList_->setCurrentRow(timeGroupIndex_);groupName_->setText(timeGroups_[timeGroupIndex_].name);
    groupStart_->setRange(timeGroupIndex_?int(timeGroups_[timeGroupIndex_-1].startSeconds)+1:0,timeGroupIndex_+1<timeGroups_.size()?int(timeGroups_[timeGroupIndex_+1].startSeconds)-1:8640000);
    groupStart_->setValue(int(timeGroups_[timeGroupIndex_].startSeconds));groupStart_->setEnabled(timeGroupIndex_>0);
    refreshPipeline(graph_.nodes().isEmpty()?-1:0);
}
QWidget *MainWindow::buildTimeGroups(){
    auto *panel=new QWidget(this);panel->setObjectName("rendererTimeGroups");auto *layout=new QVBoxLayout(panel);layout->setContentsMargins(8,8,8,8);
    layout->addWidget(new QLabel(QStringLiteral("当前时间组 · 整秒吸附"),panel));
    timeGroupList_=new QListWidget(panel);timeGroupList_->setObjectName("rendererTimeGroupList");timeGroupList_->setContextMenuPolicy(Qt::CustomContextMenu);layout->addWidget(timeGroupList_,1);
    groupName_=new QLineEdit(panel);groupName_->setObjectName("rendererTimeGroupName");layout->addWidget(groupName_);
    groupStart_=new QSpinBox(panel);groupStart_->setObjectName("rendererTimeGroupStart");groupStart_->setSuffix(QStringLiteral(" 秒开始"));layout->addWidget(groupStart_);
    auto *row=new QHBoxLayout;auto *add=new QPushButton(QStringLiteral("新建时间组"),panel);auto *remove=new QPushButton(QStringLiteral("删除"),panel);row->addWidget(add);row->addWidget(remove);layout->addLayout(row);
    connect(timeGroupList_,&QListWidget::currentRowChanged,this,[this](int index){if(index<0)return;timeGroups_=currentTimeGroups();refreshTimeGroups(index);});
    connect(groupName_,&QLineEdit::editingFinished,this,[this]{timeGroups_=currentTimeGroups();timeGroups_[timeGroupIndex_].name=groupName_->text().trimmed();refreshTimeGroups(timeGroupIndex_);activePreset_.clear();});
    connect(groupStart_,&QSpinBox::valueChanged,this,[this](int value){timeGroups_=currentTimeGroups();timeGroups_[timeGroupIndex_].startSeconds=value;refreshTimeGroups(timeGroupIndex_);activePreset_.clear();});
    connect(add,&QPushButton::clicked,this,[this]{
        timeGroups_=currentTimeGroups();const int proposed=sourcePlayer_?qMax(1,int((sourcePlayer_->snapshot().position100ns+5000000)/10000000)):1;
        bool accepted=false;const int second=QInputDialog::getInt(this,QStringLiteral("新建时间组"),QStringLiteral("开始秒数（此前保留原时间组，后面使用新滤镜链）"),proposed,1,8640000,1,&accepted);if(!accepted)return;
        for(const auto &group:timeGroups_)if(group.startSeconds==second){setStatus(QStringLiteral("该秒数已有时间组。"),true);return;}
        int index=0;while(index<timeGroups_.size() && timeGroups_[index].startSeconds<second)++index;
        timeGroups_.insert(index,{QStringLiteral("时间组 %1").arg(timeGroups_.size()+1),second,graph_,timeGroups_[timeGroupIndex_].script});activePreset_.clear();refreshTimeGroups(index);
    });
    connect(remove,&QPushButton::clicked,this,[this]{if(timeGroups_.size()==1)return;timeGroups_=currentTimeGroups();timeGroups_.removeAt(timeGroupIndex_);timeGroups_.first().startSeconds=0;activePreset_.clear();refreshTimeGroups(qMin(timeGroupIndex_,int(timeGroups_.size())-1));});
    connect(timeGroupList_,&QListWidget::customContextMenuRequested,this,[this](const QPoint &point){
        const int row=timeGroupList_->indexAt(point).row();if(row<0)return;timeGroupList_->setCurrentRow(row);
        QMenu menu(this);auto *copy=menu.addAction(QStringLiteral("复制本时间组所有滤镜"));auto *paste=menu.addAction(QStringLiteral("粘贴所有滤镜到本时间组"));paste->setEnabled(hasCopiedFilters_);
        const auto action=menu.exec(timeGroupList_->viewport()->mapToGlobal(point));if(action==copy){copiedFilters_=graph_;copiedGroupScript_=currentTimeGroups()[timeGroupIndex_].script;hasCopiedFilters_=true;}else if(action==paste){graph_=copiedFilters_;timeGroups_[timeGroupIndex_].graph=graph_;timeGroups_[timeGroupIndex_].script=copiedGroupScript_;activePreset_.clear();refreshPipeline(graph_.nodes().isEmpty()?-1:0);}
    });
    // Parameter editor is constructed by the adjacent column afterwards.
    const QSignalBlocker block(timeGroupList_);timeGroupList_->addItem(QStringLiteral("全片 · 0–片尾 秒"));timeGroupList_->setCurrentRow(0);groupStart_->setRange(0,0);groupStart_->setEnabled(false);groupName_->setText(QStringLiteral("全片"));return panel;
}
}
