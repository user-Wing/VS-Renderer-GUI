#include "player/PlayerPng.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QColorSpace>
#include <QScopeGuard>
#include <QElapsedTimer>
#include <QtEndian>
#include <libdeflate.h>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <memory>
#include <QThread>
#include <future>
#include <thread>
#include <vector>
#include <windows.h>

namespace vsr {
static QString writeScreenshotImage(const QString &path,const QImage &input,QJsonObject metadata){
    if(input.isNull())return QStringLiteral("No screenshot pixels");
    const bool hdr=metadata.value("hdr").toBool();
    if(input.colorSpace().transferFunction()==QColorSpace::TransferFunction::St2084 || input.colorSpace().transferFunction()==QColorSpace::TransferFunction::Hlg){metadata["pngPrimaries"]=9;metadata["pngTransfer"]=input.colorSpace().transferFunction()==QColorSpace::TransferFunction::St2084?16:18;}
    const auto original=hdr?input.convertToFormat(QImage::Format_RGBA32FPx4):QImage();
    QImage encoded=input;
    if(metadata.value("encoding").toString()=="scRGB-linear"){
        encoded=original.copy();
        const auto pq=[](double nits){const double p=std::pow(std::clamp(nits/10000.0,0.0,1.0),2610.0/16384.0);return float(std::pow((3424.0/4096.0+2413.0/128.0*p)/(1+2392.0/128.0*p),2523.0/32.0));};
        for(int y=0;y<encoded.height();++y){auto *row=reinterpret_cast<float *>(encoded.scanLine(y));for(int x=0;x<encoded.width();++x){auto *p=row+x*4;const double r=p[0]*80,g=p[1]*80,b=p[2]*80;
            p[0]=pq(.6274039*r+.3292830*g+.0433131*b);p[1]=pq(.0690973*r+.9195404*g+.0113623*b);p[2]=pq(.0163914*r+.0880133*g+.8955953*b);}}
        encoded.setColorSpace(QColorSpace(QColorSpace::Bt2100Pq));metadata["pngPrimaries"]=9;metadata["pngTransfer"]=16;
    }
    const auto image=encoded.convertToFormat(QImage::Format_RGBA64);
    const qsizetype stride=qsizetype(image.width())*8;
    QByteArray raw((stride+1)*image.height(),Qt::Uninitialized),above(stride,0),row(stride,Qt::Uninitialized),candidate(stride,Qt::Uninitialized);
    for(int y=0;y<image.height();++y){const auto *pixels=reinterpret_cast<const QRgba64 *>(image.constScanLine(y));
        for(int x=0;x<image.width();++x){const auto p=pixels[x];for(int c=0;c<4;++c)qToBigEndian<quint16>(c==0?p.red():c==1?p.green():c==2?p.blue():p.alpha(),reinterpret_cast<uchar *>(row.data()+x*8+c*2));}
        auto *destination=raw.data()+y*(stride+1);quint64 best=std::numeric_limits<quint64>::max();
        for(int filter=0;filter<5;++filter){quint64 score=0;for(qsizetype x=0;x<stride;++x){const int left=x>=8?uchar(row[x-8]):0,up=uchar(above[x]),upperLeft=x>=8?uchar(above[x-8]):0;int predictor=0;
            if(filter==1)predictor=left;else if(filter==2)predictor=up;else if(filter==3)predictor=(left+up)/2;else if(filter==4){const int p=left+up-upperLeft,a=std::abs(p-left),b=std::abs(p-up),c=std::abs(p-upperLeft);predictor=a<=b&&a<=c?left:b<=c?up:upperLeft;}
            const auto value=uchar(uchar(row[x])-predictor);candidate[x]=char(value);score+=std::min(int(value),256-int(value));}
            if(score<best){best=score;destination[0]=char(filter);std::memcpy(destination+1,candidate.constData(),size_t(stride));}}
        above.swap(row);
    }
    auto *compressor=libdeflate_alloc_compressor(12);if(!compressor)return QStringLiteral("PNG compressor allocation failed");
    const auto cleanup=qScopeGuard([&]{libdeflate_free_compressor(compressor);});
    const auto compress=[&](const QByteArray &bytes){QByteArray result(qsizetype(libdeflate_zlib_compress_bound(compressor,size_t(bytes.size()))),Qt::Uninitialized);const auto size=libdeflate_zlib_compress(compressor,bytes.constData(),size_t(bytes.size()),result.data(),size_t(result.size()));result.resize(qsizetype(size));return result;};
    const auto compressed=compress(raw);if(compressed.isEmpty())return QStringLiteral("PNG compression failed");
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return file.errorString();
    bool written=file.write("\x89PNG\r\n\x1a\n",8)==8;
    const auto chunk=[&](const char *type,const QByteArray &data){QByteArray block(data.size()+12,Qt::Uninitialized);qToBigEndian<quint32>(quint32(data.size()),reinterpret_cast<uchar *>(block.data()));std::memcpy(block.data()+4,type,4);std::memcpy(block.data()+8,data.constData(),size_t(data.size()));qToBigEndian<quint32>(libdeflate_crc32(0,block.constData()+4,size_t(data.size()+4)),reinterpret_cast<uchar *>(block.data()+8+data.size()));written&=file.write(block)==block.size();};
    QByteArray header(13,0);qToBigEndian<quint32>(image.width(),reinterpret_cast<uchar *>(header.data()));qToBigEndian<quint32>(image.height(),reinterpret_cast<uchar *>(header.data()+4));header[8]=16;header[9]=6;chunk("IHDR",header);
    if(metadata.value("pngPrimaries").toInt()>0 && metadata.value("pngTransfer").toInt()>0){QByteArray cicp(4,0);cicp[0]=char(metadata.value("pngPrimaries").toInt());cicp[1]=char(metadata.value("pngTransfer").toInt());cicp[3]=1;chunk("cICP",cicp);}
    const auto profile=image.colorSpace().iccProfile();if(!profile.isEmpty()){QByteArray iccp("Display",7);iccp.append('\0');iccp.append('\0');iccp.append(compress(profile));chunk("iCCP",iccp);}
    // Keep chunks below the PNG signed 31-bit length limit.
    for(qsizetype offset=0;offset<compressed.size();offset+=1024*1024)chunk("IDAT",compressed.mid(offset,1024*1024));chunk("IEND",{});
    if(!written)return file.errorString();
    metadata["width"]=image.width();metadata["height"]=image.height();metadata["pngBitDepthPerChannel"]=16;metadata["pngChannels"]="RGBA";metadata["compression"]="libdeflate level 12; adaptive PNG filters";
    if(hdr){const auto floatPath=metadata.take("floatPath").toString(path+".rgba32f");QSaveFile floating(floatPath);if(!floating.open(QIODevice::WriteOnly))return floating.errorString();
        for(int y=0;y<original.height();++y)if(floating.write(reinterpret_cast<const char *>(original.constScanLine(y)),qsizetype(original.width())*16)!=qsizetype(original.width())*16)return floating.errorString();
        if(!floating.commit())return floating.errorString();metadata["floatFile"]=QFileInfo(floatPath).fileName();metadata["floatFormat"]="RGBA float32 little-endian, interleaved, top-to-bottom";metadata["floatRowBytes"]=original.width()*16;
    }
    QSaveFile description(path+".json");if(!description.open(QIODevice::WriteOnly))return description.errorString();const auto json=QJsonDocument(metadata).toJson();if(description.write(json)!=json.size() || !description.commit())return description.errorString();
    return file.commit()?QString():file.errorString();
}
QString writeScreenshotPng(const QString &path,const QImage &input,QJsonObject metadata){
    if(!metadata.value("hdr").toBool())return writeScreenshotImage(path,input,metadata);
    const auto hdrPath=QFileInfo(path).absolutePath()+"/"+QFileInfo(path).completeBaseName()+".hdr.png";auto hdrMetadata=metadata;hdrMetadata["floatPath"]=path+".rgba32f";
    const auto error=writeScreenshotImage(hdrPath,input,hdrMetadata);if(!error.isEmpty())return error;
    auto linear=input.convertToFormat(QImage::Format_RGBA32FPx4);const auto encoding=metadata.value("encoding").toString();int transfer=metadata.value("pngTransfer").toInt();
    if(input.colorSpace().transferFunction()==QColorSpace::TransferFunction::St2084)transfer=16;else if(input.colorSpace().transferFunction()==QColorSpace::TransferFunction::Hlg)transfer=18;
    const bool bt2020=metadata.value("pngPrimaries").toInt()==9 || transfer==16 || transfer==18;
    const auto pq=[](double value){const auto p=std::pow(std::clamp(value,0.0,1.0),32.0/2523.0);return 10000*std::pow(std::max(p-3424.0/4096.0,0.0)/(2413.0/128.0-2392.0/128.0*p),16384.0/2610.0);};
    const auto hlg=[](double value){return value<=.5?value*value/3:(std::exp((value-.55991073)/.17883277)+.28466892)/12;};
    const auto srgb=[](double value){return value<=.0031308?12.92*value:1.055*std::pow(value,1/2.4)-.055;};
    QImage preview(input.size(),QImage::Format_RGBA64);preview.setColorSpace(QColorSpace(QColorSpace::SRgb));
    for(int y=0;y<linear.height();++y){const auto *row=reinterpret_cast<const float *>(linear.constScanLine(y));auto *out=reinterpret_cast<QRgba64 *>(preview.scanLine(y));for(int x=0;x<linear.width();++x){const auto *p=row+x*4;double rgb[]{p[0],p[1],p[2]};
        if(encoding=="scRGB-linear")for(auto &v:rgb)v*=80.0/203.0;
        else if(transfer==16)for(auto &v:rgb)v=pq(v)/203.0;
        else if(transfer==18){for(auto &v:rgb)v=hlg(std::max(0.0,v));const double luminance=.2627*rgb[0]+.6780*rgb[1]+.0593*rgb[2],gain=1000*std::pow(std::max(0.0,luminance),.2)/203;for(auto &v:rgb)v*=gain;}
        if(bt2020 && encoding!="scRGB-linear"){const double r=rgb[0],g=rgb[1],b=rgb[2];rgb[0]=1.660491*r-.587641*g-.072850*b;rgb[1]=-.124550*r+1.132900*g-.008349*b;rgb[2]=-.018151*r-.100579*g+1.118730*b;}
        const double luminance=std::max(0.0,.2126*rgb[0]+.7152*rgb[1]+.0722*rgb[2]);const double mapped=luminance*(2.51*luminance+.03)/(luminance*(2.43*luminance+.59)+.14);if(luminance>0)for(auto &v:rgb)v*=mapped/luminance;
        const auto sample=[](double value){return std::isfinite(value)?quint16(std::lround(std::clamp(value,0.0,1.0)*65535)):quint16(0);};out[x]=QRgba64::fromRgba64(sample(srgb(std::max(0.0,rgb[0]))),sample(srgb(std::max(0.0,rgb[1]))),sample(srgb(std::max(0.0,rgb[2]))),sample(p[3]));}}
    metadata["sourceHdr"]=true;metadata["hdr"]=false;metadata["floatEncoding"]=encoding;metadata["encoding"]="sRGB SDR preview";metadata["pngPrimaries"]=1;metadata["pngTransfer"]=13;metadata["hdrFile"]=QFileInfo(hdrPath).fileName();metadata["floatFile"]=QFileInfo(path+".rgba32f").fileName();metadata["floatRowBytes"]=input.width()*16;metadata["floatFormat"]="RGBA float32 little-endian, interleaved, top-to-bottom";metadata["sdrPreviewToneMap"]=encoding=="linear-RGB"?"ACES fitted luminance; relative linear units; sRGB gamut clip":"ACES fitted luminance; 203 nit reference white; sRGB gamut clip";
    return writeScreenshotImage(path,preview,metadata);
}
#if defined(__GNUC__) && defined(__x86_64__)
__attribute__((target("avx2"))) static void paethRow(uchar *out,const uchar *input,const uchar *above,size_t stride,bool continuation=false){
    const auto zero=_mm_setzero_si128();auto left=zero,upperLeft=zero;
    if(continuation){quint32 value=0;std::memcpy(&value,out-4,4);left=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero);if(above){std::memcpy(&value,above-4,4);upperLeft=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero);}}
    for(size_t x=0;x<stride;x+=4){
        quint32 value=0,upValue=0;std::memcpy(&value,input+x,4);if(above)std::memcpy(&upValue,above+x,4);
        const auto current=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero),up=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(upValue)),zero);
        const auto low=_mm_min_epi16(left,up),high=_mm_max_epi16(left,up);
        const auto threshold=_mm_sub_epi16(_mm_add_epi16(upperLeft,_mm_add_epi16(upperLeft,upperLeft)),_mm_add_epi16(left,up));
        auto predictor=_mm_blendv_epi8(high,upperLeft,_mm_cmpgt_epi16(threshold,low));
        predictor=_mm_blendv_epi8(low,predictor,_mm_cmpgt_epi16(high,threshold));
        left=_mm_and_si128(_mm_add_epi16(current,predictor),_mm_set1_epi16(255));upperLeft=up;
        value=quint32(_mm_cvtsi128_si32(_mm_packus_epi16(left,zero)));std::memcpy(out+x,&value,4);
    }
}
#endif
QImage decodeRgbaPng(const QString &path,const std::atomic<quint64> &generation,quint64 expected){
    QElapsedTimer timer;timer.start();
    QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()<33)return {};
    const auto *bytes=file.map(0,file.size());if(!bytes)return {};
    const auto unmap=qScopeGuard([&]{file.unmap(const_cast<uchar *>(bytes));});
    if(std::memcmp(bytes,"\x89PNG\r\n\x1a\n",8))return {};
    QByteArray compressed(qsizetype(file.size()-33),Qt::Uninitialized);qsizetype compressedSize=0;
    int width=0,height=0,dpmX=0,dpmY=0;bool end=false;QColorSpace space;
    auto *decoder=libdeflate_alloc_decompressor();if(!decoder)return {};
    const auto cleanup=qScopeGuard([&]{libdeflate_free_decompressor(decoder);});
    for(qint64 offset=8;offset+12<=file.size();){
        if(generation!=expected)return {};
        const auto size=qFromBigEndian<quint32>(bytes+offset);const auto *type=bytes+offset+4;const auto *data=type+4;
        if(size>0x7fffffffU || qint64(size)>file.size()-offset-12)return {};
        if(libdeflate_crc32(0,type,size_t(size)+4)!=qFromBigEndian<quint32>(data+size))return {};
        if(!std::memcmp(type,"IHDR",4)){
            if(offset!=8 || size!=13 || data[8]!=8 || data[9]!=6 || data[10] || data[11] || data[12])return {};
            const auto w=qFromBigEndian<quint32>(data),h=qFromBigEndian<quint32>(data+4);
            if(!w || !h || w>quint32(INT_MAX/4) || h>quint32(INT_MAX))return {};width=int(w);height=int(h);
        }else if(!std::memcmp(type,"IDAT",4)){if(!width || size>quint64(compressed.size()-compressedSize))return {};std::memcpy(compressed.data()+compressedSize,data,size);compressedSize+=size;}
        else if(!std::memcmp(type,"IEND",4)){if(size)return {};end=true;break;}
        else if(!std::memcmp(type,"sRGB",4)){space=QColorSpace(QColorSpace::SRgb);}
        else if(!std::memcmp(type,"iCCP",4)){
            quint32 name=0;while(name<size && data[name])++name;if(name>79 || name+2>=size || data[name+1])return {};
            QByteArray profile(16*1024*1024,Qt::Uninitialized);size_t written=0;
            if(libdeflate_zlib_decompress(decoder,data+name+2,size-name-2,profile.data(),profile.size(),&written)!=LIBDEFLATE_SUCCESS)return {};
            profile.resize(qsizetype(written));space=QColorSpace::fromIccProfile(profile);if(!space.isValid())return {};
        }else if(!std::memcmp(type,"pHYs",4) && size==9 && data[8]==1){dpmX=int(qFromBigEndian<quint32>(data));dpmY=int(qFromBigEndian<quint32>(data+4));}
        else if(!std::memcmp(type,"gAMA",4) || !std::memcmp(type,"cHRM",4) || !std::memcmp(type,"cICP",4) || !std::memcmp(type,"acTL",4) || !std::memcmp(type,"eXIf",4))return {};
        else if(!(type[0]&32))return {}; // Unknown critical chunks cannot be ignored.
        offset+=qint64(size)+12;
    }
    if(!end || !width || !compressedSize)return {};
    const auto parseMs=timer.elapsed();
    const size_t stride=size_t(width)*4,rawSize=(stride+1)*size_t(height);
    if(rawSize/size_t(height)!=stride+1)return {};
    compressed.resize(compressedSize);
    std::unique_ptr<uchar,decltype(&std::free)> raw(static_cast<uchar *>(std::malloc(rawSize)),std::free);if(!raw)return {};
    size_t written=0;if(libdeflate_zlib_decompress(decoder,compressed.constData(),compressed.size(),raw.get(),rawSize,&written)!=LIBDEFLATE_SUCCESS || written!=rawSize)return {};
    compressed.clear();
    const auto inflateMs=timer.elapsed();
    auto *rawPixels=raw.get();
    int filters[5]{};for(int y=0;y<height;++y){const auto filter=rawPixels[size_t(y)*(stride+1)];if(filter>4)return {};++filters[filter];}
    // Keep the in-place path under memory pressure. Large images with enough
    // headroom use a separate output so dependent rows can run concurrently.
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);GlobalMemoryStatusEx(&memory);
    int workers=rawSize>=128*1024*1024 && height>=512 && memory.ullAvailPhys>=rawSize*2?std::clamp(QThread::idealThreadCount()/2,1,16):1;
    auto *outputPixels=workers>1?static_cast<uchar *>(std::malloc(stride*size_t(height))):nullptr;
    if(!outputPixels){workers=1;outputPixels=rawPixels;}
    QImage image(outputPixels,width,height,qsizetype(stride),QImage::Format_RGBA8888,[](void *pixels){std::free(pixels);},outputPixels);if(image.isNull()){if(outputPixels!=rawPixels)std::free(outputPixels);return {};}
    if(outputPixels==rawPixels)raw.release();
    image.setColorSpace(space);image.setDotsPerMeterX(dpmX);image.setDotsPerMeterY(dpmY);
    const auto zero=_mm_setzero_si128();
