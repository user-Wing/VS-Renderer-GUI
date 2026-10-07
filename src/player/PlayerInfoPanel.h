#pragma once
#include <QLabel>
#include <QSlider>
#include <QSizeGrip>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScreen>
#include <QPainter>
#include <QScrollArea>
#include <QComboBox>
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
class PlayerInfoPanel : public QLabel {
public:
    explicit PlayerInfoPanel(QWidget *parent):QLabel(parent){
        setObjectName("playerInfoPanel");setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);setAttribute(Qt::WA_ShowWithoutActivating);setAttribute(Qt::WA_TranslucentBackground);setTextFormat(Qt::PlainText);
        for(auto *widget=parent;widget;widget=widget->parentWidget())widget->installEventFilter(this);
        setAlignment(Qt::AlignTop|Qt::AlignLeft);setMargin(12);setContentsMargins(0,0,0,30);
        baseFont_=QFont("Comic Sans MS",9);setFont(baseFont_);
        scroll_=new QScrollArea(this);scroll_->setObjectName("playerInfoTextArea");scroll_->setFrameShape(QFrame::NoFrame);scroll_->setWidgetResizable(false);
        text_=new QLabel; text_->setTextFormat(Qt::PlainText);text_->setAlignment(Qt::AlignTop|Qt::AlignLeft);text_->setMargin(0);scroll_->setWidget(text_);
        audioMode_=new QComboBox(this);audioMode_->setObjectName("playerInfoAudioMode");audioMode_->addItems({tr("音频简略信息"),tr("音频详细信息")});
        opacityText_=new QLabel(tr("透明度"),this);percent_=new QLabel(this);percent_->setObjectName("playerInfoOpacityPercent");
        opacity_=new QSlider(Qt::Horizontal,this);opacity_->setObjectName("playerInfoOpacity");opacity_->setRange(0,100);opacity_->setValue(50);opacity_->setToolTip(tr("信息面板背景透明度"));
        grip_=new PlayerInfoGrip(this);grip_->setObjectName("playerInfoResize");
        connect(opacity_,&QSlider::valueChanged,this,[this](int){layoutControls();});
        layoutControls();
    }
    QComboBox *audioMode() const {return audioMode_;}
    void showAudioPixmap(const QPixmap &pixmap){text_->setPixmap(pixmap);text_->resize(pixmap.size());const auto content=pixmap.size()+QSize(48,70);baseSize_=(content+QSize(0,qRound(content.height()*.05))).boundedTo(maximumSize());setMinimumSize(baseSize_);resize(size().expandedTo(baseSize_));layoutControls();}
    void setWidthLimit(int width){widthLimit_=width;}
    void setOverlayInsets(int top,int bottom){const bool changed=topInset_!=top || bottomInset_!=bottom;topInset_=top;bottomInset_=bottom;const auto available=availableSize();if(!changed && maximumSize()==available)return;setMaximumSize(available);setMinimumSize(baseSize_.boundedTo(available));resize(size().boundedTo(available));layoutControls();}
    void hidePanel(){restore_=false;hide();}
    void setRightSide(bool right){right_=right;audioMode_->hide();positionPanel();}
    void showText(const QString &text){
        QLabel::setText(text);text_->setText(text);setFont(baseFont_);QFontMetrics metrics(baseFont_,this);int width=0;
        const auto lines=text.split('\n');for(const auto &line:lines)width=std::max(width,metrics.horizontalAdvance(line));
        const auto available=availableSize();const int contentHeight=int(lines.size())*metrics.lineSpacing()+110;
        baseSize_=QSize(width+48,contentHeight+qRound(contentHeight*.05)).boundedTo(available);setMaximumSize(available);setMinimumSize(baseSize_);
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
    void paintEvent(QPaintEvent *) override {QPainter painter(this);painter.fillRect(rect(),QColor(23,25,30,(100-opacity_->value())*255/100));painter.setPen(QColor(145,150,160));painter.drawRect(rect().adjusted(0,0,-1,-1));}
    void resizeEvent(QResizeEvent *event) override {QLabel::resizeEvent(event);layoutControls();}
private:
    QSize availableSize() const {auto size=(parentWidget()->size()-QSize(24,24+topInset_+bottomInset_)).boundedTo(parentWidget()->screen()->availableGeometry().size()-QSize(24,24));if(widthLimit_>0)size.setWidth(std::min(size.width(),widthLimit_));return size.expandedTo(QSize(64,96));}
    void positionPanel(){
        if(!parentWidget())return;auto position=parentWidget()->mapToGlobal(QPoint(right_?parentWidget()->width()-width()-12:12,12+topInset_));const auto area=parentWidget()->screen()->availableGeometry();
        position.setX(std::clamp(position.x(),area.left(),std::max(area.left(),area.right()-width()+1)));position.setY(std::clamp(position.y(),area.top(),std::max(area.top(),area.bottom()-height()+1)));move(position);
    }
    void layoutControls(){
        if(!opacity_ || !grip_)return;
        auto font=baseFont_;if(!baseSize_.isEmpty())font.setPointSizeF(std::clamp(9.*std::min(double(width())/baseSize_.width(),double(height())/baseSize_.height()),9.,24.));setFont(font);
        opacityText_->setFont(font);percent_->setFont(font);const QFontMetrics metrics(font);const int footer=std::max(metrics.height()+20,opacity_->sizeHint().height()+12),gap=std::max(12,qRound(height()*.05));setContentsMargins(0,0,0,footer+gap);
        const int header=std::max(32,audioMode_->sizeHint().height());audioMode_->setGeometry(12,8,220,header);const int textTop=right_?12:header+16;scroll_->setGeometry(12,textTop,width()-24,std::max(1,height()-footer-gap-textTop));text_->setFont(font);text_->adjustSize();
        const int labelWidth=metrics.horizontalAdvance(tr("透明度"))+8,percentWidth=metrics.horizontalAdvance("100%")+8;
        opacityText_->setGeometry(12,height()-footer,labelWidth,metrics.height()+8);percent_->setGeometry(width()-percentWidth-28,height()-footer,percentWidth,metrics.height()+8);percent_->setText(QString::number(opacity_->value())+"%");opacity_->setGeometry(labelWidth+20,height()-footer+4,std::max(40,width()-labelWidth-percentWidth-58),metrics.height());grip_->setGeometry(width()-22,height()-22,20,20);
        setStyleSheet(QString("QLabel#playerInfoPanel{font-size:%1pt;background:transparent;color:#f4f4f4;} QLabel,QSlider,QSizeGrip,QScrollArea,QScrollArea QWidget{font-size:%1pt;background:transparent;color:#f4f4f4;} QComboBox{background:#303238;color:#f4f4f4;} QSlider::groove:horizontal{background:#666;height:4px;} QSlider::handle:horizontal{background:#c8d4ea;width:12px;margin:-4px 0;}").arg(font.pointSizeF()));update();
        positionPanel();
    }
    QSlider *opacity_=nullptr;QLabel *opacityText_=nullptr,*percent_=nullptr;QSizeGrip *grip_=nullptr;QFont baseFont_;QSize baseSize_{0,0};bool initialized_=false,restore_=false;
    QScrollArea *scroll_=nullptr;QLabel *text_=nullptr;QComboBox *audioMode_=nullptr;bool right_=false;
    int widthLimit_=0,topInset_=0,bottomInset_=0;
};
}
