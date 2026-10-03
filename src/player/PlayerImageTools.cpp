#include "player/PlayerImageTools.h"
#include "ui/PreviewPane.h"
#include "image/ImageEditorWindow.h"
#include "image/ImagePsd.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QColorSpace>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QToolButton>
#include <QVBoxLayout>
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace vsr {
namespace {
class ImageSaveFile final : public QSaveFile {
public:
    using QSaveFile::QSaveFile;
protected:
    qint64 writeData(const char *data,qint64 size) override {
        if(QThread::currentThread()->isInterruptionRequested()){setErrorString(QStringLiteral("Image output cancelled"));return -1;}
        return QSaveFile::writeData(data,size);
    }
};
QIcon toolIcon(int index) {
    QPixmap pixels(48,48);pixels.fill(Qt::transparent);pixels.setDevicePixelRatio(2);
    QPainter p(&pixels);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor(index==0?"#777777":"#eeeeee"),1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    if(index==0){p.drawRoundedRect(QRectF(3,3,16,17),2,2);p.drawLine(7,16,16,7);p.drawLine(14,7,17,10);p.drawEllipse(QPointF(8,7),1,1);}
    if(index==1 || index==2){p.save();if(index==2){p.translate(24,0);p.scale(-1,1);}p.drawArc(QRectF(5,4,14,14),30*16,290*16);p.drawLine(4,8,4,3);p.drawLine(4,8,9,8);p.restore();}
    if(index==3){p.drawLine(12,3,12,21);p.drawPolygon(QPolygonF{QPointF(3,18),QPointF(9,6),QPointF(9,18)});p.drawPolygon(QPolygonF{QPointF(21,18),QPointF(15,6),QPointF(15,18)});}
    if(index==4){p.drawLine(4,6,20,6);p.drawRoundedRect(QRectF(6,6,12,15),1,1);p.drawLine(9,3,15,3);p.drawLine(10,10,10,17);p.drawLine(14,10,14,17);}
    if(index==5){p.drawRoundedRect(QRectF(2,4,20,14),1,1);p.drawLine(8,21,16,21);p.drawLine(12,18,12,21);p.drawPolyline(QPolygonF{QPointF(4,15),QPointF(9,9),QPointF(13,13),QPointF(17,8),QPointF(20,13)});}
    if(index==6){p.drawRect(QRectF(3,10,11,11));p.drawLine(10,14,21,3);p.drawLine(15,3,21,3);p.drawLine(21,3,21,9);}
    if(index==7){p.drawLine(5,2,5,18);p.drawLine(2,5,18,5);p.drawLine(18,5,18,22);p.drawLine(5,18,22,18);p.drawLine(9,8,9,15);p.drawLine(14,8,14,15);p.drawLine(8,10,15,10);p.drawLine(8,14,15,14);}
    if(index==8){p.drawLine(4,7,19,7);p.drawLine(16,4,19,7);p.drawLine(16,10,19,7);p.drawLine(20,17,5,17);p.drawLine(8,14,5,17);p.drawLine(8,20,5,17);}
    return QIcon(pixels);
}
QString extension(const QString &format) { return format=="jpgli"?QStringLiteral("jpg"):format; }
QRectF transformedBounds(const ImageOutput &output) { return output.transform.mapRect(QRectF(QPointF(),output.image.size())); }

// Preview uses a small copy; selection coordinates remain in full-resolution, oriented pixels.
class CropView final : public QWidget {
public:
    QImage thumbnail;
    QSize fullSize;
    QRect selection;
    double ratio = 0;
    QPoint anchor;
    QRect initial;
    QRect ratioReference;
    int dragging = 0;
    explicit CropView(const ImageOutput &output, QWidget *parent) : QWidget(parent) {
        fullSize=transformedBounds(output).size().toSize();selection=QRect(QPoint(),fullSize);
        thumbnail=output.image.scaled(1400,1000,Qt::KeepAspectRatio,Qt::FastTransformation).transformed(output.transform);
        setMinimumSize(480,320);setMouseTracking(true);setObjectName("imageCropView");
    }
    QRectF imageRect() const {const QSizeF fitted=QSizeF(fullSize).scaled(size(),Qt::KeepAspectRatio);return QRectF(QPointF((width()-fitted.width())/2,(height()-fitted.height())/2),fitted);}
    QPoint pixel(const QPointF &at) const {const auto r=imageRect();return QPoint(std::clamp(qRound((at.x()-r.x())*fullSize.width()/r.width()),0,fullSize.width()),std::clamp(qRound((at.y()-r.y())*fullSize.height()/r.height()),0,fullSize.height()));}
    QRectF screenSelection() const {const auto r=imageRect();return QRectF(r.x()+double(selection.x())/fullSize.width()*r.width(),r.y()+double(selection.y())/fullSize.height()*r.height(),double(selection.width())/fullSize.width()*r.width(),double(selection.height())/fullSize.height()*r.height());}
    void publish() {setProperty("selection",selection);update();}
    void setRatio(double value,bool establishReference) {
        if(establishReference)ratioReference=selection;selection=ratioReference;
        ratio=value;if(ratio>0){int w=selection.width(),h=qRound(w/ratio);if(h>selection.height()){h=selection.height();w=qRound(h*ratio);}const QPoint center=selection.center();selection=QRect(QPoint(center.x()-w/2,center.y()-h/2),QSize(std::max(1,w),std::max(1,h))).intersected(QRect(QPoint(),fullSize));}publish();
    }
    int hit(const QPointF &point) const {
        const auto s=screenSelection();const auto expanded=s.adjusted(-8,-8,8,8);if(!expanded.contains(point))return 32;
        int edges=0;if(std::abs(point.x()-s.left())<=8)edges|=1;else if(std::abs(point.x()-s.right())<=8)edges|=2;
        if(std::abs(point.y()-s.top())<=8)edges|=4;else if(std::abs(point.y()-s.bottom())<=8)edges|=8;
        return edges?edges:s.contains(point)?16:32;
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.fillRect(rect(),QColor("#101010"));const auto r=imageRect();p.drawImage(r,thumbnail);
        const QRectF s=screenSelection();
        QPainterPath mask;mask.addRect(r);mask.addRect(s);p.fillPath(mask,QColor(0,0,0,130));p.setPen(QPen(Qt::white,1));p.drawRect(s);
        for(int i=1;i<3;++i){p.drawLine(QPointF(s.x()+s.width()*i/3,s.top()),QPointF(s.x()+s.width()*i/3,s.bottom()));p.drawLine(QPointF(s.left(),s.y()+s.height()*i/3),QPointF(s.right(),s.y()+s.height()*i/3));}
        p.setBrush(Qt::white);for(const auto &point:QList<QPointF>{s.topLeft(),s.topRight(),s.bottomLeft(),s.bottomRight(),QPointF(s.center().x(),s.top()),QPointF(s.center().x(),s.bottom()),QPointF(s.left(),s.center().y()),QPointF(s.right(),s.center().y())})p.drawRect(QRectF(point-QPointF(3,3),QSizeF(6,6)));
    }
    void mousePressEvent(QMouseEvent *e) override {if(e->button()==Qt::LeftButton && imageRect().contains(e->position())){anchor=pixel(e->position());initial=selection;dragging=hit(e->position());if(dragging==32)selection=QRect(anchor,QSize(1,1));publish();}}
    void mouseMoveEvent(QMouseEvent *e) override {
        if(!dragging){const int edge=hit(e->position());setCursor(edge==16?Qt::SizeAllCursor:edge==32?Qt::CrossCursor:(edge==1||edge==2)?Qt::SizeHorCursor:(edge==4||edge==8)?Qt::SizeVerCursor:(edge==5||edge==10)?Qt::SizeFDiagCursor:Qt::SizeBDiagCursor);return;}
        const auto end=pixel(e->position());
        if(dragging==16){const auto delta=end-anchor;selection.moveTo(std::clamp(initial.x()+delta.x(),0,fullSize.width()-initial.width()),std::clamp(initial.y()+delta.y(),0,fullSize.height()-initial.height()));publish();return;}
        int left=initial.x(),top=initial.y(),right=left+initial.width(),bottom=top+initial.height();
        if(dragging==32){left=std::min(anchor.x(),end.x());right=std::max(anchor.x(),end.x());top=std::min(anchor.y(),end.y());bottom=std::max(anchor.y(),end.y());}
        else{if(dragging&1)left=std::min(end.x(),right-1);if(dragging&2)right=std::max(end.x(),left+1);if(dragging&4)top=std::min(end.y(),bottom-1);if(dragging&8)bottom=std::max(end.y(),top+1);}
        if(ratio>0){
            const bool fromLeft=dragging==32?end.x()<anchor.x():dragging&1,fromTop=dragging==32?end.y()<anchor.y():dragging&4;
            const bool horizontal=dragging==1 || dragging==2,vertical=dragging==4 || dragging==8;
            int w=std::max(1,right-left),h=std::max(1,bottom-top);const bool heightDriven=vertical || (!horizontal && std::abs(end.y()-anchor.y())*ratio>std::abs(end.x()-anchor.x()));if(heightDriven)w=std::max(1,qRound(h*ratio));else h=std::max(1,qRound(w/ratio));
            const int fixedX=horizontal?(fromLeft?right:left):vertical?initial.center().x():fromLeft?right:left;
            const int fixedY=vertical?(fromTop?bottom:top):horizontal?initial.center().y():fromTop?bottom:top;
            const int maxW=vertical?2*std::min(fixedX,fullSize.width()-fixedX):fromLeft?fixedX:fullSize.width()-fixedX;
            const int maxH=horizontal?2*std::min(fixedY,fullSize.height()-fixedY):fromTop?fixedY:fullSize.height()-fixedY;
            if(w>maxW){w=std::max(1,maxW);h=std::max(1,qRound(w/ratio));}if(h>maxH){h=std::max(1,maxH);w=std::max(1,qRound(h*ratio));}
            left=vertical?fixedX-w/2:fromLeft?fixedX-w:fixedX;top=horizontal?fixedY-h/2:fromTop?fixedY-h:fixedY;right=left+w;bottom=top+h;
        }
        selection=QRect(left,top,std::max(1,right-left),std::max(1,bottom-top)).intersected(QRect(QPoint(),fullSize));publish();
    }
    void mouseReleaseEvent(QMouseEvent *e) override {if(e->button()==Qt::LeftButton){if(dragging)mouseMoveEvent(e);dragging=0;ratioReference=selection;}}
};
QString chooseOutput(QWidget *parent,const QString &source,const QString &suffix) {
    QFileDialog dialog(parent,QObject::tr("另存图片"),QFileInfo(source).absoluteDir().filePath(QFileInfo(source).completeBaseName()+"-edited."+suffix),QString("%1 (*.%2)").arg(suffix.toUpper(),suffix));
    dialog.setAcceptMode(QFileDialog::AcceptSave);dialog.setDefaultSuffix(suffix);
    return dialog.exec()==QDialog::Accepted?dialog.selectedFiles().value(0):QString();
}
QDialogButtonBox *saveButtons(QDialog *dialog) {
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QObject::tr("另存为"));buttons->button(QDialogButtonBox::Cancel)->setText(QObject::tr("取消"));return buttons;
}
}

