#include "image/ImageEditorTools.h"
#include <QImageWriter>
#include <QPainter>
#include <QSaveFile>
#include <algorithm>
#include <array>
#include <cmath>
#include <QBitArray>
#include <QQueue>
#include <QVector>
#include <QLineF>
#include <functional>

namespace vsr {
namespace {
using Pixel = std::array<float,4>;
Pixel readPixel(const QImage &image,int x,int y,ImagePrecision precision) {
    if(precision==ImagePrecision::Float32){const auto *p=reinterpret_cast<const float *>(image.constScanLine(y))+x*4;return {p[0],p[1],p[2],p[3]};}
    if(precision==ImagePrecision::UInt16){const auto p=reinterpret_cast<const QRgba64 *>(image.constScanLine(y))[x];return {p.red()/65535.f,p.green()/65535.f,p.blue()/65535.f,p.alpha()/65535.f};}
    const auto *p=image.constScanLine(y)+x*4;return {p[0]/255.f,p[1]/255.f,p[2]/255.f,p[3]/255.f};
}
void writePixel(QImage &image,int x,int y,const Pixel &pixel,ImagePrecision precision) {
    if(precision==ImagePrecision::Float32){auto *p=reinterpret_cast<float *>(image.scanLine(y))+x*4;std::copy(pixel.begin(),pixel.end(),p);return;}
    if(precision==ImagePrecision::UInt16){reinterpret_cast<QRgba64 *>(image.scanLine(y))[x]=QRgba64::fromRgba64(qRound(std::clamp(pixel[0],0.f,1.f)*65535),qRound(std::clamp(pixel[1],0.f,1.f)*65535),qRound(std::clamp(pixel[2],0.f,1.f)*65535),qRound(std::clamp(pixel[3],0.f,1.f)*65535));return;}
    auto *p=image.scanLine(y)+x*4;for(int c=0;c<4;++c)p[c]=qRound(std::clamp(pixel[c],0.f,1.f)*255);
}
ImageLayerInfo layerInfo(ImageDocument *document,const QUuid &id) {for(const auto &layer:document->layers())if(layer.id==id)return layer;return {};}
bool layerLocked(ImageDocument *document,const ImageLayerInfo &layer) {
    auto info=layer;const auto layers=document->layers();for(int depth=0;depth<=layers.size();++depth){if(info.locked)return true;if(info.parentId.isNull())return false;info=layerInfo(document,info.parentId);if(info.id.isNull())return false;}return true;
}
QString strokeLabel(ImageEditorTool tool) {
    switch(tool){case ImageEditorTool::Eraser:return QObject::tr("橡皮擦");case ImageEditorTool::Heal:return QObject::tr("修复笔划");case ImageEditorTool::Clone:return QObject::tr("仿制图章");case ImageEditorTool::ColorReplace:return QObject::tr("颜色替换");case ImageEditorTool::Blur:return QObject::tr("局部模糊");case ImageEditorTool::Sharpen:return QObject::tr("局部锐化");case ImageEditorTool::Dodge:return QObject::tr("减淡");case ImageEditorTool::Burn:return QObject::tr("加深");case ImageEditorTool::Sponge:return QObject::tr("海绵去色");case ImageEditorTool::HistoryBrush:return QObject::tr("历史画笔");default:return QObject::tr("画笔");}
}
bool historySourceFits(ImageDocument *document,const QUuid &id) {
    if(id.isNull() || layerInfo(document,id).group)return false;auto bounds=document->layerBounds(id);if(bounds.isEmpty())bounds=QRect(QPoint(),document->size());return qint64(bounds.width())*bounds.height()*16<=64*1024*1024;
}
QImage connectedMask(const QImage &pixels,QPoint seed,ImagePrecision precision,double tolerance) {
    QImage mask(pixels.size(),QImage::Format_Grayscale16);mask.fill(0);if(pixels.isNull() || mask.isNull() || !pixels.rect().contains(seed))return {};
    const auto target=readPixel(pixels,seed.x(),seed.y(),precision);QBitArray seen{pixels.width()*pixels.height()};QVector<int> queue;queue.reserve(std::min(pixels.width()*pixels.height(),1024*1024));queue<<seed.y()*pixels.width()+seed.x();seen.setBit(queue.first());
    for(qsizetype cursor=0;cursor<queue.size();++cursor){const int at=queue[cursor],x=at%pixels.width(),y=at/pixels.width();const auto pixel=readPixel(pixels,x,y,precision);float delta=std::abs(pixel[3]-target[3]);for(int c=0;c<3;++c)delta=std::max(delta,std::abs(pixel[c]-target[c]));if(delta>tolerance)continue;
        reinterpret_cast<quint16 *>(mask.scanLine(y))[x]=65535;const int neighbors[4]={x>0?at-1:-1,x+1<pixels.width()?at+1:-1,y>0?at-pixels.width():-1,y+1<pixels.height()?at+pixels.width():-1};for(int next:neighbors)if(next>=0 && !seen.testBit(next)){seen.setBit(next);queue<<next;}
    }return mask;
}
}
ImageEditorTools::ImageEditorTools(ImageDocument *document,QObject *parent) : QObject(parent),document_(document) {
    const auto layers=document_->layers();if(!layers.isEmpty())layer_=layers.last().id;
    cachedPrecision_=document_->precision();connect(document_,&ImageDocument::changed,this,[this]{if(cachedPrecision_!=document_->precision()){if(!cloneSource_.isNull())cloneSource_=cloneSource_.convertToFormat(document_->pixelFormat());if(!historySource_.isNull())historySource_=historySource_.convertToFormat(document_->pixelFormat());cachedPrecision_=document_->precision();}});
    if(historySourceFits(document_,layer_))captureHistorySource();
}
QImage ImageEditorTools::selectedPixels(QPoint *origin) const{
    const auto layer=layerInfo(document_,layer_);if(layer.id.isNull() || layer.group)return {};
    const auto bounds=document_->selectionBounds().intersected(document_->layerBounds(layer_));
    if(bounds.isEmpty() || qint64(bounds.width())*bounds.height()>16*1024*1024)return {};
    auto pixels=document_->readRegion(layer_,bounds.translated(-layer.offset));if(pixels.isNull())return {};
    if(document_->hasSelection()){
        const auto mask=document_->selectionRegion(bounds);if(mask.isNull())return {};
        for(int y=0;y<pixels.height();++y){const auto *row=reinterpret_cast<const quint16 *>(mask.constScanLine(y));
            for(int x=0;x<pixels.width();++x){auto pixel=readPixel(pixels,x,y,document_->precision());pixel[3]*=row[x]/65535.f;writePixel(pixels,x,y,pixel,document_->precision());}
        }
    }
    if(origin)*origin=bounds.topLeft();return pixels;
}
bool ImageEditorTools::layerFromSelection(bool cut){
    if(!document_->hasSelection())return false;
    const auto layer=layerInfo(document_,layer_);if(cut && layerLocked(document_,layer)){emit errorOccurred(tr("请先解锁图层再剪切。"));return false;}
    QPoint origin;const auto selected=selectedPixels(&origin);if(selected.isNull()){emit errorOccurred(tr("选区为空或超过 16M 像素，请缩小选区。"));return false;}
    const QRect bounds(origin,selected.size());document_->beginEdit(cut?tr("通过剪切的图层"):tr("通过复制的图层"));
    if(cut){auto source=document_->readRegion(layer_,bounds.translated(-layer.offset));const auto mask=document_->selectionRegion(bounds);
        if(source.isNull() || mask.isNull()){document_->cancelEdit();return false;}
        for(int y=0;y<source.height();++y){const auto *row=reinterpret_cast<const quint16 *>(mask.constScanLine(y));for(int x=0;x<source.width();++x){auto pixel=readPixel(source,x,y,document_->precision());pixel[3]*=1-row[x]/65535.f;writePixel(source,x,y,pixel,document_->precision());}}
        document_->writeRegion(layer_,origin-layer.offset,source);
    }
    const auto id=document_->addLayer(tr("选区图层"),selected,origin);document_->clearSelection();document_->commitEdit();setLayer(id);return true;
}
void ImageEditorTools::setLayer(const QUuid &id) {
    if(layer_==id || (!id.isNull() && layerInfo(document_,id).id.isNull()) || (id.isNull() && !document_->layers().isEmpty()))return;
    endStroke();endMove();endSelectionStroke();layer_=id;sourceSet_=false;cloneSource_={};historySource_={};if(!layerInfo(document_,id).group && historySourceFits(document_,id))captureHistorySource();emit layerChanged(id);
}
QUuid ImageEditorTools::pickLayer(QPoint point) const {
    if(!QRect(QPoint(),document_->size()).contains(point))return {};
    const auto layers=document_->layers();
    auto maskCoverage=[&](const ImageLayerInfo &layer){
        if(!layer.maskEnabled)return 1.f;const auto mask=document_->maskRegion(layer.id,QRect(point-layer.offset,QSize(1,1)));return mask.isNull()?0.f:reinterpret_cast<const quint16 *>(mask.constBits())[0]/65535.f;
    };
    std::function<float(const ImageLayerInfo &,int)> alpha;
    auto clippingBase=[&](const ImageLayerInfo &layer,int depth){
        int index=0;while(index<layers.size() && layers[index].id!=layer.id)++index;
        for(int below=index-1;below>=0;--below)if(layers[below].parentId==layer.parentId && layers[below].visible && layers[below].opacity>0 && !layers[below].clipping)return alpha(layers[below],depth+1);return 1.f;
    };
    alpha=[&](const ImageLayerInfo &layer,int depth){
        if(depth>layers.size() || !layer.visible || layer.opacity<=0)return 0.f;float value=0;
        if(layer.group){for(const auto &child:layers)if(child.parentId==layer.id){const float childAlpha=alpha(child,depth+1);value=childAlpha+value*(1-childAlpha);}}
        else{if(!document_->layerBounds(layer.id).translated(layer.offset).contains(point))return 0.f;const auto pixels=document_->readRegion(layer.id,QRect(point-layer.offset,QSize(1,1)));if(pixels.isNull())return 0.f;value=readPixel(pixels,0,0,document_->precision())[3];}
        value*=layer.opacity*maskCoverage(layer);if(layer.clipping)value*=clippingBase(layer,depth);return value;
    };
    for(auto it=layers.crbegin();it!=layers.crend();++it){
        if(it->group || !it->visible || it->opacity<=0 || layerLocked(document_,*it) || !document_->layerBounds(it->id).translated(it->offset).contains(point))continue;
        QList<ImageLayerInfo> parents;float gate=1;auto parent=it->parentId;
        for(int depth=0;!parent.isNull() && depth<=layers.size();++depth){const auto group=layerInfo(document_,parent);if(group.id.isNull() || !group.visible || group.opacity<=0){gate=0;break;}parents<<group;parent=group.parentId;}
        if(gate<=0)continue;for(const auto &group:parents){gate*=group.opacity*maskCoverage(group);if(group.clipping)gate*=clippingBase(group,0);if(gate<=0)break;}
        if(gate>0 && alpha(*it,0)>0)return it->id;
    }return {};
}
bool ImageEditorTools::autoPickLayer(QPoint point) {const auto id=pickLayer(point);if(id.isNull())return false;setLayer(id);return true;}
void ImageEditorTools::setBrushRadius(double radius) {radius_=std::clamp(radius,0.5,512.0);}
void ImageEditorTools::setBrushOpacity(double opacity) {opacity_=std::clamp(opacity,0.0,1.0);}
bool ImageEditorTools::selectPath(const QPainterPath &path,ImageSelectionMode mode) {
    endStroke();endMove();const QRect bounds=path.boundingRect().toAlignedRect().intersected(QRect(QPoint(),document_->size()));
    // The v1 raster selection contract cannot express a constant interior sparsely.
    if(qint64(bounds.width())*bounds.height()>64*1024*1024){emit errorOccurred(tr("当前选区栅格化上限为约 6711 万像素；请缩小选区。"));return false;}
    if(mode==ImageSelectionMode::Intersect && qint64(document_->size().width())*document_->size().height()>64*1024*1024){emit errorOccurred(tr("巨大画布暂不支持选区相交；请使用替换、添加或减去。"));return false;}
    const bool selected=document_->hasSelection();document_->beginEdit(tr("修改选区"));
    if(mode==ImageSelectionMode::Replace || !selected)document_->clearSelection();
    const QRect region=mode==ImageSelectionMode::Intersect?QRect(QPoint(),document_->size()):bounds;
    for(int top=region.top();top<region.y()+region.height();top+=ImageDocument::TileSize)for(int left=region.left();left<region.x()+region.width();left+=ImageDocument::TileSize){
        const QRect tile(left,top,std::min(ImageDocument::TileSize,region.x()+region.width()-left),std::min(ImageDocument::TileSize,region.y()+region.height()-top));
        QImage raster(tile.size(),QImage::Format_ARGB32_Premultiplied);raster.fill(Qt::transparent);
        {QPainter painter(&raster);painter.setRenderHint(QPainter::Antialiasing);painter.translate(-tile.x(),-tile.y());painter.fillPath(path,Qt::white);}
        QImage mask(tile.size(),QImage::Format_Grayscale16);
        QImage prior;if(selected && mode!=ImageSelectionMode::Replace)prior=document_->selectionRegion(tile);
        for(int y=0;y<tile.height();++y){auto *out=reinterpret_cast<quint16 *>(mask.scanLine(y));const auto *coverage=reinterpret_cast<const QRgb *>(raster.constScanLine(y));const auto *old=prior.isNull()?nullptr:reinterpret_cast<const quint16 *>(prior.constScanLine(y));
            for(int x=0;x<tile.width();++x){const int value=qAlpha(coverage[x])*257,before=old?old[x]:0;
                out[x]=mode==ImageSelectionMode::Add?std::max(before,value):mode==ImageSelectionMode::Subtract?quint64(before)*(65535-value)/65535:mode==ImageSelectionMode::Intersect?quint64(before)*value/65535:value;}
        }
        document_->writeSelection(tile.topLeft(),mask);
    }
    // An empty replacement still denotes an empty active selection.
    if(region.isEmpty()){QImage empty(1,1,QImage::Format_Grayscale16);empty.fill(0);document_->writeSelection(QPoint(),empty);}
    emit selectionEdited(path,mode);document_->commitEdit();return true;
}
bool ImageEditorTools::beginStroke(QPointF point,bool erase) {
    return beginStroke(point,erase?ImageEditorTool::Eraser:ImageEditorTool::Brush);
}
bool ImageEditorTools::beginStroke(QPointF point,ImageEditorTool tool) {
    endStroke();endMove();if(layerInfo(document_,layer_).id.isNull())return false;
    const auto info=layerInfo(document_,layer_);if(layerLocked(document_,info) || (info.group && !editingMask_)){emit errorOccurred(tr("请先选择未锁定的像素图层，或切换到编辑蒙版。"));return false;}
    if(editingMask_ && tool!=ImageEditorTool::Brush && tool!=ImageEditorTool::Eraser && tool!=ImageEditorTool::Blur && tool!=ImageEditorTool::Sharpen){emit errorOccurred(tr("蒙版模式支持画笔、橡皮、模糊和锐化；取样工具请切换到像素。"));return false;}
    if((tool==ImageEditorTool::Clone || tool==ImageEditorTool::Heal) && sourceSet_)sourceDelta_=sourcePoint_-point;
    if(tool==ImageEditorTool::Clone && !sourceSet_){emit errorOccurred(tr("请先按住 Alt 点击图章的取样源。"));return false;}
    if(tool==ImageEditorTool::Heal && !sourceSet_ && radius_>128){emit errorOccurred(tr("无源污点修复的半径暂限 128 像素；较大区域请 Alt 点击取样源使用源修复。"));return false;}
    if(tool==ImageEditorTool::HistoryBrush && historySource_.isNull()){emit errorOccurred(tr("请先设置历史画笔源；源区域暂限 64 MiB。"));return false;}
    replaceTarget_=document_->readRegion(layer_,QRect(point.toPoint()-info.offset,QSize(1,1)));
    strokeTool_=tool;erasing_=tool==ImageEditorTool::Eraser;stroking_=true;previous_=point;anchor_=point;document_->beginEdit(editingMask_?tr("蒙版 - %1").arg(strokeLabel(tool)):strokeLabel(tool));dab(point);return true;
}
void ImageEditorTools::continueStroke(QPointF point) {
    if(!stroking_)return;const double distance=std::hypot(point.x()-previous_.x(),point.y()-previous_.y());
    const int steps=std::max(1,int(std::ceil(distance/std::max(0.5,radius_*0.25))));
    for(int i=1;i<=steps;++i)dab(previous_+(point-previous_)*(double(i)/steps));previous_=point;
}
void ImageEditorTools::endStroke(bool cancel) {if(!stroking_)return;stroking_=false;if(cancel)document_->cancelEdit();else document_->commitEdit();}
void ImageEditorTools::dab(QPointF point) {
    if(editingMask_){maskDab(point);return;}
    if(strokeTool_!=ImageEditorTool::Brush && strokeTool_!=ImageEditorTool::Eraser && strokeTool_!=ImageEditorTool::ColorReplace){effectDab(point);return;}
    const auto layer=layerInfo(document_,layer_);if(layer.id.isNull())return;
    QImage ink(1,1,QImage::Format_RGBA32FPx4);auto *inkPixel=reinterpret_cast<float *>(ink.scanLine(0));inkPixel[0]=color_.redF();inkPixel[1]=color_.greenF();inkPixel[2]=color_.blueF();inkPixel[3]=color_.alphaF();ink.setColorSpace(QColorSpace::SRgb);
    if(document_->colorSpace().isValid())ink=ink.convertedToColorSpace(document_->colorSpace());
    const auto source=readPixel(ink,0,0,ImagePrecision::Float32);
    const QRect bounds=QRectF(point-QPointF(radius_+1,radius_+1),QSizeF(2*radius_+2,2*radius_+2)).toAlignedRect().intersected(QRect(QPoint(),document_->size()));
    for(int top=bounds.top();top<bounds.y()+bounds.height();top+=ImageDocument::TileSize)for(int left=bounds.left();left<bounds.x()+bounds.width();left+=ImageDocument::TileSize){
        const QRect region(left,top,std::min(ImageDocument::TileSize,bounds.x()+bounds.width()-left),std::min(ImageDocument::TileSize,bounds.y()+bounds.height()-top));
        QImage pixels=document_->readRegion(layer_,region.translated(-layer.offset)),selection=document_->selectionRegion(region);if(pixels.isNull() || selection.isNull())continue;
        bool changed=false;
        for(int y=0;y<region.height();++y){const auto *mask=reinterpret_cast<const quint16 *>(selection.constScanLine(y));for(int x=0;x<region.width();++x){
            const double distance=std::hypot(region.x()+x+0.5-point.x(),region.y()+y+0.5-point.y());
            const float coverage=float(std::clamp(radius_+0.5-distance,0.0,1.0)*opacity_*mask[x]/65535.0);if(coverage<=0)continue;
            auto pixel=readPixel(pixels,x,y,document_->precision());
            if(strokeTool_==ImageEditorTool::ColorReplace){const auto target=readPixel(replaceTarget_,0,0,document_->precision());float delta=0;for(int c=0;c<3;++c)delta=std::max(delta,std::abs(pixel[c]-target[c]));if(delta>tolerance_)continue;}
            if(erasing_)pixel[3]*=1-coverage;
            else if(strokeTool_==ImageEditorTool::ColorReplace){for(int c=0;c<3;++c)pixel[c]+=(source[c]-pixel[c])*coverage;}
            else {const float alpha=coverage*source[3],outAlpha=alpha+pixel[3]*(1-alpha);
                if(outAlpha>0)for(int c=0;c<3;++c)pixel[c]=(source[c]*alpha+pixel[c]*pixel[3]*(1-alpha))/outAlpha;pixel[3]=outAlpha;}
            writePixel(pixels,x,y,pixel,document_->precision());changed=true;
        }}
        if(changed)document_->writeRegion(layer_,region.topLeft()-layer.offset,pixels);
    }
}
void ImageEditorTools::maskDab(QPointF point) {
    const auto layer=layerInfo(document_,layer_);const QRect region=QRectF(point-QPointF(radius_+1,radius_+1),QSizeF(2*radius_+2,2*radius_+2)).toAlignedRect().intersected(QRect(QPoint(),document_->size()));if(region.isEmpty())return;
    auto mask=document_->maskRegion(layer_,region.translated(-layer.offset));const auto selection=document_->selectionRegion(region);const QRect input=region.adjusted(-1,-1,1,1);const auto source=document_->maskRegion(layer_,input.translated(-layer.offset));
    const auto get=[&](int x,int y){return reinterpret_cast<const quint16 *>(source.constScanLine(y-input.y()))[x-input.x()]/65535.f;};
    const float ink=strokeTool_==ImageEditorTool::Eraser?1:float(.2126*color_.redF()+.7152*color_.greenF()+.0722*color_.blueF());
    for(int y=0;y<region.height();++y){auto *out=reinterpret_cast<quint16 *>(mask.scanLine(y));const auto *coverage=reinterpret_cast<const quint16 *>(selection.constScanLine(y));for(int x=0;x<region.width();++x){const int dx=region.x()+x,dy=region.y()+y;const float amount=std::clamp(radius_+.5-std::hypot(dx+.5-point.x(),dy+.5-point.y()),0.0,1.0)*opacity_*coverage[x]/65535.;if(amount<=0)continue;const float old=get(dx,dy);float target=ink;if(strokeTool_==ImageEditorTool::Blur || strokeTool_==ImageEditorTool::Sharpen){float mean=0;for(int j=-1;j<=1;++j)for(int i=-1;i<=1;++i)mean+=get(dx+i,dy+j)/9;target=strokeTool_==ImageEditorTool::Blur?mean:old+(old-mean);}out[x]=qRound(std::clamp(old+(target-old)*amount,0.f,1.f)*65535);}}
    document_->writeMask(layer_,region.topLeft()-layer.offset,mask);
}
bool ImageEditorTools::beginMove(QPointF point) {
    endStroke();endMove();movingLayer_=layerInfo(document_,layer_);if(movingLayer_.id.isNull() || layerLocked(document_,movingLayer_))return false;movingLayers_={movingLayer_};
    if(movingLayer_.group){const auto layers=document_->layers();for(const auto &candidate:layers){auto parent=candidate.parentId;for(int depth=0;!parent.isNull() && depth<=layers.size();++depth){if(parent==movingLayer_.id){if(layerLocked(document_,candidate)){emit errorOccurred(tr("组内包含锁定图层，请先解锁再移动组。"));movingLayers_.clear();return false;}movingLayers_.append(candidate);break;}parent=layerInfo(document_,parent).parentId;}}}
    anchor_=point;moving_=true;document_->beginEdit(movingLayer_.group?tr("移动图层组"):tr("移动图层"));return true;
}
void ImageEditorTools::continueMove(QPointF point) {
    if(!moving_)return;QList<ImageLayerInfo> updates;
    for(const auto &start:movingLayers_){auto info=layerInfo(document_,start.id);if(info.id.isNull() || info.parentId!=start.parentId || layerLocked(document_,info)){endMove();emit errorOccurred(tr("图层结构或锁定状态已改变，移动已结束。"));return;}info.offset=start.offset+(point-anchor_).toPoint();updates<<info;}
    for(const auto &info:updates)document_->updateLayer(info);
}
void ImageEditorTools::endMove(bool cancel) {if(!moving_)return;moving_=false;movingLayers_.clear();if(cancel)document_->cancelEdit();else document_->commitEdit();}
QColor ImageEditorTools::sample(QPoint point) const {
    if(!QRect(QPoint(),document_->size()).contains(point))return {};
    auto image=document_->composite(QRect(point,QSize(1,1)));if(image.isNull())return {};
    if(image.colorSpace().isValid())image=image.convertedToColorSpace(QColorSpace::SRgb);
    const auto pixel=readPixel(image,0,0,document_->precision());return QColor::fromRgbF(std::clamp(pixel[0],0.f,1.f),std::clamp(pixel[1],0.f,1.f),std::clamp(pixel[2],0.f,1.f),std::clamp(pixel[3],0.f,1.f));
}
void ImageEditorTools::setCloneSource(QPointF point) {
    endStroke();const auto layer=layerInfo(document_,layer_);if(layer.id.isNull())return;
    const QRect region=QRect(point.toPoint()-QPoint(1024,1024),QSize(2048,2048)).intersected(QRect(QPoint(),document_->size()));
    cloneSource_=document_->readRegion(layer_,region.translated(-layer.offset));cloneOrigin_=region.topLeft();sourcePoint_=point;sourceSet_=!cloneSource_.isNull();
}
bool ImageEditorTools::captureHistorySource() {
    endStroke();const auto layer=layerInfo(document_,layer_);if(layer.id.isNull())return false;
    QRect region=document_->layerBounds(layer_).translated(layer.offset).intersected(QRect(QPoint(),document_->size()));if(region.isEmpty())region=QRect(QPoint(),document_->size());const int bytes=document_->precision()==ImagePrecision::Float32?16:document_->precision()==ImagePrecision::UInt16?8:4;
    if(qint64(region.width())*region.height()*bytes>64*1024*1024){emit errorOccurred(tr("历史画笔取样源暂限 64 MiB 像素数据；请选择更小的图层。"));return false;}
    historySource_=document_->readRegion(layer_,region.translated(-layer.offset));historyOrigin_=region.topLeft();return !historySource_.isNull();
}
void ImageEditorTools::effectDab(QPointF point) {
    const auto layer=layerInfo(document_,layer_);if(layer.id.isNull())return;
    const QRect bounds=QRectF(point-QPointF(radius_+1,radius_+1),QSizeF(2*radius_+2,2*radius_+2)).toAlignedRect().intersected(QRect(QPoint(),document_->size()));if(bounds.isEmpty())return;
    const int halo=strokeTool_==ImageEditorTool::Heal && !sourceSet_?std::min(16,int(radius_)+2):2;
    const QRect input=bounds.adjusted(-halo,-halo,halo,halo).intersected(QRect(QPoint(),document_->size()));
    const auto original=document_->readRegion(layer_,input.translated(-layer.offset));auto pixels=document_->readRegion(layer_,bounds.translated(-layer.offset));const auto selection=document_->selectionRegion(bounds);if(original.isNull() || pixels.isNull())return;
    const auto get=[&](int x,int y){return readPixel(original,std::clamp(x-input.x(),0,input.width()-1),std::clamp(y-input.y(),0,input.height()-1),document_->precision());};
    Pixel surrounding={};int surroundingCount=0;
    if(strokeTool_==ImageEditorTool::Heal && !sourceSet_)for(int y=input.top();y<=input.bottom();++y)for(int x=input.left();x<=input.right();++x){const double distance=std::hypot(x+.5-point.x(),y+.5-point.y());if(distance>=radius_+1 && distance<=radius_+halo){const auto p=get(x,y);for(int c=0;c<3;++c)surrounding[c]+=p[c];++surroundingCount;}}
    if(surroundingCount)for(int c=0;c<3;++c)surrounding[c]/=surroundingCount;
    QVector<Pixel> repaired;
    if(strokeTool_==ImageEditorTool::Heal && !sourceSet_ && surroundingCount){
        repaired.resize(input.width()*input.height());QBitArray interior(repaired.size());
        for(int y=0;y<input.height();++y)for(int x=0;x<input.width();++x){auto p=get(input.x()+x,input.y()+y);const double dx=input.x()+x+.5-point.x(),dy=input.y()+y+.5-point.y();if(dx*dx+dy*dy<=radius_*radius_){interior.setBit(y*input.width()+x);for(int c=0;c<3;++c)p[c]=surrounding[c];}repaired[y*input.width()+x]=p;}
        // Harmonic inpainting with fixed original boundary; native-range RGB is never clamped.
        const int iterations=std::clamp(int(radius_*4),16,96);
        for(int iteration=0;iteration<iterations;++iteration)for(int y=1;y<input.height()-1;++y)for(int x=1;x<input.width()-1;++x)if(interior.testBit(y*input.width()+x)){const int at=y*input.width()+x;for(int c=0;c<3;++c)repaired[at][c]=(repaired[at-1][c]+repaired[at+1][c]+repaired[at-input.width()][c]+repaired[at+input.width()][c])*.25f;}
    }
    Pixel sourceMean={},targetMean={};int sourceCount=0;
    if(strokeTool_==ImageEditorTool::Heal && sourceSet_)for(int y=bounds.top();y<=bounds.bottom();++y)for(int x=bounds.left();x<=bounds.right();++x){const QPoint source=(QPointF(x,y)+sourceDelta_).toPoint()-cloneOrigin_;if(cloneSource_.rect().contains(source)){const auto a=readPixel(cloneSource_,source.x(),source.y(),document_->precision()),b=get(x,y);for(int c=0;c<3;++c){sourceMean[c]+=a[c];targetMean[c]+=b[c];}++sourceCount;}}
    if(sourceCount)for(int c=0;c<3;++c){sourceMean[c]/=sourceCount;targetMean[c]/=sourceCount;}
    bool changed=false,sourceOutside=false;
    for(int y=0;y<bounds.height();++y){const auto *mask=reinterpret_cast<const quint16 *>(selection.constScanLine(y));for(int x=0;x<bounds.width();++x){
        const int dx=bounds.x()+x,dy=bounds.y()+y;const float amount=float(std::clamp(radius_+.5-std::hypot(dx+.5-point.x(),dy+.5-point.y()),0.0,1.0)*opacity_*mask[x]/65535.0);if(amount<=0)continue;
        const auto old=get(dx,dy);auto result=old;
        if(strokeTool_==ImageEditorTool::Clone || (strokeTool_==ImageEditorTool::Heal && sourceSet_)){
            const QPoint source=(QPointF(dx,dy)+sourceDelta_).toPoint()-cloneOrigin_;if(!cloneSource_.rect().contains(source)){sourceOutside=true;continue;}result=readPixel(cloneSource_,source.x(),source.y(),document_->precision());
            if(strokeTool_==ImageEditorTool::Heal){for(int c=0;c<3;++c)result[c]+=targetMean[c]-sourceMean[c];result[3]=old[3];}
        }else if(strokeTool_==ImageEditorTool::HistoryBrush){const QPoint source=QPoint(dx,dy)-historyOrigin_;if(!historySource_.rect().contains(source))continue;result=readPixel(historySource_,source.x(),source.y(),document_->precision());}
        else if(strokeTool_==ImageEditorTool::Heal){if(repaired.isEmpty())continue;const auto healed=repaired[(dy-input.y())*input.width()+dx-input.x()];for(int c=0;c<3;++c)result[c]=healed[c];}
        else if(strokeTool_==ImageEditorTool::Blur || strokeTool_==ImageEditorTool::Sharpen){Pixel average={};for(int j=-1;j<=1;++j)for(int i=-1;i<=1;++i){const auto p=get(dx+i,dy+j);for(int c=0;c<3;++c)average[c]+=p[c]/9;}for(int c=0;c<3;++c)result[c]=strokeTool_==ImageEditorTool::Blur?average[c]:old[c]+(old[c]-average[c]);}
        else if(strokeTool_==ImageEditorTool::Dodge || strokeTool_==ImageEditorTool::Burn){const float scale=strokeTool_==ImageEditorTool::Dodge?1.25f:.8f;for(int c=0;c<3;++c)result[c]*=scale;}
        else if(strokeTool_==ImageEditorTool::Sponge){const float luma=.2126f*old[0]+.7152f*old[1]+.0722f*old[2];for(int c=0;c<3;++c)result[c]=luma;}
        for(int c=0;c<4;++c)result[c]=old[c]+(result[c]-old[c])*amount;writePixel(pixels,x,y,result,document_->precision());changed=true;
    }}
    if(changed)document_->writeRegion(layer_,bounds.topLeft()-layer.offset,pixels);
    if(sourceOutside){emit errorOccurred(tr("笔划超出取样源缓存（2048 × 2048）；请重新 Alt 点击设置取样源。"));endStroke();}
}
bool ImageEditorTools::boundedCanvas(const QString &operation) const {
    if(qint64(document_->size().width())*document_->size().height()<=16*1024*1024)return true;
    emit const_cast<ImageEditorTools *>(this)->errorOccurred(tr("%1 暂限约 1678 万像素的画布，请先裁剪到工作区域。 ").arg(operation));return false;
}
bool ImageEditorTools::applySelectionMask(const QRect &region,const QImage &mask,ImageSelectionMode mode) {
    if(mode==ImageSelectionMode::Intersect && !boundedCanvas(tr("选区相交")))return false;
    const bool had=document_->hasSelection();if(!selecting_)document_->beginEdit(tr("颜色选区"));
    QImage prior;if(had && mode!=ImageSelectionMode::Replace)prior=document_->selectionRegion(region);
    if(mode==ImageSelectionMode::Replace || !had)document_->clearSelection();
    if(mode==ImageSelectionMode::Intersect){QImage black(document_->size(),QImage::Format_Grayscale16);black.fill(0);document_->writeSelection({},black);}
    QImage merged(mask.size(),QImage::Format_Grayscale16);
    for(int y=0;y<mask.height();++y){const auto *in=reinterpret_cast<const quint16 *>(mask.constScanLine(y));const auto *old=prior.isNull()?nullptr:reinterpret_cast<const quint16 *>(prior.constScanLine(y));auto *out=reinterpret_cast<quint16 *>(merged.scanLine(y));for(int x=0;x<mask.width();++x){const int a=old?old[x]:0,b=in[x];out[x]=mode==ImageSelectionMode::Add?std::max(a,b):mode==ImageSelectionMode::Subtract?quint64(a)*(65535-b)/65535:mode==ImageSelectionMode::Intersect?quint64(a)*b/65535:b;}}
    document_->writeSelection(region.topLeft(),merged);
    lastSelectionOutline_={};const int step=std::max(1,std::max(region.width(),region.height())/512);
    // A bounded contour aid; authoritative coverage retains every selected pixel.
    for(int y=0;y<mask.height();y+=step){int start=-1;const auto *row=reinterpret_cast<const quint16 *>(mask.constScanLine(y));for(int x=0;x<=mask.width();x+=step){const bool on=x<mask.width() && row[x]>32767;if(on && start<0)start=x;if(!on && start>=0){lastSelectionOutline_.addRect(region.x()+start,region.y()+y,x-start,std::min(step,mask.height()-y));start=-1;}}if(start>=0)lastSelectionOutline_.addRect(region.x()+start,region.y()+y,mask.width()-start,std::min(step,mask.height()-y));}
    lastSelectionOutline_=lastSelectionOutline_.simplified();emit selectionEdited(lastSelectionOutline_,mode);if(!selecting_)document_->commitEdit();return true;
}
void ImageEditorTools::beginSelectionStroke() {endSelectionStroke();selecting_=true;document_->beginEdit(tr("快速选择"));}
void ImageEditorTools::endSelectionStroke(bool cancel) {if(!selecting_)return;selecting_=false;if(cancel)document_->cancelEdit();else document_->commitEdit();}
bool ImageEditorTools::selectColor(QPoint seed,ImageSelectionMode mode,QRect limit) {
    endStroke();endMove();if(limit.isEmpty())limit=QRect(QPoint(),document_->size());limit=limit.intersected(QRect(QPoint(),document_->size()));if(!limit.contains(seed))return false;
    const qint64 area=qint64(limit.width())*limit.height();if(area>16*1024*1024){emit errorOccurred(tr("颜色连通选区暂限 1678 万像素；请使用快速选择的局部笔刷或缩小区域。"));return false;}
    const auto mask=connectedMask(document_->composite(limit),seed-limit.topLeft(),document_->precision(),tolerance_);if(mask.isNull())return false;
    return applySelectionMask(limit,mask,mode);
}
bool ImageEditorTools::selectObject(QRect box,ImageSelectionMode mode) {
    box=box.normalized().intersected(QRect(QPoint(),document_->size()));const qint64 area=qint64(box.width())*box.height();if(box.isEmpty())return false;if(area>16*1024*1024){emit errorOccurred(tr("框选辅助分割暂限 1678 万像素；请缩小框选范围。"));return false;}
    const auto pixels=document_->composite(box);QImage mask(box.size(),QImage::Format_Grayscale16);mask.fill(65535);if(pixels.isNull())return false;
    // Flood from the frame edge: only connected colors close to edge samples are background.
    QBitArray seen{int(area)};QVector<int> queue;QVector<Pixel> colors;
    for(int y=0;y<box.height();++y)for(int x=0;x<box.width();++x)if(x==0 || y==0 || x==box.width()-1 || y==box.height()-1){const int at=y*box.width()+x;queue<<at;colors<<readPixel(pixels,x,y,document_->precision());seen.setBit(at);}
    for(qsizetype cursor=0;cursor<queue.size();++cursor){const int at=queue[cursor],x=at%box.width(),y=at/box.width();reinterpret_cast<quint16 *>(mask.scanLine(y))[x]=0;const auto reference=colors[cursor];const int neighbors[4]={x>0?at-1:-1,x+1<box.width()?at+1:-1,y>0?at-box.width():-1,y+1<box.height()?at+box.width():-1};for(int next:neighbors)if(next>=0 && !seen.testBit(next)){const auto p=readPixel(pixels,next%box.width(),next/box.width(),document_->precision());float delta=0;for(int c=0;c<4;++c)delta=std::max(delta,std::abs(p[c]-reference[c]));if(delta<=tolerance_){seen.setBit(next);queue<<next;colors<<reference;}}}
    return applySelectionMask(box,mask,mode);
}
QPointF ImageEditorTools::magneticPoint(QPointF point) const {
    const QRect region=QRect(point.toPoint()-QPoint(12,12),QSize(25,25)).intersected(QRect(QPoint(),document_->size()));if(region.width()<3 || region.height()<3)return point;const auto pixels=document_->composite(region);if(pixels.isNull())return point;
    const auto luma=[&](int x,int y){const auto p=readPixel(pixels,x,y,document_->precision());return .2126f*p[0]+.7152f*p[1]+.0722f*p[2];};QPointF best=point;double bestScore=.02;
    for(int y=1;y<region.height()-1;++y)for(int x=1;x<region.width()-1;++x){const QPointF candidate(region.x()+x+.5,region.y()+y+.5);const double gradient=std::hypot(luma(x+1,y)-luma(x-1,y),luma(x,y+1)-luma(x,y-1));const double score=gradient/(1+QLineF(candidate,point).length()*.3);if(score>bestScore){bestScore=score;best=candidate;}}return best;
}
bool ImageEditorTools::fillPath(const QPainterPath &path,const QString &label) {
    endStroke();const auto layer=layerInfo(document_,layer_);if(layer.id.isNull() || layerLocked(document_,layer) || (layer.group && !editingMask_)){emit errorOccurred(tr("请先选择未锁定的像素图层，或切换到编辑蒙版。"));return false;}
    const QRect bounds=path.boundingRect().toAlignedRect().intersected(QRect(QPoint(),document_->size()));if(qint64(bounds.width())*bounds.height()>64*1024*1024){emit errorOccurred(tr("填充区域暂限 6711 万像素。"));return false;}
    document_->beginEdit(label.isEmpty()?tr("填充"):label);
    paintPath(path);document_->commitEdit();return true;
}
void ImageEditorTools::paintPath(const QPainterPath &path) {
    const QRect bounds=path.boundingRect().toAlignedRect().intersected(QRect(QPoint(),document_->size()));
    for(int top=bounds.top();top<=bounds.bottom();top+=256)for(int left=bounds.left();left<=bounds.right();left+=256){
        const QRect tile(left,top,std::min(256,bounds.right()-left+1),std::min(256,bounds.bottom()-top+1));QImage raster(tile.size(),QImage::Format_ARGB32_Premultiplied);raster.fill(0);{QPainter painter(&raster);painter.setRenderHint(QPainter::Antialiasing);painter.translate(-tile.topLeft());painter.fillPath(path,Qt::white);}
        QImage coverage(tile.size(),QImage::Format_Grayscale16);for(int y=0;y<tile.height();++y){auto *out=reinterpret_cast<quint16 *>(coverage.scanLine(y));for(int x=0;x<tile.width();++x)out[x]=qAlpha(raster.pixel(x,y))*257;}paintCoverage(tile,coverage);
    }
}
void ImageEditorTools::paintCoverage(const QRect &region,const QImage &coverage) {
    const auto layer=layerInfo(document_,layer_);QImage ink(1,1,QImage::Format_RGBA32FPx4);auto *value=reinterpret_cast<float *>(ink.bits());value[0]=color_.redF();value[1]=color_.greenF();value[2]=color_.blueF();value[3]=color_.alphaF();ink.setColorSpace(QColorSpace::SRgb);if(!editingMask_ && document_->colorSpace().isValid())ink=ink.convertedToColorSpace(document_->colorSpace());const auto color=readPixel(ink,0,0,ImagePrecision::Float32);
    auto pixels=editingMask_?document_->maskRegion(layer_,region.translated(-layer.offset)):document_->readRegion(layer_,region.translated(-layer.offset));const auto selection=document_->selectionRegion(region);bool changed=false;
    for(int y=0;y<region.height();++y){const auto *mask=reinterpret_cast<const quint16 *>(selection.constScanLine(y)),*shape=reinterpret_cast<const quint16 *>(coverage.constScanLine(y));for(int x=0;x<region.width();++x){const float alpha=shape[x]/65535.f*mask[x]/65535.f*opacity_*color[3];if(alpha<=0)continue;
        if(editingMask_){auto *row=reinterpret_cast<quint16 *>(pixels.scanLine(y));const float ink=.2126f*color[0]+.7152f*color[1]+.0722f*color[2];row[x]=qRound(row[x]*(1-alpha)+ink*65535*alpha);}
        else{auto pixel=readPixel(pixels,x,y,document_->precision());const float outAlpha=alpha+pixel[3]*(1-alpha);if(outAlpha>0)for(int c=0;c<3;++c)pixel[c]=(color[c]*alpha+pixel[c]*pixel[3]*(1-alpha))/outAlpha;pixel[3]=outAlpha;writePixel(pixels,x,y,pixel,document_->precision());}changed=true;}}
    if(changed){if(editingMask_)document_->writeMask(layer_,region.topLeft()-layer.offset,pixels);else document_->writeRegion(layer_,region.topLeft()-layer.offset,pixels);}
}
bool ImageEditorTools::fillSelection() {if(!boundedCanvas(tr("填充")))return false;QPainterPath path;path.addRect(QRectF(QPointF(),document_->size()));return fillPath(path);}
bool ImageEditorTools::floodFill(QPoint seed) {
    if(!boundedCanvas(tr("油漆桶")))return false;endStroke();const auto info=layerInfo(document_,layer_);if(info.id.isNull() || (info.group && !editingMask_) || layerLocked(document_,info)){emit errorOccurred(tr("请先选择未锁定的像素图层，或切换到编辑蒙版。"));return false;}
    const QRect bounds(QPoint(),document_->size());if(!bounds.contains(seed))return false;const auto source=editingMask_?document_->maskRegion(layer_,bounds.translated(-info.offset)).convertToFormat(document_->pixelFormat()):document_->composite(bounds);const auto mask=connectedMask(source,seed,document_->precision(),tolerance_);if(mask.isNull())return false;
    document_->beginEdit(tr("油漆桶"));for(int top=0;top<bounds.height();top+=256)for(int left=0;left<bounds.width();left+=256){const QRect tile(left,top,std::min(256,bounds.width()-left),std::min(256,bounds.height()-top));paintCoverage(tile,mask.copy(tile));}document_->commitEdit();return true;
}
bool ImageEditorTools::fillGradient(QPointF start,QPointF end) {
    if(!boundedCanvas(tr("渐变填充")))return false;endStroke();const auto layer=layerInfo(document_,layer_);if(layer.id.isNull() || layerLocked(document_,layer) || (layer.group && !editingMask_)){emit errorOccurred(tr("请先选择未锁定的像素图层，或切换到编辑蒙版。"));return false;}const QPointF vector=end-start;const double length=QPointF::dotProduct(vector,vector);if(length<1e-6)return false;
    QImage inks(2,1,QImage::Format_RGBA32FPx4);for(int x=0;x<2;++x){const auto color=x?endColor_:color_;auto *p=reinterpret_cast<float *>(inks.bits())+x*4;p[0]=color.redF();p[1]=color.greenF();p[2]=color.blueF();p[3]=color.alphaF();}inks.setColorSpace(QColorSpace::SRgb);if(!editingMask_ && document_->colorSpace().isValid())inks=inks.convertedToColorSpace(document_->colorSpace());const auto a=readPixel(inks,0,0,ImagePrecision::Float32),b=readPixel(inks,1,0,ImagePrecision::Float32);
    document_->beginEdit(tr("线性渐变"));const QSize size=document_->size();for(int top=0;top<size.height();top+=256)for(int left=0;left<size.width();left+=256){
        const QRect tile(left,top,std::min(256,size.width()-left),std::min(256,size.height()-top));auto pixels=editingMask_?document_->maskRegion(layer_,tile.translated(-layer.offset)):document_->readRegion(layer_,tile.translated(-layer.offset));const auto selection=document_->selectionRegion(tile);bool changed=false;
        for(int y=0;y<tile.height();++y){const auto *mask=reinterpret_cast<const quint16 *>(selection.constScanLine(y));for(int x=0;x<tile.width();++x){const float t=std::clamp(QPointF::dotProduct(QPointF(left+x+.5,top+y+.5)-start,vector)/length,0.0,1.0);Pixel color;for(int c=0;c<4;++c)color[c]=a[c]+(b[c]-a[c])*t;const float alpha=color[3]*mask[x]/65535.f*opacity_;if(alpha<=0)continue;
            if(editingMask_){auto *row=reinterpret_cast<quint16 *>(pixels.scanLine(y));const float ink=.2126f*color[0]+.7152f*color[1]+.0722f*color[2];row[x]=qRound(row[x]*(1-alpha)+ink*65535*alpha);}
            else{auto p=readPixel(pixels,x,y,document_->precision());const float out=alpha+p[3]*(1-alpha);if(out>0)for(int c=0;c<3;++c)p[c]=(color[c]*alpha+p[c]*p[3]*(1-alpha))/out;p[3]=out;writePixel(pixels,x,y,p,document_->precision());}changed=true;}}
        if(changed){if(editingMask_)document_->writeMask(layer_,tile.topLeft()-layer.offset,pixels);else document_->writeRegion(layer_,tile.topLeft()-layer.offset,pixels);}
    }document_->commitEdit();return true;
}
bool ImageEditorTools::strokePath(const QPainterPath &path) {
    if(path.isEmpty())return false;const int steps=std::clamp(int(path.length()/std::max(.5,radius_*.25)),1,100000);if(!beginStroke(path.pointAtPercent(0),ImageEditorTool::Brush))return false;for(int i=1;i<=steps;++i)continueStroke(path.pointAtPercent(double(i)/steps));endStroke();return true;
}
bool ImageEditorTools::createText(QPointF baseline) {
    endStroke();endMove();endSelectionStroke();QPainterPath path;path.addText(baseline,font_,text_);const QRect bounds=path.boundingRect().toAlignedRect().intersected(QRect(QPoint(),document_->size()));if(path.isEmpty() || bounds.isEmpty())return false;
    if(qint64(bounds.width())*bounds.height()>64*1024*1024){emit errorOccurred(tr("文字栅格化暂限 6711 万像素。"));return false;}
    const auto current=layerInfo(document_,layer_);const QUuid parent=current.group?current.id:current.parentId;if(!parent.isNull() && layerLocked(document_,layerInfo(document_,parent))){emit errorOccurred(tr("目标图层组已锁定，请先解锁再添加文字。"));return false;}
    const QUuid before=layer_;const bool wasMask=editingMask_;document_->beginEdit(tr("新建文字像素图层"));
    const auto id=document_->addLayer(tr("文字 - %1").arg(text_.simplified().left(32)),{},bounds.topLeft());auto info=layerInfo(document_,id);info.parentId=parent;document_->updateLayer(info);
    layer_=id;editingMask_=false;paintPath(path);layer_=before;editingMask_=wasMask;
    if(!document_->storageError().isEmpty()){document_->cancelEdit();emit errorOccurred(document_->storageError());return false;}
    document_->commitEdit();setEditingMask(false);setLayer(id);return true;
}
bool ImageEditorTools::invertSelection() {
    if(!boundedCanvas(tr("反选")))return false;const bool had=document_->hasSelection();document_->beginEdit(tr("反选"));const QSize size=document_->size();for(int top=0;top<size.height();top+=256)for(int left=0;left<size.width();left+=256){const QRect tile(left,top,std::min(256,size.width()-left),std::min(256,size.height()-top));auto mask=document_->selectionRegion(tile);for(int y=0;y<tile.height();++y){auto *row=reinterpret_cast<quint16 *>(mask.scanLine(y));for(int x=0;x<tile.width();++x)row[x]=had?65535-row[x]:0;}document_->writeSelection(tile.topLeft(),mask);}emit selectionInverted();document_->commitEdit();lastSelectionOutline_={};return true;
}
bool ImageEditorTools::featherSelection(int radius) {
    if(!document_->hasSelection() || !boundedCanvas(tr("选区羽化")))return false;radius=std::clamp(radius,1,128);auto source=document_->selectionRegion(QRect(QPoint(),document_->size()));QImage horizontal(source.size(),QImage::Format_Grayscale16),out(source.size(),QImage::Format_Grayscale16);
    for(int y=0;y<source.height();++y){const auto *in=reinterpret_cast<const quint16 *>(source.constScanLine(y));auto *row=reinterpret_cast<quint16 *>(horizontal.scanLine(y));quint64 sum=0;for(int x=-radius;x<=radius;++x)if(x>=0 && x<source.width())sum+=in[x];for(int x=0;x<source.width();++x){row[x]=sum/(2*radius+1);if(x-radius>=0)sum-=in[x-radius];if(x+radius+1<source.width())sum+=in[x+radius+1];}}
    for(int x=0;x<source.width();++x){quint64 sum=0;for(int y=-radius;y<=radius;++y)if(y>=0 && y<source.height())sum+=reinterpret_cast<const quint16 *>(horizontal.constScanLine(y))[x];for(int y=0;y<source.height();++y){reinterpret_cast<quint16 *>(out.scanLine(y))[x]=sum/(2*radius+1);if(y-radius>=0)sum-=reinterpret_cast<const quint16 *>(horizontal.constScanLine(y-radius))[x];if(y+radius+1<source.height())sum+=reinterpret_cast<const quint16 *>(horizontal.constScanLine(y+radius+1))[x];}}
    return applySelectionMask(QRect(QPoint(),document_->size()),out,ImageSelectionMode::Replace);
}
QString ImageEditorTools::savePng(ImageDocument *document,const QString &path) {return savePngRegion(document,QRect(QPoint(),document->size()),path);}
QString ImageEditorTools::savePngRegion(ImageDocument *document,const QRect &region,const QString &path) {
    if(document->precision()==ImagePrecision::Float32)return tr("PNG 不能保留浮点负值或大于 1 的数值；请使用 32bit 浮点 TIFF 导出。 ");
    const QRect bounded=region.intersected(QRect(QPoint(),document->size()));if(bounded.isEmpty())return tr("导出区域为空。");
    const qint64 bytes=qint64(bounded.width())*bounded.height()*(document->precision()==ImagePrecision::UInt16?8:4);
    if(bytes>256*1024*1024)return tr("当前 PNG 导出需要完整扁平图，暂限 256 MiB 像素数据；巨大图片的流式导出尚未实现。");
    const auto image=document->composite(bounded);if(image.isNull())return tr("无法生成 PNG 输出图像。");
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return file.errorString();QImageWriter writer(&file,"png");
    if(!writer.write(image))return writer.errorString();return file.commit()?QString():file.errorString();
}
}
