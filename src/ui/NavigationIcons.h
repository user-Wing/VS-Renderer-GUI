#pragma once
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

namespace vsr {
enum class NavigationGlyph { Render, Compare, Disc, Agent, Settings };
inline QIcon navigationIcon(NavigationGlyph glyph){
    QPixmap image(48,48);image.setDevicePixelRatio(2);image.fill(Qt::transparent);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor("#8ab4f8"),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    switch(glyph){
    case NavigationGlyph::Render:{
        p.drawRoundedRect(QRectF(3,4,18,13),2,2);p.drawLine(QPointF(12,17),QPointF(12,21));p.drawLine(QPointF(8,21),QPointF(16,21));
        QPainterPath play;play.moveTo(10,8);play.lineTo(15,10.5);play.lineTo(10,13);play.closeSubpath();p.drawPath(play);break;
    }
    case NavigationGlyph::Compare:
        p.drawRoundedRect(QRectF(3,4,18,16),2,2);p.drawLine(QPointF(12,4),QPointF(12,20));p.drawLine(QPointF(5,16),QPointF(9,10));p.drawLine(QPointF(15,16),QPointF(19,10));break;
    case NavigationGlyph::Disc:
        p.drawEllipse(QPointF(12,12),9,9);p.drawEllipse(QPointF(12,12),2.2,2.2);p.drawArc(QRectF(5.5,5.5,13,13),20*16,65*16);p.drawArc(QRectF(5.5,5.5,13,13),200*16,65*16);break;
    case NavigationGlyph::Agent:
        p.drawRoundedRect(QRectF(5,7,14,13),3,3);p.drawLine(QPointF(12,3),QPointF(12,7));p.drawLine(QPointF(2,11),QPointF(2,16));p.drawLine(QPointF(22,11),QPointF(22,16));
        p.drawEllipse(QPointF(9,12),.8,.8);p.drawEllipse(QPointF(15,12),.8,.8);p.drawLine(QPointF(9,16),QPointF(15,16));break;
    case NavigationGlyph::Settings:
        p.drawEllipse(QPointF(12,12),6,6);p.drawEllipse(QPointF(12,12),2.3,2.3);
        for(int i=0;i<8;++i){const double angle=i*3.141592653589793/4;p.drawLine(QPointF(12+6*std::cos(angle),12+6*std::sin(angle)),QPointF(12+9*std::cos(angle),12+9*std::sin(angle)));}break;
    }
    return QIcon(image);
}
}
