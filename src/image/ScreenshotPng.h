#pragma once
#include "color/ScreenshotColor.h"
#include <QBuffer>
#include <QImageWriter>
#include <QSaveFile>
#include <QtEndian>
#include <cstring>

namespace vsr::screenshot {
// Qt 6.10's PNG reader ignores cICP. Keep an equivalent ICC for compatibility;
// internal encoder staging uses cICP alone to avoid approximate PQ ICC parsers.
inline QString writePng(const QString &path,const QImage &pixels,const ColorPlan &plan,bool cicpOnly=false) {
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer,"png");writer.setCompression(70);
    if(!writer.write(pixels))return writer.errorString();
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return file.errorString();
    if(file.write(png.constData(),8)!=8)return file.errorString();
    for(qsizetype at=8;at+12<=png.size();) {
        const auto count=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(png.constData()+at));
        const auto type=png.mid(at+4,4);const auto size=qsizetype(count)+12;
        if(size>png.size()-at)return QStringLiteral("Invalid encoded PNG chunk");
        const bool standard=plan.cicpPrimaries && plan.cicpTransfer;
        if(!standard || (type!="cICP" && (!cicpOnly || (type!="iCCP" && type!="sRGB" && type!="gAMA" && type!="cHRM"))))
            if(file.write(png.constData()+at,size)!=size)return file.errorString();
        if(type=="IHDR" && standard) {
            QByteArray chunk(16,0);qToBigEndian<quint32>(4,reinterpret_cast<uchar*>(chunk.data()));
            std::memcpy(chunk.data()+4,"cICP",4);chunk[8]=char(plan.cicpPrimaries);chunk[9]=char(plan.cicpTransfer);chunk[10]=0;chunk[11]=1;
            quint32 crc=0xffffffffU;
            for(int i=4;i<12;++i){crc^=uchar(chunk[i]);for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320U:0);}
            qToBigEndian<quint32>(crc^0xffffffffU,reinterpret_cast<uchar*>(chunk.data()+12));
            if(file.write(chunk)!=chunk.size())return file.errorString();
        }
        at+=size;
    }
    return file.commit()?QString():file.errorString();
}
}
