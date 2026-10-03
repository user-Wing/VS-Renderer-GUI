#include "image/ImageDocument.h"
#include <QFile>
#include <QHash>
#include <QMap>
#include <QTemporaryDir>
#include <QThread>
#include <QUndoCommand>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <stdexcept>
#include <QCache>
#include <vector>

namespace vsr {
namespace {
using Pixel = std::array<float,4>;
Pixel sample(const QImage &image, int x, int y) {
    const auto *row=image.constScanLine(y);
    if(image.format()==QImage::Format_RGBA32FPx4) {Pixel p;std::memcpy(p.data(),row+x*16,16);return p;}
    if(image.format()==QImage::Format_RGBA64) {const auto *p=reinterpret_cast<const quint16 *>(row)+x*4;return {p[0]/65535.f,p[1]/65535.f,p[2]/65535.f,p[3]/65535.f};}
    const auto *p=row+x*4;return {p[0]/255.f,p[1]/255.f,p[2]/255.f,p[3]/255.f};
}
void put(QImage &image, int x, int y, const Pixel &p) {
    auto *row=image.scanLine(y);
    if(image.format()==QImage::Format_RGBA32FPx4){std::memcpy(row+x*16,p.data(),16);return;}
    if(image.format()==QImage::Format_RGBA64){auto *q=reinterpret_cast<quint16 *>(row)+x*4;for(int c=0;c<4;++c)q[c]=quint16(std::lround(std::clamp(p[c],0.f,1.f)*65535));}
    else {auto *q=row+x*4;for(int c=0;c<4;++c)q[c]=uchar(std::lround(std::clamp(p[c],0.f,1.f)*255));}
}
quint64 tileKey(int x,int y){return (quint64(quint32(x))<<32)|quint32(y);}
Pixel blend(Pixel b,Pixel s,float coverage,ImageBlendMode mode){
    const float a=std::clamp(s[3],0.f,1.f)*coverage,ba=std::clamp(b[3],0.f,1.f),alpha=a+ba*(1-a);
    Pixel result{};result[3]=alpha;
    if(alpha>0)for(int c=0;c<3;++c){
        float value=s[c];
        switch(mode){
        case ImageBlendMode::Multiply:value=b[c]*s[c];break;
        case ImageBlendMode::Screen:value=b[c]+s[c]-b[c]*s[c];break;
        case ImageBlendMode::Overlay:value=b[c]<=.5f?2*b[c]*s[c]:1-2*(1-b[c])*(1-s[c]);break;
        case ImageBlendMode::Darken:value=std::min(b[c],s[c]);break;
        case ImageBlendMode::Lighten:value=std::max(b[c],s[c]);break;
        case ImageBlendMode::ColorDodge:value=b[c]<=0?0:s[c]>=1?1:std::min(1.f,b[c]/(1-s[c]));break;
        case ImageBlendMode::ColorBurn:value=b[c]>=1?1:s[c]<=0?0:1-std::min(1.f,(1-b[c])/s[c]);break;
        case ImageBlendMode::LinearDodge:value=b[c]+s[c];break;
        case ImageBlendMode::LinearBurn:value=b[c]+s[c]-1;break;
        case ImageBlendMode::SoftLight:value=s[c]<=.5f?b[c]-(1-2*s[c])*b[c]*(1-b[c]):b[c]+(2*s[c]-1)*((b[c]<=.25f?((16*b[c]-12)*b[c]+4)*b[c]:std::sqrt(std::max(0.f,b[c])))-b[c]);break;
        case ImageBlendMode::HardLight:value=s[c]<=.5f?2*b[c]*s[c]:1-2*(1-b[c])*(1-s[c]);break;
        case ImageBlendMode::Difference:value=std::abs(b[c]-s[c]);break;
        case ImageBlendMode::Exclusion:value=b[c]+s[c]-2*b[c]*s[c];break;
        case ImageBlendMode::Subtract:value=b[c]-s[c];break;
        case ImageBlendMode::Divide:value=s[c]==0?1:b[c]/s[c];break;
        default:break;}
        result[c]=((1-a)*ba*b[c]+a*((1-ba)*s[c]+ba*value))/alpha;
    }
    return result;
}
int tileIndex(int value){return value>=0?value/ImageDocument::TileSize:(value+1)/ImageDocument::TileSize-1;}
QRect tileRect(int x,int y){return {x*ImageDocument::TileSize,y*ImageDocument::TileSize,ImageDocument::TileSize,ImageDocument::TileSize};}
float tonalCoverage(float value,const ImageBlendRange &range){
    float low=1,high=1;
    if(range[1]>0)low=range[1]>range[0]?std::clamp((value-range[0])/(range[1]-range[0]),0.f,1.f):float(value>=range[0]);
    if(range[2]<1)high=range[3]>range[2]?std::clamp((range[3]-value)/(range[3]-range[2]),0.f,1.f):float(value<=range[3]);
    return low*high;
}
float blendIfCoverage(const ImageLayerInfo &info,const Pixel &source,const Pixel &backdrop){
    float coverage=1;
    for(int c=0;c<4;++c){const float s=c?source[c-1]:.299f*source[0]+.587f*source[1]+.114f*source[2],b=c?backdrop[c-1]:.299f*backdrop[0]+.587f*backdrop[1]+.114f*backdrop[2];coverage*=tonalCoverage(s,info.blendIfSource[c])*tonalCoverage(b,info.blendIfBackdrop[c]);}
    return coverage;
}
Pixel restrictChannels(Pixel result,const Pixel &backdrop,quint8 channels){for(int c=0;c<3;++c)if(!(channels&(1<<c)))result[c]=backdrop[c];return result;}
}

struct ImageDocument::Impl {
    struct TileFile {QMutex mutex;QString path;std::shared_ptr<QTemporaryDir> directory;~TileFile(){QFile::remove(path);}};
    struct Block {
        quint64 id=0,used=0;
        std::shared_ptr<TileFile> file;
        QImage pixels;
        QImage::Format format;
        QColorSpace space;

    };
    using Blocks=QHash<quint64,std::shared_ptr<Block>>;
    struct Layer {ImageLayerInfo info;QRect bounds,maskBounds;Blocks pixels,mask;RegionReader reader,maskReader;PreviewReader preview,maskPreview;QualityPreviewReader qualityPreview,maskQualityPreview;};
    struct FrozenLayer {
        struct FrozenBlock {QImage pixels;std::shared_ptr<TileFile> file;QImage::Format format;QColorSpace space;};
        ImageLayerInfo info;QRect bounds;RegionReader reader,maskReader;QImage::Format format;QColorSpace space;
        QHash<quint64,FrozenBlock> pixels,mask;
        QMutex mutex;QCache<quint64,QImage> pixelCache{16*1024*1024},maskCache{4*1024*1024};
        explicit FrozenLayer(const Layer &layer,QImage::Format f,QColorSpace s):info(layer.info),bounds(layer.bounds),reader(layer.reader),maskReader(layer.maskReader),format(f),space(s){
            auto freeze=[](const Blocks &blocks,QHash<quint64,FrozenBlock> &out){for(auto it=blocks.cbegin();it!=blocks.cend();++it){const auto &b=*it.value();out.insert(it.key(),{b.pixels,b.file,b.format,b.space});}};
            freeze(layer.pixels,pixels);freeze(layer.mask,mask);
        }
        // Call under mutex. Only immutable image/file leases are retained; no live document/cache/QObject.
        QImage tile(int tx,int ty,bool coverage){
            const auto key=tileKey(tx,ty);auto &cache=coverage?maskCache:pixelCache;if(auto *image=cache.object(key))return *image;
            const auto &plane=coverage?mask:pixels;const auto stored=plane.constFind(key);QImage image;
            if(stored!=plane.cend()){
                image=stored->pixels;
                if(image.isNull()){
                    QMutexLocker lock(&stored->file->mutex);QFile file(stored->file->path);image=QImage(TileSize,TileSize,stored->format);
                    if(image.isNull() || !file.open(QIODevice::ReadOnly) || file.read(reinterpret_cast<char *>(image.bits()),image.sizeInBytes())!=image.sizeInBytes())throw std::runtime_error("Unable to read transformed image tile");
                    image.setColorSpace(stored->space);
                }
            }else {
                const auto &source=coverage?maskReader:reader;if(source)image=source(tileRect(tx,ty));
                else {image=QImage(TileSize,TileSize,coverage?QImage::Format_Grayscale16:format);if(coverage){for(int y=0;y<image.height();++y)std::fill_n(reinterpret_cast<quint16 *>(image.scanLine(y)),TileSize,quint16(info.maskDefault*257));}else image.fill(Qt::transparent);}
            }
            if(image.isNull() || image.size()!=QSize(TileSize,TileSize))throw std::runtime_error("Unable to read transformed image source");
            const auto required=coverage?QImage::Format_Grayscale16:format;if(image.format()!=required)image=image.convertToFormat(required);
            if(!coverage)image.setColorSpace(space);cache.insert(key,new QImage(image),int(image.sizeInBytes()));return image;
        }
        Pixel pixel(int x,int y){if(!bounds.contains(x,y))return {};const int tx=tileIndex(x),ty=tileIndex(y);return sample(tile(tx,ty,false),x-tx*TileSize,y-ty*TileSize);}
        float coverage(int x,int y){if(mask.isEmpty() && !maskReader)return info.maskDefault/255.f;const int tx=tileIndex(x),ty=tileIndex(y);const auto image=tile(tx,ty,true);return reinterpret_cast<const quint16 *>(image.constScanLine(y-ty*TileSize))[x-tx*TileSize]/65535.f;}
        Pixel interpolated(double x,double y){
            const int left=int(std::floor(x)),top=int(std::floor(y));const float fx=x-left,fy=y-top;Pixel sum{};std::array<float,3> hidden{};
            for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix){const float weight=(ix?fx:1-fx)*(iy?fy:1-fy);if(weight==0)continue;const auto p=pixel(left+ix,top+iy);const float a=std::clamp(p[3],0.f,1.f);sum[3]+=a*weight;for(int c=0;c<3;++c){sum[c]+=p[c]*a*weight;hidden[c]+=p[c]*weight;}}
            for(int c=0;c<3;++c)sum[c]=sum[3]>0?sum[c]/sum[3]:hidden[c];return sum;
        }
        float interpolatedMask(double x,double y){const int left=int(std::floor(x)),top=int(std::floor(y));const float fx=x-left,fy=y-top;float sum=0;for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix){const float weight=(ix?fx:1-fx)*(iy?fy:1-fy);if(weight>0)sum+=coverage(left+ix,top+iy)*weight;}return sum;}
        bool hasWrittenNear(double x,double y,bool coverage) const{const auto &plane=coverage?mask:pixels;if(plane.isEmpty())return false;const int left=int(std::floor(x)),top=int(std::floor(y));for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix)if(plane.contains(tileKey(tileIndex(left+ix),tileIndex(top+iy))))return true;return false;}
    };
    struct ClearFrozenCache {FrozenLayer *layer;~ClearFrozenCache(){layer->pixelCache.clear();layer->maskCache.clear();}};
    struct FrameProvider {
        std::shared_ptr<FrozenLayer> source;QRect sourceBounds;QSize targetSize;int turns=0,flipAxis=-1;
        QPointF sourcePoint(double x,double y) const{
            double u=x/targetSize.width(),v=y/targetSize.height(),a=u,b=v;
            if(turns==1){a=v;b=1-u;}else if(turns==2){a=1-u;b=1-v;}else if(turns==3){a=1-v;b=u;}
            if(flipAxis==0)a=1-a;else if(flipAxis==1)b=1-b;
            return {sourceBounds.x()+a*sourceBounds.width(),sourceBounds.y()+b*sourceBounds.height()};
        }
        QImage read(const QRect &rect,QSize output,bool mask){
            if(rect.isEmpty() || output.isEmpty())return {};
            QMutexLocker lock(&source->mutex);ClearFrozenCache clear{source.get()};QImage image(output,mask?QImage::Format_Grayscale16:source->format);if(image.isNull())throw std::runtime_error("Unable to allocate transformed image region");image.fill(Qt::transparent);if(!mask)image.setColorSpace(source->space);
            for(int y=0;y<output.height();++y)for(int x=0;x<output.width();++x){
                const double dx=rect.x()+(2.*x+1)*rect.width()/(2.*output.width()),dy=rect.y()+(2.*y+1)*rect.height()/(2.*output.height());
                if(!mask && (dx<0 || dy<0 || dx>=targetSize.width() || dy>=targetSize.height()))continue;
                const auto point=sourcePoint(dx,dy);double sx=point.x()-.5,sy=point.y()-.5;if(std::abs(sx-std::round(sx))<1e-9)sx=std::round(sx);if(std::abs(sy-std::round(sy))<1e-9)sy=std::round(sy);const int left=int(std::floor(sx)),top=int(std::floor(sy));const float fx=sx-left,fy=sy-top;
                if(!mask && fx==0 && fy==0){put(image,x,y,source->pixel(std::clamp(left,sourceBounds.left(),sourceBounds.right()),std::clamp(top,sourceBounds.top(),sourceBounds.bottom())));continue;}
                Pixel sum{};float coverage=0;std::array<float,3> hidden{};
                for(int iy=0;iy<2;++iy)for(int ix=0;ix<2;++ix){const float weight=(ix?fx:1-fx)*(iy?fy:1-fy);if(weight==0)continue;const int px=mask?left+ix:std::clamp(left+ix,sourceBounds.left(),sourceBounds.right()),py=mask?top+iy:std::clamp(top+iy,sourceBounds.top(),sourceBounds.bottom());
                    if(mask)coverage+=source->coverage(px,py)*weight;
                    else {const auto p=source->pixel(px,py);const float a=std::clamp(p[3],0.f,1.f);sum[3]+=a*weight;for(int c=0;c<3;++c){sum[c]+=p[c]*a*weight;hidden[c]+=p[c]*weight;}}
                }
                if(mask)reinterpret_cast<quint16 *>(image.scanLine(y))[x]=quint16(std::lround(std::clamp(coverage,0.f,1.f)*65535));
                else {for(int c=0;c<3;++c)sum[c]=sum[3]>0?sum[c]/sum[3]:hidden[c];put(image,x,y,sum);}
            }
            return image;
        }
    };
    struct MergeProvider {
        std::shared_ptr<FrozenLayer> bottom,top;QPoint origin;
        QImage read(const QRect &rect,QSize output){
            if(rect.isEmpty() || output.isEmpty())return {};
            QMutexLocker lowerLock(&bottom->mutex);QMutexLocker upperLock(&top->mutex);ClearFrozenCache clearBottom{bottom.get()},clearTop{top.get()};QImage image(output,bottom->format);if(image.isNull())throw std::runtime_error("Unable to allocate merged image region");image.fill(Qt::transparent);image.setColorSpace(bottom->space);
            for(int y=0;y<output.height();++y)for(int x=0;x<output.width();++x){
                const int dx=origin.x()+rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*output.width())),dy=origin.y()+rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*output.height()));Pixel result{};
                for(const auto &layer:{bottom,top}){const int lx=dx-layer->info.offset.x(),ly=dy-layer->info.offset.y();const auto p=layer->pixel(lx,ly);float coverage=layer->info.opacity*layer->info.fillOpacity;if(layer->info.maskEnabled)coverage*=layer->coverage(lx,ly);coverage*=blendIfCoverage(layer->info,p,result);result=restrictChannels(blend(result,p,coverage,layer->info.blend),result,layer->info.channels);}
                put(image,x,y,result);
            }
            return image;
        }
    };
    struct SceneProvider {
        std::vector<std::shared_ptr<FrozenLayer>> layers;QSize canvas;QImage::Format format;QColorSpace space;bool whiteBackground=false;
        QImage read(const QRect &rect,QSize output){
            if(rect.isEmpty() || output.isEmpty())return {};
            std::vector<std::unique_ptr<QMutexLocker<QMutex>>> locks;for(const auto &l:layers)locks.push_back(std::make_unique<QMutexLocker<QMutex>>(&l->mutex));
            struct Clear {std::vector<std::shared_ptr<FrozenLayer>> &layers;~Clear(){for(const auto &l:layers){l->pixelCache.clear();l->maskCache.clear();}}} clear{layers};
            auto blank=[&]{QImage image(output,format);if(image.isNull())throw std::runtime_error("Unable to allocate flattened image region");image.fill(Qt::transparent);image.setColorSpace(space);return image;};
            std::function<QImage(QUuid,QImage)> render=[&](QUuid parent,QImage out){QImage baseAlpha;
                for(const auto &l:layers){const auto &info=l->info;if(info.parentId!=parent || !info.visible || info.opacity<=0)continue;QImage source,groupAlpha;if(info.group){source=render(info.id,info.passThrough?out:blank());if(info.passThrough)groupAlpha=render(info.id,blank());}QImage thisAlpha=blank();
                    for(int y=0;y<output.height();++y)for(int x=0;x<output.width();++x){const int dx=rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*output.width())),dy=rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*output.height()));if(!QRect(QPoint(),canvas).contains(dx,dy))continue;const int lx=dx-info.offset.x(),ly=dy-info.offset.y();const auto p=info.group?sample(source,x,y):l->pixel(lx,ly),b=sample(out,x,y);float coverage=info.opacity*(info.group?1:info.fillOpacity)*blendIfCoverage(info,p,b);if(info.maskEnabled)coverage*=l->coverage(lx,ly);if(info.clipping && !baseAlpha.isNull())coverage*=sample(baseAlpha,x,y)[3];put(thisAlpha,x,y,{0,0,0,(groupAlpha.isNull()?p[3]:sample(groupAlpha,x,y)[3])*coverage});Pixel result;
                        if(info.group && info.passThrough){result={};result[3]=b[3]*(1-coverage)+p[3]*coverage;if(result[3]>0)for(int c=0;c<3;++c)result[c]=(b[c]*b[3]*(1-coverage)+p[c]*p[3]*coverage)/result[3];}else result=blend(b,p,coverage,info.blend);put(out,x,y,restrictChannels(result,b,info.channels));
                    }if(!info.clipping)baseAlpha=thisAlpha;
                }return out;};
            auto image=render({},blank());if(whiteBackground)for(int y=0;y<output.height();++y)for(int x=0;x<output.width();++x){const int dx=rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*output.width())),dy=rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*output.height()));if(QRect(QPoint(),canvas).contains(dx,dy))put(image,x,y,blend({1,1,1,1},sample(image,x,y),1,ImageBlendMode::Normal));}return image;
        }
    };
    struct State {QList<Layer> layers;Blocks selection;bool selected=false;quint64 selectionId=0;QSize size;ImagePrecision precision=ImagePrecision::UInt8;QColorSpace space;};
    ImageDocument *owner;
    QHash<QString,QString> metadata;
    State state,before;
    QUndoStack history;
    std::shared_ptr<QTemporaryDir> directory=std::make_shared<QTemporaryDir>();
    mutable QHash<quint64,std::weak_ptr<Block>> blocks;
    struct CacheEntry {std::weak_ptr<Block> block;qint64 bytes;};
    mutable QMap<quint64,CacheEntry> lru;
    mutable qint64 cachedBytes=0;
    mutable QString error;
    mutable quint64 clock=0,nextBlock=0;
    quint64 revision=0;
    quint64 nextSelectionId=0;
    qint64 budget=64*1024*1024;
    bool editing=false,modified=false;
    QString label;
    Impl(ImageDocument *o,QSize s,ImagePrecision p):owner(o){state.size=s;state.precision=p;history.setParent(o);history.setUndoLimit(100);if(!directory->isValid())error="Unable to create image tile storage";}
    QImage::Format format() const {return state.precision==ImagePrecision::Float32?QImage::Format_RGBA32FPx4:state.precision==ImagePrecision::UInt16?QImage::Format_RGBA64:QImage::Format_RGBA8888;}
    Layer *layer(const QUuid &id){for(auto &l:state.layers)if(l.info.id==id)return &l;return nullptr;}
    const Layer *layer(const QUuid &id) const {for(const auto &l:state.layers)if(l.info.id==id)return &l;return nullptr;}
    qint64 resident() const {for(auto it=lru.begin();it!=lru.end();)if(it->block.expired()){cachedBytes-=it->bytes;it=lru.erase(it);}else ++it;return cachedBytes;}
    void touch(const std::shared_ptr<Block> &b) const {
        auto old=lru.find(b->used);if(old!=lru.end()){cachedBytes-=old->bytes;lru.erase(old);}
        b->used=++clock;lru.insert(b->used,{b,b->pixels.sizeInBytes()});cachedBytes+=b->pixels.sizeInBytes();
    }
    void trim() const {
        while(cachedBytes>budget && !lru.isEmpty()){
            const auto it=lru.begin();const auto oldest=it->block.lock();
            if(!oldest){cachedBytes-=it->bytes;lru.erase(it);continue;}
            QMutexLocker fileLock(&oldest->file->mutex);
            if(!QFile::exists(oldest->file->path)){
                QFile file(oldest->file->path);
                if(!file.open(QIODevice::WriteOnly) || file.write(reinterpret_cast<const char *>(oldest->pixels.constBits()),oldest->pixels.sizeInBytes())!=oldest->pixels.sizeInBytes() || !file.flush()){
                    error="Unable to write image tile: "+file.errorString();file.close();QFile::remove(oldest->file->path);break;
                }
            }
            cachedBytes-=it->bytes;lru.erase(it);oldest->pixels={};oldest->used=0;
        }
    }
    std::shared_ptr<Block> store(const QImage &image){
        if(nextBlock%256==0)for(auto it=blocks.begin();it!=blocks.end();)if(it.value().expired())it=blocks.erase(it);else ++it;
        auto b=std::make_shared<Block>();b->id=++nextBlock;b->pixels=image;b->format=image.format();b->space=image.colorSpace();b->file=std::make_shared<TileFile>();b->file->directory=directory;b->file->path=directory->filePath(QString::number(b->id)+".tile");blocks.insert(b->id,b);touch(b);trim();return b;
    }
    QImage load(const std::shared_ptr<Block> &b) const {
        if(b->pixels.isNull()){
            QMutexLocker fileLock(&b->file->mutex);
            QImage image(TileSize,TileSize,b->format);QFile file(b->file->path);
            if(image.isNull() || !file.open(QIODevice::ReadOnly) || file.read(reinterpret_cast<char *>(image.bits()),image.sizeInBytes())!=image.sizeInBytes()) {error="Unable to read image tile: "+file.errorString();return {};}
            image.setColorSpace(b->space);b->pixels=image;
        }
        touch(b);const QImage result=b->pixels;trim();return result;
    }
    QImage region(const Blocks &plane,const QRect &rect,QImage::Format fmt,bool white) const {
        if(rect.isEmpty())return {};
        QImage out(rect.size(),fmt);if(out.isNull()){error="Unable to allocate image region";return {};}
        out.fill(white?Qt::white:Qt::transparent);if(fmt!=QImage::Format_Grayscale16)out.setColorSpace(state.space);
        const int bpp=out.depth()/8;
        for(int ty=tileIndex(rect.top());ty<=tileIndex(rect.bottom());++ty)for(int tx=tileIndex(rect.left());tx<=tileIndex(rect.right());++tx){
            const auto it=plane.constFind(tileKey(tx,ty));if(it==plane.cend())continue;
            const auto block=load(*it);if(block.isNull())return {};
            const QRect bounds=tileRect(tx,ty),part=rect.intersected(bounds);
            for(int y=part.top();y<=part.bottom();++y)std::memcpy(out.scanLine(y-rect.y())+(part.x()-rect.x())*bpp,block.constScanLine(y-bounds.y())+(part.x()-bounds.x())*bpp,size_t(part.width())*bpp);
        }
        return out;
    }
    void write(Blocks &plane,QPoint origin,const QImage &input,QImage::Format fmt,bool white){
        if(input.isNull())return;
        const QRect rect(origin,input.size());const int bpp=QImage(1,1,fmt).depth()/8;
        for(int ty=tileIndex(rect.top());ty<=tileIndex(rect.bottom());++ty)for(int tx=tileIndex(rect.left());tx<=tileIndex(rect.right());++tx){
            const auto key=tileKey(tx,ty);QImage block;
            if(plane.contains(key))block=load(plane[key]);else {block=QImage(TileSize,TileSize,fmt);block.fill(white?Qt::white:Qt::transparent);}
            if(block.isNull())return;
            const auto bounds=tileRect(tx,ty),part=rect.intersected(bounds);
            QImage converted;const QImage *image=&input;QPoint sourceOrigin=origin;
            if(input.format()!=fmt){converted=input.copy(part.translated(-origin)).convertToFormat(fmt);image=&converted;sourceOrigin=part.topLeft();if(converted.isNull()){error="Unable to convert image region";return;}}
            for(int y=part.top();y<=part.bottom();++y)std::memcpy(block.scanLine(y-bounds.y())+(part.x()-bounds.x())*bpp,image->constScanLine(y-sourceOrigin.y())+(part.x()-sourceOrigin.x())*bpp,size_t(part.width())*bpp);
            if(fmt!=QImage::Format_Grayscale16)block.setColorSpace(state.space);
            plane[key]=store(block);if(!error.isEmpty())return;
        }
        modified=true;++revision;
    }
    void apply(const State &s){state=s;++revision;emit owner->changed();}
    class Command final : public QUndoCommand {
        Impl *d;State before,after;
    public:
        Command(Impl *p,State b,State a,const QString &text):QUndoCommand(text),d(p),before(std::move(b)),after(std::move(a)){}
        void undo() override {d->apply(before);}
        void redo() override {d->apply(after);}
    };
};

