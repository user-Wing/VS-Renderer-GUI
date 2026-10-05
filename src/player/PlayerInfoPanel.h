#pragma once
#include <QLabel>
#include <QSlider>
#include <QSizeGrip>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScreen>
#include <QPainter>
#include <algorithm>

namespace vsr {
class PlayerInfoGrip final : public QSizeGrip {
public:
    using QSizeGrip::QSizeGrip;
protected:
    void paintEvent(QPaintEvent *) override {QPainter p(this);p.setPen(QColor(180,190,205));for(int d: {5,10,15})p.drawLine(width()-d,height()-2,width()-2,height()-d);}
    void mousePressEvent(QMouseEvent *event) override {anchor_=event->globalPosition();start_=parentWidget()->size();event->accept();}
    void mouseMoveEvent(QMouseEvent *event) override {if(event->buttons()&Qt::LeftButton){const auto delta=(event->globalPosition()-anchor_).toPoint();parentWidget()->resize(start_+QSize(delta.x(),delta.y()));}event->accept();}
    void mouseReleaseEvent(QMouseEvent *event) override {event->accept();}
private:
    QPointF anchor_;QSize start_;
};
class PlayerInfoPanel final : public QLabel {
public:
    explicit PlayerInfoPanel(QWidget *parent):QLabel(parent){
        setObjectName("playerInfoPanel");setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);setAttribute(Qt::WA_ShowWithoutActivating);setAttribute(Qt::WA_TranslucentBackground);setTextFormat(Qt::PlainText);
        for(auto *widget=parent;widget;widget=widget->parentWidget())widget->installEventFilter(this);
        setAlignment(Qt::AlignTop|Qt::AlignLeft);setMargin(12);setContentsMargins(0,0,0,30);
        baseFont_=QFont("Consolas",9);setFont(baseFont_);
        opacityText_=new QLabel(tr("透明度"),this);percent_=new QLabel(this);percent_->setObjectName("playerInfoOpacityPercent");
        opacity_=new QSlider(Qt::Horizontal,this);opacity_->setObjectName("playerInfoOpacity");opacity_->setRange(0,100);opacity_->setValue(50);opacity_->setToolTip(tr("信息面板背景透明度"));
        grip_=new PlayerInfoGrip(this);grip_->setObjectName("playerInfoResize");
        connect(opacity_,&QSlider::valueChanged,this,[this](int){layoutControls();});
        layoutControls();
    }
    void showText(const QString &text){
        QLabel::setText(text);QFontMetrics metrics(baseFont_);int width=0;
        const auto lines=text.split('\n');for(const auto &line:lines)width=std::max(width,metrics.horizontalAdvance(line));
        baseSize_={width+32,int(lines.size())*metrics.lineSpacing()+62};setMinimumSize(baseSize_);
        if(!initialized_){resize(baseSize_);initialized_=true;}else resize(size().expandedTo(baseSize_));
        layoutControls();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(parentWidget() && object==parentWidget()->window()){
            if(event->type()==QEvent::WindowDeactivate){restore_=isVisible();hide();}
            else if(event->type()==QEvent::WindowActivate && restore_){restore_=false;show();}
        }
        if(isVisible()&&(event->type()==QEvent::Move||event->type()==QEvent::Resize))positionPanel();return QLabel::eventFilter(object,event);
    }
    void paintEvent(QPaintEvent *event) override {QPainter painter(this);painter.fillRect(rect(),QColor(23,25,30,(100-opacity_->value())*255/100));painter.setPen(QColor(145,150,160));painter.drawRect(rect().adjusted(0,0,-1,-1));painter.end();QLabel::paintEvent(event);}
    void resizeEvent(QResizeEvent *event) override {QLabel::resizeEvent(event);layoutControls();}
private:
    void positionPanel(){
        if(!parentWidget())return;auto position=parentWidget()->mapToGlobal(QPoint(12,12));const auto area=parentWidget()->screen()->availableGeometry();
        position.setX(std::clamp(position.x(),area.left(),std::max(area.left(),area.right()-width()+1)));position.setY(std::clamp(position.y(),area.top(),std::max(area.top(),area.bottom()-height()+1)));move(position);
    }
    void layoutControls(){
        if(!opacity_ || !grip_)return;
        opacityText_->setGeometry(12,height()-28,55,22);percent_->setGeometry(width()-77,height()-28,48,22);percent_->setText(QString::number(opacity_->value())+"%");opacity_->setGeometry(70,height()-26,std::max(40,width()-151),18);grip_->setGeometry(width()-22,height()-22,20,20);
        auto font=baseFont_;if(!baseSize_.isEmpty())font.setPointSizeF(std::clamp(9.*std::min(double(width())/baseSize_.width(),double(height())/baseSize_.height()),9.,24.));setFont(font);
        setStyleSheet(QString("QLabel#playerInfoPanel{background:transparent;color:#f4f4f4;font-family:Consolas;font-size:%1pt;} QLabel,QSlider,QSizeGrip{background:transparent;color:#f4f4f4;} QSlider::groove:horizontal{background:#666;height:4px;} QSlider::handle:horizontal{background:#c8d4ea;width:12px;margin:-4px 0;}").arg(font.pointSizeF()));update();
        positionPanel();
    }
    QSlider *opacity_=nullptr;QLabel *opacityText_=nullptr,*percent_=nullptr;QSizeGrip *grip_=nullptr;QFont baseFont_;QSize baseSize_;bool initialized_=false,restore_=false;
};
}
