#include "player/PlayerImageTools.h"
#include <QColorSpace>
#include <QThread>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

namespace vsr {
QImage PlayerImageTools::resample(const QImage &input,const QSize &size,const QString &algorithm) {
    if(input.size()==size)return input;
    if(algorithm=="nearest")return input.scaled(size,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    if(algorithm=="bilinear")return input.scaled(size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    const bool wide=input.depth()>32;
    const auto image=wide?input.convertToFormat(QImage::Format_RGBA64_Premultiplied):input.format()==QImage::Format_RGB32?input:input.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage output(size,wide?QImage::Format_RGBA64_Premultiplied:QImage::Format_ARGB32_Premultiplied);if(output.isNull())return {};
    output.setColorSpace(input.colorSpace());
    for(const auto &key:input.textKeys())output.setText(key,input.text(key));
    const bool jinc=algorithm=="jinc";const double support=jinc?2:algorithm=="lanczos3"?3:4;
    constexpr int tableSize=16384;std::array<double,tableSize+1> table;
    const auto sinc=[](double x){return std::abs(x)<1e-9?1:std::sin(3.141592653589793*x)/(3.141592653589793*x);};
    const auto bessel=[](double x){return std::abs(x)<1e-9?1:2*std::cyl_bessel_j(1,3.141592653589793*x)/(3.141592653589793*x);};
    for(int i=0;i<=tableSize;++i){const double x=support*i/tableSize;table[i]=i==tableSize?0:jinc?bessel(x)*bessel(x/support):sinc(x)*sinc(x/support);}
    const auto weight=[&](double distance){const double x=std::abs(distance)*tableSize/support;if(x>=tableSize)return 0.0;const int index=int(x);return table[index]+(table[index+1]-table[index])*(x-index);};
    const double rx=double(image.width())/size.width(),ry=double(image.height())/size.height(),sx=std::max(1.0,rx),sy=std::max(1.0,ry);
    auto *target=output.bits();const auto stride=output.bytesPerLine();std::atomic<int> next{0};auto *caller=QThread::currentThread();
    const auto work=[&]{for(int y;(y=next.fetch_add(1))<size.height();){if(caller->isInterruptionRequested())return;const double cy=(y+.5)*ry-.5;const int firstY=int(std::ceil(cy-support*sy)),lastY=int(std::floor(cy+support*sy));
        for(int x=0;x<size.width();++x){const double cx=(x+.5)*rx-.5;std::array<double,4> total{};double sum=0;
            for(int py=firstY;py<=lastY;++py){const double dy=(py-cy)/sy;const double wy=jinc?1:weight(dy);if(wy==0)continue;const auto *row=image.constScanLine(std::clamp(py,0,image.height()-1));
                for(int px=int(std::ceil(cx-support*sx));px<=int(std::floor(cx+support*sx));++px){const double dx=(px-cx)/sx,w=jinc?weight(std::sqrt(dx*dx+dy*dy)):weight(dx)*wy;if(w==0)continue;const int column=std::clamp(px,0,image.width()-1);sum+=w;
                    if(wide){const auto c=reinterpret_cast<const QRgba64 *>(row)[column];total[0]+=w*c.red();total[1]+=w*c.green();total[2]+=w*c.blue();total[3]+=w*c.alpha();}
                    else{const auto c=reinterpret_cast<const QRgb *>(row)[column];total[0]+=w*qRed(c);total[1]+=w*qGreen(c);total[2]+=w*qBlue(c);total[3]+=w*qAlpha(c);}
                }
            }
            const int maximum=wide?65535:255;const int alpha=std::clamp(qRound(total[3]/sum),0,maximum);const auto component=[&](int n){return std::clamp(qRound(total[n]/sum),0,alpha);};
            if(wide)reinterpret_cast<QRgba64 *>(target+y*stride)[x]=QRgba64::fromRgba64(component(0),component(1),component(2),alpha);
            else reinterpret_cast<QRgb *>(target+y*stride)[x]=qRgba(component(0),component(1),component(2),alpha);
        }
    }};
    std::vector<std::thread> workers;for(int i=1;i<std::clamp(QThread::idealThreadCount(),1,4);++i)workers.emplace_back(work);work();for(auto &worker:workers)worker.join();
    return caller->isInterruptionRequested()?QImage():output;
}
}