ImageDocument::ImageDocument(QSize size,ImagePrecision precision,QObject *parent):QObject(parent),d(std::make_unique<Impl>(this,size,precision)){}
ImageDocument::~ImageDocument()=default;
QSize ImageDocument::size() const{return d->state.size;}
ImagePrecision ImageDocument::precision() const{return d->state.precision;}
QImage::Format ImageDocument::pixelFormat() const{return d->format();}
QColorSpace ImageDocument::colorSpace() const{return d->state.space;}
void ImageDocument::setColorSpace(const QColorSpace &space){const bool own=!d->editing;if(own)beginEdit(tr("工作色彩空间"));d->state.space=space;d->modified=true;++d->revision;if(own)commitEdit();}
void ImageDocument::setMetadata(const QString &key,const QString &value){d->metadata[key]=value;}
QString ImageDocument::metadata(const QString &key) const{return d->metadata.value(key);}
QList<ImageLayerInfo> ImageDocument::layers() const{QList<ImageLayerInfo> out;for(const auto &l:d->state.layers)out<<l.info;return out;}
void ImageDocument::beginEdit(const QString &label){Q_ASSERT(thread()==QThread::currentThread());Q_ASSERT(!d->editing);d->before=d->state;d->label=label;d->editing=true;d->modified=false;}
void ImageDocument::commitEdit(){
    if(!d->editing)return;
    if(!d->error.isEmpty()){cancelEdit();return;}
    d->editing=false;
    if(d->modified)d->history.push(new Impl::Command(d.get(),d->before,d->state,d->label));
    d->before={};d->modified=false;
}
void ImageDocument::cancelEdit(){if(!d->editing)return;d->state=d->before;d->before={};d->editing=false;d->modified=false;++d->revision;emit changed();}
QUuid ImageDocument::addLayer(const QString &name,const QImage &image,QPoint offset){
    const bool own=!d->editing;if(own)beginEdit(tr("添加图层"));
    Impl::Layer layer;layer.info.id=QUuid::createUuid();layer.info.name=name;layer.info.offset=offset;layer.info.extent=image.size();layer.bounds=QRect(QPoint(),image.size());
    if(!image.isNull())d->write(layer.pixels,{},image,d->format(),false);
    const auto id=layer.info.id;d->state.layers<<std::move(layer);d->modified=true;++d->revision;if(own)commitEdit();return id;
}
void ImageDocument::removeLayer(const QUuid &id){
    const bool own=!d->editing;if(own)beginEdit(tr("删除图层"));
    QSet<QUuid> ids{id};bool added=true;while(added){added=false;for(const auto &l:d->state.layers)if(ids.contains(l.info.parentId) && !ids.contains(l.info.id)){ids.insert(l.info.id);added=true;}}
    for(qsizetype i=d->state.layers.size();i-->0;)if(ids.contains(d->state.layers[i].info.id)){d->state.layers.removeAt(i);d->modified=true;++d->revision;}
    if(own)commitEdit();
}
QUuid ImageDocument::addLazyLayer(const QString &name,QSize size,QPoint offset,RegionReader reader,PreviewReader preview,QualityPreviewReader qualityPreview){
    const bool own=!d->editing;if(own)beginEdit(tr("导入图层"));
    const auto id=addLayer(name,{},offset);auto *l=d->layer(id);l->info.extent=size;l->bounds=QRect(QPoint(),size);l->reader=std::move(reader);l->preview=std::move(preview);l->qualityPreview=std::move(qualityPreview);
    if(own)commitEdit();return id;
}
void ImageDocument::setLazyMask(const QUuid &id,RegionReader reader,PreviewReader preview,QRect bounds,QualityPreviewReader qualityPreview){if(auto *l=d->layer(id)){l->maskReader=std::move(reader);l->maskPreview=std::move(preview);l->maskBounds=bounds;l->maskQualityPreview=std::move(qualityPreview);}}
QRect ImageDocument::maskBounds(const QUuid &id) const{const auto *l=d->layer(id);return l?l->maskBounds:QRect{};}
QList<QRect> ImageDocument::layerRegions(const QUuid &id) const{
    QList<QRect> out;const auto *l=d->layer(id);if(!l)return out;QSet<quint64> keys;
    if(l->reader)for(int y=0;y<l->info.extent.height();y+=TileSize)for(int x=0;x<l->info.extent.width();x+=TileSize)keys.insert(tileKey(x/TileSize,y/TileSize));
    for(auto it=l->pixels.cbegin();it!=l->pixels.cend();++it)keys.insert(it.key());
    for(auto key:keys){const auto rect=tileRect(int(quint32(key>>32)),int(quint32(key))).intersected(l->bounds);if(!rect.isEmpty())out<<rect;}return out;
}
QRect ImageDocument::layerBounds(const QUuid &id) const{const auto *l=d->layer(id);return l?l->bounds:QRect{};}
QUuid ImageDocument::duplicateLayer(const QUuid &id){
    if(!d->layer(id))return {};
    const bool own=!d->editing;if(own)beginEdit(tr("复制图层"));QHash<QUuid,QUuid> ids{{id,QUuid::createUuid()}};bool added=true;
    while(added){added=false;for(const auto &l:d->state.layers)if(ids.contains(l.info.parentId) && !ids.contains(l.info.id)){ids.insert(l.info.id,QUuid::createUuid());added=true;}}
    const auto original=d->state.layers;for(auto l:original)if(ids.contains(l.info.id)){const auto previous=l.info.id;l.info.id=ids.value(previous);if(ids.contains(l.info.parentId))l.info.parentId=ids.value(l.info.parentId);if(previous==id)l.info.name+=tr(" 副本");d->state.layers<<std::move(l);}
    d->modified=true;++d->revision;if(own)commitEdit();return ids.value(id);
}
bool ImageDocument::reframeLayer(const QUuid &id,const QRect &target,int turns,int flipAxis,QString *error){
    auto reject=[&](const QString &message){if(error)*error=message;return false;};
    const auto *selected=d->layer(id);if(!selected || target.isEmpty())return reject(tr("Choose a layer and a non-empty target rectangle"));
    QSet<QUuid> ids{id};bool added=true;while(added){added=false;for(const auto &l:d->state.layers)if(ids.contains(l.info.parentId) && !ids.contains(l.info.id)){ids.insert(l.info.id);added=true;}}
    QRect original;
    for(const auto &l:d->state.layers)if(ids.contains(l.info.id)){
        if(l.info.locked)return reject(tr("An affected layer or group is locked"));
        for(auto parent=l.info.parentId;!parent.isNull();){const auto *p=d->layer(parent);if(!p)break;if(p->info.locked)return reject(tr("A parent group is locked"));parent=p->info.parentId;}
        if(!l.info.group)original=original.united(l.bounds.translated(l.info.offset));
    }
    if(original.isEmpty())return reject(tr("The selected layer has no pixel bounds"));
    const bool own=!d->editing;if(own)beginEdit(tr("变换图层"));
    if(!turns && flipAxis<0 && target.size()==original.size()){
        const auto delta=target.topLeft()-original.topLeft();if(!delta.isNull()){for(auto &l:d->state.layers)if(ids.contains(l.info.id))l.info.offset+=delta;d->modified=true;++d->revision;}
        if(own)commitEdit();return true;
    }
    auto mappedPoint=[&](QPointF p){double u=(p.x()-original.x())/original.width(),v=(p.y()-original.y())/original.height();if(flipAxis==0)u=1-u;else if(flipAxis==1)v=1-v;double a=u,b=v;if(turns==1){a=1-v;b=u;}else if(turns==2){a=1-u;b=1-v;}else if(turns==3){a=v;b=1-u;}return QPointF(target.x()+a*target.width(),target.y()+b*target.height());};
    auto mappedRect=[&](QRect rect){if(rect.isEmpty())return QRect{};const QPointF corners[]={QPointF(rect.x(),rect.y()),QPointF(rect.x()+rect.width(),rect.y()),QPointF(rect.x(),rect.y()+rect.height()),QPointF(rect.x()+rect.width(),rect.y()+rect.height())};double left=std::numeric_limits<double>::max(),top=left,right=-left,bottom=-left;for(auto p:corners){p=mappedPoint(p);left=std::min(left,p.x());top=std::min(top,p.y());right=std::max(right,p.x());bottom=std::max(bottom,p.y());}return QRect(qRound(left),qRound(top),std::max(1,qRound(right)-qRound(left)),std::max(1,qRound(bottom)-qRound(top)));};
    for(auto &layer:d->state.layers)if(ids.contains(layer.info.id)){
        if(!layer.info.group && layer.bounds.isEmpty())continue;
        auto provider=std::make_shared<Impl::FrameProvider>();provider->source=std::make_shared<Impl::FrozenLayer>(layer,d->format(),d->state.space);
        const QRect oldAbsolute=layer.info.group?original:layer.bounds.translated(layer.info.offset);
        const QRect newAbsolute=layer.info.group?target:mappedRect(oldAbsolute);
        provider->sourceBounds=oldAbsolute.translated(-layer.info.offset);provider->targetSize=newAbsolute.size();provider->turns=turns;provider->flipAxis=flipAxis;
        const auto newMask=mappedRect(layer.maskBounds.translated(layer.info.offset)).translated(-newAbsolute.topLeft());
        const bool hasMask=!layer.maskBounds.isEmpty() || !layer.mask.isEmpty() || bool(layer.maskReader);
        layer.info.offset=newAbsolute.topLeft();layer.pixels.clear();layer.mask.clear();layer.qualityPreview={};layer.maskQualityPreview={};
        if(!layer.info.group){layer.bounds=QRect(QPoint(),newAbsolute.size());layer.info.extent=newAbsolute.size();layer.reader=[provider](const QRect &r){return provider->read(r,r.size(),false);};layer.preview=[provider](const QRect &r,QSize size){return provider->read(r,size,false);};}
        if(hasMask){layer.maskBounds=newMask;layer.maskReader=[provider](const QRect &r){return provider->read(r,r.size(),true);};layer.maskPreview=[provider](const QRect &r,QSize size){return provider->read(r,size,true);};}
    }
    d->modified=true;++d->revision;if(own)commitEdit();return true;
}
bool ImageDocument::transformLayer(const QUuid &id,const QRect &target,QString *error){return reframeLayer(id,target,0,-1,error);}
bool ImageDocument::flipLayer(const QUuid &id,bool horizontal,QString *error){
    const auto *selected=d->layer(id);if(!selected){if(error)*error=tr("No layer selected");return false;}QSet<QUuid> ids{id};bool added=true;while(added){added=false;for(const auto &l:d->state.layers)if(ids.contains(l.info.parentId) && !ids.contains(l.info.id)){ids.insert(l.info.id);added=true;}}QRect bounds;for(const auto &l:d->state.layers)if(ids.contains(l.info.id) && !l.info.group)bounds=bounds.united(l.bounds.translated(l.info.offset));return reframeLayer(id,bounds,0,horizontal?0:1,error);
}
bool ImageDocument::rotateLayer(const QUuid &id,int quarterTurns,QString *error){
    const int turns=(quarterTurns%4+4)%4;QSet<QUuid> ids{id};bool added=true;while(added){added=false;for(const auto &l:d->state.layers)if(ids.contains(l.info.parentId) && !ids.contains(l.info.id)){ids.insert(l.info.id);added=true;}}QRect bounds;for(const auto &l:d->state.layers)if(ids.contains(l.info.id) && !l.info.group)bounds=bounds.united(l.bounds.translated(l.info.offset));QRect target=bounds;if(turns%2)target=QRect(bounds.x()+(bounds.width()-bounds.height())/2,bounds.y()+(bounds.height()-bounds.width())/2,bounds.height(),bounds.width());return reframeLayer(id,target,turns,-1,error);
}
QUuid ImageDocument::mergeDown(const QUuid &id,QString *error){
    auto reject=[&](const QString &message){if(error)*error=message;return QUuid{};};
    int upper=-1,lower=-1;for(int i=0;i<d->state.layers.size();++i)if(d->state.layers[i].info.id==id){upper=i;break;}
    if(upper<0)return reject(tr("No layer selected"));const auto parent=d->state.layers[upper].info.parentId;
    for(int i=upper-1;i>=0;--i)if(d->state.layers[i].info.parentId==parent){lower=i;break;}
    if(lower<0)return reject(tr("There is no lower layer in the same group"));
    for(int index:{lower,upper}){const auto &l=d->state.layers[index];if(l.info.group || l.info.locked || !l.info.visible)return reject(tr("Merge down requires two visible, unlocked pixel layers"));if(l.info.blend!=ImageBlendMode::Normal || l.info.clipping || l.info.channels!=7 || std::any_of(l.info.blendIfBackdrop.cbegin(),l.info.blendIfBackdrop.cend(),[](const auto &range){return range!=ImageBlendRange{0,0,1,1};}))return reject(tr("Merge down requires Normal blending without clipping, channel restrictions or backdrop Blend If; use Merge Visible to bake advanced compositing"));}
    for(auto ancestor=parent;!ancestor.isNull();){const auto *p=d->layer(ancestor);if(!p)break;if(p->info.locked)return reject(tr("A parent group is locked"));ancestor=p->info.parentId;}
    for(int i=upper+1;i<d->state.layers.size();++i)if(d->state.layers[i].info.parentId==parent){if(d->state.layers[i].info.clipping)return reject(tr("A clipping layer above depends on this layer; merge it explicitly first"));break;}
    auto &bottom=d->state.layers[lower];const auto &top=d->state.layers[upper];const QRect bounds=bottom.bounds.translated(bottom.info.offset).united(top.bounds.translated(top.info.offset));if(bounds.isEmpty())return reject(tr("The layers have no pixel bounds"));
    auto provider=std::make_shared<Impl::MergeProvider>();provider->bottom=std::make_shared<Impl::FrozenLayer>(bottom,d->format(),d->state.space);provider->top=std::make_shared<Impl::FrozenLayer>(top,d->format(),d->state.space);provider->origin=bounds.topLeft();
    const auto mergedId=bottom.info.id;const bool own=!d->editing;if(own)beginEdit(tr("向下合并图层"));auto &destination=d->state.layers[lower];
    destination.info.offset=bounds.topLeft();destination.info.extent=bounds.size();destination.info.opacity=1;destination.info.fillOpacity=1;destination.info.blendIfSource=ImageLayerInfo{}.blendIfSource;destination.info.maskEnabled=true;destination.info.maskDefault=255;destination.bounds=QRect(QPoint(),bounds.size());destination.maskBounds={};destination.pixels.clear();destination.mask.clear();destination.maskReader={};destination.maskPreview={};destination.qualityPreview={};destination.maskQualityPreview={};destination.reader=[provider](const QRect &r){return provider->read(r,r.size());};destination.preview=[provider](const QRect &r,QSize size){return provider->read(r,size);};d->state.layers.removeAt(upper);d->modified=true;++d->revision;if(own)commitEdit();return mergedId;
}
QUuid ImageDocument::createGroup(const QString &name,const QUuid &parentId){
    if(!parentId.isNull()){const auto *parent=d->layer(parentId);if(!parent || !parent->info.group || parent->info.locked)return {};}
    const bool own=!d->editing;if(own)beginEdit(tr("创建图层组"));const auto id=addLayer(name);auto *group=d->layer(id);group->info.group=true;group->info.passThrough=true;group->info.parentId=parentId;if(own)commitEdit();return id;
}
QUuid ImageDocument::groupLayer(const QUuid &id,const QString &name,QString *error){
    const auto *layer=d->layer(id);if(!layer || layer->info.locked){if(error)*error=tr("Choose an unlocked layer");return {};}
    const auto parent=layer->info.parentId;for(auto ancestor=parent;!ancestor.isNull();){const auto *p=d->layer(ancestor);if(!p)break;if(p->info.locked){if(error)*error=tr("A parent group is locked");return {};}ancestor=p->info.parentId;}
    if(layer->info.clipping){if(error)*error=tr("Group the clipping base and its dependent layers together");return {};}
    for(int i=0;i<d->state.layers.size();++i)if(d->state.layers[i].info.id==id){for(int next=i+1;next<d->state.layers.size();++next)if(d->state.layers[next].info.parentId==parent){if(d->state.layers[next].info.clipping){if(error)*error=tr("A clipping layer above depends on this layer");return {};}break;}break;}
    const bool own=!d->editing;if(own)beginEdit(tr("从图层创建组"));const auto group=createGroup(name,parent);auto *current=d->layer(id);current->info.parentId=group;
    int index=0;for(;index<d->state.layers.size();++index)if(d->state.layers[index].info.id==id)break;auto created=d->state.layers.takeLast();d->state.layers.insert(index,created);d->modified=true;++d->revision;if(own)commitEdit();return group;
}
QUuid ImageDocument::mergeVisible(QString *error){
    QSet<QUuid> visible;for(const auto &l:d->state.layers){bool shown=l.info.visible;for(auto parent=l.info.parentId;shown && !parent.isNull();){const auto *p=d->layer(parent);if(!p)break;shown=p->info.visible;parent=p->info.parentId;}if(shown)visible.insert(l.info.id);}
    if(visible.isEmpty()){if(error)*error=tr("There are no visible layers");return {};}
    auto scene=std::make_shared<Impl::SceneProvider>();scene->canvas=size();scene->format=d->format();scene->space=colorSpace();for(const auto &l:d->state.layers)scene->layers.push_back(std::make_shared<Impl::FrozenLayer>(l,d->format(),colorSpace()));
    const bool own=!d->editing;if(own)beginEdit(tr("合并可见图层"));QList<Impl::Layer> hidden;for(auto l:d->state.layers)if(!visible.contains(l.info.id)){if(visible.contains(l.info.parentId)){l.info.parentId=QUuid{};l.info.visible=false;}hidden<<std::move(l);}d->state.layers=std::move(hidden);
    const auto id=addLazyLayer(tr("合并可见图层"),size(),{},[scene](const QRect &r){return scene->read(r,r.size());},[scene](const QRect &r,QSize output){return scene->read(r,output);});if(own)commitEdit();return id;
}
QUuid ImageDocument::flatten(QString *error){
    if(d->state.layers.isEmpty()){if(error)*error=tr("There are no layers to flatten");return {};}
    auto scene=std::make_shared<Impl::SceneProvider>();scene->canvas=size();scene->format=d->format();scene->space=colorSpace();scene->whiteBackground=true;for(const auto &l:d->state.layers)scene->layers.push_back(std::make_shared<Impl::FrozenLayer>(l,d->format(),colorSpace()));
    const bool own=!d->editing;if(own)beginEdit(tr("拼合图像"));d->state.layers.clear();const auto id=addLazyLayer(tr("背景"),size(),{},[scene](const QRect &r){return scene->read(r,r.size());},[scene](const QRect &r,QSize output){return scene->read(r,output);});d->layer(id)->info.locked=true;if(own)commitEdit();return id;
}
void ImageDocument::resizeCanvas(QSize size,QPoint offset){
    if(size.isEmpty())return;const bool own=!d->editing;if(own)beginEdit(tr("画布大小"));
    d->state.size=size;for(auto &l:d->state.layers)l.info.offset+=offset;
    if(!offset.isNull() && d->state.selected){Impl::Blocks shifted;for(auto it=d->state.selection.cbegin();it!=d->state.selection.cend();++it){const auto rect=tileRect(int(quint32(it.key()>>32)),int(quint32(it.key())));d->write(shifted,rect.topLeft()+offset,d->load(it.value()),QImage::Format_Grayscale16,false);}d->state.selection=std::move(shifted);d->state.selectionId=++d->nextSelectionId;}
    d->modified=true;++d->revision;if(own)commitEdit();
}
void ImageDocument::crop(const QRect &rect){const auto clipped=rect.intersected(QRect(QPoint(),size()));if(!clipped.isEmpty())resizeCanvas(clipped.size(),-clipped.topLeft());}
void ImageDocument::convertPrecision(ImagePrecision precision){
    if(precision==d->state.precision)return;const bool own=!d->editing;if(own)beginEdit(tr("转换精度"));d->state.precision=precision;
    for(auto &l:d->state.layers){
        const auto format=d->format();if(l.reader){const auto reader=l.reader;l.reader=[reader,format](const QRect &r){return reader(r).convertToFormat(format);};}if(l.preview){const auto preview=l.preview;l.preview=[preview,format](const QRect &r,QSize size){return preview(r,size).convertToFormat(format);};}if(l.qualityPreview){const auto preview=l.qualityPreview;l.qualityPreview=[preview,format](const QRect &r,QSize size,ImageSamplingQuality quality){return preview(r,size,quality).convertToFormat(format);};}
        for(auto it=l.pixels.begin();it!=l.pixels.end();++it)it.value()=d->store(d->load(it.value()).convertToFormat(format));
    }
    d->modified=true;++d->revision;if(own)commitEdit();
}
std::unique_ptr<ImageDocument> ImageDocument::snapshot() const{
    auto result=std::make_unique<ImageDocument>(size(),precision());auto *copy=result->d.get();copy->state=d->state;copy->metadata=d->metadata;copy->budget=d->budget;
    QHash<const Impl::Block *,std::shared_ptr<Impl::Block>> cloned;
    auto clone=[&](Impl::Blocks &plane){for(auto it=plane.begin();it!=plane.end();++it){const auto original=it.value();if(!cloned.contains(original.get())){auto block=std::make_shared<Impl::Block>(*original);block->used=0;block->id=++copy->nextBlock;copy->blocks.insert(block->id,block);if(!block->pixels.isNull())copy->touch(block);cloned.insert(original.get(),block);}it.value()=cloned.value(original.get());}};
    for(auto &layer:copy->state.layers){clone(layer.pixels);clone(layer.mask);}clone(copy->state.selection);copy->trim();return result;
}
void ImageDocument::updateLayer(const ImageLayerInfo &info,const QString &label){
    const bool own=!d->editing;if(own)beginEdit(label.isEmpty()?tr("图层属性"):label);
    if(auto *layer=d->layer(info.id)){layer->info=info;layer->info.opacity=std::clamp(info.opacity,0.f,1.f);layer->info.fillOpacity=std::clamp(info.fillOpacity,0.f,1.f);layer->info.channels&=7;for(auto *ranges:{&layer->info.blendIfSource,&layer->info.blendIfBackdrop})for(auto &range:*ranges){for(auto &value:range)value=std::clamp(value,0.f,1.f);range[1]=std::max(range[0],range[1]);range[3]=std::max(range[2],range[3]);}d->modified=true;++d->revision;}
    if(own)commitEdit();
}
void ImageDocument::moveLayer(const QUuid &id,int index){
    const bool own=!d->editing;if(own)beginEdit(tr("图层排序"));
    for(qsizetype i=0;i<d->state.layers.size();++i)if(d->state.layers[i].info.id==id){d->state.layers.move(i,std::clamp(index,0,int(d->state.layers.size())-1));d->modified=true;++d->revision;break;}
    if(own)commitEdit();
}
QImage ImageDocument::readRegion(const QUuid &id,const QRect &rect) const{const auto *l=d->layer(id);if(!l)return {};QImage out;
    if(l->reader){try{out=l->reader(rect);}catch(const std::exception &e){d->error=QString::fromUtf8(e.what());return {};}if(out.format()!=d->format())out=out.convertToFormat(d->format());out.setColorSpace(d->state.space);}
    else return d->region(l->pixels,rect,d->format(),false);
    if(out.isNull())return {};
    const int bpp=out.depth()/8;
    for(auto it=l->pixels.cbegin();it!=l->pixels.cend();++it){const int tx=int(quint32(it.key()>>32)),ty=int(quint32(it.key()));const QRect bounds=tileRect(tx,ty),part=rect.intersected(bounds);if(part.isEmpty())continue;const auto block=d->load(it.value());if(block.isNull())return {};for(int y=part.top();y<=part.bottom();++y)std::memcpy(out.scanLine(y-rect.y())+(part.x()-rect.x())*bpp,block.constScanLine(y-bounds.y())+(part.x()-bounds.x())*bpp,size_t(part.width())*bpp);}
    return out;}
