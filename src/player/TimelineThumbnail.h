#pragma once
#include <QLabel>
#include <QSlider>
#include <QMouseEvent>
#include <QTimer>
#include <QProcess>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QCache>
#include <QScreen>
#include <QApplication>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QCryptographicHash>
#include <functional>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace vsr {
// Separate, cancellable single-frame decoder; never seeks the playback session.
class TimelineThumbnail final : public QObject {
    QSlider *slider_;QLabel *label_;QTimer debounce_,deadline_;QProcess process_;QCache<QString,QPixmap> cache_{32};
    std::function<QString()> source_;std::function<qint64()> duration_;std::function<bool()> enabled_;
    QString wanted_,running_,path_;qint64 second_=0;QPoint global_;QByteArray bytes_;
    void display(const QPixmap &p){label_->setPixmap(p);label_->adjustSize();auto point=global_-QPoint(label_->width()/2,label_->height()+20);if(auto *screen=QGuiApplication::screenAt(global_)){const auto area=screen->availableGeometry();point.setX(qBound(area.left(),point.x(),qMax(area.left(),area.right()-label_->width())));point.setY(qMax(area.top(),point.y()));}label_->move(point);label_->show();}
    void stop(){wanted_.clear();debounce_.stop();deadline_.stop();label_->hide();if(process_.state()!=QProcess::NotRunning)process_.kill();}
    void launch(){if(wanted_.isEmpty()||path_!=source_()||!enabled_()){stop();return;}if(process_.state()!=QProcess::NotRunning){process_.kill();debounce_.start(100);return;}running_=wanted_;bytes_.clear();const auto override=qEnvironmentVariable("VSR_THUMBNAIL_FFMPEG");const auto ff=override.isEmpty()?QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"):override;if(!QFileInfo::exists(ff)){label_->setText(QStringLiteral("缩略图需要随包 ffmpeg.exe"));return;}
        process_.start(ff,{"-hide_banner","-loglevel","error","-threads","1","-ss",QString::number(second_),"-i",path_,"-an","-sn","-dn","-frames:v","1","-vf","scale=240:135:force_original_aspect_ratio=decrease","-threads","1","-f","image2pipe","-vcodec","png","pipe:1"});deadline_.start(10000);
    }
protected:
    bool eventFilter(QObject *o,QEvent *e) override {
        if(o!=slider_)return false;
        if(e->type()==QEvent::Leave||e->type()==QEvent::Hide||e->type()==QEvent::MouseButtonPress){stop();return false;}
        if(e->type()!=QEvent::MouseMove)return false;
        if(!enabled_()||slider_->isSliderDown()||duration_()<=0){stop();return false;}
        const auto source=source_();QFileInfo file(source);if(!file.isFile()||file.suffix().compare("mpls",Qt::CaseInsensitive)==0){stop();return false;}
        const auto *mouse=static_cast<QMouseEvent*>(e);QStyleOptionSlider opt;opt.initFrom(slider_);opt.orientation=Qt::Horizontal;opt.minimum=slider_->minimum();opt.maximum=slider_->maximum();opt.sliderPosition=slider_->sliderPosition();opt.sliderValue=slider_->value();
        const auto groove=slider_->style()->subControlRect(QStyle::CC_Slider,&opt,QStyle::SC_SliderGroove,slider_),handle=slider_->style()->subControlRect(QStyle::CC_Slider,&opt,QStyle::SC_SliderHandle,slider_);
        const auto value=QStyle::sliderValueFromPosition(opt.minimum,opt.maximum,qRound(mouse->position().x())-groove.left()-handle.width()/2,qMax(1,groove.width()-handle.width()));
        second_=qBound<qint64>(0,qint64(double(value-opt.minimum)/qMax(1,opt.maximum-opt.minimum)*duration_()/1e7),qMax<qint64>(0,duration_()/10000000-1));global_=mouse->globalPosition().toPoint();path_=source;
        const auto key=source+"|"+QString::number(file.size())+"|"+QString::number(file.lastModified().toMSecsSinceEpoch())+"|"+QString::number(second_);
        if(key==wanted_){if(const auto *p=cache_.object(key))display(*p);return false;}wanted_=key;label_->hide();if(const auto *p=cache_.object(key)){display(*p);return false;}debounce_.start(220);return false;
    }
public:
    TimelineThumbnail(QSlider *slider,std::function<QString()> source,std::function<qint64()> duration,std::function<bool()> enabled,QObject *parent):QObject(parent),slider_(slider),source_(std::move(source)),duration_(std::move(duration)),enabled_(std::move(enabled)){
        label_=new QLabel(nullptr,Qt::ToolTip|Qt::FramelessWindowHint);label_->setAttribute(Qt::WA_ShowWithoutActivating);label_->setStyleSheet("QLabel{background:#15171a;color:white;border:1px solid #66717e;padding:5px;}");label_->setObjectName("playerTimelineThumbnail");slider_->installEventFilter(this);
        debounce_.setSingleShot(true);deadline_.setSingleShot(true);connect(&debounce_,&QTimer::timeout,this,[this]{launch();});connect(&deadline_,&QTimer::timeout,this,[this]{process_.kill();});
        connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{bytes_+=process_.readAllStandardOutput();if(bytes_.size()>4*1024*1024)process_.kill();});
        connect(&process_,&QProcess::started,this,[this]{
#ifdef Q_OS_WIN
            HANDLE child=OpenProcess(PROCESS_SET_INFORMATION,FALSE,DWORD(process_.processId()));if(child){SetPriorityClass(child,BELOW_NORMAL_PRIORITY_CLASS);CloseHandle(child);}
#endif
        });
        connect(&process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){deadline_.stop();bytes_+=process_.readAllStandardOutput();if(code==0&&status==QProcess::NormalExit){QPixmap p;if(p.loadFromData(bytes_,"PNG")){cache_.insert(running_,new QPixmap(p));if(running_==wanted_&&path_==source_()&&enabled_()&&slider_->underMouse())display(p);}}});
    }
    ~TimelineThumbnail() override {process_.disconnect(this);if(process_.state()!=QProcess::NotRunning){process_.kill();process_.waitForFinished(1000);}delete label_;}
};
}
