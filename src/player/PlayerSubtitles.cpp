#include "player/PlayerSubtitles.h"
#include "backend/SubtitleAbi.h"
#include <QLibrary>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <array>

namespace vsr {
namespace {
QString assColor(const QVariantMap &style,const QString &name,const QString &fallback) {
    QColor color(style.value(name,fallback).toString()); if(!color.isValid()) color=QColor(fallback);
    const int alpha=255-style.value(name+"Opacity",100).toInt()*255/100;
    return QString("&H%1%2%3%4").arg(alpha,2,16,QChar('0')).arg(color.blue(),2,16,QChar('0')).arg(color.green(),2,16,QChar('0')).arg(color.red(),2,16,QChar('0')).toUpper();
}
QString srtToAss(const QString &srt,const QVariantMap &style,int slot) {
    QString font=style.value("font","Comic Sans MS").toString(); font.remove(','); font.remove('\n');
    QString text="[Script Info]\nScriptType: v4.00+\nPlayResX: 1920\nPlayResY: 1080\nWrapStyle: 0\nScaledBorderAndShadow: yes\n\n[V4+ Styles]\nFormat: Name,Fontname,Fontsize,PrimaryColour,SecondaryColour,OutlineColour,BackColour,Bold,Italic,Underline,StrikeOut,ScaleX,ScaleY,Spacing,Angle,BorderStyle,Outline,Shadow,Alignment,MarginL,MarginR,MarginV,Encoding\n";
    const int alignment=slot==1?8:style.value("alignment",2).toInt();
    text+=QString("Style: Default,%1,%2,%3,&H000000FF,%4,%5,%6,%7,0,0,%8,%9,%10,0,%11,%12,%13,%14,%15,%16,%17,1\n")
        .arg(font).arg(style.value("size",36).toInt()).arg(assColor(style,"color","#ffffff"),assColor(style,"outlineColor","#000000"),assColor(style,"shadowColor","#000000"))
        .arg(style.value("bold",false).toBool()?-1:0).arg(style.value("italic",false).toBool()?-1:0).arg(style.value("scaleX",100).toInt()).arg(style.value("scaleY",100).toInt()).arg(style.value("spacing",0).toInt())
        .arg(style.value("box",false).toBool()?3:1).arg(style.value("outline",2).toDouble()).arg(style.value("shadow",2).toDouble()).arg(alignment).arg(style.value("left",40).toInt()).arg(style.value("right",40).toInt()).arg(style.value("margin",48).toInt());
    text+="\n[Events]\nFormat: Layer,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text\n";
    const QRegularExpression cues(R"((\d{1,2}:\d{2}:\d{2})[,\.](\d{3})\s*-->\s*(\d{1,2}:\d{2}:\d{2})[,\.](\d{3})[^\n]*\n(.*?)(?=\n\s*\n|\z))",QRegularExpression::DotMatchesEverythingOption);
    auto matches=cues.globalMatch(srt); while(matches.hasNext()) { const auto cue=matches.next(); QString body=cue.captured(5).trimmed(); body.replace("\\","\\\\"); body.replace("{","\\{"); body.replace("}","\\}"); body.remove('\r'); body.replace('\n',"\\N");
        body.replace("<i>","{\\i1}");body.replace("</i>","{\\i0}");body.replace("<b>","{\\b1}");body.replace("</b>","{\\b0}"); body.remove(QRegularExpression("<[^>]*>"));
        text+=QString("Dialogue: 0,%1.%2,%3.%4,Default,,0,0,0,,%5\n").arg(cue.captured(1),cue.captured(2).left(2),cue.captured(3),cue.captured(4).left(2),body);
    } return text;
}
}
struct PlayerSubtitles::Impl {
    QLibrary library{QDir(QCoreApplication::applicationDirPath()).filePath("FFF.Native.dll")};
    using OpenAss=int(*)(const char*,const char*,int,void**); using RenderAss=int(*)(void*,int64_t,int32_t,int32_t,SubtitleBitmap*); using Copy=int(*)(void*,void*,uint32_t); using Destroy=void(*)(void*);
    using OpenBitmap=int(*)(const char*,int,void**); using ReadBitmap=int(*)(void*,SubtitleBitmap*); using SeekBitmap=int(*)(void*,int64_t);
    OpenAss openAss=nullptr;RenderAss renderAss=nullptr;Copy copyAss=nullptr,copyBitmap=nullptr;Destroy destroyAss=nullptr,destroyBitmap=nullptr;OpenBitmap openBitmap=nullptr;ReadBitmap readBitmap=nullptr;SeekBitmap seekBitmap=nullptr;
    struct Track { void *handle=nullptr; bool bitmap=false,srt=false,hasNext=false,eof=false; SubtitleBitmap frame,next; QImage image,nextImage; qint64 last=-1; }; std::array<Track,2> tracks; QSize emptyCanvas;
    qint64 renderedTime=-1; QSize renderedCanvas,renderedVideo; bool renderedVisible=false; QImage renderedImage;
    Impl() { library.setLoadHints(QLibrary::PreventUnloadHint); library.load();
        openAss=reinterpret_cast<OpenAss>(library.resolve("FFF3FP_OpenAssSubtitle"));renderAss=reinterpret_cast<RenderAss>(library.resolve("FFF3FP_RenderAssSubtitle"));copyAss=reinterpret_cast<Copy>(library.resolve("FFF3FP_CopyAssSubtitlePixels"));destroyAss=reinterpret_cast<Destroy>(library.resolve("FFF3FP_DestroyAssSubtitle"));
        openBitmap=reinterpret_cast<OpenBitmap>(library.resolve("FFF3FP_OpenBitmapSubtitle"));readBitmap=reinterpret_cast<ReadBitmap>(library.resolve("FFF3FP_ReadBitmapSubtitle"));copyBitmap=reinterpret_cast<Copy>(library.resolve("FFF3FP_CopyBitmapSubtitlePixels"));seekBitmap=reinterpret_cast<SeekBitmap>(library.resolve("FFF3FP_SeekBitmapSubtitle"));destroyBitmap=reinterpret_cast<Destroy>(library.resolve("FFF3FP_DestroyBitmapSubtitle"));
    }
    void clear(int slot) { renderedTime=-1; emptyCanvas={}; renderedImage={}; auto &t=tracks[slot]; if(t.handle) { if(t.bitmap && destroyBitmap) destroyBitmap(t.handle); else if(destroyAss) destroyAss(t.handle); } t={}; }
    ~Impl(){clear(0);clear(1);}
    QImage pixels(Track &t) {
        const auto &f=t.frame; if(f.pixelBytes==0 || f.width<=0 || f.height<=0 || f.stride<f.width*4 || f.pixelBytes>200000000) return {};
        QByteArray bytes(static_cast<int>(f.pixelBytes),Qt::Uninitialized); auto copy=t.bitmap?copyBitmap:copyAss;
        if(!copy || copy(t.handle,bytes.data(),f.pixelBytes)!=0) return {};
        return QImage(reinterpret_cast<const uchar *>(bytes.constData()),f.width,f.height,f.stride,QImage::Format_ARGB32_Premultiplied).copy();
    }
};
PlayerSubtitles::PlayerSubtitles():impl_(std::make_unique<Impl>()),worker_(new QObject) { worker_->moveToThread(&thread_);connect(&thread_,&QThread::finished,worker_,&QObject::deleteLater);thread_.start(); }
PlayerSubtitles::~PlayerSubtitles() { QMetaObject::invokeMethod(worker_,[this]{impl_.reset();},Qt::BlockingQueuedConnection);thread_.quit();thread_.wait(); }
void PlayerSubtitles::load(int slot,const QString &path,int stream,const QString &codec,const QVariantMap &style) {
    QMetaObject::invokeMethod(worker_,[this,slot,path,stream,codec,style]{
        impl_->clear(slot); if(path.isEmpty()) return; auto &t=impl_->tracks[slot]; QString actual=path; int index=stream; QTemporaryDir temp;
        t.srt=codec=="subrip" || QFileInfo(path).suffix().compare("srt",Qt::CaseInsensitive)==0;
        if(t.srt && stream<0) {
            QFile input(actual); if(!input.open(QIODevice::ReadOnly)) {emit errorOccurred(tr("无法读取字幕：%1").arg(actual));return;}
            const auto ass=srtToAss(QString::fromUtf8(input.readAll()),style,slot); input.close(); actual=temp.filePath("subtitle.ass"); QFile output(actual); if(!output.open(QIODevice::WriteOnly)) return; output.write(ass.toUtf8());output.close();index=-1;
        }
        t.bitmap=codec=="hdmv_pgs_subtitle" || codec=="dvd_subtitle" || QFileInfo(path).suffix().compare("sup",Qt::CaseInsensitive)==0;
        const auto utf8=actual.toUtf8(); int result=-7;
        if(t.bitmap && impl_->openBitmap) result=impl_->openBitmap(utf8.constData(),index,&t.handle);
        else if(impl_->openAss) { const auto fonts=(QFileInfo(path).absolutePath()+"\n"+QDir(QCoreApplication::applicationDirPath()).filePath("fonts")).toUtf8(); result=impl_->openAss(utf8.constData(),fonts.constData(),index,&t.handle); }
        if(result!=0) {impl_->clear(slot);emit errorOccurred(tr("字幕加载失败：%1(%2)").arg(path).arg(result));}
    },Qt::QueuedConnection);
}
void PlayerSubtitles::render(qint64 time,const QSize &canvas,const QSize &video,bool visible) {
    if(canvas.isEmpty() || pending_.exchange(true)) return;
    QMetaObject::invokeMethod(worker_,[this,time,canvas,video,visible]{
        if(impl_->renderedTime==time && impl_->renderedCanvas==canvas && impl_->renderedVideo==video && impl_->renderedVisible==visible) {pending_.store(false);return;}
        impl_->renderedTime=time;impl_->renderedCanvas=canvas;impl_->renderedVideo=video;impl_->renderedVisible=visible;
        const bool empty=!visible || (!impl_->tracks[0].handle && !impl_->tracks[1].handle);
        if(empty && impl_->emptyCanvas==canvas) {pending_.store(false);return;}
        impl_->emptyCanvas=empty?canvas:QSize();
        QImage image(canvas,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);QPainter painter(&image);
        const QSize fitted=video.isEmpty()?canvas:video.scaled(canvas,Qt::KeepAspectRatio);const QPoint origin((canvas.width()-fitted.width())/2,(canvas.height()-fitted.height())/2);
        if(visible) for(int slot=0;slot<2;++slot) { auto &t=impl_->tracks[slot]; if(!t.handle) continue;
            if(!t.bitmap) { t.frame={}; if(!impl_->renderAss || impl_->renderAss(t.handle,time,fitted.width(),fitted.height(),&t.frame)!=0) continue; if(t.frame.flags&8)impl_->renderedTime=-1; if(!(t.frame.flags&16) || t.image.isNull())t.image=impl_->pixels(t); }
            else {
                if(t.last>=0 && (time<t.last || time>t.last+50000000)) { impl_->seekBitmap(t.handle,qMax<qint64>(0,time-300000000)); t.frame={}; t.image={};t.nextImage={};t.hasNext=t.eof=false; }
                for(int n=0;n<128;++n) {
                    if(!t.hasNext) {
                        if(t.eof)break;SubtitleBitmap next;
                        if(impl_->readBitmap(t.handle,&next)!=0)break;
                        if(next.flags&2){t.eof=true;break;}if(next.flags&8)continue;
                        const auto previous=t.frame;t.frame=next;t.nextImage=impl_->pixels(t);t.frame=previous;
                        if(!next.pixelBytes)impl_->copyBitmap(t.handle,nullptr,0);
                        t.next=next;t.hasNext=true;
                    }
                    if(t.next.start>time)break;
                    t.frame=t.next;t.image=t.nextImage;t.hasNext=false;
                }
                t.last=time;if(time<t.frame.start || (t.frame.end>0 && time>=t.frame.end)) continue;
            }
            if(t.image.isNull()) continue;
            const double sx=t.bitmap && t.frame.canvasWidth>0?double(fitted.width())/t.frame.canvasWidth:1;
            const double sy=t.bitmap && t.frame.canvasHeight>0?double(fitted.height())/t.frame.canvasHeight:1;
            int y=qRound(t.frame.y*sy); if(slot==1 && !t.srt) y=qMax(0,fitted.height()-y-qRound(t.image.height()*sy));
            painter.drawImage(QRect(origin.x()+qRound(t.frame.x*sx),origin.y()+y,qRound(t.image.width()*sx),qRound(t.image.height()*sy)),t.image);
        }
        painter.end();
        if(image==impl_->renderedImage){pending_.store(false);return;}
        impl_->renderedImage=image;pending_.store(false);emit imageReady(image);
    },Qt::QueuedConnection);
}
}
