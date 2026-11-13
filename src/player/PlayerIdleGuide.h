#pragma once
#include <QWidget>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSaveFile>
#include <algorithm>

namespace vsr {
// Idle-only overlay. Targets live in the transport tool window, so use global coordinates.
class PlayerIdleGuide final : public QWidget {
public:
    PlayerIdleGuide(QWidget *parent,QWidget *controls,QWidget *timeline,const QString &path)
        :QWidget(parent),controls_(controls),timeline_(timeline),path_(path) {
        setObjectName("playerIdleGuide");setAttribute(Qt::WA_TransparentForMouseEvents);hide();
        setStyleSheet(QStringLiteral("background:transparent;"));
        QFile defaults(":/filters/player-guide.json");if(defaults.open(QIODevice::ReadOnly))config_=QJsonDocument::fromJson(defaults.readAll()).object();
        reload(true);
        watcher_=new QFileSystemWatcher(this);watcher_->addPath(QFileInfo(path_).absolutePath());
        const auto changed=[this]{reload(false);if(QFileInfo::exists(path_) && !watcher_->files().contains(path_))watcher_->addPath(path_);};
        connect(watcher_,&QFileSystemWatcher::fileChanged,this,changed);connect(watcher_,&QFileSystemWatcher::directoryChanged,this,changed);
        if(QFileInfo::exists(path_))watcher_->addPath(path_);
    }
    QString configPath() const {return path_;}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QColor(config_.value("color").toString("#a5a9af")));
        const auto center=config_.value("center").toObject();auto type=font();type.setPixelSize(std::clamp(center.value("fontSize").toInt(14),8,64));painter.setFont(type);
        const auto text=center.value("text").toString();const int centerWidth=qMax(1,width()-32);
        const auto centerBounds=painter.fontMetrics().boundingRect(QRect(0,0,centerWidth,height()),Qt::AlignCenter|Qt::TextWordWrap,text);
        painter.drawText(QRect(qRound(width()*center.value("x").toDouble(.5))-centerWidth/2,qRound(height()*center.value("y").toDouble(.34))-centerBounds.height()/2,centerWidth,centerBounds.height()),Qt::AlignCenter|Qt::TextWordWrap,text);
        type.setPixelSize(std::clamp(config_.value("fontSize").toInt(13),8,48));painter.setFont(type);
        const int bottom=qMin(height()-8,mapFromGlobal(timeline_->mapToGlobal(QPoint())).y()-config_.value("baselineGap").toInt(12));
        for(const auto &value:config_.value("guides").toArray()) {
            const auto guide=value.toObject();int sum=0,count=0;
            for(const auto &target:guide.value("targets").toArray())if(auto *widget=controls_->findChild<QWidget*>(target.toString());widget && widget->isVisible()){sum+=mapFromGlobal(widget->mapToGlobal(widget->rect().center())).x();++count;}
            if(!count)continue;
            const int x=sum/count+guide.value("xOffset").toInt(),length=std::clamp(guide.value("length").toInt(100),0,qMax(0,bottom-32));
            const int textWidth=std::clamp(guide.value("width").toInt(100),24,qMax(24,width()-16));
            const int left=std::clamp(x-textWidth/2+guide.value("textOffsetX").toInt(),8,qMax(8,width()-textWidth-8));
            const auto label=guide.value("text").toString();const auto bounds=painter.fontMetrics().boundingRect(QRect(0,0,textWidth,height()),Qt::AlignCenter|Qt::TextWordWrap,label);
            auto pen=painter.pen();pen.setStyle(Qt::DashLine);painter.setPen(pen);painter.drawLine(x,bottom-length,x,bottom);
            pen.setStyle(Qt::SolidLine);painter.setPen(pen);painter.drawText(QRect(left,bottom-length-bounds.height()-6,textWidth,bounds.height()),Qt::AlignCenter|Qt::TextWordWrap,label);
        }
    }
private:
    void reload(bool initialize) {
        QJsonObject root;QFile file(path_);
        if(file.exists()) {if(!file.open(QIODevice::ReadOnly))return;const auto bytes=file.readAll();file.close();const auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject())return;root=doc.object();}
        if(root.value("playerGuide").isObject())config_=root.value("playerGuide").toObject();
        else if(initialize) {root.insert("playerGuide",config_);QSaveFile output(path_);if(output.open(QIODevice::WriteOnly)){output.write(QJsonDocument(root).toJson(QJsonDocument::Indented));output.commit();}}
        update();
    }
    QWidget *controls_,*timeline_;
    QString path_;
    QJsonObject config_;
    QFileSystemWatcher *watcher_=nullptr;
};
}
