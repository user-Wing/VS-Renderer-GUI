#include "image/ImageAdjustments.h"
#include <QColorTransform>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace vsr {
namespace {
using Pixel = std::array<float,4>;
Pixel pixel(const QImage &image, int x, int y) {
    const auto *row=image.constScanLine(y);
    if(image.format()==QImage::Format_RGBA32FPx4){Pixel p;std::memcpy(p.data(),row+x*16,16);return p;}
    if(image.format()==QImage::Format_RGBA64){const auto *p=reinterpret_cast<const quint16 *>(row)+x*4;return {p[0]/65535.f,p[1]/65535.f,p[2]/65535.f,p[3]/65535.f};}
    const auto *p=row+x*4;return {p[0]/255.f,p[1]/255.f,p[2]/255.f,p[3]/255.f};
}
void put(QImage &image,int x,int y,const Pixel &p) {
    auto *row=image.scanLine(y);
    if(image.format()==QImage::Format_RGBA32FPx4){std::memcpy(row+x*16,p.data(),16);return;}
    if(image.format()==QImage::Format_RGBA64){auto *q=reinterpret_cast<quint16 *>(row)+x*4;for(int c=0;c<4;++c)q[c]=quint16(std::lround(std::clamp(p[c],0.f,1.f)*65535));}
    else {auto *q=row+x*4;for(int c=0;c<4;++c)q[c]=uchar(std::lround(std::clamp(p[c],0.f,1.f)*255));}
}
double curve(double value,const QVector<QPointF> &points) {
    if(points.isEmpty())return value;
    int i=1;while(i<points.size()-1 && value>points[i].x())++i;
    const auto a=points[i-1],b=points[i];
    return a.y()+(value-a.x())*(b.y()-a.y())/(b.x()-a.x());
}
bool spatial(ImageAdjustmentKind kind){return kind==ImageAdjustmentKind::GaussianBlur || kind==ImageAdjustmentKind::UnsharpMask || kind==ImageAdjustmentKind::Median || kind==ImageAdjustmentKind::EdgeDetect;}
QImage filter(const QImage &source,const ImageAdjustmentSpec &s) {
    QImage result=source;result.detach();
    const int radius=s.kind==ImageAdjustmentKind::EdgeDetect?1:s.radius;
    if(s.kind==ImageAdjustmentKind::GaussianBlur || s.kind==ImageAdjustmentKind::UnsharpMask){
        QVector<double> kernel(2*radius+1);const double sigma=std::max(.5,radius/3.),denominator=2*sigma*sigma;double sum=0;for(int i=-radius;i<=radius;++i)sum+=(kernel[i+radius]=std::exp(-i*i/denominator));for(auto &w:kernel)w/=sum;
        QImage horizontal(source.size(),QImage::Format_RGBA32FPx4);if(horizontal.isNull())return {};
        for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x){Pixel accumulator{};for(int dx=-radius;dx<=radius;++dx){const auto p=pixel(source,std::clamp(x+dx,0,source.width()-1),y);const float weight=kernel[dx+radius]*std::clamp(p[3],0.f,1.f);for(int c=0;c<3;++c)accumulator[c]+=p[c]*weight;accumulator[3]+=weight;}put(horizontal,x,y,accumulator);}
        for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x){auto original=pixel(source,x,y);Pixel accumulator{};for(int dy=-radius;dy<=radius;++dy){const auto p=pixel(horizontal,x,std::clamp(y+dy,0,source.height()-1));for(int c=0;c<4;++c)accumulator[c]+=p[c]*kernel[dy+radius];}if(accumulator[3]>0)for(int c=0;c<3;++c){const float blurred=accumulator[c]/accumulator[3];original[c]=s.kind==ImageAdjustmentKind::UnsharpMask?original[c]+(original[c]-blurred)*s.amount:blurred;}put(result,x,y,original);}
    }else if(s.kind==ImageAdjustmentKind::Median){
        std::array<std::vector<float>,3> samples;for(auto &channel:samples)channel.reserve((2*radius+1)*(2*radius+1));
        for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x){for(auto &channel:samples)channel.clear();for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){const auto p=pixel(source,std::clamp(x+dx,0,source.width()-1),std::clamp(y+dy,0,source.height()-1));if(p[3]>0)for(int c=0;c<3;++c)samples[c].push_back(p[c]);}auto p=pixel(source,x,y);for(int c=0;c<3;++c)if(!samples[c].empty()){auto &values=samples[c];const auto middle=values.begin()+values.size()/2;std::nth_element(values.begin(),middle,values.end());p[c]=*middle;}put(result,x,y,p);}
    }else {
        constexpr int kx[3][3]={{-1,0,1},{-2,0,2},{-1,0,1}},ky[3][3]={{-1,-2,-1},{0,0,0},{1,2,1}};
        for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x){double gx=0,gy=0;for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const auto p=pixel(source,std::clamp(x+dx,0,source.width()-1),std::clamp(y+dy,0,source.height()-1));const double value=(.2126*p[0]+.7152*p[1]+.0722*p[2])*p[3];gx+=value*kx[dy+1][dx+1];gy+=value*ky[dy+1][dx+1];}auto p=pixel(source,x,y);p[0]=p[1]=p[2]=std::hypot(gx,gy)/4;put(result,x,y,p);}
    }
    return result;
}
Pixel adjusted(Pixel p,const ImageAdjustmentSpec &s) {
    const Pixel original=p;
    const double luma=.2126*p[0]+.7152*p[1]+.0722*p[2];
    switch(s.kind){
    case ImageAdjustmentKind::Exposure:for(int c=0;c<3;++c)p[c]*=std::exp2(s.exposure);break;
    case ImageAdjustmentKind::BrightnessContrast:for(int c=0;c<3;++c)p[c]=(p[c]-.5)*std::exp2(s.contrast*2)+.5+s.brightness;break;
    case ImageAdjustmentKind::Levels:for(int c=0;c<3;++c){const double t=(p[c]-s.inputBlack)/(s.inputWhite-s.inputBlack);p[c]=s.outputBlack+std::copysign(std::pow(std::abs(t),1/s.gamma),t)*(s.outputWhite-s.outputBlack);}break;
    case ImageAdjustmentKind::Curves:for(int c=0;c<3;++c)p[c]=curve(curve(p[c],s.curves[0]),s.curves[c+1]);break;
    case ImageAdjustmentKind::HueSaturation:{
        // Rotate the chroma around the neutral axis without an SDR-only HSV clamp.
        const double y=.299*p[0]+.587*p[1]+.114*p[2],i=.596*p[0]-.274*p[1]-.322*p[2],q=.211*p[0]-.523*p[1]+.312*p[2];
        const double angle=s.hue*3.14159265358979323846/180;
        const double ri=(i*std::cos(angle)-q*std::sin(angle))*s.saturation,rq=(i*std::sin(angle)+q*std::cos(angle))*s.saturation;
        p[0]=y+.956*ri+.621*rq+s.lightness;p[1]=y-.272*ri-.647*rq+s.lightness;p[2]=y-1.106*ri+1.703*rq+s.lightness;
        if(s.hue==0){for(int c=0;c<3;++c)p[c]=y+(original[c]-y)*s.saturation+s.lightness;}
        break;
    }
    case ImageAdjustmentKind::ColorBalance:p[0]*=s.redGain;p[1]*=s.greenGain;p[2]*=s.blueGain;break;
    case ImageAdjustmentKind::ChannelMixer:for(int c=0;c<3;++c)p[c]=original[0]*s.channelMixer[c][0]+original[1]*s.channelMixer[c][1]+original[2]*s.channelMixer[c][2];break;
    case ImageAdjustmentKind::Invert:for(int c=0;c<3;++c)p[c]=1-p[c];break;
    case ImageAdjustmentKind::Grayscale:for(int c=0;c<3;++c)p[c]=luma;break;
    case ImageAdjustmentKind::Threshold:for(int c=0;c<3;++c)p[c]=luma>=s.threshold?1:0;break;
    case ImageAdjustmentKind::Posterize:for(int c=0;c<3;++c)p[c]=std::round(p[c]*(s.posterizeLevels-1))/(s.posterizeLevels-1);break;
    default:break;
    }
    return p;
}
bool fail(QString *error,const QString &message){if(error)*error=message;return false;}
}

