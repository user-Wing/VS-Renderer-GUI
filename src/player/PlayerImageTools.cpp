#include "player/PlayerImageTools.h"
#include "ui/PreviewPane.h"
#include "player/PhotoCraftEditor.h"
#include "image/ImageHdr.h"
#include "player/PlayerPng.h"
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QToolButton>
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
bool nativeDocument(const QString &path) {
    const QStringList formats{"psd","psb","pcraft","dng","cr2","cr3","nef","nrw","arw","pef","orf","rw2","raf"};
    return QFileInfo::exists(path) && formats.contains(QFileInfo(path).suffix().toLower());
}

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
    connect(PhotoCraftEditor::instance(),&PhotoCraftEditor::errorOccurred,this,&PlayerImageTools::errorOccurred);
    const QStringList names{tr("PhotoCraft 图像编辑器"),tr("左转"),tr("右转"),tr("镜像"),tr("删除到回收站"),tr("设置为桌面背景"),tr("高精度导出和格式转换")};
    const QStringList ids{"edit","rotateLeft","rotateRight","mirror","recycle","wallpaper","convert"};
    for(int i=0;i<names.size();++i){auto *button=new QToolButton(this);button->setObjectName("imageTool_"+ids[i]);button->setAccessibleName(names[i]);button->setToolTip(names[i]);button->setIcon(toolIcon(i==6?8:i));button->setIconSize(QSize(24,24));button->setFixedSize(40,40);button->setToolButtonStyle(Qt::ToolButtonIconOnly);button->setEnabled(false);row->addWidget(button);
        button->setFixedSize(32,32);button->setIconSize(QSize(21,21));
        connect(button,&QToolButton::clicked,this,[this,i]{
            if(task_ || (!ready_ && !(i==0 && nativeDocument(source_))))return;
            if(i==0)editImage();
            else if(i<=3){const QTransform change=i==1?QTransform(0,-1,1,0,0,0):i==2?QTransform(0,1,-1,0,0,0):QTransform::fromScale(-1,1);pane_->setImageTransform(pane_->imageTransform()*change);}
            else if(i==4)recycleImage();else if(i==5)wallpaper();else convertImage();
        });
    }
    row->addStretch();hide();
}
PlayerImageTools::~PlayerImageTools() {if(task_){task_->requestInterruption();task_->wait();}}
void PlayerImageTools::setSource(const QString &path) {source_=path;setReady(false);setVisible(!path.isEmpty());}
void PlayerImageTools::setReady(bool ready) {ready_=ready;const bool document=nativeDocument(source_);for(auto *b:findChildren<QToolButton *>())b->setEnabled(!task_ && (ready || (b->objectName()=="imageTool_edit" && document)));}
void PlayerImageTools::editImage() {
    if(nativeDocument(source_)){PhotoCraftEditor::instance()->open(source_);return;}
    const auto suffix=QFileInfo(source_).suffix().toLower();
    const auto transform=pane_->imageTransform();const auto image=transform.isIdentity()?pane_->image():pane_->image().transformed(transform);
    const bool floating=image.format()==QImage::Format_RGBA32FPx4 || image.format()==QImage::Format_RGBA16FPx4 || image.format()==QImage::Format_RGBX32FPx4 || image.format()==QImage::Format_RGBX16FPx4 || image.format()==QImage::Format_RGBA32FPx4_Premultiplied || image.format()==QImage::Format_RGBA16FPx4_Premultiplied;
    const bool large=image.sizeInBytes()>=256*1024*1024;
    const QStringList nativeFormats{"psd","psb","pcraft","png","jpg","jpeg","tif","tiff","webp","gif","bmp","tga","ico","qoi","exr","hdr","pbm","pgm","ppm","pam","pfm","dng","cr2","cr3","nef","nrw","arw","pef","orf","rw2","raf"};
    if(!large && QFileInfo::exists(source_) && transform.isIdentity() && nativeFormats.contains(suffix) && !(floating && suffix=="png")){
        PhotoCraftEditor::instance()->open(source_);return;
    }
    if(image.isNull()){emit errorOccurred(tr("无法准备编辑图片，图像为空或内存不足。"));return;}
    const auto directory=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/photocraft-imports";
    if(!QDir().mkpath(directory)){emit errorOccurred(tr("无法创建高精度图像交换目录。"));return;}
    const auto path=directory+"/"+QFileInfo(source_).completeBaseName().left(96)+"-VSP-view-copy-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+(large?".vspimage":floating?".tif":".png");
    auto error=std::make_shared<QString>();
    task_=QThread::create([image,path,floating,large,name=QFileInfo(source_).fileName(),error]{
        if(large)*error=writeEditorImport(image,path,name);
        else if(floating){ImageDocument document(image.size(),ImagePrecision::Float32);document.setColorSpace(image.colorSpace());document.addLayer("VSP",image);ImageHdr::exportFloatTiff(&document,path,error.get());}
        else *error=writeScreenshotPng(path,image,{});
    });
    connect(task_,&QThread::finished,this,[this,path,error]{auto *finished=task_;task_=nullptr;finished->deleteLater();setReady(ready_);if(!error->isEmpty()){emit errorOccurred(*error);return;}PhotoCraftEditor::instance()->open(path);emit statusChanged(tr("已打开高精度视图副本，请在 PhotoCraft 中另存为。"));});
    setReady(ready_);emit statusChanged(tr("正在准备高精度图像…"));task_->start();
}
QString PlayerImageTools::writeEditorImport(const QImage &image,const QString &path,const QString &name) {
    if(image.isNull())return QStringLiteral("Empty editor import");
    const bool floating=image.pixelFormat().typeInterpretation()==QPixelFormat::FloatingPoint;
    const int bits=floating?32:image.depth()>32?16:8;
    const auto format=floating?QImage::Format_RGBA32FPx4:bits==16?QImage::Format_RGBA64:QImage::Format_RGBA8888;
    ImageSaveFile pixels(path+".raw");if(!pixels.open(QIODevice::WriteOnly))return pixels.errorString();
    const int rows=std::max(1,32*1024*1024/(image.width()*(bits/8)*4));
    for(int y=0;y<image.height();y+=rows){
        const auto band=image.copy(0,y,image.width(),std::min(rows,image.height()-y)).convertToFormat(format);
        if(band.isNull())return QStringLiteral("Cannot allocate editor import band");
        const qint64 rowBytes=qint64(image.width())*(bits/8)*4;
        for(int row=0;row<band.height();++row)if(pixels.write(reinterpret_cast<const char *>(band.constScanLine(row)),rowBytes)!=rowBytes)return pixels.errorString();
    }
    if(!pixels.commit())return pixels.errorString();
    const auto icc=image.colorSpace().iccProfile();
    if(!icc.isEmpty()){ImageSaveFile profile(path+".icc");if(!profile.open(QIODevice::WriteOnly)||profile.write(icc)!=icc.size()||!profile.commit())return profile.errorString();}
    ImageSaveFile manifest(path);if(!manifest.open(QIODevice::WriteOnly))return manifest.errorString();
    const auto json=QJsonDocument(QJsonObject{{"version",1},{"width",image.width()},{"height",image.height()},{"bits",bits},{"name",name},{"icc",!icc.isEmpty()},{"temporary",true}}).toJson(QJsonDocument::Compact);
    if(manifest.write(json)!=json.size()||!manifest.commit())return manifest.errorString();
    return {};
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
        else if(desktop){if(SystemParametersInfoW(SPI_SETDESKWALLPAPER,0,const_cast<wchar_t *>(reinterpret_cast<const wchar_t *>(output.destination.utf16())),SPIF_UPDATEINIFILE|SPIF_SENDCHANGE))emit statusChanged(tr("已设置为桌面背景"));else emit errorOccurred(tr("设置桌面背景失败(Windows 错误 %1)").arg(GetLastError()));}
        else emit statusChanged(tr("已保存：%1").arg(output.destination));
    },Qt::QueuedConnection);});
    task_->setParent(this);connect(task_,&QThread::finished,this,[this]{auto *finished=task_;task_=nullptr;finished->deleteLater();setReady(ready_);});setReady(ready_);task_->start();
}
void PlayerImageTools::convertImage() {
    auto output=currentOutput();auto *dialog=new QDialog(window());dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(tr("压缩和转换格式 · AWJimage"));dialog->setMinimumWidth(640);auto *form=new QFormLayout(dialog);
    auto *format=new QComboBox(dialog);format->setObjectName("imageConvertFormat");for(const auto &id:QStringList{"avif","webp","jxl","jpgli","png"})format->addItem(id=="jpgli"?QStringLiteral("JPEG(JPEGli)"):id.toUpper(),id);
    auto *mode=new QComboBox(dialog);mode->addItems({tr("固定编码质量"),tr("目标视觉质量(自动搜索)")});mode->setObjectName("imageConvertMode");
    auto *quality=new QSpinBox(dialog);quality->setRange(1,100);quality->setValue(70);quality->setObjectName("imageConvertQuality");
    auto *speed=new QSpinBox(dialog);speed->setRange(0,10);speed->setValue(5);speed->setObjectName("imageConvertSpeed");
    auto *chroma=new QComboBox(dialog);chroma->addItems({"420","422","444","auto"});chroma->setToolTip(tr("AVIF 使用 YUV；4:2:0 为兼容优先，auto 跟随源采样。"));
    auto *depth=new QComboBox(dialog);depth->setObjectName("imageConvertDepth");depth->addItem(tr("自动(8→10，最高12bit)"),"auto");for(const auto &bits:QStringList{"8","10","12"})depth->addItem(bits,bits);auto *alpha=new QComboBox(dialog);alpha->addItem(tr("自动保留透明"),"auto");alpha->addItem(tr("强制保留"),"force");alpha->addItem(tr("移除透明"),"off");
    form->addRow(tr("输出格式"),format);form->addRow(tr("压缩模式"),mode);form->addRow(tr("质量(1–100)"),quality);form->addRow(tr("速度(0慢/10快)"),speed);form->addRow(tr("色度采样"),chroma);form->addRow(tr("AVIF 位深"),depth);form->addRow(tr("Alpha"),alpha);
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