PlayerImageTools::PlayerImageTools(PreviewPane *pane,QWidget *parent) : QWidget(parent),pane_(pane) {
    setObjectName("playerImageToolbar");setFixedHeight(38);setStyleSheet("#playerImageToolbar{background:#1e1e1e;} QToolButton{background:transparent;border:0;border-radius:4px;} QToolButton:hover{background:#42464d;} QToolButton:disabled{color:#777;}");
    auto *row=new QHBoxLayout(this);row->setContentsMargins(12,3,12,3);row->setSpacing(12);
    const QStringList names{tr("详细编辑"),tr("左转"),tr("右转"),tr("镜像"),tr("删除到回收站"),tr("设置为桌面背景"),tr("调整图像大小"),tr("方格裁剪"),tr("压缩和转换格式")};
    const QStringList ids{"edit","rotateLeft","rotateRight","mirror","recycle","wallpaper","resize","crop","convert"};
    for(int i=0;i<names.size();++i){auto *button=new QToolButton(this);button->setObjectName("imageTool_"+ids[i]);button->setAccessibleName(names[i]);button->setToolTip(names[i]);button->setIcon(toolIcon(i));button->setIconSize(QSize(24,24));button->setFixedSize(40,40);button->setToolButtonStyle(Qt::ToolButtonIconOnly);button->setEnabled(false);row->addWidget(button);
        button->setFixedSize(32,32);button->setIconSize(QSize(21,21));
        connect(button,&QToolButton::clicked,this,[this,i]{
            if(task_ || (!ready_ && !(i==0 && (QFileInfo(source_).suffix().compare("psd",Qt::CaseInsensitive)==0 || QFileInfo(source_).suffix().compare("psb",Qt::CaseInsensitive)==0))))return;
            if(i==0)editImage();
            else if(i<=3){const QTransform change=i==1?QTransform(0,-1,1,0,0,0):i==2?QTransform(0,1,-1,0,0,0):QTransform::fromScale(-1,1);pane_->setImageTransform(pane_->imageTransform()*change);}
            else if(i==4)recycleImage();else if(i==5)wallpaper();else if(i==6)resizeImage();else if(i==7)cropImage();else convertImage();
        });
    }
    row->addStretch();hide();
}
PlayerImageTools::~PlayerImageTools() {if(task_){task_->requestInterruption();task_->wait();}}
void PlayerImageTools::setSource(const QString &path) {source_=path;setReady(false);setVisible(!path.isEmpty());}
void PlayerImageTools::setReady(bool ready) {ready_=ready;const auto suffix=QFileInfo(source_).suffix().toLower();for(auto *b:findChildren<QToolButton *>())b->setEnabled(!task_ && (ready || (b->objectName()=="imageTool_edit" && (suffix=="psd" || suffix=="psb"))));}
void PlayerImageTools::editImage() {
    auto existingEditor=[]()->ImageEditorWindow *{for(auto *widget:qApp->topLevelWidgets())if(auto *editor=qobject_cast<ImageEditorWindow *>(widget))return editor;return nullptr;};
    const auto suffix=QFileInfo(source_).suffix().toLower();
    if(suffix=="psd" || suffix=="psb"){
        struct Result{std::unique_ptr<ImageDocument> document;QString error;QStringList warnings;};auto result=std::make_shared<Result>();const auto path=source_;
        task_=QThread::create([result,path]{result->document=ImagePsd::load(path,&result->error,&result->warnings);if(result->document){const auto size=result->document->size();result->document->compositePreview(QRect(QPoint(),size),size.scaled({640,900},Qt::KeepAspectRatio));result->error=result->document->storageError();result->document->moveToThread(qApp->thread());}});
        connect(task_,&QThread::finished,this,[this,result,path,existingEditor]{task_=nullptr;setReady(ready_);if(!result->error.isEmpty()){emit errorOccurred(result->error);return;}auto *editor=existingEditor();if(editor)editor->addDocument(result->document.release(),path);else editor=new ImageEditorWindow(result->document.release(),path,window());editor->setWindowFlag(Qt::Window);editor->show();editor->raise();editor->activateWindow();if(!result->warnings.isEmpty())QMessageBox::information(editor,tr("PSD 导入说明"),result->warnings.join('\n'));});connect(task_,&QThread::finished,task_,&QObject::deleteLater);setReady(ready_);emit statusChanged(tr("正在读取 PSD 图层与可见预览…"));task_->start();return;
    }
    const auto transform=pane_->imageTransform();const auto image=transform.isIdentity()?pane_->image():pane_->image().transformed(transform);
    if(image.isNull()){emit errorOccurred(tr("无法准备编辑图片，图像为空或内存不足。"));return;}
    auto *editor=existingEditor();if(editor)editor->addImage(image,source_);else editor=new ImageEditorWindow(image,source_,window());editor->setWindowFlag(Qt::Window);editor->show();editor->raise();editor->activateWindow();
}
ImageOutput PlayerImageTools::currentOutput() const {ImageOutput output;output.source=source_;output.image=pane_->image();output.transform=pane_->imageTransform();return output;}
QString PlayerImageTools::backendPath() {return QDir(QCoreApplication::applicationDirPath()).filePath("runtime/awj/AWJ.exe");}
QStringList PlayerImageTools::conversionArguments(const ImageOutput &o,const QString &input,const QString &directory) {
    QStringList args{"-i",input,"-o",directory,"--format",o.format,"--template","result",o.visualQuality?"--visual-quality":"--quality",QString::number(o.quality),"--alpha",o.alpha,"--image-size-limit","none","--keep-metadata","--no-summary","--no-log"};
    if(o.format=="avif"){int bits=o.image.text("sourceBitDepth").toInt();if(bits<=0)bits=o.image.depth()>32?16:8;const auto depth=o.bitDepth=="auto"?QString::number(bits<=10?10:12):o.bitDepth;args<<"--avif-encoder"<<"aom"<<"--avif-color-representation"<<"yuv"<<"--chroma"<<o.chroma<<"--bit-depth"<<depth;}
    if(o.format=="jpgli")args<<"--chroma"<<o.chroma;
    if(o.format=="avif" || o.format=="webp" || o.format=="jxl")args<<"--speed"<<QString::number(o.speed);
    if(o.visualQuality)args<<"--visual-quality-gpu";
    return args;
}
QString PlayerImageTools::writeOutput(const ImageOutput &o) {
    if(o.image.isNull())return tr("没有可输出的图片。");
    if(QFileInfo(o.source).absoluteFilePath().compare(QFileInfo(o.destination).absoluteFilePath(),Qt::CaseInsensitive)==0)return tr("请选择另存路径，不能覆盖正在查看的原图。");
    QTemporaryDir temporary;if(!temporary.isValid())return tr("无法创建转换临时目录。");
    const bool edited=!o.transform.isIdentity() || !o.crop.isEmpty() || !o.size.isEmpty();
    QImage image=o.image;
    if(edited || o.format.isEmpty()) {
        if(!o.crop.isEmpty()){
            const auto bounds=transformedBounds(o);
            const QRect source=o.transform.inverted().mapRect(QRectF(o.crop).translated(bounds.topLeft())).toAlignedRect().intersected(image.rect());
            image=image.copy(source);
        }
        if(!o.size.isEmpty() && o.resizeAlgorithm!="anime4k"){
            const bool swap=std::abs(o.transform.m12())>.5;
            const QSize size=swap?QSize(o.size.height(),o.size.width()):o.size;
            image=resample(image,size,o.resizeAlgorithm);
        }
        image=image.transformed(o.transform);
        if(image.isNull())return tr("图像处理失败，内存不足或尺寸无效。");
    }
    if(!o.size.isEmpty() && o.resizeAlgorithm=="anime4k"){
        if(o.size.width()<image.width() || o.size.height()<image.height())return tr("Anime4K 用于放大，请用 Jinc 或 Lanczos 进行缩小。");
        const auto input=temporary.filePath("anime-input.png"),result=temporary.filePath("anime-output.png"),script=temporary.filePath("resize-anime.py");
        ImageSaveFile staged(input);if(!staged.open(QIODevice::WriteOnly))return staged.errorString();QImageWriter writer(&staged,"png");if(!writer.write(image))return writer.errorString();if(!staged.commit())return staged.errorString();
        QFile resource(":/image-tools/resize-anime.py"),file(script);if(!resource.open(QIODevice::ReadOnly) || !file.open(QIODevice::WriteOnly))return tr("无法准备 Anime4K 脚本。");file.write(resource.readAll());file.close();
        const QDir runtime(QCoreApplication::applicationDirPath());QProcess process;process.setProcessChannelMode(QProcess::MergedChannels);process.start(runtime.filePath("runtime/python/python.exe"),{script,input,result,runtime.filePath("shaders/anime4k-a-fast.glsl"),runtime.filePath("ffmpeg.exe"),QString::number(o.size.width()),QString::number(o.size.height())});
        if(!process.waitForStarted())return process.errorString();QByteArray log;while(!process.waitForFinished(100)){log+=process.readAll();if(log.size()>65536)log=log.right(65536);if(QThread::currentThread()->isInterruptionRequested()){process.kill();process.waitForFinished();return tr("已取消输出。");}}log+=process.readAll();
        if(process.exitCode()!=0 || process.exitStatus()!=QProcess::NormalExit)return tr("Anime4K 调整尺寸失败：%1").arg(QString::fromUtf8(log.right(4096)));
        const auto color=image.colorSpace();image=QImage(result);image.setColorSpace(color);if(image.isNull())return tr("无法读取 Anime4K 输出。");
    }
    if(QThread::currentThread()->isInterruptionRequested())return tr("已取消输出。");
    ImageSaveFile destination(o.destination);
    if(o.format.isEmpty()){
        if(!destination.open(QIODevice::WriteOnly))return destination.errorString();QImageWriter writer(&destination,o.destination.endsWith(".bmp",Qt::CaseInsensitive)?"bmp":"png");
        if(!writer.write(image))return writer.errorString();return destination.commit()?QString():destination.errorString();
    }
    QString input=o.source;
    if(edited){input=temporary.filePath("input.png");ImageSaveFile staged(input);if(!staged.open(QIODevice::WriteOnly))return staged.errorString();QImageWriter writer(&staged,"png");if(!writer.write(image))return writer.errorString();if(!staged.commit())return staged.errorString();}
    QProcess process;process.setWorkingDirectory(temporary.path());process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(backendPath(),conversionArguments(o,input,temporary.path()));if(!process.waitForStarted())return tr("无法启动 AWJimage：%1").arg(process.errorString());
    QByteArray log;
    while(!process.waitForFinished(100)){
        log+=process.readAll();if(log.size()>65536)log=log.right(65536);
        if(QThread::currentThread()->isInterruptionRequested()){process.kill();process.waitForFinished();return tr("已取消输出。");}
    }
    log+=process.readAll();
    if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)return tr("AWJimage 转换失败：%1").arg(QString::fromUtf8(log.right(4096)));
    QFile result(temporary.filePath("result."+extension(o.format)));if(!result.open(QIODevice::ReadOnly))return tr("AWJimage 未生成输出文件：%1").arg(result.errorString());
    if(!destination.open(QIODevice::WriteOnly))return destination.errorString();
    while(!result.atEnd()){const auto bytes=result.read(4*1024*1024);if(bytes.isEmpty())return result.errorString();if(destination.write(bytes)!=bytes.size())return tr("无法写入输出：%1").arg(destination.errorString());}
    return destination.commit()?QString():destination.errorString();
}
void PlayerImageTools::startOutput(const ImageOutput &output,bool desktop) {
    if(task_){emit errorOccurred(tr("图片输出正在进行，请等待完成。"));return;}
    emit statusChanged(desktop?tr("正在准备桌面背景…"):tr("正在输出图片…"));
    task_=QThread::create([this,output,desktop]{const auto error=writeOutput(output);QMetaObject::invokeMethod(this,[this,output,desktop,error]{
        if(!error.isEmpty())emit errorOccurred(error);
        else if(desktop){if(SystemParametersInfoW(SPI_SETDESKWALLPAPER,0,const_cast<wchar_t *>(reinterpret_cast<const wchar_t *>(output.destination.utf16())),SPIF_UPDATEINIFILE|SPIF_SENDCHANGE))emit statusChanged(tr("已设置为桌面背景"));else emit errorOccurred(tr("设置桌面背景失败（Windows 错误 %1）").arg(GetLastError()));}
        else emit statusChanged(tr("已保存：%1").arg(output.destination));
    },Qt::QueuedConnection);});
    task_->setParent(this);connect(task_,&QThread::finished,this,[this]{auto *finished=task_;task_=nullptr;finished->deleteLater();setReady(ready_);});setReady(ready_);task_->start();
}
void PlayerImageTools::resizeImage() {
    auto output=currentOutput();const QSize original=transformedBounds(output).size().toSize();
    auto *dialog=new QDialog(window());dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(tr("调整图像大小"));dialog->setMinimumWidth(560);auto *form=new QFormLayout(dialog);
    auto *width=new QSpinBox(dialog),*height=new QSpinBox(dialog);width->setObjectName("imageResizeWidth");height->setObjectName("imageResizeHeight");
    for(auto *s:{width,height})s->setRange(1,1000000);width->setValue(original.width());height->setValue(original.height());
    auto *aspect=new QCheckBox(tr("保持宽高比"),dialog);aspect->setChecked(true);aspect->setObjectName("imageResizeAspect");form->addRow(tr("宽度"),width);form->addRow(tr("高度"),height);form->addRow(aspect);
    auto *algorithm=new QComboBox(dialog);algorithm->setObjectName("imageResizeAlgorithm");for(const auto &id:QStringList{"jinc","lanczos4","lanczos3","bilinear","nearest","anime4k"})algorithm->addItem(id=="anime4k"?tr("Anime4K Mode A Fast（仅放大 / Vulkan）"):id=="lanczos4"?QStringLiteral("Lanczos 4 taps"):id=="jinc"?QStringLiteral("Jinc（EWA 2 lobes）"):id,id);form->addRow(tr("缩放算法"),algorithm);algorithm->setMinimumContentsLength(28);
    connect(width,&QSpinBox::valueChanged,dialog,[=](int v){if(aspect->isChecked()){const QSignalBlocker block(height);height->setValue(std::max(1,qRound(double(v)*original.height()/original.width())));}});
    connect(height,&QSpinBox::valueChanged,dialog,[=](int v){if(aspect->isChecked()){const QSignalBlocker block(width);width->setValue(std::max(1,qRound(double(v)*original.width()/original.height())));}});
    auto *buttons=saveButtons(dialog);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,this,[=,this]() mutable {output.destination=chooseOutput(dialog,output.source,"png");if(output.destination.isEmpty())return;output.size=QSize(width->value(),height->value());output.resizeAlgorithm=algorithm->currentData().toString();startOutput(output);dialog->accept();});dialog->show();
}
void PlayerImageTools::cropImage() {
    auto output=currentOutput();auto *dialog=new QDialog(window());dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(tr("方格裁剪 · 拖动选择区域"));dialog->resize(840,640);auto *layout=new QVBoxLayout(dialog);
    auto *view=new CropView(output,dialog);layout->addWidget(view,1);view->publish();auto *row=new QHBoxLayout;layout->addLayout(row);auto *ratio=new QComboBox(dialog);ratio->setObjectName("imageCropRatio");ratio->addItems({tr("自由比例"),"1:1","16:9","4:3","3:4","9:16",tr("自定义比例")});row->addWidget(ratio);
    auto *ratioWidth=new QSpinBox(dialog),*ratioHeight=new QSpinBox(dialog);ratioWidth->setObjectName("imageCropRatioWidth");ratioHeight->setObjectName("imageCropRatioHeight");for(auto *spin:{ratioWidth,ratioHeight}){spin->setRange(1,10000);spin->setValue(1);row->addWidget(spin);}row->addStretch();row->addWidget(new QLabel(tr("PNG · Lossless 无损输出"),dialog));
    const auto updateRatio=[=](bool establish){const int index=ratio->currentIndex();ratioWidth->setVisible(index==6);ratioHeight->setVisible(index==6);const QList<QSize> values{{0,1},{1,1},{16,9},{4,3},{3,4},{9,16}};const QSize value=index==6?QSize(ratioWidth->value(),ratioHeight->value()):values[index];view->setRatio(double(value.width())/value.height(),establish);};connect(ratio,&QComboBox::currentIndexChanged,dialog,[=]{updateRatio(true);});connect(ratioWidth,&QSpinBox::valueChanged,dialog,[=]{updateRatio(false);});connect(ratioHeight,&QSpinBox::valueChanged,dialog,[=]{updateRatio(false);});updateRatio(true);
    auto *buttons=saveButtons(dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,this,[=,this]() mutable {if(view->selection.isEmpty())return;output.destination=chooseOutput(dialog,output.source,"png");if(output.destination.isEmpty())return;output.crop=view->selection;startOutput(output);dialog->accept();});dialog->show();
}
void PlayerImageTools::convertImage() {
    auto output=currentOutput();auto *dialog=new QDialog(window());dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(tr("压缩和转换格式 · AWJimage"));dialog->setMinimumWidth(640);auto *form=new QFormLayout(dialog);
    auto *format=new QComboBox(dialog);format->setObjectName("imageConvertFormat");for(const auto &id:QStringList{"avif","webp","jxl","jpgli","png"})format->addItem(id=="jpgli"?QStringLiteral("JPEG（JPEGli）"):id.toUpper(),id);
    auto *mode=new QComboBox(dialog);mode->addItems({tr("固定编码质量"),tr("目标视觉质量（自动搜索）")});mode->setObjectName("imageConvertMode");
    auto *quality=new QSpinBox(dialog);quality->setRange(1,100);quality->setValue(70);quality->setObjectName("imageConvertQuality");
    auto *speed=new QSpinBox(dialog);speed->setRange(0,10);speed->setValue(5);speed->setObjectName("imageConvertSpeed");
    auto *chroma=new QComboBox(dialog);chroma->addItems({"420","422","444","auto"});chroma->setToolTip(tr("AVIF 使用 YUV；4:2:0 为兼容优先，auto 跟随源采样。"));
    auto *depth=new QComboBox(dialog);depth->setObjectName("imageConvertDepth");depth->addItem(tr("自动（8→10，最高12bit）"),"auto");for(const auto &bits:QStringList{"8","10","12"})depth->addItem(bits,bits);auto *alpha=new QComboBox(dialog);alpha->addItem(tr("自动保留透明"),"auto");alpha->addItem(tr("强制保留"),"force");alpha->addItem(tr("移除透明"),"off");
    form->addRow(tr("输出格式"),format);form->addRow(tr("压缩模式"),mode);form->addRow(tr("质量（1–100）"),quality);form->addRow(tr("速度（0慢/10快）"),speed);form->addRow(tr("色度采样"),chroma);form->addRow(tr("AVIF 位深"),depth);form->addRow(tr("Alpha"),alpha);
    auto *note=new QLabel(tr("固定质量只编码一次；视觉质量会反复编码并测量，适合追求体积。\nAVIF 使用 AOM，默认 YUV 4:2:0；GPU 仅加速视觉质量指标。\nPNG 100 无损；JPEG 100 为最高质量，仍为有损。"),dialog);note->setWordWrap(true);form->addRow(note);
    const auto update=[=]{const auto id=format->currentData().toString();speed->setEnabled(id=="avif"||id=="webp"||id=="jxl");chroma->setEnabled(id=="avif"||id=="jpgli");depth->setEnabled(id=="avif");alpha->setEnabled(id!="jpgli");if(id=="jpgli")alpha->setCurrentIndex(2);if(id=="png"){mode->setCurrentIndex(0);quality->setValue(100);}mode->setEnabled(id!="png");};connect(format,&QComboBox::currentIndexChanged,dialog,update);update();
    auto *buttons=saveButtons(dialog);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,this,[=,this]() mutable {output.format=format->currentData().toString();output.destination=chooseOutput(dialog,output.source,extension(output.format));if(output.destination.isEmpty())return;output.quality=quality->value();output.speed=speed->value();output.visualQuality=mode->currentIndex()==1;output.chroma=chroma->currentText();output.bitDepth=depth->currentData().toString();output.alpha=alpha->currentData().toString();startOutput(output);dialog->accept();});dialog->show();
}
void PlayerImageTools::recycleImage() {
    const auto path=source_;if(QMessageBox::question(this,tr("删除到回收站"),tr("将 %1 移入回收站？").arg(QFileInfo(path).fileName()))!=QMessageBox::Yes)return;
    if(!QFile::moveToTrash(path)){emit errorOccurred(tr("无法移入回收站：%1").arg(path));return;}emit imageRemoved(path);
}
void PlayerImageTools::wallpaper() {
    auto output=currentOutput();const auto directory=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/wallpaper";
    if(!QDir().mkpath(directory)){emit errorOccurred(tr("无法创建桌面背景目录。"));return;}
    output.destination=directory+"/VSPlayer-wallpaper.bmp";
    const auto screen=window()->screen();output.size=transformedBounds(output).size().toSize().scaled(screen?screen->size()*screen->devicePixelRatio():QSize(3840,2160),Qt::KeepAspectRatio);startOutput(output,true);
}
}
