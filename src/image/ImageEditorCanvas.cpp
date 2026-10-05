#include "image/ImageEditorCanvas.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QMenu>
#include <QThread>
#include <QApplication>
#include "image/ImageHdrSurface.h"
#include <QLineF>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <windows.h>
#undef near

namespace vsr {
namespace {
bool brushTool(ImageEditorTool tool) {
    switch(tool){case ImageEditorTool::Brush:case ImageEditorTool::Eraser:case ImageEditorTool::Heal:case ImageEditorTool::Clone:case ImageEditorTool::ColorReplace:case ImageEditorTool::Blur:case ImageEditorTool::Sharpen:case ImageEditorTool::Dodge:case ImageEditorTool::Burn:case ImageEditorTool::Sponge:case ImageEditorTool::HistoryBrush:return true;default:return false;}
}
bool rectangleGesture(ImageEditorTool tool) {return tool==ImageEditorTool::Rectangle || tool==ImageEditorTool::ObjectSelection || tool==ImageEditorTool::ShapeRectangle || tool==ImageEditorTool::Slice;}
QList<QPointF> handles(const QRectF &r){return {r.topLeft(),r.topRight(),r.bottomRight(),r.bottomLeft(),QPointF(r.left(),r.center().y()),QPointF(r.center().x(),r.top()),QPointF(r.right(),r.center().y()),QPointF(r.center().x(),r.bottom())};}
bool samePreviewSettings(const ImageHdrPreviewSettings &a,const ImageHdrPreviewSettings &b){return a.exposure==b.exposure&&a.whitePoint==b.whitePoint&&a.toneMap==b.toneMap&&a.channel==b.channel;}
}
ImageEditorCanvas::ImageEditorCanvas(ImageEditorTools *tools,QWidget *parent) : QWidget(parent),tools_(tools) {
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(GlobalMemoryStatusEx(&memory))tools_->document()->setCacheBudget(std::clamp<qint64>(memory.ullAvailPhys/8,64ll*1024*1024,1024ll*1024*1024));
    hdrSurface_=new ImageHdrSurface(this);
    setObjectName("imageEditorCanvas");setMinimumSize(320,240);setFocusPolicy(Qt::StrongFocus);setMouseTracking(true);
    connect(tools_->document(),&ImageDocument::changed,this,[this]{
        // Selection identity survives both new branches and rolling history limits.
        selectionOutline_=selectionOutlines_.value(tools_->document()->selectionId());refreshPreview();
    });
    connect(tools_,&ImageEditorTools::selectionEdited,this,[this](const QPainterPath &path,ImageSelectionMode mode){if(mode==ImageSelectionMode::Replace)selectionOutline_=path;else if(mode==ImageSelectionMode::Add)selectionOutline_=selectionOutline_.united(path);else if(mode==ImageSelectionMode::Subtract)selectionOutline_=selectionOutline_.subtracted(path);else selectionOutline_=selectionOutline_.intersected(path);selectionOutlines_[tools_->document()->selectionId()]=selectionOutline_;update();});
    connect(tools_,&ImageEditorTools::selectionInverted,this,[this]{QPainterPath all;all.addRect(QRectF(QPointF(),tools_->document()->size()));selectionOutline_=all.subtracted(selectionOutline_);selectionOutlines_[tools_->document()->selectionId()]=selectionOutline_;update();});
    connect(tools_,&ImageEditorTools::layerChanged,this,[this]{cancelGesture();refreshPreview();update();});
}
ImageEditorCanvas::~ImageEditorCanvas(){if(previewThread_){previewThread_->requestInterruption();previewThread_->wait();}}
bool ImageEditorCanvas::setNativeHdrEnabled(bool enabled){hdrSurface_->setGeometry(rect());const bool active=hdrSurface_->setHdrEnabled(enabled);update();emit viewChanged();return active;}
bool ImageEditorCanvas::nativeHdrActive() const{return hdrSurface_->hdrActive();}
QString ImageEditorCanvas::nativeHdrStatus() const{return hdrSurface_->status();}
QPointF ImageEditorCanvas::documentPoint(QPointF point) const {return (point-origin_)/zoom_;}
QPointF ImageEditorCanvas::screenPoint(QPointF point) const {return origin_+point*zoom_;}
void ImageEditorCanvas::setTool(ImageEditorTool tool) {cancelGesture();tool_=tool;if(tool!=ImageEditorTool::Crop)cropRect_={};setCursor(tool==ImageEditorTool::Hand?Qt::OpenHandCursor:tool==ImageEditorTool::Move?Qt::SizeAllCursor:Qt::CrossCursor);update();}
void ImageEditorCanvas::fitToWindow() {
    fit_=true;const auto size=tools_->document()->size();zoom_=std::min(double(std::max(1,width()-32))/size.width(),double(std::max(1,height()-32))/size.height());
    zoom_=std::max(zoom_,1e-8);origin_=QPointF((width()-size.width()*zoom_)/2,(height()-size.height()*zoom_)/2);refreshPreview();emit viewChanged();
}
void ImageEditorCanvas::actualSize() {fit_=false;zoom_=1;const auto size=tools_->document()->size();origin_=QPointF((width()-size.width())/2.0,(height()-size.height())/2.0);refreshPreview();emit viewChanged();}
void ImageEditorCanvas::zoomBy(double factor,QPointF anchor){
    if(!std::isfinite(factor)||factor<=0)return;if(anchor.isNull())anchor=QPointF(width()/2.,height()/2.);const auto at=documentPoint(anchor);fit_=false;
    const double minimum=std::min(double(width())/tools_->document()->size().width(),double(height())/tools_->document()->size().height())/8;
    zoom_=std::clamp(zoom_*factor,std::max(1e-8,minimum),64.);origin_=anchor-at*zoom_;current_=documentPoint(anchor);emit cursorPositionChanged(current_,true);refreshPreview();emit viewChanged();
}
void ImageEditorCanvas::invalidateDocumentPreview(){++previewInvalidation_;refreshPreview();}
void ImageEditorCanvas::cancelGesture() {movePixels_={};moveBackdrop_={};tools_->endStroke(true);tools_->endMove(true);tools_->endSelectionStroke(true);if(freeTransform_){freeTransform_=false;tools_->document()->cancelEdit();}transformHandle_=-1;transformRect_={};dragging_=panning_=false;polygon_.clear();refreshPreview();update();}
QRectF ImageEditorCanvas::selectedLayerBounds() const {
    const auto layers=tools_->document()->layers();QSet<QUuid> ids{tools_->layer()};bool added=true;
    while(added){added=false;for(const auto &l:layers)if(ids.contains(l.parentId)&&!ids.contains(l.id)){ids.insert(l.id);added=true;}}
    QRect bounds;for(const auto &l:layers)if(ids.contains(l.id)&&!l.group)bounds=bounds.united(tools_->document()->layerBounds(l.id).translated(l.offset));return QRectF(bounds);
}
void ImageEditorCanvas::startFreeTransform(){
    cancelGesture();if(selectedLayerBounds().isEmpty())return;
    const auto layers=tools_->document()->layers();auto id=tools_->layer();for(int depth=0;!id.isNull()&&depth<=layers.size();++depth){QUuid parent;for(const auto &l:layers)if(l.id==id){if(l.locked){emit previewError(tr("请先解锁图层及其父组，再进行自由变换。"));return;}parent=l.parentId;break;}id=parent;}
    tool_=ImageEditorTool::Move;emit toolChanged(tool_);freeTransform_=true;
    tools_->document()->beginEdit(tr("自由变换"));setFocus();update();
}
int ImageEditorCanvas::layerHandle(QPointF point) const {
    const auto bounds=selectedLayerBounds();if(bounds.isEmpty())return -1;
    const double insetX=std::min(bounds.width()/4,7/zoom_),insetY=std::min(bounds.height()/4,7/zoom_);
    if(bounds.adjusted(insetX,insetY,-insetX,-insetY).contains(point))return 0;
    const auto points=handles(bounds);for(int i=0;i<points.size();++i)if(QLineF(point,points[i]).length()<=7/zoom_)return i+1;
    return bounds.contains(point)?0:-1;
}
void ImageEditorCanvas::updateLayerTransform(QPointF point,bool proportional){
    auto r=transformAnchorRect_;if(transformHandle_==0){transformRect_=r.translated(point-anchor_);return;}
    const bool left=transformHandle_==1||transformHandle_==4||transformHandle_==5;
    const bool right=transformHandle_==2||transformHandle_==3||transformHandle_==7;
    const bool top=transformHandle_==1||transformHandle_==2||transformHandle_==6;
    const bool bottom=transformHandle_==3||transformHandle_==4||transformHandle_==8;
    if(left)r.setLeft(std::min(point.x(),r.right()-1));if(right)r.setRight(std::max(point.x(),r.left()+1));
    if(top)r.setTop(std::min(point.y(),r.bottom()-1));if(bottom)r.setBottom(std::max(point.y(),r.top()+1));
    if(proportional){const auto original=transformAnchorRect_;const double ratio=original.width()/original.height();
        const bool verticalOnly=!left&&!right;const bool horizontalOnly=!top&&!bottom;
        double w=r.width(),h=r.height();if(verticalOnly)w=h*ratio;else if(horizontalOnly)h=w/ratio;else if(std::abs(w/original.width()-1)>=std::abs(h/original.height()-1))h=w/ratio;else w=h*ratio;
        const double x=left?original.right()-w:right?original.left():original.center().x()-w/2;
        const double y=top?original.bottom()-h:bottom?original.top():original.center().y()-h/2;
        r=QRectF(x,y,w,h);
    }transformRect_=r;update();
}
void ImageEditorCanvas::finishLayerTransform(){
    if(transformHandle_<0)return;const auto r=transformRect_;transformHandle_=-1;transformRect_={};dragging_=false;
    const QRect target(QPoint(qRound(r.x()),qRound(r.y())),QSize(std::max(1,qRound(r.width())),std::max(1,qRound(r.height()))));
    QString error;if(!tools_->document()->transformLayer(tools_->layer(),target,&error)&&!error.isEmpty())emit previewError(error);
    refreshPreview();update();
}
void ImageEditorCanvas::setMaskPreview(bool enabled){maskPreview_=enabled;refreshPreview();}
void ImageEditorCanvas::setCropAspect(double ratio){cropAspect_=std::isfinite(ratio)&&ratio>0?ratio:0;if(cropAspect_>0&&!cropRect_.isEmpty()){cropRect_.setHeight(cropRect_.width()/cropAspect_);update();}}
void ImageEditorCanvas::setPreviewChannel(int channel) {hdrSettings_.channel=std::clamp(channel,-1,3);refreshPreview();}
void ImageEditorCanvas::setHdrPreviewSettings(const ImageHdrPreviewSettings &settings) {hdrSettings_=settings;refreshPreview();}
void ImageEditorCanvas::fillVectorPath() {tools_->fillPath(vectorPath_,tr("填充路径"));}
void ImageEditorCanvas::strokeVectorPath() {tools_->strokePath(vectorPath_);}
void ImageEditorCanvas::selectVectorPath() {if(tools_->selectPath(vectorPath_,defaultSelectionMode_)){selectionOutline_=vectorPath_;selectionOutlines_[tools_->document()->selectionId()]=selectionOutline_;update();}}
QPainterPath ImageEditorCanvas::gesturePath() const {
    QPainterPath path;if(rectangleGesture(tool_))path.addRect(QRectF(anchor_,current_).normalized());
    else if(tool_==ImageEditorTool::Ellipse || tool_==ImageEditorTool::ShapeEllipse)path.addEllipse(QRectF(anchor_,current_).normalized());
    else if(!polygon_.isEmpty()){path.addPolygon(polygon_);path.closeSubpath();}return path;
}
void ImageEditorCanvas::rememberColorSelection(const QPainterPath &prior) {
    const auto path=tools_->lastSelectionOutline();if(selectionMode_==ImageSelectionMode::Replace)selectionOutline_=path;else if(selectionMode_==ImageSelectionMode::Add)selectionOutline_=prior.united(path);else if(selectionMode_==ImageSelectionMode::Subtract)selectionOutline_=prior.subtracted(path);else selectionOutline_=prior.intersected(path);selectionOutlines_[tools_->document()->selectionId()]=selectionOutline_;update();
}
void ImageEditorCanvas::quickSelect(QPointF point) {
    const auto prior=selectionOutline_;const int radius=std::clamp(int(tools_->brushRadius()*3),3,512);const QRect area(point.toPoint()-QPoint(radius,radius),QSize(radius*2+1,radius*2+1));if(tools_->selectColor(point.toPoint(),selectionMode_,area)){rememberColorSelection(prior);if(selectionMode_==ImageSelectionMode::Replace)selectionMode_=ImageSelectionMode::Add;}
}
void ImageEditorCanvas::finishSelection() {
    const auto path=gesturePath(),prior=selectionOutline_;if(tools_->selectPath(path,selectionMode_)){if(selectionMode_==ImageSelectionMode::Replace)selectionOutline_=path;else if(selectionMode_==ImageSelectionMode::Add)selectionOutline_=prior.united(path);else if(selectionMode_==ImageSelectionMode::Subtract)selectionOutline_=prior.subtracted(path);else selectionOutline_=prior.intersected(path);
        selectionOutlines_[tools_->document()->selectionId()]=selectionOutline_;}
    polygon_.clear();dragging_=false;update();
}
void ImageEditorCanvas::prepareMovePreview() {
    moveDelta_={};moveRegion_=bufferedRegion(visibleDocumentRegion());if(moveRegion_.isEmpty())return;
    const auto layers=tools_->document()->layers();QSet<QUuid> moving{tools_->layer()};bool added=true;
    while(added){added=false;for(const auto &layer:layers)if(moving.contains(layer.parentId)&&!moving.contains(layer.id)){moving.insert(layer.id);added=true;}}
    QSet<QUuid> parents;auto parent=tools_->layer();for(int i=0;!parent.isNull()&&i<=layers.size();++i){QUuid next;for(const auto &layer:layers)if(layer.id==parent){next=layer.parentId;break;}if(!next.isNull())parents.insert(next);parent=next;}
    auto background=tools_->document()->snapshot(),foreground=tools_->document()->snapshot();
    background->removeLayer(tools_->layer());
    for(auto layer:layers)if(!moving.contains(layer.id)&&!parents.contains(layer.id)){layer.visible=false;foreground->updateLayer(layer);}
    const auto output=previewOutput(moveRegion_);
    moveBackdrop_=ImageHdr::preview(background->compositePreview(moveRegion_,output,ImageSamplingQuality::Bilinear),hdrSettings_);
    movePixels_=ImageHdr::preview(foreground->compositePreview(moveRegion_,output,ImageSamplingQuality::Bilinear),hdrSettings_);
    // The drag is a display proxy; pixels and native-precision history change once on release.
    hdrSurface_->hide();update();
}
QRect ImageEditorCanvas::visibleDocumentRegion() const{return QRectF(documentPoint(QPointF()),documentPoint(QPointF(width(),height()))).toAlignedRect().intersected(QRect(QPoint(),tools_->document()->size()));}
QRect ImageEditorCanvas::bufferedRegion(const QRect &visible) const {const int x=std::max(2,visible.width()/4),y=std::max(2,visible.height()/4);return visible.adjusted(-x,-y,x,y).intersected(QRect(QPoint(),tools_->document()->size()));}
QSize ImageEditorCanvas::previewOutput(const QRect &region) const {
    const double density=std::min(1.,zoom_*devicePixelRatioF()*2);double w=std::max(1.,std::ceil(region.width()*density)),h=std::max(1.,std::ceil(region.height()*density));
    // At most 256 MiB of native RGBA32F staging, independently of document dimensions.
    const double reduction=std::min({1.,8192./w,8192./h,std::sqrt((16.*1024*1024)/(w*h))});
    return QSize(std::max(1,int(std::floor(w*reduction))),std::max(1,int(std::floor(h*reduction))));
}
bool ImageEditorCanvas::previewCovers(const QRect &visible) const {
    if(preview_.isNull()||previewGeneration_!=previewInvalidation_||previewRevision_!=tools_->document()->revision()||!samePreviewSettings(previewSettings_,hdrSettings_)||previewMask_!=maskPreview_||(maskPreview_&&previewLayer_!=tools_->layer())||!previewRegion_.contains(visible))return false;
    const auto region=bufferedRegion(visible);const auto output=previewOutput(region);
    return double(nativePreview_.width())/previewRegion_.width()+1e-8>=double(output.width())/region.width()&&double(nativePreview_.height())/previewRegion_.height()+1e-8>=double(output.height())/region.height();
}
void ImageEditorCanvas::updatePreviewTarget(){previewTarget_=QRectF(screenPoint(previewRegion_.topLeft()),QSizeF(previewRegion_.size())*zoom_);}
void ImageEditorCanvas::refreshPreview() {
    if(dragging_ && tool_==ImageEditorTool::Move && !movePixels_.isNull()){update();return;}
    updatePreviewTarget();update();const auto visible=visibleDocumentRegion();if(visible.isEmpty()){if(previewThread_)refreshAgain_=true;return;}if(previewCovers(visible))return;
    if(previewThread_){refreshAgain_=true;return;}if(refreshPending_)return;refreshPending_=true;
    QTimer::singleShot(0,this,[this]{
        refreshPending_=false;if(previewThread_){refreshAgain_=true;return;}
        const auto visible=visibleDocumentRegion();if(visible.isEmpty()||previewCovers(visible))return;const auto region=bufferedRegion(visible);const auto output=previewOutput(region);
        struct Result {std::unique_ptr<ImageDocument> snapshot;QImage native,display;QString error;};auto result=std::make_shared<Result>();result->snapshot=tools_->document()->snapshot();
        const auto revision=tools_->document()->revision(),generation=previewInvalidation_;const auto settings=hdrSettings_;const auto layerId=tools_->layer();const bool showMask=maskPreview_;
        previewThread_=QThread::create([result,region,output,settings,layerId,showMask]{if(showMask){QPoint offset;for(const auto &layer:result->snapshot->layers())if(layer.id==layerId)offset=layer.offset;result->native=result->snapshot->maskPreview(layerId,region.translated(-offset),output,ImageSamplingQuality::Bilinear);}else result->native=result->snapshot->compositePreview(region,output,ImageSamplingQuality::Bilinear);result->error=result->snapshot->storageError();if(!QThread::currentThread()->isInterruptionRequested())result->display=ImageHdr::preview(result->native,settings);result->snapshot.reset();});
        result->snapshot->moveToThread(previewThread_);auto *thread=previewThread_;
        connect(thread,&QThread::finished,this,[this,result,region,settings,layerId,showMask,revision,generation]{previewThread_=nullptr;
            const auto visible=visibleDocumentRegion();const bool produced=!result->display.isNull();
            if(!previewCovers(visible)&&generation==previewInvalidation_&&revision==tools_->document()->revision()&&samePreviewSettings(settings,hdrSettings_)&&showMask==maskPreview_&&(!showMask||layerId==tools_->layer())){nativePreview_=std::move(result->native);preview_=std::move(result->display);previewRegion_=region;previewRevision_=revision;previewGeneration_=generation;previewSettings_=settings;previewLayer_=layerId;previewMask_=showMask;if(region==QRect(QPoint(),tools_->document()->size())){overview_=preview_;overviewNative_=nativePreview_;}if(!dragging_){movePixels_={};moveBackdrop_={};if(hdrSurface_->hdrActive())hdrSurface_->show();}updatePreviewTarget();update();emit previewReady();if(!result->error.isEmpty())emit previewError(result->error);}
            if(refreshAgain_ || revision!=tools_->document()->revision()||(!visible.isEmpty()&&!previewCovers(visible)&&produced)){refreshAgain_=false;refreshPreview();}
        });connect(thread,&QThread::finished,thread,&QObject::deleteLater);thread->start();
    });
}
void ImageEditorCanvas::paintEvent(QPaintEvent *) {
    QPainter painter(this);painter.fillRect(rect(),palette().color(QPalette::Window));const QRectF imageRect(screenPoint(QPointF()),QSizeF(tools_->document()->size())*zoom_);
    const QRect exposed=imageRect.toAlignedRect().intersected(rect());
    for(int y=exposed.top();y<=exposed.bottom();y+=16)for(int x=exposed.left();x<=exposed.right();x+=16)painter.fillRect(QRect(x,y,std::min(16,exposed.right()-x+1),std::min(16,exposed.bottom()-y+1)),((x-exposed.left())/16+(y-exposed.top())/16)%2?QColor("#505050"):QColor("#707070"));
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if(!overview_.isNull())painter.drawImage(imageRect,overview_);
    if(!movePixels_.isNull()){
        const QRectF target(screenPoint(moveRegion_.topLeft()),QSizeF(moveRegion_.size())*zoom_);
        painter.drawImage(target,moveBackdrop_);painter.save();painter.setClipRect(imageRect);painter.drawImage(target.translated(moveDelta_*zoom_),movePixels_);painter.restore();
    }else if(!preview_.isNull())painter.drawImage(previewTarget_,preview_);
    paintGuides(painter);painter.end();
    if(hdrSurface_->hdrActive() && !nativePreview_.isNull() && movePixels_.isNull()){
        QImage overlay(size()*devicePixelRatioF(),QImage::Format_ARGB32_Premultiplied);overlay.setDevicePixelRatio(devicePixelRatioF());overlay.fill(Qt::transparent);QPainter guides(&overlay);paintGuides(guides);guides.end();
        const bool overview=!overviewNative_.isNull() && !previewTarget_.contains(imageRect.intersected(QRectF(rect())));
        hdrSurface_->present(hdrSettings_.channel<0?(overview?overviewNative_:nativePreview_):(overview?overview_:preview_),overview?imageRect:previewTarget_,overlay,hdrSettings_.exposure);
        if(!hdrSurface_->hdrActive())emit viewChanged();
    }
}
void ImageEditorCanvas::paintGuides(QPainter &painter){
    const QRectF imageRect(screenPoint(QPointF()),QSizeF(tools_->document()->size())*zoom_);
    if(showGrid_ && zoom_>=.05){painter.setPen(QPen(QColor(150,150,150,80),1));const double step=std::max(16.0,64*zoom_);for(double x=std::fmod(origin_.x(),step);x<width();x+=step)painter.drawLine(QPointF(x,0),QPointF(x,height()));for(double y=std::fmod(origin_.y(),step);y<height();y+=step)painter.drawLine(QPointF(0,y),QPointF(width(),y));}
    painter.setPen(QPen(QColor("#999999"),1));painter.drawRect(imageRect);
    painter.setRenderHint(QPainter::Antialiasing);painter.translate(origin_);painter.scale(zoom_,zoom_);
    painter.setBrush(Qt::NoBrush);
    // Outline is a gesture aid; the authoritative coverage always lives in ImageDocument.
    for(int pass=0;pass<2;++pass){painter.setPen(QPen(pass?Qt::white:Qt::black,(pass?1:2)/zoom_,pass?Qt::DashLine:Qt::SolidLine));painter.drawPath(selectionOutline_);
        if(dragging_ && (rectangleGesture(tool_) || tool_==ImageEditorTool::Ellipse || tool_==ImageEditorTool::ShapeEllipse || tool_==ImageEditorTool::Lasso || tool_==ImageEditorTool::MagneticLasso))painter.drawPath(gesturePath());
        if((tool_==ImageEditorTool::Polygon || tool_==ImageEditorTool::Path) && !polygon_.isEmpty()){painter.drawPolyline(polygon_);painter.drawLine(polygon_.last(),current_);}
        if(brushTool(tool_) || tool_==ImageEditorTool::QuickSelection)painter.drawEllipse(current_,tools_->brushRadius(),tools_->brushRadius());
        if(dragging_ && (tool_==ImageEditorTool::Gradient || tool_==ImageEditorTool::Ruler))painter.drawLine(anchor_,current_);
        if(rulerVisible_)painter.drawLine(rulerStart_,rulerEnd_);
        if(!slice_.isEmpty())painter.drawRect(slice_);
        if(!cropRect_.isEmpty())painter.drawRect(cropRect_);
        if(!vectorPath_.isEmpty())painter.drawPath(vectorPath_);
    }
    if(!cropRect_.isEmpty()){painter.setPen(QPen(Qt::white,1/zoom_));painter.setBrush(QColor("#303030"));for(const QPointF &p:QList<QPointF>{cropRect_.topLeft(),cropRect_.topRight(),cropRect_.bottomLeft(),cropRect_.bottomRight(),QPointF(cropRect_.center().x(),cropRect_.top()),QPointF(cropRect_.center().x(),cropRect_.bottom()),QPointF(cropRect_.left(),cropRect_.center().y()),QPointF(cropRect_.right(),cropRect_.center().y())})painter.drawRect(QRectF(p-QPointF(3/zoom_,3/zoom_),QSizeF(6/zoom_,6/zoom_)));}
    if(tool_==ImageEditorTool::Move||freeTransform_){const auto bounds=transformHandle_>=0?transformRect_:selectedLayerBounds().translated(!movePixels_.isNull()?moveDelta_:QPointF());if(!bounds.isEmpty()){
        painter.setPen(QPen(QColor("#69b7ff"),1.5/zoom_));painter.setBrush(Qt::NoBrush);painter.drawRect(bounds);
        painter.setBrush(palette().color(QPalette::Window));for(const auto &point:handles(bounds))painter.drawRect(QRectF(point-QPointF(3.5/zoom_,3.5/zoom_),QSizeF(7/zoom_,7/zoom_)));
    }}
}
void ImageEditorCanvas::resizeEvent(QResizeEvent *event) {QWidget::resizeEvent(event);hdrSurface_->setGeometry(rect());if(fit_)fitToWindow();else refreshPreview();}
void ImageEditorCanvas::mousePressEvent(QMouseEvent *event) {
    setFocus();lastScreen_=event->position();current_=documentPoint(event->position());emit cursorPositionChanged(current_,true);
    if(event->button()==Qt::RightButton && tools_->document()->hasSelection()){emit selectionContextMenuRequested(event->globalPosition().toPoint());return;}
    if(event->button()==Qt::RightButton && tool_==ImageEditorTool::Path && !vectorPath_.isEmpty()){QMenu menu(this);menu.addAction(tr("描边路径"),this,&ImageEditorCanvas::strokeVectorPath);menu.addAction(tr("填充路径"),this,&ImageEditorCanvas::fillVectorPath);menu.addAction(tr("路径转选区"),this,&ImageEditorCanvas::selectVectorPath);menu.exec(event->globalPosition().toPoint());return;}
    if(event->button()==Qt::MiddleButton || (event->button()==Qt::LeftButton && tool_==ImageEditorTool::Hand)){panning_=true;fit_=false;setCursor(Qt::ClosedHandCursor);return;}
    if(event->button()!=Qt::LeftButton)return;
    if(tool_==ImageEditorTool::Move){
        int handle=layerHandle(current_);if(handle<0&&!freeTransform_){tools_->autoPickLayer(current_.toPoint());handle=layerHandle(current_);}
        if(handle<0)return;const auto layers=tools_->document()->layers();auto id=tools_->layer();for(int depth=0;!id.isNull()&&depth<=layers.size();++depth){QUuid parent;for(const auto &l:layers)if(l.id==id){if(l.locked){emit previewError(tr("当前图层或父组已锁定，请先解锁再移动或缩放。"));return;}parent=l.parentId;break;}id=parent;}
        anchor_=current_;transformAnchorRect_=selectedLayerBounds();
        if(handle>0||freeTransform_){transformHandle_=handle;transformRect_=transformAnchorRect_;dragging_=true;update();return;}
        dragging_=tools_->beginMove(current_);if(dragging_)prepareMovePreview();return;
    }
    if(!QRectF(QPointF(),tools_->document()->size()).contains(current_))return;
    if(tool_==ImageEditorTool::Zoom){zoomBy(event->modifiers().testFlag(Qt::AltModifier)?.5:2,event->position());return;}
    if((tool_==ImageEditorTool::Clone || tool_==ImageEditorTool::Heal) && event->modifiers().testFlag(Qt::AltModifier)){tools_->setCloneSource(current_);return;}
    if(tool_==ImageEditorTool::Text){if(tools_->createText(current_)){setTool(ImageEditorTool::Move);emit toolChanged(tool_);}refreshPreview();return;}
    if(tool_==ImageEditorTool::Fill){tools_->floodFill(current_.toPoint());refreshPreview();return;}
    if(tool_==ImageEditorTool::Eyedropper){const auto color=tools_->sample(current_.toPoint());if(color.isValid()){tools_->setBrushColor(color);emit colorPicked(color);}return;}
    selectionMode_=event->modifiers().testFlag(Qt::ShiftModifier)?(event->modifiers().testFlag(Qt::AltModifier)?ImageSelectionMode::Intersect:ImageSelectionMode::Add):event->modifiers().testFlag(Qt::AltModifier)?ImageSelectionMode::Subtract:defaultSelectionMode_;
    if(tool_==ImageEditorTool::MagicWand){const auto prior=selectionOutline_;if(tools_->selectColor(current_.toPoint(),selectionMode_))rememberColorSelection(prior);return;}
    if(tool_==ImageEditorTool::Polygon || tool_==ImageEditorTool::Path){if(polygon_.isEmpty())anchor_=current_;polygon_<<current_;update();return;}
    anchor_=current_;dragging_=true;
    if(tool_==ImageEditorTool::Crop){cropAnchorRect_=cropRect_;cropHandle_=0;const double near=8/zoom_;const QList<QPointF> points={cropRect_.topLeft(),cropRect_.topRight(),cropRect_.bottomRight(),cropRect_.bottomLeft(),QPointF(cropRect_.left(),cropRect_.center().y()),QPointF(cropRect_.center().x(),cropRect_.top()),QPointF(cropRect_.right(),cropRect_.center().y()),QPointF(cropRect_.center().x(),cropRect_.bottom())};if(!cropRect_.isEmpty())for(int i=0;i<points.size();++i)if(QLineF(current_,points[i]).length()<=near){cropHandle_=i+2;break;}if(cropHandle_==0 && cropRect_.contains(current_))cropHandle_=1;return;}
    if(brushTool(tool_))dragging_=tools_->beginStroke(current_,tool_);
    else if(tool_==ImageEditorTool::Move)dragging_=tools_->beginMove(current_);
    else if(tool_==ImageEditorTool::QuickSelection){tools_->beginSelectionStroke();quickSelect(current_);}
    else if(tool_==ImageEditorTool::Lasso || tool_==ImageEditorTool::MagneticLasso){polygon_.clear();polygon_<<(tool_==ImageEditorTool::MagneticLasso?tools_->magneticPoint(current_):current_);}refreshPreview();update();
}
void ImageEditorCanvas::mouseMoveEvent(QMouseEvent *event) {
    current_=documentPoint(event->position());
    if(panning_){origin_+=event->position()-lastScreen_;lastScreen_=event->position();current_=documentPoint(event->position());emit cursorPositionChanged(current_,true);refreshPreview();emit viewChanged();return;}
    emit cursorPositionChanged(current_,true);
    if(dragging_ && tool_==ImageEditorTool::Move && transformHandle_<0 && !movePixels_.isNull()){moveDelta_=current_-anchor_;update();return;}
    if(dragging_&&transformHandle_>=0){updateLayerTransform(current_,event->modifiers().testFlag(Qt::ShiftModifier));update();return;}
    if(!dragging_&&tool_==ImageEditorTool::Move){const int h=layerHandle(current_);setCursor(h==1||h==3?Qt::SizeFDiagCursor:h==2||h==4?Qt::SizeBDiagCursor:h==5||h==7?Qt::SizeHorCursor:h==6||h==8?Qt::SizeVerCursor:h==0?Qt::SizeAllCursor:Qt::ArrowCursor);}
    if(dragging_){if(brushTool(tool_))tools_->continueStroke(current_);else if(tool_==ImageEditorTool::Move)tools_->continueMove(current_);else if(tool_==ImageEditorTool::QuickSelection)quickSelect(current_);else if(tool_==ImageEditorTool::Lasso || tool_==ImageEditorTool::MagneticLasso){const auto point=tool_==ImageEditorTool::MagneticLasso?tools_->magneticPoint(current_):current_;if(polygon_.isEmpty() || QLineF(polygon_.last(),point).length()>=1/zoom_)polygon_<<point;}
        else if(tool_==ImageEditorTool::Crop){const QPointF point=current_;if(cropHandle_==0)cropRect_=QRectF(anchor_,point).normalized();else if(cropHandle_==1)cropRect_=cropAnchorRect_.translated(point-anchor_);else{cropRect_=cropAnchorRect_;switch(cropHandle_){case 2:cropRect_.setTopLeft(point);break;case 3:cropRect_.setTopRight(point);break;case 4:cropRect_.setBottomRight(point);break;case 5:cropRect_.setBottomLeft(point);break;case 6:cropRect_.setLeft(point.x());break;case 7:cropRect_.setTop(point.y());break;case 8:cropRect_.setRight(point.x());break;case 9:cropRect_.setBottom(point.y());break;}cropRect_=cropRect_.normalized();}if(cropAspect_>0 && !cropRect_.isEmpty()){
            const double w=cropRect_.width(),h=cropRect_.height();
            if(cropHandle_==7 || cropHandle_==9)cropRect_.setWidth(h*cropAspect_);else cropRect_.setHeight(w/cropAspect_);
            const QRectF bounds(QPointF(),tools_->document()->size());if(cropRect_.height()>bounds.height()){cropRect_.setHeight(bounds.height());cropRect_.setWidth(bounds.height()*cropAspect_);}if(cropRect_.width()>bounds.width()){cropRect_.setWidth(bounds.width());cropRect_.setHeight(bounds.width()/cropAspect_);}if(cropRect_.right()>bounds.right())cropRect_.moveRight(bounds.right());if(cropRect_.bottom()>bounds.bottom())cropRect_.moveBottom(bounds.bottom());if(cropRect_.left()<0)cropRect_.moveLeft(0);if(cropRect_.top()<0)cropRect_.moveTop(0);
        }else cropRect_=cropRect_.intersected(QRectF(QPointF(),tools_->document()->size()));}refreshPreview();}update();
}
void ImageEditorCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if(panning_ && (event->button()==Qt::LeftButton || event->button()==Qt::MiddleButton)){panning_=false;setCursor(tool_==ImageEditorTool::Hand?Qt::OpenHandCursor:Qt::CrossCursor);return;}
    if(event->button()!=Qt::LeftButton || !dragging_)return;
    if(transformHandle_>=0){mouseMoveEvent(event);finishLayerTransform();return;}
    mouseMoveEvent(event);if(brushTool(tool_))tools_->endStroke();else if(tool_==ImageEditorTool::Move){tools_->continueMove(current_);tools_->endMove();}else if(tool_==ImageEditorTool::QuickSelection)tools_->endSelectionStroke();
    else if(tool_==ImageEditorTool::Gradient)tools_->fillGradient(anchor_,current_);
    else if(tool_==ImageEditorTool::Ruler){rulerStart_=anchor_;rulerEnd_=current_;rulerVisible_=true;const QLineF line(anchor_,current_);emit rulerMeasured(line.length(),std::atan2(current_.y()-anchor_.y(),current_.x()-anchor_.x())*180/3.141592653589793);}
    else if(tool_==ImageEditorTool::ObjectSelection){const auto prior=selectionOutline_;if(tools_->selectObject(QRectF(anchor_,current_).normalized().toAlignedRect(),selectionMode_))rememberColorSelection(prior);}
    else if(tool_==ImageEditorTool::ShapeRectangle || tool_==ImageEditorTool::ShapeEllipse)tools_->fillPath(gesturePath(),tr("栅格化形状"));
    else if(tool_==ImageEditorTool::Slice){slice_=QRectF(anchor_,current_).normalized().toAlignedRect().intersected(QRect(QPoint(),tools_->document()->size()));if(!slice_.isEmpty())emit sliceCreated(slice_);}
    else if(tool_!=ImageEditorTool::Crop)finishSelection();dragging_=false;refreshPreview();update();
}
void ImageEditorCanvas::mouseDoubleClickEvent(QMouseEvent *event) {if(event->button()!=Qt::LeftButton)return;if(tool_==ImageEditorTool::Polygon && polygon_.size()>=3)finishSelection();else if(tool_==ImageEditorTool::Path && polygon_.size()>=2){vectorPath_={};vectorPath_.addPolygon(polygon_);polygon_.clear();update();}}
void ImageEditorCanvas::wheelEvent(QWheelEvent *event) {
    if(event->modifiers().testFlag(Qt::ControlModifier)){zoomBy(std::pow(1.2,event->angleDelta().y()/120.0),event->position());event->accept();return;}
    QPointF delta=event->pixelDelta().isNull()?QPointF(event->angleDelta())/3.:QPointF(event->pixelDelta());
    if(event->modifiers().testFlag(Qt::ShiftModifier))delta=QPointF(delta.y()!=0?delta.y():delta.x(),0);fit_=false;origin_+=delta;current_=documentPoint(event->position());emit cursorPositionChanged(current_,true);refreshPreview();emit viewChanged();event->accept();
}
void ImageEditorCanvas::keyPressEvent(QKeyEvent *event) {
    if(event->key()==Qt::Key_Escape){cancelGesture();cropRect_={};return;}
    if(!(event->modifiers()&(Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier))&&(event->key()==Qt::Key_Plus||event->key()==Qt::Key_Equal||event->key()==Qt::Key_Minus)){zoomBy(event->key()==Qt::Key_Minus?1/1.2:1.2);return;}
    if(freeTransform_&&(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter)){finishLayerTransform();freeTransform_=false;tools_->document()->commitEdit();update();return;}
    if(tool_==ImageEditorTool::Crop && (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) && !cropRect_.isEmpty()){tools_->document()->crop(cropRect_.toAlignedRect());cropRect_={};selectionOutline_={};fitToWindow();return;}
    if(tool_==ImageEditorTool::Path && (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) && polygon_.size()>=2){vectorPath_={};vectorPath_.addPolygon(polygon_);polygon_.clear();strokeVectorPath();update();return;}
    if(tool_==ImageEditorTool::Polygon){if((event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) && polygon_.size()>=3){finishSelection();return;}if(event->key()==Qt::Key_Backspace && !polygon_.isEmpty()){polygon_.removeLast();update();return;}}
    QWidget::keyPressEvent(event);
}
void ImageEditorCanvas::focusOutEvent(QFocusEvent *event) {cancelGesture();QWidget::focusOutEvent(event);}
void ImageEditorCanvas::leaveEvent(QEvent *event){emit cursorPositionChanged(current_,false);QWidget::leaveEvent(event);}
}
