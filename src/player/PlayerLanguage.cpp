#include "player/PlayerLanguage.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAbstractButton>
#include <QAction>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
namespace vsr {
PlayerLanguage::PlayerLanguage(const QString &language) {
    const QDir directory(QDir(QCoreApplication::applicationDirPath()).filePath("languages"));QDir().mkpath(directory.path());
    for(const auto &name:QStringList{"zh_CN","en_US"})if(!QFile::exists(directory.filePath(name+".json")))QFile::copy(":/languages/"+name+".json",directory.filePath(name+".json"));
    QFile defaults(":/languages/en_US.json");if(defaults.open(QIODevice::ReadOnly)){const auto entries=QJsonDocument::fromJson(defaults.readAll()).object();for(auto it=entries.begin();it!=entries.end();++it)english_.insert(it.key(),it.value().toString());}
    QFile file(directory.filePath("en_US.json"));if(file.open(QIODevice::ReadOnly)){const auto entries=QJsonDocument::fromJson(file.readAll()).object();for(auto it=entries.begin();it!=entries.end();++it)english_.insert(it.key(),it.value().toString());}
    englishEnabled_=language=="en_US";qApp->installTranslator(this);
}
PlayerLanguage::~PlayerLanguage(){qApp->removeTranslator(this);}
void PlayerLanguage::setLanguage(const QString &language){englishEnabled_=language=="en_US";}
QString PlayerLanguage::translate(const char *,const char *source,const char *,int) const {
    if(!englishEnabled_)return {};return english_.value(QString::fromUtf8(source));
}
void PlayerLanguage::updateWidgets(QWidget *root) const {
    const auto text=[this](const QString &value){if(englishEnabled_)return english_.value(value,value);for(auto it=english_.begin();it!=english_.end();++it)if(it.value()==value)return it.key();return value;};
    auto widgets=root->findChildren<QWidget *>();widgets.prepend(root);
    for(auto *widget:widgets) {
        widget->setToolTip(text(widget->toolTip()));widget->setWindowTitle(text(widget->windowTitle()));
        if(auto *label=qobject_cast<QLabel *>(widget))label->setText(text(label->text()));
        if(auto *button=qobject_cast<QAbstractButton *>(widget))button->setText(text(button->text()));
        if(auto *group=qobject_cast<QGroupBox *>(widget))group->setTitle(text(group->title()));
        if(auto *combo=qobject_cast<QComboBox *>(widget))for(int n=0;n<combo->count();++n)combo->setItemText(n,text(combo->itemText(n)));
        if(auto *list=qobject_cast<QListWidget *>(widget))for(int n=0;n<list->count();++n)list->item(n)->setText(text(list->item(n)->text()));
    }
    for(auto *action:root->findChildren<QAction *>()){action->setText(text(action->text()));action->setToolTip(text(action->toolTip()));}
}
}
