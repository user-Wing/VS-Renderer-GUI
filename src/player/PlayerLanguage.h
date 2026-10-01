#pragma once
#include <QTranslator>
#include <QHash>
class QWidget;
namespace vsr {
class PlayerLanguage final : public QTranslator {
public:
    explicit PlayerLanguage(const QString &language);
    ~PlayerLanguage() override;
    void setLanguage(const QString &language);
    QString translate(const char *,const char *source,const char *,int) const override;
    bool isEmpty() const override {return false;}
    void updateWidgets(QWidget *root) const;
private:
    QHash<QString,QString> english_;
    bool englishEnabled_=false;
};
}
