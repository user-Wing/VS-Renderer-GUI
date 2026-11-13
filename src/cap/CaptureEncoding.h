#pragma once
#include <QImage>
#include <QSettings>
#include <QStringList>
#include <cstring>
namespace cap {
struct AvifOptions {
    QString format="444p12",color="auto";int quality=70,speed=6;
    static AvifOptions load(QSettings &s){AvifOptions o;o.format=s.value("avif/pixelFormat","444p12").toString();if(!QStringList{"444p12","444p10","420p10"}.contains(o.format))o.format="444p12";o.color=s.value("avif/color","auto").toString();if(!QStringList{"auto","srgb","pq2020"}.contains(o.color))o.color="auto";o.quality=qBound(1,s.value("avif/quality",70).toInt(),100);o.speed=qBound(0,s.value("avif/speed",6).toInt(),10);return o;}
    QStringList pixelArguments() const{return {"--chroma",format.startsWith("444")?"444":"420","--bit-depth",format.endsWith("12")?"12":"10"};}
};
inline QSize evenSize(QSize size){return QSize((size.width()+1)&~1,(size.height()+1)&~1);}
// Edge padding only. Never resample or quantize the high-precision editing canvas.
inline QImage evenImage(const QImage &source){
    if(source.isNull()||evenSize(source.size())==source.size())return source;
    QImage out(evenSize(source.size()),source.format());if(out.isNull())return {};
    out.setColorSpace(source.colorSpace());for(const auto &key:source.textKeys())out.setText(key,source.text(key));const int pixelBytes=source.depth()/8;
    for(int y=0;y<source.height();++y){auto *row=out.scanLine(y);std::memcpy(row,source.constScanLine(y),source.width()*pixelBytes);if(out.width()!=source.width())std::memcpy(row+source.width()*pixelBytes,row+(source.width()-1)*pixelBytes,pixelBytes);}
    if(out.height()!=source.height())std::memcpy(out.scanLine(out.height()-1),out.constScanLine(source.height()-1),out.bytesPerLine());return out;
}
}
