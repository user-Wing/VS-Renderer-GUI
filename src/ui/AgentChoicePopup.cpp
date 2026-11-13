#include "ui/AgentChoicePopup.h"
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
namespace {
constexpr int checkedRole=Qt::UserRole+1,descriptionRole=Qt::UserRole+2;
class ChoiceDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &,const QModelIndex &index) const override {return {330,index.data(descriptionRole).toString().isEmpty()?45:65};}
    void paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        painter->save();painter->setRenderHint(QPainter::Antialiasing);const auto row=option.rect.adjusted(3,2,-3,-2);const bool checked=index.data(checkedRole).toBool();
        if(option.state.testFlag(QStyle::State_Selected) || option.state.testFlag(QStyle::State_MouseOver)){painter->setPen(Qt::NoPen);painter->setBrush(QColor("#3b424d"));painter->drawRoundedRect(row,9,9);}
        const auto description=index.data(descriptionRole).toString();auto font=option.font;font.setFamilies({"Segoe UI","Microsoft YaHei UI"});font.setPixelSize(14);font.setWeight(checked?QFont::DemiBold:QFont::Normal);painter->setFont(font);painter->setPen(QColor("#f1f3f7"));
        const QRect title(row.left()+13,row.top()+(description.isEmpty()?0:9),row.width()-53,description.isEmpty()?row.height():22);painter->drawText(title,Qt::AlignLeft|Qt::AlignVCenter,QFontMetrics(font).elidedText(index.data(Qt::DisplayRole).toString(),Qt::ElideRight,title.width()));
        if(!description.isEmpty()){font.setPixelSize(12);font.setWeight(QFont::Normal);painter->setFont(font);painter->setPen(QColor("#aeb7c5"));const QRect detail(title.left(),row.top()+32,title.width(),20);painter->drawText(detail,Qt::AlignLeft|Qt::AlignVCenter,QFontMetrics(font).elidedText(description,Qt::ElideRight,detail.width()));}
        if(checked){painter->setPen(QPen(QColor("#e2eaf5"),2.2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));QPainterPath tick;const QPointF center(row.right()-21,row.center().y());tick.moveTo(center+QPointF(-5,0));tick.lineTo(center+QPointF(-1,4));tick.lineTo(center+QPointF(6,-4));painter->drawPath(tick);}painter->restore();
    }
};
}

AgentChoiceButton::AgentChoiceButton(QWidget *parent):QPushButton(parent)
{
    setProperty("agentControl",true);setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
}

QSize AgentChoiceButton::minimumSizeHint() const {return {0,QPushButton::minimumSizeHint().height()};}
QSize AgentChoiceButton::sizeHint() const {auto size=QPushButton::sizeHint();size.rwidth()+=22;return size;}