#if defined(__GNUC__) && defined(__x86_64__)
    const bool avx2=__builtin_cpu_supports("avx2");
#endif
    const auto unfilter=[&](int y,size_t begin,size_t end){
        const auto filter=rawPixels[size_t(y)*(stride+1)];const auto *input=rawPixels+size_t(y)*(stride+1)+1+begin;auto *out=outputPixels+size_t(y)*stride+begin;const auto *above=y?outputPixels+size_t(y-1)*stride+begin:nullptr;const auto count=end-begin;
        if(filter==0){std::memmove(out,input,count);return;}
#if defined(__GNUC__) && defined(__x86_64__)
        if(filter==4 && avx2){paethRow(out,input,above,count,begin>0);return;}
#endif
        if(filter==2){for(size_t x=0;x<count;x+=16){if(x+16<=count){const auto a=_mm_loadu_si128(reinterpret_cast<const __m128i *>(input+x));const auto b=above?_mm_loadu_si128(reinterpret_cast<const __m128i *>(above+x)):zero;_mm_storeu_si128(reinterpret_cast<__m128i *>(out+x),_mm_add_epi8(a,b));}else for(;x<count;++x)out[x]=uchar(input[x]+(above?above[x]:0));}return;}
        auto left=zero,upperLeft=zero;
        if(begin){quint32 value=0;std::memcpy(&value,out-4,4);left=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero);if(above){std::memcpy(&value,above-4,4);upperLeft=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero);}}
        for(size_t x=0;x<count;x+=4){
            quint32 value=0,upValue=0;std::memcpy(&value,input+x,4);if(above)std::memcpy(&upValue,above+x,4);
            auto current=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(value)),zero);const auto up=_mm_unpacklo_epi8(_mm_cvtsi32_si128(int(upValue)),zero);auto predictor=left;
            if(filter==3)predictor=_mm_srli_epi16(_mm_add_epi16(left,up),1);
            else if(filter==4){
                const auto low=_mm_min_epi16(left,up),high=_mm_max_epi16(left,up);
                const auto threshold=_mm_sub_epi16(_mm_add_epi16(upperLeft,_mm_add_epi16(upperLeft,upperLeft)),_mm_add_epi16(left,up));
                const auto chooseHigh=_mm_andnot_si128(_mm_cmpgt_epi16(threshold,low),_mm_set1_epi16(-1));
                const auto chooseLow=_mm_andnot_si128(_mm_cmpgt_epi16(high,threshold),_mm_set1_epi16(-1));
                predictor=_mm_or_si128(_mm_or_si128(_mm_and_si128(chooseHigh,high),_mm_and_si128(chooseLow,low)),_mm_andnot_si128(_mm_or_si128(chooseHigh,chooseLow),upperLeft));
            }
            left=_mm_and_si128(_mm_add_epi16(current,predictor),_mm_set1_epi16(255));upperLeft=up;
            value=quint32(_mm_cvtsi128_si32(_mm_packus_epi16(left,zero)));std::memcpy(out+x,&value,4);
        }
    };
    if(workers==1){for(int y=0;y<height;++y){if(generation!=expected)return {};unfilter(y,0,stride);}}
    else {
        // Wavefront blocks preserve Paeth's left/previous-row dependencies.
        // A separate output allocation prevents parallel rows overwriting raw input.
        auto progress=std::make_unique<std::atomic<size_t>[]>(height);std::atomic<int> next{0};std::vector<std::future<void>> jobs;
        for(int i=0;i<workers;++i)jobs.push_back(std::async(std::launch::async,[&]{
            for(int y=next.fetch_add(1);y<height;y=next.fetch_add(1))for(size_t begin=0;begin<stride;begin+=4096){
                const size_t end=std::min(stride,begin+4096);const auto filter=rawPixels[size_t(y)*(stride+1)];
                while(y>0 && filter>=2 && progress[y-1].load(std::memory_order_acquire)<end && generation==expected)std::this_thread::yield();
                if(generation!=expected)return;unfilter(y,begin,end);progress[y].store(end,std::memory_order_release);
            }
        }));for(auto &job:jobs)job.get();if(generation!=expected)return {};
    }
    image.setText("sourceBitDepth","8");image.setText("sourcePixelFormat","PNG RGBA8");image.setText("pngStages",QString("parse=%1 inflate=%2 unfilter=%3 ms; filters=%4/%5/%6/%7/%8; predictorThreads=%9").arg(parseMs).arg(inflateMs-parseMs).arg(timer.elapsed()-inflateMs).arg(filters[0]).arg(filters[1]).arg(filters[2]).arg(filters[3]).arg(filters[4]).arg(workers));return image;
}
}
