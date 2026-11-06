#pragma once
// Page keys and per-image brightness follow ModelScope-Manager's skin_theme.py;
// Qt painting is independent because the reference uses Python/Qt bindings.
#include <QWidget>
#include <QFileInfo>
#include <algorithm>
#include <QPainter>
#include <QImageReader>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QFileDialog>
#include <QDir>
#include <QRegularExpression>
#include <QEvent>
namespace vsr {
class PageTheme final : public QObject {
    QWidget *target_;QImage image_;int transparency_=85,brightness_=20;
public:
    explicit PageTheme(QWidget *target):QObject(target),target_(target){setObjectName("pageTheme");target->installEventFilter(this);}
    void load(const QString &path,const QString &key){
        QSettings settings(path,QSettings::IniFormat);settings.beginGroup(key);
        const auto name=settings.value("image").toString();const auto file=QDir(QFileInfo(path).absolutePath()).absoluteFilePath(name);
        image_={};if(!name.isEmpty()){QImageReader reader(file);const auto size=reader.size();if(size.width()>4096 || size.height()>4096)reader.setScaledSize(size.scaled(4096,4096,Qt::KeepAspectRatio));image_=reader.read();}
        transparency_=std::clamp(settings.value("transparency",85).toInt(),0,100);brightness_=std::clamp(settings.value("brightness",20).toInt(),0,100);target_->update();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==target_ && event->type()==QEvent::Paint && !image_.isNull()){
            QPainter painter(target_);painter.fillRect(target_->rect(),QColor("#101010"));painter.setRenderHint(QPainter::SmoothPixmapTransform);
            const auto scaled=image_.size().scaled(target_->size(),Qt::KeepAspectRatioByExpanding);const QRect destination((target_->width()-scaled.width())/2,(target_->height()-scaled.height())/2,scaled.width(),scaled.height());
            painter.setOpacity((100-transparency_)/100.);painter.drawImage(destination,image_);painter.setOpacity(1);painter.fillRect(target_->rect(),QColor(0,0,0,(100-brightness_)*255/100));
        }
        return QObject::eventFilter(object,event);
    }
};
inline void applyPageTheme(QWidget *page,const QString &path,const QString &key){
    if(!page)return;PageTheme *theme=nullptr;for(auto *object:page->children())if(object->objectName()=="pageTheme")theme=static_cast<PageTheme*>(object);
    if(!theme)theme=new PageTheme(page);theme->load(path,key);
}
inline bool editPageThemes(QWidget *parent,const QString &path,const QStringList &pages){
    QDialog dialog(parent);dialog.setObjectName("pageThemeEditor");dialog.setWindowTitle(QObject::tr("多页面背景主题"));auto *form=new QFormLayout(&dialog);QSettings source(path,QSettings::IniFormat);
    QList<QLineEdit*> images;QList<QSpinBox*> transparency,brightness;
    for(const auto &page:pages){
        auto *row=new QWidget(&dialog);auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);
        auto *image=new QLineEdit(source.value(page+"/image").toString(),row);image->setPlaceholderText(QObject::tr("未设置背景图"));layout->addWidget(image,1);images<<image;
        auto *browse=new QPushButton(QObject::tr("浏览"),row);layout->addWidget(browse);QObject::connect(browse,&QPushButton::clicked,&dialog,[&dialog,image]{const auto file=QFileDialog::getOpenFileName(&dialog,QObject::tr("选择背景图片"),{},"Images (*.png *.jpg *.jpeg *.webp *.avif *.bmp)");if(!file.isEmpty())image->setText(file);});
        auto *alpha=new QSpinBox(row),*light=new QSpinBox(row);alpha->setRange(0,100);light->setRange(0,100);alpha->setValue(source.value(page+"/transparency",85).toInt());light->setValue(source.value(page+"/brightness",20).toInt());alpha->setSuffix(QObject::tr("% 透明"));light->setSuffix(QObject::tr("% 亮度"));layout->addWidget(alpha);layout->addWidget(light);transparency<<alpha;brightness<<light;form->addRow(page,row);
    }
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(850,dialog.sizeHint().height());if(dialog.exec()!=QDialog::Accepted)return false;
    for(int i=0;i<pages.size();++i){source.setValue(pages[i]+"/image",images[i]->text().trimmed());source.setValue(pages[i]+"/transparency",transparency[i]->value());source.setValue(pages[i]+"/brightness",brightness[i]->value());}source.sync();return source.status()==QSettings::NoError;
}
}
