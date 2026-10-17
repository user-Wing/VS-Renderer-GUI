#pragma once
#include <QColorSpace>
#include <QImage>
#include <algorithm>
#include <array>
#include <cmath>

namespace vsr {
// Shared by desktop SDR companions and the player's raster preview.
// Original HDR pixels are retained separately; scRGB 1.0 represents 80 nits.
inline QImage imageForSdrDisplay(const QImage &original, bool scRgb = false, double desktopWhiteNits = 0) {
    if(original.isNull())return {};
    const auto transfer=original.colorSpace().transferFunction();
    const bool pq=transfer==QColorSpace::TransferFunction::St2084;
    const bool hlg=transfer==QColorSpace::TransferFunction::Hlg;
    if(!scRgb && !pq && !hlg)return original;
    if(desktopWhiteNits<=0)desktopWhiteNits=original.text("capSdrWhiteNits").toDouble();
    const bool desktop=std::isfinite(desktopWhiteNits) && desktopWhiteNits>0;
    const double whiteNits=desktop?desktopWhiteNits:203.0;
    const auto source=original.convertToFormat(scRgb?QImage::Format_RGBA32FPx4:QImage::Format_RGBA64);
    QImage result(source.size(),QImage::Format_RGBA64);
    if(result.isNull())return {};
    static const auto pqNits=[]{
        std::array<double,65536> table{};
        for(size_t i=0;i<table.size();++i){
            const double p=std::pow(double(i)/65535.0,32.0/2523.0);
            table[i]=10000.0*std::pow(std::max(0.0,p-3424.0/4096.0)/(2413.0/128.0-2392.0/128.0*p),16384.0/2610.0);
        }
        return table;
    }();
    static const auto srgbCodes=[]{
        std::array<double,65537> table{};
        for(size_t i=0;i<table.size();++i){const double x=double(i)/65536.0;table[i]=(x<=.0031308?12.92*x:1.055*std::pow(x,1.0/2.4)-.055)*65535.0;}
        return table;
    }();
    const auto pack=[](double x){
        if(!std::isfinite(x) || x<=0)return quint16(0);
        if(x>=1)return quint16(65535);
        const double index=x*65536.0;const auto i=size_t(index);
        return quint16(std::lround(srgbCodes[i]+(index-i)*(srgbCodes[i+1]-srgbCodes[i])));
    };
    for(int y=0;y<source.height();++y){
        const auto *pixels=reinterpret_cast<const QRgba64*>(source.constScanLine(y));
        const auto *floats=reinterpret_cast<const float*>(source.constScanLine(y));
        auto *out=reinterpret_cast<QRgba64*>(result.scanLine(y));
        for(int x=0;x<source.width();++x){
            const auto pixel=scRgb?QRgba64::fromRgba64(0,0,0,65535):pixels[x];
            double rgb[]{pixel.red()/65535.0,pixel.green()/65535.0,pixel.blue()/65535.0};
            if(scRgb)for(int c=0;c<3;++c)rgb[c]=floats[x*4+c]*80.0/whiteNits;
            else if(pq){rgb[0]=pqNits[pixel.red()]/whiteNits;rgb[1]=pqNits[pixel.green()]/whiteNits;rgb[2]=pqNits[pixel.blue()]/whiteNits;}
            else {
                for(auto &v:rgb)v=v<=.5?v*v/3.0:(std::exp((v-.55991073)/.17883277)+.28466892)/12.0;
                const double l=.2627*rgb[0]+.6780*rgb[1]+.0593*rgb[2],gain=1000.0*std::pow(std::max(0.0,l),.2)/whiteNits;
                for(auto &v:rgb)v*=gain;
            }
            const double r=rgb[0],g=rgb[1],b=rgb[2];
            if(!scRgb && original.colorSpace().primaries()==QColorSpace::Primaries::Bt2020){
                rgb[0]=1.660491*r-.587641*g-.072850*b;rgb[1]=-.124550*r+1.132900*g-.008349*b;rgb[2]=-.018151*r-.100579*g+1.118730*b;
            }else if(!scRgb && original.colorSpace().primaries()==QColorSpace::Primaries::DciP3D65){
                rgb[0]=1.224940*r-.224940*g;rgb[1]=-.042057*r+1.042057*g;rgb[2]=-.019638*r-.078636*g+1.098274*b;
            }
            // Desktop SDR pixels already include the OS reference-white gain.
            // Undo that gain without applying a second, film-style tone curve.
            if(!desktop){
                const double lum=std::max(0.0,.2126*rgb[0]+.7152*rgb[1]+.0722*rgb[2]);
                const double mapped=lum*(2.51*lum+.03)/(lum*(2.43*lum+.59)+.14);
                if(lum>0)for(auto &v:rgb)v*=mapped/lum;
            }
            const auto alpha=scRgb?quint16(std::lround(std::clamp(double(floats[x*4+3]),0.0,1.0)*65535.0)):pixel.alpha();
            out[x]=QRgba64::fromRgba64(pack(rgb[0]),pack(rgb[1]),pack(rgb[2]),alpha);
        }
    }
    result.setColorSpace(QColorSpace::SRgb);
    return result;
}
}
