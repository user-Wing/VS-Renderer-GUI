#include "image/ImageHdr.h"
#include <QColorTransform>
#include <QDataStream>
#include <QSaveFile>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace vsr {
namespace {
bool fail(QString *error,const QString &message){if(error)*error=message;return false;}
double encodeSrgb(double linear){return linear<=.0031308?12.92*linear:1.055*std::pow(linear,1/2.4)-.055;}
QByteArray integers(std::initializer_list<quint64> values,int bytes){QByteArray result;QDataStream stream(&result,QIODevice::WriteOnly);stream.setByteOrder(QDataStream::LittleEndian);for(auto v:values){if(bytes==2)stream<<quint16(v);else if(bytes==4)stream<<quint32(v);else stream<<quint64(v);}return result;}
struct Tag {quint16 id,type;quint64 count;QByteArray data;};
}
QImage ImageHdr::preview(const QImage &source,const ImageHdrPreviewSettings &settings) {
    if(source.isNull() || !std::isfinite(settings.exposure) || !std::isfinite(settings.whitePoint) || settings.whitePoint<=0)return {};
    const bool floating=source.format()==QImage::Format_RGBA32FPx4||source.format()==QImage::Format_RGBA16FPx4||source.format()==QImage::Format_RGBX32FPx4||source.format()==QImage::Format_RGBX16FPx4;
    if(!floating&&settings.exposure==0&&!settings.toneMap&&settings.channel<0){
        QImage result=source.colorSpace().isValid()&&source.colorSpace()!=QColorSpace(QColorSpace::SRgb)?source.convertedToColorSpace(QColorSpace::SRgb,QImage::Format_RGBA8888):source.convertToFormat(QImage::Format_RGBA8888);
        if(!result.isNull())result.setColorSpace(QColorSpace::SRgb);return result;
    }
    QImage native=source.convertToFormat(QImage::Format_RGBA32FPx4);
    if(native.colorSpace().isValid())native=native.colorTransformed(native.colorSpace().transformationToColorSpace(QColorSpace::SRgbLinear),QImage::Format_RGBA32FPx4);
    else {native.setColorSpace(floating?QColorSpace::SRgbLinear:QColorSpace::SRgb);native=native.colorTransformed(native.colorSpace().transformationToColorSpace(QColorSpace::SRgbLinear),QImage::Format_RGBA32FPx4);}
    if(native.isNull())return {};
    QImage result(native.size(),QImage::Format_RGBA8888);result.setColorSpace(QColorSpace::SRgb);
    if(result.isNull())return {};
    const double gain=std::exp2(std::clamp(settings.exposure,-32.,32.)),white2=settings.whitePoint*settings.whitePoint;
    for(int y=0;y<native.height();++y){const auto *row=reinterpret_cast<const float *>(native.constScanLine(y));auto *out=result.scanLine(y);for(int x=0;x<native.width();++x){
        const auto *p=row+x*4;double rgb[3]={p[0]*gain,p[1]*gain,p[2]*gain};
        if(settings.toneMap){const double luminance=.2126*rgb[0]+.7152*rgb[1]+.0722*rgb[2];if(luminance>0){const double mapped=luminance*(1+luminance/white2)/(1+luminance);for(auto &v:rgb)v*=mapped/luminance;}}
        for(int c=0;c<3;++c){const double v=settings.channel==3?p[3]:settings.channel>=0 && settings.channel<=2?rgb[settings.channel]:rgb[c];const double encoded=settings.channel==3?v:encodeSrgb(std::max(0.,v));out[x*4+c]=std::isfinite(encoded)?uchar(std::lround(std::clamp(encoded,0.,1.)*255)):0;}
        out[x*4+3]=settings.channel==3?255:std::isfinite(p[3])?uchar(std::lround(std::clamp(double(p[3]),0.,1.)*255)):0;
    }}
    return result;
}
bool ImageHdr::exportFloatTiff(ImageDocument *doc,const QString &path,QString *error) {
    if(!doc || doc->size().isEmpty())return fail(error,QStringLiteral("No image document to export"));
    const quint64 width=doc->size().width(),height=doc->size().height(),rowBytes=width*16;
    const quint64 rowsPerStrip=std::max<quint64>(1,std::min<quint64>(64,8*1024*1024/rowBytes));
    if(rowBytes>256*1024*1024)return fail(error,QStringLiteral("A TIFF scanline exceeds the 256 MiB export allocation limit"));
    const quint64 strips=(height+rowsPerStrip-1)/rowsPerStrip,pixelBytes=rowBytes*height;
    // Conservative metadata bound makes classic offset overflow impossible after the header is assembled.
    const bool big=pixelBytes+strips*16+quint64(doc->colorSpace().iccProfile().size())+4096>std::numeric_limits<quint32>::max();
    const int pointerSize=big?8:4;const quint16 offsetType=big?16:4;
    const auto icc=doc->colorSpace().iccProfile();
    QList<Tag> tags={{256,4,1,integers({width},4)},{257,4,1,integers({height},4)},
        {258,3,4,integers({32,32,32,32},2)},{259,3,1,integers({1},2)},{262,3,1,integers({2},2)},
        {273,offsetType,strips,QByteArray(qsizetype(strips*pointerSize),0)},{277,3,1,integers({4},2)},
        {278,4,1,integers({rowsPerStrip},4)},{279,offsetType,strips,QByteArray(qsizetype(strips*pointerSize),0)},
        {284,3,1,integers({1},2)},{338,3,1,integers({2},2)},{339,3,4,integers({3,3,3,3},2)}};
    if(!icc.isEmpty())tags.append({34675,7,quint64(icc.size()),icc});
    const quint64 headerBytes=big?16:8,ifdBytes=(big?8:2)+tags.size()*(big?20:12)+pointerSize;
    quint64 pixelOffset=headerBytes+ifdBytes;
    for(const auto &tag:tags)if(tag.data.size()>pointerSize){pixelOffset=(pixelOffset+7)&~quint64(7);pixelOffset+=tag.data.size();}
    pixelOffset=(pixelOffset+7)&~quint64(7);
    for(auto &tag:tags)if(tag.id==273 || tag.id==279){tag.data.clear();QDataStream stream(&tag.data,QIODevice::WriteOnly);stream.setByteOrder(QDataStream::LittleEndian);for(quint64 strip=0;strip<strips;++strip){const quint64 v=tag.id==273?pixelOffset+strip*rowsPerStrip*rowBytes:std::min(rowsPerStrip,height-strip*rowsPerStrip)*rowBytes;if(big)stream<<v;else stream<<quint32(v);}}
    QByteArray header;QDataStream stream(&header,QIODevice::WriteOnly);stream.setByteOrder(QDataStream::LittleEndian);stream<<quint16(0x4949)<<quint16(big?43:42);
    if(big)stream<<quint16(8)<<quint16(0)<<quint64(headerBytes)<<quint64(tags.size());else stream<<quint32(headerBytes)<<quint16(tags.size());
    quint64 extraOffset=headerBytes+ifdBytes;QByteArray extras;
    for(const auto &tag:tags){stream<<tag.id<<tag.type;if(big)stream<<tag.count;else stream<<quint32(tag.count);
        if(tag.data.size()<=pointerSize){stream.writeRawData(tag.data.constData(),tag.data.size());for(int i=tag.data.size();i<pointerSize;++i)stream<<quint8(0);}
        else {while(extraOffset%8){extras.append(char(0));++extraOffset;}if(big)stream<<extraOffset;else stream<<quint32(extraOffset);extras.append(tag.data);extraOffset+=tag.data.size();}
    }
    if(big)stream<<quint64(0);else stream<<quint32(0);
    header.append(extras);while(quint64(header.size())<pixelOffset)header.append(char(0));
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return fail(error,file.errorString());
    if(file.write(header)!=header.size())return fail(error,file.errorString());
    for(quint64 y=0;y<height;y+=rowsPerStrip){
        const auto image=doc->composite({0,int(y),int(width),int(std::min(rowsPerStrip,height-y))}).convertToFormat(QImage::Format_RGBA32FPx4);
        if(image.isNull() || !doc->storageError().isEmpty())return fail(error,doc->storageError().isEmpty()?QStringLiteral("Unable to allocate TIFF strip"):doc->storageError());
        for(int line=0;line<image.height();++line){
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            if(file.write(reinterpret_cast<const char *>(image.constScanLine(line)),qint64(rowBytes))!=qint64(rowBytes))return fail(error,file.errorString());
#else
            QByteArray row(qsizetype(rowBytes),0);for(quint64 i=0;i<rowBytes;i+=4){quint32 bits;std::memcpy(&bits,image.constScanLine(line)+i,4);qToLittleEndian(bits,row.data()+i);}if(file.write(row)!=qint64(rowBytes))return fail(error,file.errorString());
#endif
        }
    }
    if(!file.commit())return fail(error,file.errorString());return true;
}
}
