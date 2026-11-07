#include "ui/CompareView.h"
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include <QVBoxLayout>
namespace vsr {
CompareView::CompareView(QWidget *parent,const QString &leftTitle,const QString &rightTitle):QWidget(parent),view_(new MultiCompareView(this)) {
    setMinimumSize(640,240);auto *layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->addWidget(view_);
    view_->setSourceControlsVisible(false);sourcePane()->setTitle(leftTitle);sourcePane()->setBadge(QStringLiteral("Fit"));processedPane()->setTitle(rightTitle);processedPane()->setBadge(QStringLiteral("VS · 待渲染"));updateLayout();
}
PreviewPane *CompareView::sourcePane() const { return view_->pane(0); }
PreviewPane *CompareView::processedPane() const { return view_->pane(1); }
PreviewPane *CompareView::addProcessedPane(){visible_<<true;auto *pane=view_->pane(visible_.size()-1);pane->setTitle(QString("路 %1").arg(visible_.size()-1));pane->setBadge(QStringLiteral("VS · 待渲染"));updateLayout();return pane;}
void CompareView::removeProcessedPane(int route){if(route<1 || route+1>=visible_.size())return;view_->removeSource(route+1);visible_.removeAt(route+1);updateLayout();}
void CompareView::setRouteVisible(int route,bool visible){if(route>=0 && route+1<visible_.size()){visible_[route+1]=visible;updateLayout();}}
double CompareView::splitRatio() const { return view_->divisionX(); }
void CompareView::setSplitRatio(double ratio){view_->setDivision(ratio,.5);}
void CompareView::setMode(CompareMode mode){if(mode_!=mode){mode_=mode;updateLayout();}}
void CompareView::updateLayout(){
    QList<int> assignments;for(int i=0;i<visible_.size();++i)if(visible_[i])assignments<<i;
    const auto layout=mode_==CompareMode::Slider?(assignments.size()==4?MultiCompareView::FourWipe:assignments.size()==3?MultiCompareView::ThreeLeft:MultiCompareView::Wipe):(assignments.size()==4?MultiCompareView::Four:assignments.size()==3?MultiCompareView::ThreeNormal:MultiCompareView::Pair);
    view_->setSlots(assignments,layout);emit visiblePaneCountChanged(assignments.size());
}
}