bool ImageAdjustments::validate(const ImageAdjustmentSpec &s,QString *error) {
    for(double v:{s.exposure,s.brightness,s.contrast,s.gamma,s.inputBlack,s.inputWhite,s.outputBlack,s.outputWhite,s.hue,s.saturation,s.lightness,s.redGain,s.greenGain,s.blueGain,s.threshold,s.amount})if(!std::isfinite(v))return fail(error,QStringLiteral("Adjustment contains a non-finite value"));
    if(s.gamma<=0 || s.inputWhite<=s.inputBlack || s.posterizeLevels<2 || s.posterizeLevels>65536 || std::abs(s.exposure)>32)return fail(error,QStringLiteral("Invalid gamma, input range, exposure or posterize levels"));
    if(spatial(s.kind) && (s.radius<1 || s.radius>64 || s.amount<0 || (s.kind==ImageAdjustmentKind::Median && s.radius>8)))return fail(error,QStringLiteral("Filter radius must be 1–64 (median: 1–8), with a non-negative amount"));
    for(const auto &points:s.curves){if(points.isEmpty())continue;if(points.size()<2)return fail(error,QStringLiteral("A curve needs at least two points"));for(int i=0;i<points.size();++i)if(!std::isfinite(points[i].x()) || !std::isfinite(points[i].y()) || (i && points[i].x()<=points[i-1].x()))return fail(error,QStringLiteral("Curve points must have finite, strictly increasing input values"));}
    for(const auto &row:s.channelMixer)for(double v:row)if(!std::isfinite(v))return fail(error,QStringLiteral("Channel mixer contains a non-finite value"));
    return true;
}
QImage ImageAdjustments::process(const QImage &source,const ImageAdjustmentSpec &spec) {
    if(source.isNull() || !validate(spec))return {};
    // Exposure stops describe linear light. Decode and re-encode a tagged RGB working space
    // in float so an 8/16-bit source does not quantize the intermediate HDR values.
    if(spec.kind==ImageAdjustmentKind::Exposure && spec.exposure!=0 && source.colorSpace().isValid() && source.colorSpace().transferFunction()!=QColorSpace::TransferFunction::Linear){
        const auto linear=source.colorSpace().withTransferFunction(QColorSpace::TransferFunction::Linear);
        if(linear.isValid()){
            const auto decoded=source.colorTransformed(source.colorSpace().transformationToColorSpace(linear),QImage::Format_RGBA32FPx4);
            if(decoded.isNull())return {};
            auto output=decoded;output.detach();
            for(int y=0;y<output.height();++y)for(int x=0;x<output.width();++x)put(output,x,y,adjusted(pixel(output,x,y),spec));
            output=output.colorTransformed(linear.transformationToColorSpace(source.colorSpace()),source.format());output.setColorSpace(source.colorSpace());return output;
        }
    }
    QImage result=source;
    if(result.format()!=QImage::Format_RGBA8888 && result.format()!=QImage::Format_RGBA64 && result.format()!=QImage::Format_RGBA32FPx4)result=result.convertToFormat(source.depth()>32?QImage::Format_RGBA64:QImage::Format_RGBA8888);
    result.detach();
    if(spatial(spec.kind))return filter(result,spec);
    for(int y=0;y<result.height();++y)for(int x=0;x<result.width();++x)put(result,x,y,adjusted(pixel(result,x,y),spec));
    return result;
}
bool ImageAdjustments::apply(ImageDocument *doc,const QUuid &id,const ImageAdjustmentSpec &spec,const QString &label,QString *error) {
    if(!doc || !validate(spec,error))return false;
    const auto layers=doc->layers();auto it=std::find_if(layers.begin(),layers.end(),[&](const auto &l){return l.id==id;});
    if(it==layers.end())return fail(error,QStringLiteral("No pixel layer is selected"));
    if(it->group || it->locked)return fail(error,QStringLiteral("Choose an unlocked pixel layer before applying an adjustment"));
    auto parent=it->parentId;for(int depth=0;!parent.isNull() && depth<layers.size();++depth){const auto group=std::find_if(layers.begin(),layers.end(),[&](const auto &l){return l.id==parent;});if(group==layers.end())break;if(group->locked)return fail(error,QStringLiteral("The selected layer belongs to a locked group"));parent=group->parentId;}
    const auto regions=doc->layerRegions(id);if(regions.isEmpty())return true;
    doc->beginEdit(label);
    for(const QRect &part:regions){
        const QImage original=doc->readOriginalRegion(id,part);
        const int halo=spatial(spec.kind)?spec.kind==ImageAdjustmentKind::EdgeDetect?1:spec.radius:0;
        const QRect inputRegion=part.adjusted(-halo,-halo,halo,halo).intersected(doc->layerBounds(id));
        QImage result=process(halo?doc->readOriginalRegion(id,inputRegion):original,spec);
        if(halo)result=result.copy(QRect(part.topLeft()-inputRegion.topLeft(),part.size()));
        if(result.isNull()){doc->cancelEdit();return fail(error,QStringLiteral("Unable to allocate adjustment tile"));}
        if(doc->hasSelection()){
            const auto selection=doc->selectionRegion(part.translated(it->offset));
            if(selection.isNull()){doc->cancelEdit();return fail(error,QStringLiteral("Unable to read selection"));}
            for(int py=0;py<part.height();++py){const auto *mask=reinterpret_cast<const quint16 *>(selection.constScanLine(py));for(int px=0;px<part.width();++px){auto a=pixel(original,px,py),b=pixel(result,px,py);const float coverage=mask[px]/65535.f;for(int c=0;c<3;++c)b[c]=a[c]+(b[c]-a[c])*coverage;put(result,px,py,b);}}
        }
        doc->writeRegion(id,part.topLeft(),result);
        if(!doc->storageError().isEmpty()){doc->cancelEdit();return fail(error,doc->storageError());}
    }
    doc->commitEdit();return true;
}
bool ImageAdjustments::convertColorSpace(ImageDocument *doc,const QColorSpace &target,QString *error) {
    if(!doc || !doc->colorSpace().isValid() || !target.isValid())return fail(error,QStringLiteral("Both source and destination need a valid RGB color profile"));
    if(doc->colorSpace()==target)return true;
    const auto transform=doc->colorSpace().transformationToColorSpace(target);
    doc->beginEdit(QStringLiteral("Convert color profile"));
    for(const auto &layer:doc->layers()){
        for(const auto &part:doc->layerRegions(layer.id)){
            const auto source=doc->readRegion(layer.id,part);auto converted=source.colorTransformed(transform,doc->pixelFormat());
            if(converted.isNull()){doc->cancelEdit();return fail(error,QStringLiteral("Unable to convert image color profile"));}
            doc->writeRegion(layer.id,part.topLeft(),converted);
            if(!doc->storageError().isEmpty()){doc->cancelEdit();return fail(error,doc->storageError());}
        }
    }
    doc->setColorSpace(target);doc->commitEdit();return true;
}
std::array<QVector<quint64>,4> ImageAdjustments::histogram(const QImage &source,int bins) {
    bins=std::clamp(bins,2,65536);std::array<QVector<quint64>,4> result;for(auto &channel:result)channel.fill(0,bins);
    QImage image=source;if(image.format()!=QImage::Format_RGBA8888 && image.format()!=QImage::Format_RGBA64 && image.format()!=QImage::Format_RGBA32FPx4)image=image.convertToFormat(QImage::Format_RGBA32FPx4);
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){const auto p=pixel(image,x,y);if(p[3]==0)continue;for(int c=0;c<4;++c)if(std::isfinite(p[c]))++result[c][std::clamp(int(std::clamp(p[c],0.f,1.f)*(bins-1)),0,bins-1)];}
    return result;
}
}
