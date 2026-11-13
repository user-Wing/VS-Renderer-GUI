#include "agent/AgentLocalImages.h"
#include "agent/AgentSession.h"
#include <QBuffer>
#include <QColorSpace>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QImageIOHandler>
#include <QImageReader>
#include <QtConcurrentRun>

namespace vsr {
namespace {
using Cancel=std::shared_ptr<std::atomic_bool>;
constexpr qint64 maxSourceBytes=32*1024*1024,maxPixels=64*1024*1024;
constexpr int maxDataUrlBytes=512*1024,maxDimension=32768;
QJsonObject failure(const QString &error) { return {{"ok",false},{"error",error}}; }
QJsonObject cancelled() { return {{"ok",false},{"cancelled",true},{"error","Image reading cancelled; no attachment was queued."}}; }
QByteArray encode(const QImage &image,const char *format,int quality=-1)
{
    QByteArray bytes;QBuffer buffer(&bytes);if(!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer,format,quality))return {};return bytes;
}
bool fits(const QByteArray &bytes,const QByteArray &mime)
{
    return !bytes.isEmpty() && 13+mime.size()+((bytes.size()+2)/3)*4<=maxDataUrlBytes;
}
QJsonObject prepareImage(const QString &path,const Cancel &stop)
{
    if(stop->load())return cancelled();QFile source(path);if(!QFileInfo(path).isFile() || !source.open(QIODevice::ReadOnly))return failure("Cannot open a regular local image: "+path);
    const qint64 sourceBytes=source.size();if(sourceBytes<=0 || sourceBytes>maxSourceBytes)return failure("Image source must be between 1 byte and 32 MiB.");
    QImageReader reader(&source);reader.setDecideFormatFromContent(true);reader.setAutoTransform(true);
    const auto original=reader.size();if(!original.isValid() || original.width()>maxDimension || original.height()>maxDimension || qint64(original.width())*original.height()>maxPixels)return failure("Image dimensions are unknown or exceed 32768 per edge / 64 Mi pixels.");
    auto oriented=original;if(reader.transformation() & QImageIOHandler::TransformationRotate90)oriented.transpose();
    const auto format=QString::fromLatin1(reader.format());const bool firstFrameOnly=reader.supportsAnimation();
    if(original.width()>1024 || original.height()>1024){auto size=original.scaled(1024,1024,Qt::KeepAspectRatio);size.setWidth(qMax(1,size.width()));size.setHeight(qMax(1,size.height()));reader.setScaledSize(size);}
    auto image=reader.read();if(image.isNull())return failure("Image decoding failed: "+reader.errorString());if(stop->load())return cancelled();
    if(image.width()>1024 || image.height()>1024){auto size=image.size().scaled(1024,1024,Qt::KeepAspectRatio);size.setWidth(qMax(1,size.width()));size.setHeight(qMax(1,size.height()));image=image.scaled(size,Qt::KeepAspectRatio,Qt::SmoothTransformation);}
    if(image.colorSpace().isValid())image.convertToColorSpace(QColorSpace::SRgb);
    QByteArray encoded,mime;
    for(int attempt=0;attempt<10;++attempt){
        if(stop->load())return cancelled();encoded=encode(image,"PNG");mime="image/png";if(fits(encoded,mime))break;
        if(!image.hasAlphaChannel())for(int quality:{90,80,65,50}){encoded=encode(image,"JPEG",quality);mime="image/jpeg";if(fits(encoded,mime))break;}
        if(fits(encoded,mime))break;
        image=image.scaled(qMax(1,image.width()*3/4),qMax(1,image.height()*3/4),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    }
    if(!fits(encoded,mime))return failure("Unable to encode a valid image attachment within 512 KiB.");
    const auto dataUrl=QByteArray("data:")+mime+";base64,"+encoded.toBase64();if(dataUrl.size()>maxDataUrlBytes)return failure("Encoded image exceeds the attachment size limit.");
    return {{"ok",true},{"path",path},{"mime",QString::fromLatin1(mime)},{"dataUrl",QString::fromLatin1(dataUrl)},
        {"width",image.width()},{"height",image.height()},{"original_width",oriented.width()},{"original_height",oriented.height()},
        {"resized",image.size()!=oriented},{"bytes",encoded.size()},{"source_bytes",sourceBytes},{"source_format",format},{"first_frame_only",firstFrameOnly}};
}
bool imageRoute(const AgentProvider &provider)
{
    return provider.modelCapability.value("imageInputSupported")==QJsonValue(true);
}
}

AgentLocalImages::AgentLocalImages(AgentSession *session):QObject(session),session_(session)
{
    if(session){connect(session,&AgentSession::toolExecutionCancelled,this,[this]{cancelJobs();});connect(session,&AgentSession::conversationReset,this,[this]{cancelJobs();});}
}
AgentLocalImages::~AgentLocalImages() { cancelJobs(); }
void AgentLocalImages::cancelJobs() { for(const auto &weak:jobs_)if(const auto job=weak.lock())job->store(true); }
void AgentLocalImages::registerTools()
{
    if(!session_ || registered_)return;registered_=true;
    const QJsonObject schema{{"type","object"},{"properties",QJsonObject{{"path",QJsonObject{{"type","string"},{"description","Absolute local image path or path relative to this Session's cwd."}}},{"cwd",QJsonObject{{"type","string"},{"description","Optional local working directory for this operation."}}}}},{"required",QJsonArray{"path"}},{"additionalProperties",false}};
    for(const auto &name:{"read_image","view_image"}){
        const QPointer<AgentLocalImages> self(this);session_->registerAsyncTool(name,"Decode a local image and queue a real visual attachment for the next model request. Success means queued, not yet analyzed. Requires explicit image-input capability; original remains unchanged. First frame only for animations; output fits 1024 pixels per edge and 512 KiB data URI.",schema,[self](const QJsonObject &args,AgentSession::Completion completion){
            if(!self || !self->session_){completion(failure("Image module is unavailable."));return;}const auto provider=self->session_->provider();if(!imageRoute(provider)){completion(failure("Select a source whose current model explicitly declares image input support."));return;}
            const auto raw=args.value("path").toString();if(raw.trimmed().isEmpty()){completion(failure("path must be non-empty."));return;}
            const auto cwd=QDir(self->session_->workingDirectory()).absoluteFilePath(QDir::fromNativeSeparators(args.value("cwd").toString(".")));if(!QFileInfo(cwd).isDir()){completion(failure("cwd is not an existing local directory."));return;}
            const auto path=QDir::cleanPath(QDir(cwd).absoluteFilePath(QDir::fromNativeSeparators(raw)));auto stop=std::make_shared<std::atomic_bool>(false);self->jobs_.removeIf([](const auto &weak){return weak.expired();});self->jobs_.append(stop);auto *watcher=new QFutureWatcher<QJsonObject>(self);
            connect(watcher,&QFutureWatcher<QJsonObject>::finished,self,[self,watcher,stop,provider,completion]{
                auto result=watcher->result();watcher->deleteLater();if(!self || !self->session_ || stop->load()){completion(cancelled());return;}if(!result.value("ok").toBool()){completion(result);return;}
                const auto current=self->session_->provider();if(current.baseUrl!=provider.baseUrl || current.model!=provider.model || !imageRoute(current)){completion(failure("The image-capable route changed while decoding; retry using the current model."));return;}
                const QJsonObject image{{"mime",result.value("mime")},{"dataUrl",result.value("dataUrl")},{"path",result.value("path")},{"width",result.value("width")},{"height",result.value("height")}};
                if(!self->session_->queueToolImageAttachment(image)){completion(failure("The Session refused the image attachment; check current image capability, active tool group and image count limits."));return;}
                result.remove("dataUrl");result.insert("attachment_queued",true);completion(result);
            });watcher->setFuture(QtConcurrent::run([path,stop]{return prepareImage(path,stop);}));
        });session_->setToolEnvironmentAccess(name,true,false);session_->setToolDelegationAllowed(name,false);session_->setToolImageInputRequired(name,true);
    }
}
}
