#include "player/PlayerImage.h"
#include "image/RawFormats.h"
#include <QLibrary>
#include "player/PlayerPng.h"
#include "image/ImagePsd.h"
#include "image/ImageJxl.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QElapsedTimer>
#include <QProcess>
#include <QFile>
#include <QColorSpace>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTransform>
#include <QScopeGuard>
#include <avif/avif.h>
#include <turbojpeg.h>
#include <QImageIOHandler>
#include <limits>
#include <QTemporaryDir>
#include <algorithm>

namespace vsr {
namespace {
struct HeifGrid { QString filter; QColorSpace color; };

HeifGrid heifGrid(const QByteArray &json)
{
    const auto groups = QJsonDocument::fromJson(json).object().value("stream_groups").toArray();
    QJsonObject group;
    for (const auto &value : groups) {
        const auto candidate = value.toObject();
        if (candidate.value("type") != "Tile Grid") continue;
        const bool primary = candidate.value("disposition").toObject().value("default").toInt() == 1;
        if (group.isEmpty() || primary) group = candidate;
        if (primary) break;
    }
    const auto components = group.value("components").toArray();
    const auto streams = group.value("streams").toArray();
    if (components.isEmpty() || streams.isEmpty()) return {};
    const auto grid = components.first().toObject();
    const auto tiles = grid.value("subcomponents").toArray();
    const int w = grid.value("width").toInt(), h = grid.value("height").toInt();
    const int x = grid.value("horizontal_offset").toInt(), y = grid.value("vertical_offset").toInt();
    const int cw = grid.value("coded_width").toInt(), ch = grid.value("coded_height").toInt();
    if (tiles.isEmpty() || tiles.size() > 64 || x < 0 || y < 0 || w <= 0 || h <= 0 ||
        cw <= x || ch <= y || qint64(cw) * ch > 67108864 || w > cw - x || h > ch - y) return {};
    QString inputs;
    QStringList offsets;
    for (const auto &value : tiles) {
        const auto tile = value.toObject();
        const auto i = tile.value("stream_index").toInt(-1);
        const int left = tile.value("tile_horizontal_offset").toInt(-1);
        const int top = tile.value("tile_vertical_offset").toInt(-1);
        if (i < 0 || i >= streams.size() || left < 0 || top < 0 || left >= cw || top >= ch) return {};
        const int index = streams.at(i).toObject().value("index").toInt(-1);
        if (index < 0 || index > 1024) return {};
        inputs += QStringLiteral("[0:%1]").arg(index);
        offsets.append(QStringLiteral("%1_%2").arg(left).arg(top));
    }
    const auto first = streams.first().toObject();
    const auto primaries = first.value("color_primaries").toString();
    const auto transfer = first.value("color_transfer").toString();
    const auto gamut = primaries == "smpte432" ? QColorSpace::Primaries::DciP3D65 :
        primaries == "bt2020" ? QColorSpace::Primaries::Bt2020 :
        primaries == "bt709" ? QColorSpace::Primaries::SRgb : QColorSpace::Primaries::Custom;
    const auto curve = transfer == "iec61966-2-1" ? QColorSpace::TransferFunction::SRgb :
        transfer == "smpte2084" ? QColorSpace::TransferFunction::St2084 :
        transfer == "arib-std-b67" ? QColorSpace::TransferFunction::Hlg : QColorSpace::TransferFunction::Custom;
    if (tiles.size() > 1)
        inputs += QStringLiteral("xstack=inputs=%1:layout=%2:fill=black,").arg(tiles.size()).arg(offsets.join('|'));
    inputs += QStringLiteral("crop=w=%1:h=%2:x=%3:y=%4:exact=1[still]").arg(w).arg(h).arg(x).arg(y);
    return {inputs, gamut != QColorSpace::Primaries::Custom && curve != QColorSpace::TransferFunction::Custom ?
        QColorSpace(gamut, curve) : QColorSpace()};
}
}

PlayerImage::PlayerImage(QObject *parent) : QObject(parent), worker_(new QObject) {
    // Large still images must not inherit Qt's small default allocation limit.
    QImageReader::setAllocationLimit(0);
    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    thread_.start();
}
PlayerImage::~PlayerImage() { cancel(); thread_.quit(); thread_.wait(); }
bool PlayerImage::supports(const QString &path) {
    const auto suffix=QFileInfo(path).suffix().toLower().toLatin1();
    return playerRawExtensions().contains(QString::fromLatin1(suffix)) || QImageReader::supportedImageFormats().contains(suffix) ||
        QList<QByteArray>{"jpg","jpeg","jpe","jfif","png","webp","bmp","gif","tif","tiff","avif","heic","heif","jxl","jp2","j2k","j2c","jpf","jpm","jls","exr","tga","pcx","psd","psb","dds","qoi","hdr","wbmp","pbm","pgm","ppm","pnm"}.contains(suffix);
}
void PlayerImage::cancel() { ++generation_; }
void PlayerImage::openPage(const QString &path,int page){
    const auto generation=++generation_;QMetaObject::invokeMethod(worker_,[this,path,page,generation]{QImageReader reader(path);reader.setAutoTransform(true);QImage image;if(page==0 || reader.jumpToImage(page))image=reader.read();const auto error=reader.errorString();QMetaObject::invokeMethod(this,[this,generation,image,error]{if(generation!=generation_)return;if(image.isNull())emit failed(error);else emit loaded(image);},Qt::QueuedConnection);},Qt::QueuedConnection);
}
void PlayerImage::open(const QString &path) {
    const auto generation=++generation_;
    const auto runtime=QCoreApplication::applicationDirPath();
    QMetaObject::invokeMethod(worker_, [this,path,runtime,generation] {
        if(generation!=generation_)return;
        QElapsedTimer elapsed;elapsed.start();
        QTemporaryDir temporary;
        QString decoded=path,error;
        const auto suffix=QFileInfo(path).suffix().toLower();
        QImage image;
        QString decoderName;
        if(playerRawExtensions().contains(suffix)) {
            QLibrary native(QDir(runtime).filePath("FFF.Native.dll"));
            using Decode = int (*)(const char*, void (*)(void*, const quint16*, int, int, int), void*);
            const auto decode=reinterpret_cast<Decode>(native.resolve("VSR3FP_DecodeRawImage"));
            if(!decode)error="Native LibRaw decoding is unavailable; update FFF.Native.dll and libraw.dll.";
            else {
                const int result=decode(path.toUtf8().constData(),[](void *opaque,const quint16 *pixels,int width,int height,int stride){
                    auto &image=*static_cast<QImage *>(opaque);
                    image=QImage(width,height,QImage::Format_RGBA64);
                    if(image.isNull())return;
                    for(int y=0;y<height;++y){
                        const auto *src=reinterpret_cast<const quint16 *>(reinterpret_cast<const char *>(pixels)+qint64(y)*stride);
                        auto *dst=reinterpret_cast<QRgba64 *>(image.scanLine(y));
                        for(int x=0;x<width;++x)dst[x]=QRgba64::fromRgba64(src[x*3],src[x*3+1],src[x*3+2],65535);
                    }
                },&image);
                if(result!=0 || image.isNull())error="LibRaw could not decode this RAW image (native result "+QString::number(result)+").";
                else {image.setColorSpace(QColorSpace::SRgb);image.setText("sourceBitDepth","16");image.setText("sourcePixelFormat","RAW developed RGB48");decoderName="3FP / LibRaw / 16-bit RGB";}
            }
        }
        if(suffix=="jxl") {
            image=readJxl(path,&error);
            if(!image.isNull())decoderName="libjxl 0.12 / lossless HDR color";
        }
        if(suffix=="png"){
            image=decodeRgbaPng(path,generation_,generation);
            if(!image.isNull())decoderName="libdeflate / SIMD PNG / mapped input";
        }
        if(QList<QString>{"jpg","jpeg","jpe","jfif"}.contains(suffix)) {
            // Map compressed input and write SIMD output directly into Qt's pixels.
            // Qt's JPEG plugin otherwise converts/copies an RGB scanline to RGB32.
            QFile file(path);
            if(file.open(QIODevice::ReadOnly))if(auto *bytes=file.map(0,file.size())) {
                const auto unmap=qScopeGuard([&file,bytes]{file.unmap(bytes);});
                auto decoder=tj3Init(TJINIT_DECOMPRESS);
                if(decoder) {
                    const auto cleanup=qScopeGuard([decoder]{tj3Destroy(decoder);});
                    tj3Set(decoder,TJPARAM_STOPONWARNING,1);
                    if(tj3DecompressHeader(decoder,bytes,file.size())==0 && tj3Get(decoder,TJPARAM_PRECISION)==8 && tj3Get(decoder,TJPARAM_COLORSPACE)!=TJCS_CMYK && tj3Get(decoder,TJPARAM_COLORSPACE)!=TJCS_YCCK) {
                        image=QImage(tj3Get(decoder,TJPARAM_JPEGWIDTH),tj3Get(decoder,TJPARAM_JPEGHEIGHT),QImage::Format_RGB32);
                        image.setText("sourceBitDepth","8");
                        const char *sampling[]{"4:4:4","4:2:2","4:2:0","Gray","4:4:0","4:1:1","Unknown"};
                        image.setText("sourcePixelFormat",QString("JPEG %1").arg(QString::fromLatin1(sampling[std::clamp(tj3Get(decoder,TJPARAM_SUBSAMP),0,6)])));
                        unsigned char *icc=nullptr;size_t length=0;
                        if(tj3GetICCProfile(decoder,&icc,&length)==0 && icc){image.setColorSpace(QColorSpace::fromIccProfile(QByteArray(reinterpret_cast<char *>(icc),length)));tj3Free(icc);}
                        if(!image.isNull() && tj3Decompress8(decoder,bytes,file.size(),image.bits(),image.bytesPerLine(),TJPF_BGRA)==0) {
                            decoderName="libjpeg-turbo 3.2 / SIMD / mapped input";
                            QImageReader metadata(path);const int transformation=metadata.transformation();
                            if(transformation & QImageIOHandler::TransformationMirror)image=image.flipped(Qt::Horizontal);
                            if(transformation & QImageIOHandler::TransformationFlip)image=image.flipped(Qt::Vertical);
                            if(transformation & QImageIOHandler::TransformationRotate90)image=image.transformed(QTransform().rotate(90));
                        } else {error=QString::fromUtf8(tj3GetErrorStr(decoder));image={};}
                    }
                }
            }
        }
        if(suffix=="psd" || suffix=="psb"){
            auto document=ImagePsd::load(path,&error,nullptr,[this,generation]{return generation!=generation_;});if(document && generation==generation_){image=document->composite(QRect(QPoint(),document->size()),[this,generation]{return generation!=generation_;});error=document->storageError();if(!image.isNull()){image.setText("sourceBitDepth",document->precision()==ImagePrecision::Float32?"32":document->precision()==ImagePrecision::UInt16?"16":"8");image.setText("sourcePixelFormat",QString("PSD RGB · %1 layers").arg(document->layers().size()));decoderName="Native layered PSD/PSB";}}
        }
        if(suffix=="avif") {
            QFile file(path);
            if(!file.open(QIODevice::ReadOnly))error=file.errorString();
            else {
                struct Input { QFile *file; QByteArray buffer; std::atomic<quint64> *generation; quint64 expected; } input{&file,{},&generation_,generation};
                avifIO io{};io.sizeHint=file.size();io.data=&input;
                io.read=[](avifIO *io,uint32_t flags,uint64_t offset,size_t size,avifROData *out)->avifResult {
                    auto *input=static_cast<Input *>(io->data);
                    if(input->expected!=*input->generation || flags || offset>quint64(input->file->size()))return AVIF_RESULT_IO_ERROR;
                    if(!input->file->seek(offset))return AVIF_RESULT_IO_ERROR;
                    input->buffer=input->file->read(qMin<quint64>(size,quint64(input->file->size())-offset));
                    if(input->file->error()!=QFileDevice::NoError)return AVIF_RESULT_IO_ERROR;
                    out->data=reinterpret_cast<const uint8_t *>(input->buffer.constData());out->size=input->buffer.size();return AVIF_RESULT_OK;
                };
                auto *decoder=avifDecoderCreate();
                if(!decoder)error="Unable to allocate AVIF decoder";
                else {
                    const auto cleanup=qScopeGuard([decoder]{avifDecoderDestroy(decoder);});
                    decoder->maxThreads=std::clamp(QThread::idealThreadCount(),1,16);
                    decoder->imageSizeLimit=std::numeric_limits<uint32_t>::max();decoder->imageDimensionLimit=0;
                    decoder->codecChoice=AVIF_CODEC_CHOICE_DAV1D;
                    avifDecoderSetIO(decoder,&io);
                    const char *stage="parse";auto result=avifDecoderParse(decoder);
                    if(result==AVIF_RESULT_OK && generation==generation_){stage="decode";result=avifDecoderNextImage(decoder);}
                    if(result==AVIF_RESULT_OK && generation==generation_) {
                        const auto *source=decoder->image;
                        image=QImage(source->width,source->height,source->depth>8?QImage::Format_RGBA64:QImage::Format_RGBA8888);
                        image.setText("sourceBitDepth",QString::number(source->depth));image.setText("sourcePixelFormat",QString("%1 · CICP %2/%3/%4%5").arg(QString::fromLatin1(avifPixelFormatToString(source->yuvFormat))).arg(source->colorPrimaries).arg(source->transferCharacteristics).arg(source->matrixCoefficients).arg(source->matrixCoefficients==AVIF_MATRIX_COEFFICIENTS_IDENTITY?QString(" (GBR)"):QString()));
                        if(image.isNull())error="Unable to allocate AVIF pixels";
                        else {
                            avifRGBImage rgb;avifRGBImageSetDefaults(&rgb,source);rgb.depth=source->depth>8?16:8;
                            rgb.maxThreads=decoder->maxThreads;rgb.pixels=image.bits();rgb.rowBytes=image.bytesPerLine();
                            stage="RGB conversion";result=avifImageYUVToRGB(source,&rgb);
                            if(result==AVIF_RESULT_OK) {
                                if(source->icc.size)image.setColorSpace(QColorSpace::fromIccProfile(QByteArray(reinterpret_cast<const char *>(source->icc.data),source->icc.size)));
                                if(!image.colorSpace().isValid()){
                                    auto primaries=QColorSpace::Primaries::Custom;auto transfer=QColorSpace::TransferFunction::Custom;
                                    if(source->colorPrimaries==AVIF_COLOR_PRIMARIES_BT709)primaries=QColorSpace::Primaries::SRgb;
                                    else if(source->colorPrimaries==AVIF_COLOR_PRIMARIES_BT2020)primaries=QColorSpace::Primaries::Bt2020;
                                    else if(source->colorPrimaries==AVIF_COLOR_PRIMARIES_SMPTE432)primaries=QColorSpace::Primaries::DciP3D65;
                                    if(source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_SRGB)transfer=QColorSpace::TransferFunction::SRgb;
                                    else if(source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_LINEAR)transfer=QColorSpace::TransferFunction::Linear;
                                    else if(source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_PQ)transfer=QColorSpace::TransferFunction::St2084;
                                    else if(source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_HLG)transfer=QColorSpace::TransferFunction::Hlg;
                                    else if(source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_BT2020_10BIT || source->transferCharacteristics==AVIF_TRANSFER_CHARACTERISTICS_BT2020_12BIT)transfer=QColorSpace::TransferFunction::Bt2020;
                                    if(primaries!=QColorSpace::Primaries::Custom && transfer!=QColorSpace::TransferFunction::Custom)image.setColorSpace(QColorSpace(primaries,transfer));
                                }
                                if(source->transformFlags & AVIF_TRANSFORM_IROT)image=image.transformed(QTransform().rotate(-90*source->irot.angle));
                                if(source->transformFlags & AVIF_TRANSFORM_IMIR)image=image.mirrored(source->imir.axis!=0,source->imir.axis==0);
                            }
                        }
                    }
                    if(result!=AVIF_RESULT_OK){image={};error=QString("AVIF %1: %2").arg(QString::fromLatin1(stage),QString::fromLatin1(avifResultToString(result)))+" · "+QString::fromUtf8(decoder->diag.error);}
                }
            }
        }
        if(image.isNull() && suffix!="avif" && error.isEmpty() && generation==generation_) {
            QImageReader reader(decoded);reader.setAutoTransform(true);image=reader.read();
            if(image.isNull())error=reader.errorString();
            // FFmpeg provides additional still-image codecs absent from Qt's plugins.
            if(image.isNull() && suffix!="avif" && temporary.isValid()) {
                decoded=temporary.filePath("image.png");QProcess process;
                const auto bundled=QDir(runtime).filePath("ffmpeg.exe");
                QStringList args{"-v","error","-nostdin"};
                HeifGrid grid;
                if (suffix == "heic" || suffix == "heif") {
                    QProcess probe;
                    const auto probePath = QDir(runtime).filePath("ffprobe.exe");
                    probe.start(QFileInfo::exists(probePath) ? probePath : QStringLiteral("ffprobe.exe"),
                        {"-v","error","-show_stream_groups","-of","json",path});
                    if (probe.waitForFinished(5000) && !probe.exitCode())
                        grid = heifGrid(probe.readAllStandardOutput());
                    if (probe.state() != QProcess::NotRunning) { probe.kill(); probe.waitForFinished(); }
                    if (generation != generation_) return;
                }
                if (!grid.filter.isEmpty()) args << "-threads" << "1" << "-noautorotate";
                args << "-i" << path;
                if (!grid.filter.isEmpty()) args << "-filter_complex_threads" << "2" << "-filter_complex" << grid.filter
                    << "-map" << "[still]" << "-pix_fmt" << "rgb48be";
                args << "-frames:v" << "1" << "-y" << decoded;
                process.start(QFileInfo::exists(bundled)?bundled:QStringLiteral("ffmpeg.exe"), args);
                if(process.waitForStarted()) {
                    while(!process.waitForFinished(100))if(generation!=generation_){process.kill();process.waitForFinished();return;}
                    QImageReader fallback(decoded);fallback.setAutoTransform(true);image=fallback.read();
                    if (!image.isNull() && !grid.filter.isEmpty()) {
                        if (grid.color.isValid()) image.setColorSpace(grid.color);
                        decoderName = "FFmpeg HEIF grid / RGB16";
                    }
                    if(image.isNull())error=QString::fromUtf8(process.readAllStandardError())+fallback.errorString();
                }
            }
        }
        if(generation!=generation_)return;
        if(suffix=="png" && !image.isNull()) {
            // A VSCap/VSPlayer companion JSON restores PQ when a PNG decoder
            // ignores the high-dynamic-range ICC transfer characteristic.
            QFile sidecar(path+".json");
            if(sidecar.open(QIODevice::ReadOnly)) {
                const auto meta=QJsonDocument::fromJson(sidecar.read(16384)).object();
                const int transfer=meta.value("pngTransfer").toInt();
                if(transfer==16 || meta.value("pngColorSpace").toString()=="BT.2100-PQ")
                    image.setColorSpace(QColorSpace::Bt2100Pq);
                else if(transfer==18)image.setColorSpace(QColorSpace(QColorSpace::Primaries::Bt2020,QColorSpace::TransferFunction::Hlg));
            }
        }
        image.setText("decodeMilliseconds",QString::number(elapsed.elapsed()));
        image.setText("decoder",suffix=="avif"?QString("libavif / dav1d"):decoderName.isEmpty()?QString("Qt / FFmpeg"):decoderName);
        QMetaObject::invokeMethod(this,[this,generation,image,error] {
            if(generation!=generation_)return;
            if(image.isNull())emit failed(error);else emit loaded(image);
        },Qt::QueuedConnection);
    },Qt::QueuedConnection);
}
}