void ImageDocument::writeRegion(const QUuid &id,QPoint origin,const QImage &pixels,const QString &label){
    const bool own=!d->editing;if(own)beginEdit(label.isEmpty()?tr("编辑像素"):label);
    if(auto *l=d->layer(id)){if(l->info.group || l->info.locked){if(own)cancelEdit();return;}
        if(l->reader){const QRect rect(origin,pixels.size());for(int ty=tileIndex(rect.top());ty<=tileIndex(rect.bottom());++ty)for(int tx=tileIndex(rect.left());tx<=tileIndex(rect.right());++tx){const auto key=tileKey(tx,ty);if(!l->pixels.contains(key)){const auto base=readRegion(id,tileRect(tx,ty));if(base.isNull()){if(own)cancelEdit();return;}l->pixels[key]=d->store(base);}}}
        d->write(l->pixels,origin,pixels,d->format(),false);l->bounds=l->bounds.united(QRect(origin,pixels.size()));
    }
    if(own)commitEdit();
}
QImage ImageDocument::readOriginalRegion(const QUuid &id,const QRect &rect) const{
    if(!d->editing)return readRegion(id,rect);
    std::swap(d->state,d->before);
    struct Restore{Impl *d;~Restore(){std::swap(d->state,d->before);}} restore{d.get()};
    return readRegion(id,rect);
}
QImage ImageDocument::maskRegion(const QUuid &id,const QRect &rect) const{const auto *l=d->layer(id);if(!l)return {};if(!l->maskReader)return d->region(l->mask,rect,QImage::Format_Grayscale16,l->info.maskDefault!=0);
    QImage out;try{out=l->maskReader(rect);}catch(const std::exception &e){d->error=QString::fromUtf8(e.what());return {};}if(out.isNull())return {};for(auto it=l->mask.cbegin();it!=l->mask.cend();++it){const auto bounds=tileRect(int(quint32(it.key()>>32)),int(quint32(it.key()))),part=bounds.intersected(rect);if(part.isEmpty())continue;auto block=d->load(it.value());if(block.isNull())return {};for(int y=part.top();y<=part.bottom();++y)std::memcpy(out.scanLine(y-rect.y())+(part.x()-rect.x())*2,block.constScanLine(y-bounds.y())+(part.x()-bounds.x())*2,size_t(part.width())*2);}return out;}
