#pragma once
#include <QWidget>
#include <QPainter>
#include <algorithm>

namespace vsr {
// Layered tool windows allow alpha blending over the native video surface.
class PlayerChromePanel final : public QWidget {
public:
    explicit PlayerChromePanel(QWidget *parent):QWidget(parent,Qt::Tool|Qt::FramelessWindowHint){
        setAttribute(Qt::WA_TranslucentBackground);setAttribute(Qt::WA_ShowWithoutActivating);
    }
    void setTransparency(int value){alpha_=(100-std::clamp(value,0,100))*255/100;update();}
protected:
    void paintEvent(QPaintEvent *) override {QPainter painter(this);painter.fillRect(rect(),QColor(32,33,36,alpha_));}
private:
    int alpha_=128;
};
}
