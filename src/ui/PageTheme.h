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
#include <QApplication>
#include <QStyle>
#include <QSlider>
#include <QTimer>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QAbstractScrollArea>
#include <QLabel>
#include <QColorDialog>
#include <QComboBox>
#include <QMessageBox>
#include "ui/ThemePackage.h"
namespace vsr {
class PageTheme final : public QWidget {
    QWidget *target_;QImage image_;QColor color_;int transparency_=20,brightness_=80;QString path_,key_,originalStyle_;bool scheduled_=false;
    void styleContainers(){
        auto widgets=target_->findChildren<QWidget*>();widgets.prepend(target_);
        for(auto *widget:widgets){if(widget==this || widget->isWindow() && widget!=target_)continue;bool preview=false;
            for(auto *p=widget;p && p!=target_;p=p->parentWidget())if(p->objectName()=="previewPane")preview=true;
            if(preview)continue;
            const bool container=!qobject_cast<QAbstractButton*>(widget) && !qobject_cast<QComboBox*>(widget) && !qobject_cast<QLineEdit*>(widget) && !qobject_cast<QAbstractSpinBox*>(widget) && !qobject_cast<QSlider*>(widget);
            if(container && !widget->property("pageThemeContainer").toBool()){widget->setProperty("pageThemeContainer",true);widget->style()->unpolish(widget);widget->style()->polish(widget);widget->setAutoFillBackground(false);}
        }
        lower();
    }
public:
    explicit PageTheme(QWidget *target):QWidget(target),target_(target),originalStyle_(target->styleSheet()){
        setObjectName("pageTheme");setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);target->installEventFilter(this);setGeometry(target->rect());lower();
    }
    void reload(const QString &path){if(QFileInfo(path).absoluteFilePath()==QFileInfo(path_).absoluteFilePath())load(path_,key_);}
    void load(const QString &path,const QString &key){
        path_=path;key_=key;QSettings settings(path,QSettings::IniFormat);settings.beginGroup(key);color_=QColor(settings.value("color").toString());
        const auto name=settings.value("image").toString();image_={};if(!name.isEmpty()){QImageReader reader(QDir(QFileInfo(path).absolutePath()).absoluteFilePath(name));const auto size=reader.size();if(size.width()>4096 || size.height()>4096)reader.setScaledSize(size.scaled(4096,4096,Qt::KeepAspectRatio));image_=reader.read();}
        transparency_=std::clamp(settings.value("transparency",20).toInt(),0,100);brightness_=std::clamp(settings.value("brightness",80).toInt(),0,180);
        const bool active=!image_.isNull() || color_.isValid();
        const auto base=originalStyle_.isEmpty() || originalStyle_.contains('{')?originalStyle_:QString("%1{%2}").arg(target_->objectName().isEmpty()?QString("QWidget"):QString("QWidget#%1").arg(target_->objectName()),originalStyle_);
        target_->setStyleSheet(base+(active?QStringLiteral("\nQWidget[pageThemeContainer=\"true\"]{background:transparent;color:#e8eaed;} QAbstractScrollArea[pageThemeContainer=\"true\"]{background:rgba(24,26,32,100);} QAbstractScrollArea[pageThemeContainer=\"true\"] QWidget{background:transparent;}"):QString()));
        setVisible(active);if(active)styleContainers();update();target_->update();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==target_){if(event->type()==QEvent::Resize)setGeometry(target_->rect());
            if((event->type()==QEvent::ChildAdded || event->type()==QEvent::Show) && !scheduled_){scheduled_=true;QTimer::singleShot(0,this,[this]{scheduled_=false;if(isVisible())styleContainers();});}}
        return QWidget::eventFilter(object,event);
    }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);painter.fillRect(rect(),color_.isValid()?color_:QColor("#101010"));painter.setRenderHint(QPainter::SmoothPixmapTransform);
        if(!image_.isNull()){const auto scaled=image_.size().scaled(size(),Qt::KeepAspectRatioByExpanding);painter.setOpacity((100-transparency_)/100.);painter.drawImage(QRect((width()-scaled.width())/2,(height()-scaled.height())/2,scaled.width(),scaled.height()),image_);painter.setOpacity(1);painter.fillRect(rect(),QColor(8,9,12,std::clamp(qRound((180-brightness_)*210./160),0,210)));}
        if(auto *label=qobject_cast<QLabel*>(target_)){painter.setFont(label->font());painter.setPen(label->palette().color(QPalette::WindowText));painter.drawText(rect(),label->alignment()|(label->wordWrap()?Qt::TextWordWrap:0),label->text());}
    }
};
inline void applyPageTheme(QWidget *page,const QString &path,const QString &key){
    if(!page)return;PageTheme *theme=nullptr;for(auto *object:page->children())if(object->objectName()=="pageTheme")theme=static_cast<PageTheme*>(object);
    if(!theme)theme=new PageTheme(page);theme->load(path,key);
}
inline bool editPageThemes(QWidget *parent,const QString &path,const QStringList &pages){
    QDialog dialog(parent);dialog.setObjectName("pageThemeEditor");dialog.setWindowTitle(QObject::tr("多页面背景主题"));auto *form=new QFormLayout(&dialog);QSettings source(path,QSettings::IniFormat);
    auto *library=new QComboBox(&dialog);library->setObjectName("pageThemeLibrary");library->addItem(QObject::tr("当前主题"),path);form->addRow(QObject::tr("主题库"),library);
    auto *name=new QLineEdit(source.value("skin_name",QObject::tr("自定义主题")).toString(),&dialog);form->addRow(QObject::tr("主题名称"),name);
    const auto root=QFileInfo(path).absolutePath();
    QDirIterator packs(QDir(root).filePath("themes"),{"skin.ini"},QDir::Files,QDirIterator::Subdirectories);while(packs.hasNext()){const auto ini=packs.next();QSettings pack(ini,QSettings::IniFormat);library->addItem(pack.value("skin_name",QFileInfo(ini).dir().dirName()).toString(),ini);}
    QList<QLineEdit*> images;QList<QSpinBox*> transparency,brightness;QList<QPushButton*> colors;
    for(const auto &page:pages){
        auto *row=new QWidget(&dialog);auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);
        auto *image=new QLineEdit(row);image->setPlaceholderText(QObject::tr("未设置背景图"));layout->addWidget(image,1);images<<image;
        auto *browse=new QPushButton(QObject::tr("浏览"),row);layout->addWidget(browse);QObject::connect(browse,&QPushButton::clicked,&dialog,[&dialog,image]{const auto file=QFileDialog::getOpenFileName(&dialog,QObject::tr("选择背景图片"),{},"Images (*.png *.jpg *.jpeg *.webp *.avif *.bmp)");if(!file.isEmpty())image->setText(file);});
        auto *color=new QPushButton(row);colors<<color;layout->addWidget(color);QObject::connect(color,&QPushButton::clicked,&dialog,[&dialog,color]{const auto selected=QColorDialog::getColor(QColor(color->text()),&dialog);if(selected.isValid())color->setText(selected.name());});
        auto *alpha=new QSpinBox(row),*light=new QSpinBox(row);alpha->setRange(0,100);light->setRange(20,180);alpha->setSuffix(QObject::tr("% 透明"));light->setSuffix(QObject::tr("% 亮度"));layout->addWidget(alpha);layout->addWidget(light);transparency<<alpha;brightness<<light;form->addRow(page,row);
    }
    const auto load=[&](const QString &ini){QSettings pack(ini,QSettings::IniFormat);name->setText(pack.value("skin_name",QObject::tr("自定义主题")).toString());for(int i=0;i<pages.size();++i){const auto page=pages[i];const auto image=pack.value(page+"/image").toString();images[i]->setText(image.isEmpty()?QString():QDir(QFileInfo(ini).absolutePath()).absoluteFilePath(image));colors[i]->setText(pack.value(page+"/color","#101010").toString());transparency[i]->setValue(pack.value(page+"/transparency",20).toInt());brightness[i]->setValue(pack.value(page+"/brightness",80).toInt());}};
    const auto save=[&](QSettings &pack){pack.setValue("skin_name",name->text().trimmed());for(int i=0;i<pages.size();++i){pack.setValue(pages[i]+"/image",images[i]->text().trimmed());pack.setValue(pages[i]+"/color",colors[i]->text());pack.setValue(pages[i]+"/transparency",transparency[i]->value());pack.setValue(pages[i]+"/brightness",brightness[i]->value());}pack.sync();};
    load(path);QObject::connect(library,&QComboBox::currentIndexChanged,&dialog,[&]{load(library->currentData().toString());});
    auto *actions=new QDialogButtonBox(&dialog);auto *folder=actions->addButton(QObject::tr("导入文件夹"),QDialogButtonBox::ActionRole);auto *archive=actions->addButton(QObject::tr("导入 ZIP/7z"),QDialogButtonBox::ActionRole);auto *exportZip=actions->addButton(QObject::tr("导出 ZIP"),QDialogButtonBox::ActionRole);form->addRow(actions);
    const auto importTheme=[&](const QString &input){if(input.isEmpty())return;QString installed,error;if(!ThemePackage::importPack(input,path,installed,error)){QMessageBox::warning(&dialog,QObject::tr("导入主题失败"),error);return;}QSettings pack(installed,QSettings::IniFormat);library->addItem(pack.value("skin_name",QFileInfo(input).baseName()).toString(),installed);library->setCurrentIndex(library->count()-1);};
    QObject::connect(folder,&QPushButton::clicked,&dialog,[&]{importTheme(QFileDialog::getExistingDirectory(&dialog,QObject::tr("选择包含 skin.ini 的主题文件夹")));});
    QObject::connect(archive,&QPushButton::clicked,&dialog,[&]{importTheme(QFileDialog::getOpenFileName(&dialog,QObject::tr("导入主题包"),{},"Themes (*.zip *.7z)"));});
    QObject::connect(exportZip,&QPushButton::clicked,&dialog,[&]{auto output=QFileDialog::getSaveFileName(&dialog,QObject::tr("导出主题包"),name->text()+".zip","ZIP (*.zip)");if(output.isEmpty())return;if(!output.endsWith(".zip",Qt::CaseInsensitive))output+=".zip";QTemporaryDir temp;QSettings pack(temp.filePath("themes.ini"),QSettings::IniFormat);QSettings selected(library->currentData().toString(),QSettings::IniFormat);for(const auto &key:selected.allKeys()){auto value=selected.value(key);if(key.endsWith("/image") && !value.toString().isEmpty())value=QDir(QFileInfo(selected.fileName()).absolutePath()).absoluteFilePath(value.toString());pack.setValue(key,value);}save(pack);QString error;if(!ThemePackage::exportPack(pack.fileName(),output,error))QMessageBox::warning(&dialog,QObject::tr("导出主题失败"),error);});
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(980,dialog.sizeHint().height());if(dialog.exec()!=QDialog::Accepted)return false;save(source);for(auto *widget:QApplication::allWidgets())if(auto *theme=dynamic_cast<PageTheme*>(widget))theme->reload(path);return source.status()==QSettings::NoError;
}
}
