#include "ImageJxl.h"
#include <QColorSpace>
#include <QFile>
#include <QSaveFile>
#include <QScopeGuard>
#include <QThread>
#include <QXmlStreamReader>
#include <jxl/decode.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>
#include <algorithm>
#include <cstring>

namespace vsr {
QString writeJxl(const QString &path,const QImage &input) {
    if(input.isNull())return QStringLiteral("No JPEG XL pixels");
    auto image=input.convertToFormat(QImage::Format_RGBA64);
    const auto space=image.colorSpace().isValid()?image.colorSpace():QColorSpace(QColorSpace::SRgb);
    auto *encoder=JxlEncoderCreate(nullptr);
    if(!encoder)return QStringLiteral("JPEG XL encoder allocation failed");
    const auto cleanup=qScopeGuard([&]{JxlEncoderDestroy(encoder);});
    auto *runner=JxlThreadParallelRunnerCreate(nullptr,std::clamp(QThread::idealThreadCount(),1,8));
    const auto threads=qScopeGuard([&]{JxlThreadParallelRunnerDestroy(runner);});
    if(!runner || JxlEncoderSetParallelRunner(encoder,JxlThreadParallelRunner,runner)!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL thread initialization failed");
    JxlBasicInfo info;JxlEncoderInitBasicInfo(&info);
    info.xsize=image.width();info.ysize=image.height();info.bits_per_sample=16;
    info.num_color_channels=3;info.num_extra_channels=1;info.alpha_bits=16;
    info.uses_original_profile=JXL_TRUE;
    if(JxlEncoderSetBasicInfo(encoder,&info)!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL basic info rejected");
    JxlColorEncoding color;JxlColorEncodingSetToSRGB(&color,JXL_FALSE);
    bool structured=true;
    switch(space.primaries()) {
    case QColorSpace::Primaries::SRgb:break;
    case QColorSpace::Primaries::Bt2020:color.primaries=JXL_PRIMARIES_2100;break;
    case QColorSpace::Primaries::DciP3D65:color.primaries=JXL_PRIMARIES_P3;break;
    default:structured=false;
    }
    switch(space.transferFunction()) {
    case QColorSpace::TransferFunction::SRgb:break;
    case QColorSpace::TransferFunction::Linear:color.transfer_function=JXL_TRANSFER_FUNCTION_LINEAR;break;
    case QColorSpace::TransferFunction::St2084:color.transfer_function=JXL_TRANSFER_FUNCTION_PQ;break;
    case QColorSpace::TransferFunction::Hlg:color.transfer_function=JXL_TRANSFER_FUNCTION_HLG;break;
    default:structured=false;
    }
    const auto icc=space.iccProfile();
    const auto colorStatus=structured?JxlEncoderSetColorEncoding(encoder,&color):JxlEncoderSetICCProfile(encoder,reinterpret_cast<const uint8_t*>(icc.constData()),icc.size());
    if(colorStatus!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL color encoding rejected");
    const auto white=input.text("capSdrWhiteNits");
    if(!white.isEmpty()) {
        const auto xml=QString("<vscap capSdrWhiteNits=\"%1\"/>").arg(white.toDouble(),0,'g',17).toUtf8();
        if(JxlEncoderUseBoxes(encoder)!=JXL_ENC_SUCCESS || JxlEncoderAddBox(encoder,"xml ",reinterpret_cast<const uint8_t*>(xml.constData()),xml.size(),JXL_FALSE)!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL metadata rejected");
    }
    auto *frame=JxlEncoderFrameSettingsCreate(encoder,nullptr);
    if(!frame || JxlEncoderSetFrameLossless(frame,JXL_TRUE)!=JXL_ENC_SUCCESS || JxlEncoderFrameSettingsSetOption(frame,JXL_ENC_FRAME_SETTING_EFFORT,5)!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL lossless settings rejected");
    const JxlPixelFormat format{4,JXL_TYPE_UINT16,JXL_NATIVE_ENDIAN,0};
    if(JxlEncoderAddImageFrame(frame,&format,image.constBits(),image.sizeInBytes())!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL pixels rejected");
    JxlEncoderCloseInput(encoder);
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return file.errorString();
    QByteArray buffer(1024*1024,Qt::Uninitialized);
    JxlEncoderStatus status;
    do {
        auto *next=reinterpret_cast<uint8_t*>(buffer.data());size_t available=buffer.size();
        status=JxlEncoderProcessOutput(encoder,&next,&available);
        const qint64 count=buffer.size()-available;
        if(file.write(buffer.constData(),count)!=count)return file.errorString();
    } while(status==JXL_ENC_NEED_MORE_OUTPUT);
    if(status!=JXL_ENC_SUCCESS)return QStringLiteral("JPEG XL encoding failed");
    return file.commit()?QString():file.errorString();
}

QImage readJxl(const QString &path,QString *error) {
    const auto fail=[&](const QString &message){if(error)*error=message;return QImage();};
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return fail(file.errorString());
    auto *bytes=file.map(0,file.size());if(!bytes)return fail(QStringLiteral("Unable to map JPEG XL input"));
    const auto unmap=qScopeGuard([&]{file.unmap(bytes);});
    auto *decoder=JxlDecoderCreate(nullptr);if(!decoder)return fail(QStringLiteral("JPEG XL decoder allocation failed"));
    const auto cleanup=qScopeGuard([&]{JxlDecoderDestroy(decoder);});
    auto *runner=JxlThreadParallelRunnerCreate(nullptr,std::clamp(QThread::idealThreadCount(),1,8));
    const auto threads=qScopeGuard([&]{JxlThreadParallelRunnerDestroy(runner);});
    if(!runner || JxlDecoderSetParallelRunner(decoder,JxlThreadParallelRunner,runner)!=JXL_DEC_SUCCESS || JxlDecoderSubscribeEvents(decoder,JXL_DEC_BASIC_INFO|JXL_DEC_COLOR_ENCODING|JXL_DEC_FULL_IMAGE|JXL_DEC_BOX|JXL_DEC_BOX_COMPLETE)!=JXL_DEC_SUCCESS)return fail(QStringLiteral("JPEG XL decoder initialization failed"));
    JxlDecoderSetInput(decoder,bytes,file.size());JxlDecoderCloseInput(decoder);
    QImage image;QColorSpace space;QByteArray xml(4096,0);bool xmlActive=false,complete=false;QString white;
    const JxlPixelFormat format{4,JXL_TYPE_UINT16,JXL_NATIVE_ENDIAN,0};
    for(;;) {
        const auto status=JxlDecoderProcessInput(decoder);
        if(status==JXL_DEC_BASIC_INFO) {
            JxlBasicInfo info{};
            if(JxlDecoderGetBasicInfo(decoder,&info)!=JXL_DEC_SUCCESS || info.xsize>INT_MAX/8 || info.ysize>INT_MAX)return fail(QStringLiteral("Invalid JPEG XL dimensions"));
            image=QImage(int(info.xsize),int(info.ysize),QImage::Format_RGBA64);
            if(image.isNull())return fail(QStringLiteral("Unable to allocate JPEG XL pixels"));
            image.setText("sourceBitDepth",QString::number(info.bits_per_sample));image.setText("sourcePixelFormat","JPEG XL RGBA");
        } else if(status==JXL_DEC_COLOR_ENCODING) {
            JxlColorEncoding color{};
            if(JxlDecoderGetColorAsEncodedProfile(decoder,JXL_COLOR_PROFILE_TARGET_DATA,&color)==JXL_DEC_SUCCESS) {
                const auto p=color.primaries==JXL_PRIMARIES_SRGB?QColorSpace::Primaries::SRgb:color.primaries==JXL_PRIMARIES_2100?QColorSpace::Primaries::Bt2020:color.primaries==JXL_PRIMARIES_P3?QColorSpace::Primaries::DciP3D65:QColorSpace::Primaries::Custom;
                const auto t=color.transfer_function==JXL_TRANSFER_FUNCTION_SRGB?QColorSpace::TransferFunction::SRgb:color.transfer_function==JXL_TRANSFER_FUNCTION_PQ?QColorSpace::TransferFunction::St2084:color.transfer_function==JXL_TRANSFER_FUNCTION_HLG?QColorSpace::TransferFunction::Hlg:color.transfer_function==JXL_TRANSFER_FUNCTION_LINEAR?QColorSpace::TransferFunction::Linear:QColorSpace::TransferFunction::Custom;
                if(color.white_point==JXL_WHITE_POINT_D65 && p!=QColorSpace::Primaries::Custom && t!=QColorSpace::TransferFunction::Custom)space=QColorSpace(p,t);
            }
            if(!space.isValid()) {
                size_t size=0;
                if(JxlDecoderGetICCProfileSize(decoder,JXL_COLOR_PROFILE_TARGET_DATA,&size)==JXL_DEC_SUCCESS && size<=16*1024*1024) {
                    QByteArray icc(qsizetype(size),Qt::Uninitialized);
                    if(JxlDecoderGetColorAsICCProfile(decoder,JXL_COLOR_PROFILE_TARGET_DATA,reinterpret_cast<uint8_t*>(icc.data()),size)==JXL_DEC_SUCCESS)space=QColorSpace::fromIccProfile(icc);
                }
            }
        } else if(status==JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t size=0;
            if(image.isNull() || JxlDecoderImageOutBufferSize(decoder,&format,&size)!=JXL_DEC_SUCCESS || size!=size_t(image.sizeInBytes()) || JxlDecoderSetImageOutBuffer(decoder,&format,image.bits(),size)!=JXL_DEC_SUCCESS)return fail(QStringLiteral("JPEG XL output buffer rejected"));
        } else if(status==JXL_DEC_BOX) {
            JxlBoxType type;
            if(JxlDecoderGetBoxType(decoder,type,JXL_TRUE)==JXL_DEC_SUCCESS && !std::memcmp(type,"xml ",4))xmlActive=JxlDecoderSetBoxBuffer(decoder,reinterpret_cast<uint8_t*>(xml.data()),xml.size())==JXL_DEC_SUCCESS;
        } else if(status==JXL_DEC_BOX_COMPLETE && xmlActive) {
            const size_t unused=JxlDecoderReleaseBoxBuffer(decoder);xmlActive=false;
            QXmlStreamReader reader(xml.left(xml.size()-qsizetype(unused)));
            while(!reader.atEnd())if(reader.readNext()==QXmlStreamReader::StartElement && reader.name()==u"vscap")white=reader.attributes().value("capSdrWhiteNits").toString();
        } else if(status==JXL_DEC_BOX_NEED_MORE_OUTPUT) {
            JxlDecoderReleaseBoxBuffer(decoder);xmlActive=false; // Unrelated large XML is optional metadata.
        } else if(status==JXL_DEC_FULL_IMAGE)complete=true;
        else if(status==JXL_DEC_SUCCESS) {
            if(!complete)return fail(QStringLiteral("JPEG XL contains no complete image"));
            image.setColorSpace(space);if(!white.isEmpty())image.setText("capSdrWhiteNits",white);return image;
        } else return fail(QStringLiteral("Invalid or truncated JPEG XL image"));
    }
}
}
