#include "player/PlayerPng.h"
#include <QFile>
#include <QColorSpace>
#include <QScopeGuard>
#include <QElapsedTimer>
#include <QtEndian>
#include <libdeflate.h>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <QThread>
#include <future>
#include <thread>
#include <vector>
#include <windows.h>

namespace vsr {
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