void AgentChoiceButton::paintEvent(QPaintEvent *)
{
    QStyleOptionButton option;initStyleOption(&option);option.text=fontMetrics().elidedText(text(),Qt::ElideRight,qMax(0,width()-39));QPainter painter(this);style()->drawControl(QStyle::CE_PushButton,&option,&painter,this);
    painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QPen(palette().color(isEnabled()?QPalette::Active:QPalette::Disabled,QPalette::ButtonText),1.3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    const QPointF point(width()-15,height()/2.);QPainterPath arrow;arrow.moveTo(point+QPointF(-3,-1));arrow.lineTo(point+QPointF(0,2));arrow.lineTo(point+QPointF(3,-1));painter.drawPath(arrow);
}

AgentChoicePopup::AgentChoicePopup(const QString &title,const QString &description,const QList<AgentChoice> &choices,const QString &selected,QWidget *parent)
    :QWidget(parent,Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint)
{
    setObjectName("agentChoicePopup");setAttribute(Qt::WA_TranslucentBackground);setAttribute(Qt::WA_DeleteOnClose);auto font=QFont("Segoe UI");font.setPixelSize(14);setFont(font);
    setStyleSheet(QStringLiteral("QWidget#agentChoicePanel{background:#292e37;border:1px solid #667284;border-radius:15px;} QLabel{background:transparent;color:#aeb7c5;border:0;} QListWidget{background:transparent;color:#eef2f7;border:0;outline:0;} QListWidget QWidget{background:transparent;color:#eef2f7;} QListWidget::item{border:0;} QScrollBar:vertical{background:transparent;width:8px;margin:3px;} QScrollBar::handle:vertical{background:#7d899a;border-radius:4px;min-height:24px;} QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;} QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));
    auto *outside=new QVBoxLayout(this);outside->setContentsMargins(9,9,9,9);auto *panel=new QWidget(this);panel->setObjectName("agentChoicePanel");outside->addWidget(panel);auto *layout=new QVBoxLayout(panel);layout->setContentsMargins(14,15,14,11);layout->setSpacing(8);
    auto *heading=new QLabel(title,panel);heading->setObjectName("agentChoiceTitle");auto headingFont=font;headingFont.setWeight(QFont::DemiBold);heading->setFont(headingFont);layout->addWidget(heading);
    if(!description.isEmpty()){auto *detail=new QLabel(description,panel);detail->setObjectName("agentChoiceDescription");auto detailFont=font;detailFont.setPixelSize(12);detail->setFont(detailFont);detail->setWordWrap(true);detail->setMaximumHeight(38);detail->setToolTip(description);layout->addWidget(detail);}
    list_=new QListWidget(panel);list_->setObjectName("agentChoiceList");list_->setFont(font);list_->setItemDelegate(new ChoiceDelegate(list_));list_->setMouseTracking(true);list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);layout->addWidget(list_);
    int selectedRow=0;for(const auto &choice:choices){auto *item=new QListWidgetItem(choice.label,list_);item->setData(Qt::UserRole,choice.id);item->setData(checkedRole,choice.id==selected);item->setData(descriptionRole,choice.description);item->setToolTip(choice.label+(choice.description.isEmpty()?QString():"\n"+choice.description));if(choice.id==selected)selectedRow=list_->count()-1;}
    if(list_->count())list_->setCurrentRow(selectedRow);
    connect(list_,&QListWidget::itemClicked,this,[this]{chooseCurrent();});connect(list_,&QListWidget::itemActivated,this,[this]{chooseCurrent();});
}

void AgentChoicePopup::openFor(QWidget *anchor)
{
    const auto point=anchor->mapToGlobal(QPoint(0,0));auto *screen=QGuiApplication::screenAt(point);if(!screen)screen=QGuiApplication::primaryScreen();const auto available=screen->availableGeometry();
    const auto inset=available.adjusted(8,8,-8,-8);const auto bounds=inset.isValid()?inset:available;const int width=std::min(std::max(340,anchor->width()+80),bounds.width());setFixedWidth(width);int rows=0;for(int i=0;i<list_->count();++i)rows+=list_->sizeHintForRow(i);list_->setFixedHeight(std::max(1,std::min({rows+4,420,bounds.height()-160})));adjustSize();setFixedHeight(qMin(height(),bounds.height()));
    const int x=std::clamp(point.x()+anchor->width()-this->width(),bounds.left(),std::max(bounds.left(),bounds.right()-this->width()+1));const int below=point.y()+anchor->height()+4;const int preferred=below+height()<=bounds.bottom()?below:point.y()-height()-4;const int y=std::clamp(preferred,bounds.top(),std::max(bounds.top(),bounds.bottom()-height()+1));move(x,y);show();raise();list_->setFocus();if(list_->currentItem())list_->scrollToItem(list_->currentItem());
}

void AgentChoicePopup::chooseCurrent()
{
    if(chosen_ || !list_->currentItem())return;
    chosen_=true;const auto id=list_->currentItem()->data(Qt::UserRole).toString();close();emit chosen(id);
}

void AgentChoicePopup::keyPressEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Escape){close();return;}QWidget::keyPressEvent(event);
}

void AgentChoicePopup::paintEvent(QPaintEvent *)
{
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);for(int i=8;i>0;--i){painter.setPen(Qt::NoPen);painter.setBrush(QColor(0,0,0,3+(8-i)));painter.drawRoundedRect(rect().adjusted(i,i+1,-i,-i+1),17,17);}
}
}
