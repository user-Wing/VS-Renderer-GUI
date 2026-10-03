#include "image/ImagePsd.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QMutex>
#include <QThread>
#include <QMutexLocker>
#include <QStringDecoder>
#include <QtEndian>
#include <zlib.h>
#include <array>
#include <future>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <limits>

namespace vsr {
namespace {
void fail(const QString &message){throw std::runtime_error(message.toUtf8().constData());}
class Reader {
public:
    QFile f;
    explicit Reader(const QString &path):f(path){if(!f.open(QIODevice::ReadOnly))fail(f.errorString());}
    QByteArray bytes(qint64 count){if(count<0 || count>f.size()-f.pos())fail("Truncated PSD section");auto b=f.read(count);if(b.size()!=count)fail("Unable to read PSD");return b;}
    quint64 u(int count){const auto b=bytes(count);quint64 v=0;for(uchar c:b)v=(v<<8)|c;return v;}
    qint32 i(){return qint32(u(4));}
    void seek(qint64 pos){if(pos<0 || pos>f.size() || !f.seek(pos))fail("Invalid PSD section offset");}
    qint64 section(int count=4){const quint64 length=u(count);if(length>quint64(f.size()-f.pos()))fail("Invalid PSD section length");return f.pos()+qint64(length);}
    QRect rect(){const auto top=i(),left=i(),bottom=i(),right=i();if(qint64(right)-left<0 || qint64(bottom)-top<0 || qint64(right)-left>300000 || qint64(bottom)-top>300000)fail("Invalid PSD layer dimensions");return {left,top,right-left,bottom-top};}
};
struct Channel {int id;quint64 length;qint64 offset=0;};
struct Record {
    QString name;QRect bounds,maskBounds;QList<Channel> channels;
    QByteArray blend="norm";int section=0;uchar opacity=255,flags=0,maskDefault=255,maskFlags=0,clipping=0;
    bool complex=false,locked=false;QUuid id,parent;
    ImageLayerInfo blending;
};
ImageBlendMode blendMode(const QByteArray &key,QStringList &warnings){
    static const QHash<QByteArray,ImageBlendMode> modes{
        {"norm",ImageBlendMode::Normal},{"pass",ImageBlendMode::Normal},{"mul ",ImageBlendMode::Multiply},{"scrn",ImageBlendMode::Screen},
        {"over",ImageBlendMode::Overlay},{"dark",ImageBlendMode::Darken},{"lite",ImageBlendMode::Lighten},{"div ",ImageBlendMode::ColorDodge},
        {"idiv",ImageBlendMode::ColorBurn},{"lddg",ImageBlendMode::LinearDodge},{"lbrn",ImageBlendMode::LinearBurn},{"sLit",ImageBlendMode::SoftLight},
        {"hLit",ImageBlendMode::HardLight},{"diff",ImageBlendMode::Difference},{"smud",ImageBlendMode::Exclusion},{"fsub",ImageBlendMode::Subtract},{"fdiv",ImageBlendMode::Divide}};
    if(!modes.contains(key))warnings<<QString("Unsupported blend mode %1; imported as Normal").arg(QString::fromLatin1(key));
    return modes.value(key,ImageBlendMode::Normal);
}
QList<Record> records(Reader &r,qint64 end,int version){
    if(end-r.f.pos()<2)return {};
    const int count=std::abs(qint16(r.u(2)));QList<Record> out;out.reserve(count);
    for(int n=0;n<count;++n){
        Record layer;layer.bounds=r.rect();const int channels=int(r.u(2));if(channels>56)fail("Invalid PSD channel count");
        for(int c=0;c<channels;++c)layer.channels<<Channel{qint16(r.u(2)),r.u(version==2?8:4)};
        if(r.bytes(4)!="8BIM")fail("Invalid PSD blend signature");layer.blend=r.bytes(4);
        layer.opacity=uchar(r.u(1));layer.clipping=uchar(r.u(1));layer.flags=uchar(r.u(1));r.u(1);
        const auto extraEnd=r.section();if(extraEnd>end)fail("PSD layer record exceeds section");
        const auto maskEnd=r.section();
        if(maskEnd-r.f.pos()>=18){layer.maskBounds=r.rect();layer.maskDefault=uchar(r.u(1));layer.maskFlags=uchar(r.u(1));}
        r.seek(maskEnd);const auto rangesEnd=r.section();if(rangesEnd>extraEnd)fail("PSD blending ranges exceed layer record");
        for(int c=0;c<4 && rangesEnd-r.f.pos()>=8;++c){for(auto *range:{&layer.blending.blendIfSource[c],&layer.blending.blendIfBackdrop[c]})for(auto &value:*range)value=float(r.u(1))/255.f;}r.seek(rangesEnd);
        const int nameLength=int(r.u(1));const auto name=r.bytes(nameLength);
        // Legacy Photoshop files on Chinese Windows store this Pascal name in GB18030.
        QStringDecoder decoder("GB18030");layer.name=decoder.isValid()?decoder(name):QString::fromLatin1(name);
        r.seek(r.f.pos()+((4-(nameLength+1)%4)%4));
        while(r.f.pos()+12<=extraEnd){
            const auto signature=r.bytes(4);if(signature!="8BIM" && signature!="8B64")break;
            const auto key=r.bytes(4);const bool longLength=version==2 && (key=="LMsk" || key=="Lr16" || key=="Lr32" || key=="Layr" || key=="Mt16" || key=="Mt32" || key=="Mtrn" || key=="Alph" || key=="FMsk" || key=="lnk2" || key=="FEid" || key=="FXid" || key=="PxSD");
            const auto tagEnd=r.section(longLength?8:4);if(tagEnd>extraEnd)fail("PSD layer tag exceeds record");
            if(key=="luni" && tagEnd-r.f.pos()>=4){const auto length=r.u(4);if(length>quint64((tagEnd-r.f.pos())/2))fail("Invalid PSD layer name");QString text;for(quint64 i=0;i<length;++i)text+=QChar(ushort(r.u(2)));layer.name=text;}
            else if((key=="lsct" || key=="lsdk") && tagEnd-r.f.pos()>=4){layer.section=int(r.u(4));if(tagEnd-r.f.pos()>=8){r.bytes(4);layer.blend=r.bytes(4);}}
            else if(key=="lspf" && tagEnd-r.f.pos()>=4)layer.locked=(r.u(4)&7)==7;
            else if(key=="iOpa" && tagEnd-r.f.pos()>=1)layer.blending.fillOpacity=float(r.u(1))/255.f;
            else if(key=="brst"){while(tagEnd-r.f.pos()>=4){const auto channel=r.i();if(channel>=0 && channel<3)layer.blending.channels&=quint8(~(1<<channel));}}
            else if(key=="TySh" || key=="SoLd" || key=="SoLE" || key=="vmsk" || key=="lrFX" || key=="lfx2" || key=="brit" || key=="curv" || key=="levl")layer.complex=true;
            r.seek(tagEnd+(tagEnd%2));
        }
        r.seek(extraEnd);out<<layer;
    }
    for(auto &layer:out)for(auto &channel:layer.channels){if(channel.length<2 || channel.length>quint64(end-r.f.pos()))fail("Invalid PSD channel payload");channel.offset=r.f.pos();r.seek(r.f.pos()+qint64(channel.length));}
    return out;
}
QByteArray unpack(const QByteArray &data,int expected){
    QByteArray out;out.reserve(expected);int at=0;
    while(at<data.size() && out.size()<expected){const int code=qint8(data[at++]);if(code>=0){const int n=code+1;if(at+n>data.size() || out.size()+n>expected)fail("Invalid PSD RLE literal");out.append(data.constData()+at,n);at+=n;}else if(code!=-128){const int n=1-code;if(at>=data.size() || out.size()+n>expected)fail("Invalid PSD RLE run");out.append(n,data[at++]);}}
    if(out.size()!=expected)fail("Truncated PSD RLE row");return out;
}
void predict(QByteArray &row,int depth,int width){
    auto *p=reinterpret_cast<uchar *>(row.data());
    if(depth==16){quint16 previous=0;for(int x=0;x<width;++x){previous=quint16(previous+qFromBigEndian<quint16>(p+x*2));qToBigEndian(previous,p+x*2);}}
    else {for(int x=1;x<row.size();++x)p[x]=uchar(p[x]+p[x-1]);if(depth==32){const QByteArray planar(row.constData(),row.size());for(int x=0;x<width;++x)for(int b=0;b<4;++b)p[x*4+b]=uchar(planar[b*width+x]);}}
}
// Each channel is decoded once into a temporary file; RAM is one compressed buffer
// and one scanline. Hidden layers do not reach this code during initial display.
class Plane {
    QString path_;Channel channel_;QSize size_;int depth_,version_;
    QMutex mutex_;std::unique_ptr<QTemporaryFile> raw_;QString error_;
public:
    Plane(QString path,Channel channel,QSize size,int depth,int version):path_(std::move(path)),channel_(channel),size_(size),depth_(depth),version_(version){}
    void ensure(std::function<bool()> cancelled = {}){
        QMutexLocker lock(&mutex_);if(!error_.isEmpty())fail(error_);if(raw_)return;
        try {
            Reader r(path_);r.seek(channel_.offset);const int compression=int(r.u(2)),rowBytes=size_.width()*(depth_/8);const auto end=channel_.offset+qint64(channel_.length);
            auto raw=std::make_unique<QTemporaryFile>();if(!raw->open())fail(raw->errorString());
            QList<quint32> lengths;if(compression==1)for(int y=0;y<size_.height();++y)lengths<<quint32(r.u(version_==2?4:2));
            z_stream stream{};const bool zipped=compression==2 || compression==3;
            if(compression>3)fail("Unsupported PSD channel compression");
            if(zipped && inflateInit(&stream)!=Z_OK)fail("Unable to initialize PSD ZIP decoder");
            struct End {z_stream *s;bool active;~End(){if(active)inflateEnd(s);}} cleanup{&stream,zipped};
            QByteArray input;int status=Z_OK;
            for(int y=0;y<size_.height();++y){
                if(cancelled && cancelled())fail("PSD decode cancelled");
                QByteArray row;
                if(compression==0)row=r.bytes(rowBytes);
                else if(compression==1){if(lengths[y]>quint64(end-r.f.pos()))fail("PSD RLE exceeds channel");row=unpack(r.bytes(lengths[y]),rowBytes);}
                else {
                    row.resize(rowBytes);stream.next_out=reinterpret_cast<Bytef *>(row.data());stream.avail_out=rowBytes;
                    while(stream.avail_out){
                        if(!stream.avail_in){if(r.f.pos()>=end)fail("Truncated PSD ZIP channel");input=r.bytes(std::min<qint64>(65536,end-r.f.pos()));stream.next_in=reinterpret_cast<Bytef *>(input.data());stream.avail_in=uInt(input.size());}
                        const auto beforeIn=stream.avail_in,beforeOut=stream.avail_out;status=inflate(&stream,Z_NO_FLUSH);
                        if((status!=Z_OK && status!=Z_STREAM_END) || (beforeIn==stream.avail_in && beforeOut==stream.avail_out) || (status==Z_STREAM_END && (stream.avail_out || y+1<size_.height())))fail("Invalid PSD ZIP channel");
                    }
                    if(compression==3)predict(row,depth_,size_.width());
                }
                if(raw->write(row)!=row.size())fail(raw->errorString());
            }
            if(zipped && status!=Z_STREAM_END){
                uchar extra;stream.next_out=&extra;stream.avail_out=1;
                while(status==Z_OK && stream.avail_out){if(!stream.avail_in){if(r.f.pos()>=end)fail("Truncated PSD ZIP trailer");input=r.bytes(std::min<qint64>(65536,end-r.f.pos()));stream.next_in=reinterpret_cast<Bytef *>(input.data());stream.avail_in=uInt(input.size());}status=inflate(&stream,Z_NO_FLUSH);}
                if(status!=Z_STREAM_END || !stream.avail_out)fail("PSD ZIP decompressed size mismatch");
            }
            if(!raw->flush())fail(raw->errorString());raw_=std::move(raw);
        }catch(const std::exception &e){if(!cancelled || !cancelled())error_=QString::fromUtf8(e.what());throw;}
    }
    QByteArray row(int y){ensure();QMutexLocker lock(&mutex_);const int bytes=size_.width()*(depth_/8);if(!raw_->seek(qint64(y)*bytes))fail("Unable to seek PSD plane");auto data=raw_->read(bytes);if(data.size()!=bytes)fail("Unable to read PSD plane");return data;}
};
struct Source {
    Record record;int depth;QColorSpace space;
    QHash<int,std::shared_ptr<Plane>> planes;
    QMutex mutex;bool pixelsReady=false,maskReady=false;
    void ensure(bool mask){
        QMutexLocker lock(&mutex);auto &ready=mask?maskReady:pixelsReady;if(ready)return;
        std::vector<std::future<void>> jobs;
        for(auto it=planes.cbegin();it!=planes.cend();++it)if((it.key()==-2)==mask)jobs.push_back(std::async(std::launch::async,[p=it.value(),thread=QThread::currentThread()]{p->ensure([thread]{return thread->isInterruptionRequested();});}));
        for(auto &job:jobs)job.get();
        ready=true;
    }
    QImage read(const QRect &rect,QSize output,bool mask,ImageSamplingQuality quality=ImageSamplingQuality::Nearest){
        try {
            ensure(mask);const auto format=mask?QImage::Format_Grayscale16:depth==32?QImage::Format_RGBA32FPx4:depth==16?QImage::Format_RGBA64:QImage::Format_RGBA8888;
            QImage image(output,format);if(image.isNull())fail("Unable to allocate PSD region");image.fill(mask?((record.maskDefault!=0)!=bool(record.maskFlags&4)?Qt::white:Qt::black):Qt::transparent);if(!mask)image.setColorSpace(space);
            const auto bounds=mask?QRect(record.maskBounds.topLeft()-((record.maskFlags&1)?QPoint():record.bounds.topLeft()),record.maskBounds.size()):QRect(QPoint(),record.bounds.size());
            if(quality==ImageSamplingQuality::Bilinear && output!=rect.size()){
                std::array<QHash<int,QByteArray>,2> filteredRows;int previousTop=std::numeric_limits<int>::min();
                for(int y=0;y<output.height();++y){const double fy=rect.y()+(2.*y+1)*rect.height()/(2.*output.height())-.5-bounds.y();const int top=int(std::floor(fy));const float wy=fy-top;
                    if(top!=previousTop){previousTop=top;for(int iy=0;iy<2;++iy){filteredRows[iy].clear();if(top+iy>=0 && top+iy<bounds.height())for(auto it=planes.cbegin();it!=planes.cend();++it)if((it.key()==-2)==mask)filteredRows[iy].insert(it.key(),it.value()->row(top+iy));}}
                    for(int x=0;x<output.width();++x){const double fx=rect.x()+(2.*x+1)*rect.width()/(2.*output.width())-.5-bounds.x();const int left=int(std::floor(fx));const float wx=fx-left;std::array<float,4> sum{};std::array<float,3> hidden{};float coverage=0;
                        for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix){const float weight=(ix?wx:1-wx)*(iy?wy:1-wy);if(weight==0)continue;const int sx=left+ix;const bool inside=sx>=0 && sx<bounds.width() && top+iy>=0 && top+iy<bounds.height();
                            auto value=[&](int channel,float fallback){if(!inside || !filteredRows[iy].contains(channel))return fallback;const auto *p=reinterpret_cast<const uchar *>(filteredRows[iy][channel].constData())+sx*(depth/8);if(depth==8)return p[0]/255.f;if(depth==16)return qFromBigEndian<quint16>(p)/65535.f;const auto bits=qFromBigEndian<quint32>(p);float f;std::memcpy(&f,&bits,4);return f;};
                            if(mask){float v=value(-2,record.maskDefault/255.f);if(record.maskFlags&4)v=1-v;coverage+=v*weight;}else{const float a=inside?std::clamp(value(-1,planes.contains(0)?1:0),0.f,1.f):0;sum[3]+=a*weight;for(int c=0;c<3;++c){const float v=value(c,0);sum[c]+=v*a*weight;hidden[c]+=v*weight;}}
                        }
                        if(mask)reinterpret_cast<quint16 *>(image.scanLine(y))[x]=quint16(std::clamp(coverage,0.f,1.f)*65535.f+.5f);else{for(int c=0;c<3;++c)sum[c]=sum[3]>0?sum[c]/sum[3]:hidden[c];for(int c=0;c<4;++c){const float v=sum[c];if(depth==32)reinterpret_cast<float *>(image.scanLine(y))[x*4+c]=v;else if(depth==16)reinterpret_cast<quint16 *>(image.scanLine(y))[x*4+c]=quint16(std::clamp(v,0.f,1.f)*65535.f+.5f);else image.scanLine(y)[x*4+c]=uchar(std::clamp(v,0.f,1.f)*255.f+.5f);}}
                    }
                }return image;
            }
            int previous=-1;QHash<int,QByteArray> rows;
            for(int y=0;y<output.height();++y){
                const int sy=rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*output.height()))-bounds.y();if(sy<0 || sy>=bounds.height())continue;
                if(sy!=previous){previous=sy;rows.clear();for(auto it=planes.cbegin();it!=planes.cend();++it)if((it.key()==-2)==mask)rows.insert(it.key(),it.value()->row(sy));}
                for(int x=0;x<output.width();++x){
                    const int sx=rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*output.width()))-bounds.x();if(sx<0 || sx>=bounds.width())continue;
                    auto value=[&](int channel,float fallback){if(!rows.contains(channel))return fallback;const auto *p=reinterpret_cast<const uchar *>(rows[channel].constData())+sx*(depth/8);if(depth==8)return p[0]/255.f;if(depth==16)return qFromBigEndian<quint16>(p)/65535.f;const auto bits=qFromBigEndian<quint32>(p);float f;std::memcpy(&f,&bits,4);return f;};
                    if(mask){auto v=value(-2,record.maskDefault/255.f);if(record.maskFlags&4)v=1-v;reinterpret_cast<quint16 *>(image.scanLine(y))[x]=quint16(std::clamp(v,0.f,1.f)*65535.f+.5f);}
                    else for(int c=0;c<4;++c){const float v=value(c==3?-1:c,c==3?(planes.contains(0)?1:0):0);if(depth==32)reinterpret_cast<float *>(image.scanLine(y))[x*4+c]=v;else if(depth==16)reinterpret_cast<quint16 *>(image.scanLine(y))[x*4+c]=quint16(std::clamp(v,0.f,1.f)*65535.f+.5f);else image.scanLine(y)[x*4+c]=uchar(std::clamp(v,0.f,1.f)*255.f+.5f);}
                }
            }
            return image;
        }catch(const std::exception &e){fail(QString("PSD layer '%1': %2").arg(record.name,QString::fromUtf8(e.what())));}
        return {};
    }
};
}
std::unique_ptr<ImageDocument> ImagePsd::load(const QString &path,QString *error,QStringList *warnings){
    try {
        Reader r(path);if(r.bytes(4)!="8BPS")fail("Not a PSD/PSB file");const int version=int(r.u(2));if(version!=1 && version!=2)fail("Unsupported PSD version");r.bytes(6);r.u(2);
        const int height=int(r.u(4)),width=int(r.u(4)),depth=int(r.u(2)),mode=int(r.u(2));if(width<1 || height<1 || width>300000 || height>300000 || (depth!=8 && depth!=16 && depth!=32))fail("Unsupported PSD dimensions/depth");
        if(mode!=3)fail("PSD editing currently supports RGB documents; convert CMYK/Lab to RGB first");
        const auto colorEnd=r.section();const auto colorData=r.bytes(colorEnd-r.f.pos());const auto resourcesEnd=r.section();QColorSpace space;QByteArray icc;
        while(r.f.pos()+12<=resourcesEnd){if(r.bytes(4)!="8BIM")fail("Invalid PSD image resource");const int id=int(r.u(2)),length=int(r.u(1));r.bytes(length);if((length+1)%2)r.bytes(1);const auto end=r.section();if(end>resourcesEnd)fail("Invalid PSD image resource size");if(id==1039){icc=r.bytes(end-r.f.pos());space=QColorSpace::fromIccProfile(icc);}r.seek(end+(end%2));}
        r.seek(resourcesEnd);const auto layerEnd=r.section(version==2?8:4);const auto mainEnd=r.section(version==2?8:4);QList<Record> layers;
        if(mainEnd>r.f.pos())layers=records(r,mainEnd,version);r.seek(mainEnd);
        if(r.f.pos()+4<=layerEnd){r.seek(r.section());}
        while(r.f.pos()+12<=layerEnd){
            const auto signature=r.bytes(4);if(signature!="8BIM" && signature!="8B64")break;const auto key=r.bytes(4);
            const bool longLength=version==2 && (key=="Lr16" || key=="Lr32" || key=="Layr" || key=="LMsk" || key=="Mt16" || key=="Mt32" || key=="Mtrn");
            const auto end=r.section(longLength?8:4);const auto start=r.f.pos();if(end>layerEnd)fail("Invalid PSD global layer tag");
            if((depth==16 && key=="Lr16") || (depth==32 && key=="Lr32") || (layers.isEmpty() && key=="Layr"))layers=records(r,end,version);
            r.seek(end+((4-(end-start)%4)%4));
        }
        if(layers.isEmpty())fail("PSD has no editable pixel layer records");
        auto document=std::make_unique<ImageDocument>(QSize(width,height),depth==32?ImagePrecision::Float32:depth==16?ImagePrecision::UInt16:ImagePrecision::UInt8);
        document->beginEdit("Import PSD");document->setColorSpace(space);document->setMetadata("sourcePath",path);document->setMetadata("sourceICC",QString::fromLatin1(icc.toBase64()));document->setMetadata("sourcePSDColorData",QString::fromLatin1(colorData.toBase64()));
        QStringList notes;QList<QUuid> stack;QList<Record *> ordered;
        // PSD records are bottom-to-top; parse the reverse view to assign parent groups.
        for(auto it=layers.rbegin();it!=layers.rend();++it){auto &layer=*it;if(layer.section==3){if(!stack.isEmpty())stack.removeLast();continue;}layer.parent=stack.isEmpty()?QUuid{}:stack.last();layer.id=QUuid::createUuid();if(layer.section==1 || layer.section==2)stack<<layer.id;ordered<<&layer;}
        QHash<QUuid,QUuid> ids;
        for(auto it=ordered.crbegin();it!=ordered.crend();++it){
            const auto &layer=**it;auto source=std::make_shared<Source>();source->record=layer;source->depth=depth;source->space=space;
            for(const auto &channel:layer.channels)if(channel.id>=-2 && channel.id<=2){const auto size=channel.id==-2?layer.maskBounds.size():layer.bounds.size();if(!size.isEmpty())source->planes.insert(channel.id,std::make_shared<Plane>(path,channel,size,depth,version));}
            QUuid id;
            if(layer.section==1 || layer.section==2)id=document->addLayer(layer.name,{},layer.bounds.topLeft());
            else {
                id=document->addLazyLayer(layer.name,layer.bounds.size(),layer.bounds.topLeft(),[source](const QRect &rect){return source->read(rect,rect.size(),false);},[source](const QRect &rect,QSize size){return source->read(rect,size,false);},[source](const QRect &rect,QSize size,ImageSamplingQuality quality){return source->read(rect,size,false,quality);});
                if(layer.complex)notes<<QString(source->planes.contains(0)?"'%1': complex object imported as its raster pixels":"'%1': unsupported object has no raster RGB; it is not rendered").arg(layer.name);
            }
            if(source->planes.contains(-2))document->setLazyMask(id,[source](const QRect &rect){return source->read(rect,rect.size(),true);},[source](const QRect &rect,QSize size){return source->read(rect,size,true);},QRect(layer.maskBounds.topLeft()-((layer.maskFlags&1)?QPoint():layer.bounds.topLeft()),layer.maskBounds.size()),[source](const QRect &rect,QSize size,ImageSamplingQuality quality){return source->read(rect,size,true,quality);});
            ids.insert(layer.id,id);auto info=document->layers().last();info.group=layer.section==1 || layer.section==2;info.passThrough=layer.blend=="pass";info.visible=!(layer.flags&2);info.opacity=layer.opacity/255.f;info.maskEnabled=!(layer.maskFlags&2);info.maskDefault=(layer.maskFlags&4)?255-layer.maskDefault:layer.maskDefault;info.clipping=layer.clipping!=0;info.locked=layer.locked;info.blend=blendMode(layer.blend,notes);info.fillOpacity=layer.blending.fillOpacity;info.channels=layer.blending.channels;info.blendIfSource=layer.blending.blendIfSource;info.blendIfBackdrop=layer.blending.blendIfBackdrop;document->updateLayer(info);
        }
        for(const auto *layer:ordered)if(!layer->parent.isNull()){auto list=document->layers();for(auto &info:list)if(info.id==ids.value(layer->id)){info.parentId=ids.value(layer->parent);document->updateLayer(info);break;}}
        document->setMetadata("importWarnings",notes.join('\n'));document->commitEdit();document->history()->clear();document->history()->setClean();if(warnings)*warnings=notes;return document;
    }catch(const std::exception &e){if(error)*error=QString::fromUtf8(e.what());return {};}
}
namespace {
void write(QIODevice &out,const QByteArray &bytes){if(out.write(bytes)!=bytes.size())fail(out.errorString());}
void number(QIODevice &out,quint64 value,int count){QByteArray bytes(count,0);for(int i=count-1;i>=0;--i){bytes[i]=char(value);value>>=8;}write(out,bytes);}
void rectangle(QIODevice &out,const QRect &r){number(out,quint32(r.y()),4);number(out,quint32(r.x()),4);number(out,quint32(r.y()+r.height()),4);number(out,quint32(r.x()+r.width()),4);}
void patch(QIODevice &out,qint64 position,quint64 value,int bytes){const auto end=out.pos();if(!out.seek(position))fail("Unable to patch PSD section");number(out,value,bytes);if(!out.seek(end))fail("Unable to restore PSD output position");}
void tag(QIODevice &out,const QByteArray &key,const QByteArray &payload){write(out,"8BIM");write(out,key);number(out,payload.size(),4);write(out,payload);if(payload.size()%2)number(out,0,1);}
QByteArray blendKey(ImageBlendMode mode){
    static const QByteArray keys[]{"norm","mul ","scrn","over","dark","lite","div ","idiv","lddg","lbrn","sLit","hLit","diff","smud","fsub","fdiv"};
    return keys[int(mode)];
}
void encodePredict(QByteArray &row,int depth,int width){
    if(depth==16){auto *p=reinterpret_cast<uchar *>(row.data());for(int x=width-1;x>0;--x)qToBigEndian(quint16(qFromBigEndian<quint16>(p+x*2)-qFromBigEndian<quint16>(p+(x-1)*2)),p+x*2);}
    else {if(depth==32){const auto interleaved=row;for(int x=0;x<width;++x)for(int c=0;c<4;++c)row[c*width+x]=interleaved[x*4+c];}for(int i=row.size()-1;i>0;--i)row[i]=char(uchar(row[i])-uchar(row[i-1]));}
}
QByteArray channelRow(const QImage &image,int y,int channel,int depth){
    QByteArray row(image.width()*(depth/8),0);auto *out=reinterpret_cast<uchar *>(row.data());
    for(int x=0;x<image.width();++x){
        if(channel==-2){const auto mask=reinterpret_cast<const quint16 *>(image.constScanLine(y))[x];if(depth==8)out[x]=uchar((mask+128)/257);else if(depth==16)qToBigEndian(mask,out+x*2);else {const float v=mask/65535.f;quint32 bits;std::memcpy(&bits,&v,4);qToBigEndian(bits,out+x*4);}}
        else {const int c=channel==-1?3:channel;if(depth==8)out[x]=image.constScanLine(y)[x*4+c];else if(depth==16)qToBigEndian(reinterpret_cast<const quint16 *>(image.constScanLine(y))[x*4+c],out+x*2);else {quint32 bits;std::memcpy(&bits,image.constScanLine(y)+x*16+c*4,4);qToBigEndian(bits,out+x*4);}}
    }
    return row;
}
class ZipWriter {
    z_stream stream{};QIODevice &out;std::array<uchar,65536> buffer;
public:
    explicit ZipWriter(QIODevice &device):out(device){if(deflateInit(&stream,6)!=Z_OK)fail("Unable to initialize PSD ZIP encoder");}
    ~ZipWriter(){deflateEnd(&stream);}
    void feed(const QByteArray &bytes,bool finish=false){
        stream.next_in=reinterpret_cast<Bytef *>(const_cast<char *>(bytes.constData()));stream.avail_in=uInt(bytes.size());int status;
        do{stream.next_out=buffer.data();stream.avail_out=buffer.size();status=deflate(&stream,finish?Z_FINISH:Z_NO_FLUSH);if(status!=Z_OK && status!=Z_STREAM_END)fail("PSD ZIP encode failed");const auto n=buffer.size()-stream.avail_out;if(out.write(reinterpret_cast<const char *>(buffer.data()),n)!=qint64(n))fail(out.errorString());}while(stream.avail_in || !stream.avail_out || (finish && status!=Z_STREAM_END));
    }
};
struct OutputLayer {ImageLayerInfo info;QRect bounds;bool endGroup=false;QList<qint64> patches;QRect maskBounds;};
}
QString ImagePsd::save(ImageDocument *document,const QString &path){
    try {
        if(!document || document->size().isEmpty())fail("Empty PSD document");
        if(QFileInfo(path).absoluteFilePath().compare(QFileInfo(document->metadata("sourcePath")).absoluteFilePath(),Qt::CaseInsensitive)==0)fail("Save to a new PSD path to preserve the source file");
        const int width=document->size().width(),height=document->size().height(),depth=document->precision()==ImagePrecision::Float32?32:document->precision()==ImagePrecision::UInt16?16:8;
        if(width>300000 || height>300000)fail("PSD/PSB dimensions cannot exceed 300000 pixels");
        const bool psb=QFileInfo(path).suffix().compare("psb",Qt::CaseInsensitive)==0;if(!psb && (width>30000 || height>30000))fail("Use PSB for dimensions exceeding 30000 pixels");const int lengthBytes=psb?8:4;const auto colorData=depth==32?QByteArray::fromBase64(document->metadata("sourcePSDColorData").toLatin1()):QByteArray{};if(depth==32 && !colorData.contains("hdrt"))fail("32-bit Photoshop PSD requires HDR color data. Export Float32 TIFF, or save an imported 32-bit Photoshop PSD instead.");
        QList<OutputLayer> layers;const auto all=document->layers();
        std::function<void(QUuid)> append=[&](QUuid parent){for(auto it=all.cbegin();it!=all.cend();++it)if(it->parentId==parent){if(it->group){auto end=*it;end.name="</Layer group>";layers<<OutputLayer{end,{},true};append(it->id);}layers<<OutputLayer{*it,it->group?QRect{}:document->layerBounds(it->id).translated(it->offset)};}};append({});
        if(layers.size()>32767)fail("Too many PSD layers");
        QSaveFile out(path);if(!out.open(QIODevice::WriteOnly))fail(out.errorString());
        write(out,"8BPS");number(out,psb?2:1,2);write(out,QByteArray(6,0));number(out,4,2);number(out,height,4);number(out,width,4);number(out,depth,2);number(out,3,2);number(out,colorData.size(),4);write(out,colorData);
        QByteArray resources;const auto icc=document->colorSpace().iccProfile();if(!icc.isEmpty()){resources="8BIM";resources.append(char(4));resources.append(char(15));resources.append(QByteArray(2,0));const quint32 length=qToBigEndian(quint32(icc.size()));resources.append(reinterpret_cast<const char *>(&length),4);resources+=icc;if(icc.size()%2)resources.append(char(0));}number(out,resources.size(),4);write(out,resources);
        const auto outerPatch=out.pos();number(out,0,lengthBytes);const auto outerStart=out.pos();
        qint64 layerPatch=out.pos();number(out,0,lengthBytes);qint64 layerStart=out.pos();
        if(depth>8){number(out,0,4);write(out,"8BIM");write(out,depth==16?"Lr16":"Lr32");layerPatch=out.pos();number(out,0,lengthBytes);layerStart=out.pos();}
        number(out,quint16(-qint16(layers.size())),2);
        for(auto &layer:layers){
            layer.maskBounds=layer.endGroup?QRect{}:document->maskBounds(layer.info.id).translated(layer.info.offset);if(layer.maskBounds.isEmpty()&&!layer.info.group)layer.maskBounds=layer.bounds;rectangle(out,layer.bounds);const int channels=layer.info.group?(layer.maskBounds.isEmpty()?0:1):5;number(out,channels,2);
            for(int c:{-1,0,1,2,-2})if(channels && (!layer.info.group || c==-2)){number(out,quint16(c),2);layer.patches<<out.pos();number(out,0,lengthBytes);}
            write(out,"8BIM");write(out,layer.info.group && layer.info.passThrough?QByteArray("pass"):blendKey(layer.info.blend));number(out,quint8(std::clamp(layer.info.opacity,0.f,1.f)*255+.5f),1);number(out,layer.info.clipping,1);number(out,layer.info.visible?8:10,1);number(out,0,1);
            const auto extraPatch=out.pos();number(out,0,4);const auto extraStart=out.pos();
            if(channels){number(out,20,4);rectangle(out,layer.maskBounds);number(out,layer.info.maskDefault,1);number(out,layer.info.maskEnabled?0:2,1);number(out,0,2);}else number(out,0,4);
            number(out,32,4);for(int c=0;c<4;++c)for(const auto *range:{&layer.info.blendIfSource[c],&layer.info.blendIfBackdrop[c]})for(float value:*range)number(out,quint8(std::clamp(value,0.f,1.f)*255+.5f),1);
            number(out,0,1);number(out,0,3);QByteArray unicode;const auto n=qToBigEndian(quint32(layer.info.name.size()));unicode.append(reinterpret_cast<const char *>(&n),4);for(const auto c:layer.info.name){const auto v=qToBigEndian(c.unicode());unicode.append(reinterpret_cast<const char *>(&v),2);}tag(out,"luni",unicode);
            QByteArray fill(4,0);fill[0]=char(std::clamp(layer.info.fillOpacity,0.f,1.f)*255+.5f);tag(out,"iOpa",fill);
            QByteArray restrictions;for(int c=0;c<3;++c)if(!(layer.info.channels&(1<<c))){const auto v=qToBigEndian(quint32(c));restrictions.append(reinterpret_cast<const char *>(&v),4);}if(!restrictions.isEmpty())tag(out,"brst",restrictions);
            if(layer.info.group){const auto v=qToBigEndian(quint32(layer.endGroup?3:1));QByteArray payload(reinterpret_cast<const char *>(&v),4);payload+="8BIM";payload+=layer.info.passThrough?"pass":blendKey(layer.info.blend);tag(out,"lsct",payload);}
            if(layer.info.locked){const auto v=qToBigEndian(quint32(7));tag(out,"lspf",QByteArray(reinterpret_cast<const char *>(&v),4));}
            patch(out,extraPatch,out.pos()-extraStart,4);
        }
        for(const auto &layer:layers)if(!layer.endGroup && (!layer.info.group || !layer.maskBounds.isEmpty())){
            int index=0;
            for(int c:{-1,0,1,2,-2})if(!layer.info.group || c==-2){
                const QRect local=(c==-2?layer.maskBounds:layer.bounds).translated(-layer.info.offset);const auto start=out.pos();number(out,3,2);ZipWriter zip(out);
                const int stripe=std::max(1,int((8*1024*1024)/(std::max(1,local.width())*(c==-2?2:depth/2))));
                for(int y=0;y<local.height();y+=stripe){const QRect rect(local.x(),local.y()+y,local.width(),std::min(stripe,local.height()-y));auto pixels=c==-2?document->maskRegion(layer.info.id,rect):document->readRegion(layer.info.id,rect);if(pixels.isNull())fail("Unable to read PSD layer for export");for(int row=0;row<pixels.height();++row){auto bytes=channelRow(pixels,row,c,depth);encodePredict(bytes,depth,local.width());zip.feed(bytes);}}
                zip.feed({},true);const auto length=out.pos()-start;if(!psb && length>0xffffffffLL)fail("Use PSB for a channel larger than 4 GiB");patch(out,layer.patches[index++],length,lengthBytes);
            }
        }
        if((out.pos()-layerStart)%2)number(out,0,1);const auto layerLength=out.pos()-layerStart;if(!psb && layerLength>0xffffffffLL)fail("Use PSB for a layer section larger than 4 GiB");patch(out,layerPatch,layerLength,lengthBytes);
        if(depth==8)number(out,0,4);else while((out.pos()-layerStart)%4)number(out,0,1);
        const auto outerLength=out.pos()-outerStart;if(!psb && outerLength>0xffffffffLL)fail("Use PSB for an outer section larger than 4 GiB");patch(out,outerPatch,outerLength,lengthBytes);
        // A valid merged RGBA image is mandatory for other readers and viewers.
        number(out,2,2);ZipWriter merged(out);
        const int stripe=std::max(1,int((8*1024*1024)/(std::max(1,width)*(depth/2))));
        for(int c:{0,1,2,-1})for(int y=0;y<height;y+=stripe){const auto pixels=document->composite(QRect(0,y,width,std::min(stripe,height-y)));if(pixels.isNull())fail("Unable to composite PSD preview");for(int row=0;row<pixels.height();++row)merged.feed(channelRow(pixels,row,c,depth));}
        merged.feed({},true);if(!out.commit())fail(out.errorString());document->history()->setClean();return {};
    }catch(const std::exception &e){return QString::fromUtf8(e.what());}
}
}