void ImageDocument::writeMask(const QUuid &id,QPoint origin,const QImage &coverage,const QString &label){
    const bool own=!d->editing;if(own)beginEdit(label.isEmpty()?tr("编辑蒙版"):label);
    if(auto *l=d->layer(id)){if(l->maskReader){const QRect rect(origin,coverage.size());for(int ty=tileIndex(rect.top());ty<=tileIndex(rect.bottom());++ty)for(int tx=tileIndex(rect.left());tx<=tileIndex(rect.right());++tx){const auto key=tileKey(tx,ty);if(!l->mask.contains(key)){const auto base=maskRegion(id,tileRect(tx,ty));if(base.isNull()){if(own)cancelEdit();return;}l->mask[key]=d->store(base);}}}d->write(l->mask,origin,coverage,QImage::Format_Grayscale16,l->info.maskDefault!=0);l->maskBounds=l->maskBounds.united(QRect(origin,coverage.size()));}
    if(own)commitEdit();
}
QImage ImageDocument::selectionRegion(const QRect &rect) const{return d->region(d->state.selection,rect,QImage::Format_Grayscale16,!d->state.selected);}
QImage ImageDocument::maskPreview(const QUuid &id,const QRect &rect,QSize outputSize,ImageSamplingQuality quality) const{
    const auto *layer=d->layer(id);if(!layer || rect.isEmpty() || outputSize.isEmpty())return {};
    QImage out;
    if(quality==ImageSamplingQuality::Bilinear){try{Impl::FrozenLayer source(*layer,d->format(),d->state.space);if(layer->maskQualityPreview)out=layer->maskQualityPreview(rect,outputSize,quality);const bool preview=!out.isNull();if(!preview)out=QImage(outputSize,QImage::Format_Grayscale16);if(out.isNull())return {};for(int y=0;y<out.height();++y)for(int x=0;x<out.width();++x){const double sx=rect.x()+(2.*x+1)*rect.width()/(2.*out.width())-.5,sy=rect.y()+(2.*y+1)*rect.height()/(2.*out.height())-.5;if(!preview || source.hasWrittenNear(sx,sy,true))reinterpret_cast<quint16 *>(out.scanLine(y))[x]=quint16(std::lround(source.interpolatedMask(sx,sy)*65535));}return out;}catch(const std::exception &e){d->error=QString::fromUtf8(e.what());return {};}}
    try{if(layer->maskPreview)out=layer->maskPreview(rect,outputSize);}catch(const std::exception &e){d->error=QString::fromUtf8(e.what());return {};}
    if(out.isNull()){out=QImage(outputSize,QImage::Format_Grayscale16);out.fill(layer->info.maskDefault?Qt::white:Qt::black);}
    quint64 previous=std::numeric_limits<quint64>::max();QImage mask;
    for(int y=0;y<out.height();++y)for(int x=0;x<out.width();++x){
        const int lx=rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*out.width())),ly=rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*out.height()));
        const int tx=tileIndex(lx),ty=tileIndex(ly);const auto key=tileKey(tx,ty);if(previous!=key){previous=key;mask=layer->mask.contains(key)?d->load(layer->mask[key]):QImage{};}
        if(!mask.isNull())reinterpret_cast<quint16 *>(out.scanLine(y))[x]=reinterpret_cast<const quint16 *>(mask.constScanLine(ly-ty*TileSize))[lx-tx*TileSize];
    }
    return out;
}
void ImageDocument::writeSelection(QPoint origin,const QImage &coverage,const QString &label){
    const bool own=!d->editing;if(own)beginEdit(label.isEmpty()?tr("选区"):label);
    if(!coverage.isNull()){d->state.selected=true;d->state.selectionId=++d->nextSelectionId;d->write(d->state.selection,origin,coverage,QImage::Format_Grayscale16,false);}
    if(own)commitEdit();
}
void ImageDocument::clearSelection(){const bool own=!d->editing;if(own)beginEdit(tr("取消选区"));if(d->state.selected){d->state.selection.clear();d->state.selected=false;d->state.selectionId=0;d->modified=true;++d->revision;}if(own)commitEdit();}
bool ImageDocument::hasSelection() const{return d->state.selected;}
quint64 ImageDocument::selectionId() const{return d->state.selectionId;}
QImage ImageDocument::composite(const QRect &rect) const{return compositePreview(rect,rect.size());}
QImage ImageDocument::compositePreview(const QRect &rect,QSize outputSize,ImageSamplingQuality quality) const{
    if(rect.isEmpty() || outputSize.isEmpty())return {};
    auto blank=[&]{QImage image(outputSize,d->format());image.fill(Qt::transparent);image.setColorSpace(d->state.space);return image;};
    std::function<QImage(QUuid,QImage)> render=[&](QUuid parent,QImage out){
        QImage baseAlpha;
        for(const auto &layer:d->state.layers){
            if(layer.info.parentId!=parent || !layer.info.visible || layer.info.opacity<=0)continue;
            QImage source;
            std::unique_ptr<Impl::FrozenLayer> filtered;if(quality==ImageSamplingQuality::Bilinear)filtered=std::make_unique<Impl::FrozenLayer>(layer,d->format(),d->state.space);
            QImage groupAlpha;
            if(layer.info.group){if(layer.info.passThrough){source=render(layer.info.id,out);groupAlpha=render(layer.info.id,blank());}else source=render(layer.info.id,blank());}
            else if(filtered && layer.qualityPreview)source=layer.qualityPreview(rect.translated(-layer.info.offset),outputSize,quality).convertToFormat(d->format());
            else if(!filtered && layer.preview)source=layer.preview(rect.translated(-layer.info.offset),outputSize).convertToFormat(d->format());
            else if(!filtered && layer.reader && outputSize==rect.size())source=readRegion(layer.info.id,rect.translated(-layer.info.offset));
            if(source.isNull() && layer.pixels.isEmpty() && !layer.reader)continue;
            quint64 previous=std::numeric_limits<quint64>::max();QImage pixels,mask;
            QImage thisAlpha=blank();QImage sourceMask;if(layer.info.maskEnabled){if(filtered && layer.maskQualityPreview)sourceMask=layer.maskQualityPreview(rect.translated(-layer.info.offset),outputSize,quality);else if(!filtered && layer.maskPreview)sourceMask=layer.maskPreview(rect.translated(-layer.info.offset),outputSize);}
            for(int y=0;y<out.height();++y)for(int x=0;x<out.width();++x){
                const int dx=rect.x()+int((qint64(2)*x+1)*rect.width()/(qint64(2)*out.width())),dy=rect.y()+int((qint64(2)*y+1)*rect.height()/(qint64(2)*out.height()));
                if(!QRect(QPoint(),d->state.size).contains(dx,dy))continue;
                const int lx=dx-layer.info.offset.x(),ly=dy-layer.info.offset.y(),tx=tileIndex(lx),ty=tileIndex(ly);const auto key=tileKey(tx,ty);
                if(!filtered && previous!=key){previous=key;const auto it=layer.pixels.constFind(key);pixels=it==layer.pixels.cend()?QImage{}:d->load(*it);mask={};if(layer.info.maskEnabled && layer.mask.contains(key))mask=d->load(layer.mask[key]);}
                if(!d->error.isEmpty())return QImage{};
                const int sx=lx-tx*TileSize,sy=ly-ty*TileSize;Pixel p{};
                const double fx=rect.x()+(2.*x+1)*rect.width()/(2.*out.width())-.5-layer.info.offset.x(),fy=rect.y()+(2.*y+1)*rect.height()/(2.*out.height())-.5-layer.info.offset.y();
                if(filtered && !layer.info.group && (source.isNull() || filtered->hasWrittenNear(fx,fy,false)))p=filtered->interpolated(fx,fy);
                else if(!pixels.isNull())p=sample(pixels,sx,sy);
                else if(!source.isNull())p=sample(source,x,y);
                else if(layer.reader){const auto tile=readRegion(layer.info.id,tileRect(tx,ty));if(!tile.isNull())p=sample(tile,sx,sy);}
                const auto backdrop=sample(out,x,y);float coverage=layer.info.opacity*(layer.info.group?1:layer.info.fillOpacity)*blendIfCoverage(layer.info,p,backdrop);
                if(filtered && layer.info.maskEnabled && (sourceMask.isNull() || filtered->hasWrittenNear(fx,fy,true)))coverage*=filtered->interpolatedMask(fx,fy);
                else if(!mask.isNull())coverage*=reinterpret_cast<const quint16 *>(mask.constScanLine(sy))[sx]/65535.f;else if(!sourceMask.isNull())coverage*=reinterpret_cast<const quint16 *>(sourceMask.constScanLine(y))[x]/65535.f;else if(layer.info.maskEnabled)coverage*=layer.info.maskDefault/255.f;
                if(layer.info.clipping && !baseAlpha.isNull())coverage*=sample(baseAlpha,x,y)[3];
                put(thisAlpha,x,y,{0,0,0,(groupAlpha.isNull()?p[3]:sample(groupAlpha,x,y)[3])*coverage});
                if(layer.info.group && layer.info.passThrough){const auto b=backdrop;Pixel mixed{};mixed[3]=b[3]*(1-coverage)+p[3]*coverage;if(mixed[3]>0)for(int c=0;c<3;++c)mixed[c]=(b[c]*b[3]*(1-coverage)+p[c]*p[3]*coverage)/mixed[3];put(out,x,y,restrictChannels(mixed,b,layer.info.channels));}
                else put(out,x,y,restrictChannels(blend(backdrop,p,coverage,layer.info.blend),backdrop,layer.info.channels));
            }
            if(!layer.info.clipping)baseAlpha=thisAlpha;
        }
        return out;
    };
    try{return render({},blank());}catch(const std::exception &e){d->error=QString::fromUtf8(e.what());return {};}
}
QUndoStack *ImageDocument::history(){return &d->history;}
quint64 ImageDocument::revision() const{return d->revision;}
void ImageDocument::setCacheBudget(qint64 bytes){d->budget=std::max<qint64>(0,bytes);d->trim();}
qint64 ImageDocument::residentBytes() const{return d->resident();}
qint64 ImageDocument::storedBytes() const{qint64 bytes=0;for(const auto &entry:d->blocks)if(auto b=entry.lock())bytes+=QFile(b->file->path).size();return bytes;}
QString ImageDocument::storageError() const{return d->error;}
}
