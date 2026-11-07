#include "player/PlayerMenu.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionMenuItem>
#include <QToolTip>
#include <QEnterEvent>
#include <QCursor>
#include <cmath>

namespace vsr {
PlayerMenu::PlayerMenu(QWidget *parent) : QMenu(parent) {
    setToolTipsVisible(true);
    descriptionTimer_.setInterval(200);
    connect(&descriptionTimer_,&QTimer::timeout,this,&PlayerMenu::showDescription);
    connect(this,&QMenu::hovered,this,[this](QAction *action){
        if(action==descriptionAction_)return;
        clearDescription();
        if(action && actions().contains(action) && action->menu() && !action->property("playerDescription").toString().isEmpty()){descriptionAction_=action;descriptionTimer_.start();}
    });
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu,false);QApplication::setEffectEnabled(Qt::UI_FadeMenu,false);
    setWindowFlags(windowFlags()|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("QMenu{background:transparent;border:none;padding:6px;} QMenu::item{padding:8px 40px 8px 48px;} QMenu::indicator{width:0px;} QMenu::separator{height:9px;}");
    animation_.setTimerType(Qt::PreciseTimer);animation_.setInterval(8);
    connect(&animation_,&QTimer::timeout,this,[this]{
        const qreal t=qMin(1.0,elapsed_.elapsed()/180.0);reveal_=1-std::pow(1-t,3);
        setProperty("menuReveal",reveal_);update();if(t>=1)animation_.stop();
    });
}
QMenu *PlayerMenu::add(QMenu *parent,const QString &title) {
    auto *menu=new PlayerMenu(parent);menu->setTitle(title);parent->addMenu(menu);return menu;
}
void PlayerMenu::showEvent(QShowEvent *event) {
    QMenu::showEvent(event);reveal_=0;setProperty("menuReveal",0.0);elapsed_.restart();animation_.start();
}
void PlayerMenu::hideEvent(QHideEvent *event) {animation_.stop();clearDescription();QMenu::hideEvent(event);}
void PlayerMenu::enterEvent(QEnterEvent *event) {
    QMenu::enterEvent(event);if(auto *parent=dynamic_cast<PlayerMenu*>(parentWidget()))parent->clearDescription();
}
void PlayerMenu::leaveEvent(QEvent *event) {
    QMenu::leaveEvent(event);if(descriptionAction_ && !actionGeometry(descriptionAction_).contains(mapFromGlobal(QCursor::pos())))clearDescription();
}
void PlayerMenu::clearDescription() {
    descriptionTimer_.stop();if(descriptionAction_)QToolTip::hideText();descriptionAction_.clear();
}
void PlayerMenu::showDescription() {
    if(!isVisible() || !descriptionAction_ || activeAction()!=descriptionAction_){clearDescription();return;}
    const auto area=actionGeometry(descriptionAction_);const auto text=descriptionAction_->property("playerDescription").toString();
    if(text.isEmpty()){clearDescription();return;}
    // A submenu can cancel the platform's delayed ToolTip event while its parent stays hovered.
    QToolTip::showText(mapToGlobal(QPoint(area.left()+12,area.bottom()+4)),text,this,area);
}
void PlayerMenu::paintEvent(QPaintEvent *) {
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_Source);painter.fillRect(rect(),Qt::transparent);painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QPainterPath border;border.addRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),8,8);
    painter.setClipPath(border);painter.setClipRect(QRectF(0,0,width(),height()*reveal_),Qt::IntersectClip);
    int opacity=100;for(auto *owner=parentWidget();owner;owner=owner->parentWidget())if(owner->property("playerMenuOpacity").isValid()){opacity=owner->property("playerMenuOpacity").toInt();break;}
    painter.fillPath(border,QColor(41,43,48,qBound(10,opacity,100)*255/100));painter.setPen(QColor("#555860"));painter.drawPath(border);
    painter.translate(0,-8*(1-reveal_));
    for(auto *action:actions()) {
        if(!action->isVisible())continue;
        const auto area=actionGeometry(action);
        if(action->isSeparator()){painter.setPen(QColor("#50535a"));painter.drawLine(area.left()+8,area.center().y(),area.right()-8,area.center().y());continue;}
        QStyleOptionMenuItem option;initStyleOption(&option,action);painter.setFont(option.font);
        if(action==activeAction() && action->isEnabled()){painter.setPen(Qt::NoPen);painter.setBrush(QColor("#426da7"));painter.drawRoundedRect(area.adjusted(1,0,-1,0),4,4);}
        const QColor text=action->isEnabled()?QColor("#e8eaed"):QColor("#858890");
        if(action->isCheckable()) {
            // Text starts 48 px from the row edge; center the box in that gutter.
            const QRectF box(area.left()+16,area.center().y()-8,16,16);
            painter.setPen(QPen(action->isChecked()?QColor("#8ab4f8"):text,1.3));
            painter.setBrush(action->isChecked()?QColor("#0067c0"):QColor("#303238"));painter.drawRoundedRect(box,3,3);
            if(action->isChecked()){QPainterPath check;check.moveTo(box.left()+3,box.top()+8);check.lineTo(box.left()+7,box.top()+12);check.lineTo(box.left()+13,box.top()+4);painter.setPen(QPen(Qt::white,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.drawPath(check);}
        }
        painter.setPen(text);painter.drawText(area.adjusted(48,0,-36,0),Qt::AlignVCenter|Qt::AlignLeft|Qt::TextShowMnemonic,action->text());
        if(action->menu()){const QPointF center(area.right()-16,area.center().y());QPainterPath arrow;arrow.moveTo(center+QPointF(-2,-4));arrow.lineTo(center+QPointF(2,0));arrow.lineTo(center+QPointF(-2,4));painter.setPen(QPen(text,1.5));painter.drawPath(arrow);}
    }
}
}
