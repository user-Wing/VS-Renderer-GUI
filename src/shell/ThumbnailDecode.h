#pragma once
// Stream-backed FFmpeg decoder, used only by the isolated thumbnail worker.
#include <windows.h>
#include <initguid.h>
#include <thumbcache.h>
#include <propsys.h>
#include <algorithm>
#include <atomic>
#include <string>
#include <cerrno>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace {
HMODULE module=nullptr;
struct Decoder {
    HMODULE util=nullptr,codec=nullptr,format=nullptr,scale=nullptr;
#define API(name) decltype(&::name) name=nullptr
    API(av_malloc);API(av_free);API(av_freep);API(av_frame_alloc);API(av_frame_free);
    API(av_dict_set);API(av_dict_free);
    API(avformat_alloc_context);API(avformat_open_input);API(avformat_find_stream_info);
    API(av_find_best_stream);API(av_read_frame);API(av_seek_frame);API(avformat_close_input);
    API(avio_alloc_context);API(avio_context_free);
    API(avcodec_find_decoder);API(avcodec_alloc_context3);API(avcodec_parameters_to_context);
    API(avcodec_open2);API(avcodec_send_packet);API(avcodec_receive_frame);API(avcodec_free_context);
    API(av_packet_alloc);API(av_packet_free);API(av_packet_unref);
    API(sws_getContext);API(sws_scale);API(sws_freeContext);
    API(sws_setColorspaceDetails);API(sws_getCoefficients);
#undef API
    ~Decoder(){for(auto lib:{scale,format,codec,util})if(lib)FreeLibrary(lib);}
    bool load(){
        wchar_t path[32768];const auto count=GetModuleFileNameW(module,path,32768);if(!count || count==32768)return false;
        const auto folder=std::wstring(path).substr(0,std::wstring(path).find_last_of(L"\\/")+1);
        const auto open=[&](const wchar_t *name){return LoadLibraryExW((folder+name).c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);};
        util=open(L"avutil-61.dll");codec=open(L"avcodec-63.dll");format=open(L"avformat-63.dll");scale=open(L"swscale-10.dll");
        if(!util||!codec||!format||!scale)return false;
#define LOAD(lib,name) name=reinterpret_cast<decltype(name)>(GetProcAddress(lib,#name));if(!name)return false
        LOAD(util,av_malloc);LOAD(util,av_free);LOAD(util,av_freep);LOAD(util,av_frame_alloc);LOAD(util,av_frame_free);LOAD(util,av_dict_set);LOAD(util,av_dict_free);
        LOAD(format,avformat_alloc_context);LOAD(format,avformat_open_input);LOAD(format,avformat_find_stream_info);LOAD(format,av_find_best_stream);LOAD(format,av_read_frame);LOAD(format,av_seek_frame);LOAD(format,avformat_close_input);LOAD(format,avio_alloc_context);LOAD(format,avio_context_free);
        LOAD(codec,avcodec_find_decoder);LOAD(codec,avcodec_alloc_context3);LOAD(codec,avcodec_parameters_to_context);LOAD(codec,avcodec_open2);LOAD(codec,avcodec_send_packet);LOAD(codec,avcodec_receive_frame);LOAD(codec,avcodec_free_context);LOAD(codec,av_packet_alloc);LOAD(codec,av_packet_free);LOAD(codec,av_packet_unref);
        LOAD(scale,sws_getContext);LOAD(scale,sws_scale);LOAD(scale,sws_freeContext);LOAD(scale,sws_setColorspaceDetails);LOAD(scale,sws_getCoefficients);
#undef LOAD
        return true;
    }
};
struct Input {
    IStream *stream;ULONGLONG deadline;uint64_t bytes=0;
    static int interrupt(void *opaque){return GetTickCount64()>static_cast<Input*>(opaque)->deadline;}
    static int read(void *opaque,uint8_t *buffer,int size){
        auto &i=*static_cast<Input*>(opaque);if(interrupt(opaque)||i.bytes>=256ULL*1024*1024)return AVERROR_EXIT;
        ULONG count=0;const auto hr=i.stream->Read(buffer,ULONG(size),&count);i.bytes+=count;
        if(FAILED(hr))return AVERROR(EIO);return count?int(count):AVERROR_EOF;
    }
    static int64_t seek(void *opaque,int64_t offset,int origin){
        auto &i=*static_cast<Input*>(opaque);if(interrupt(opaque))return AVERROR_EXIT;
        if(origin==AVSEEK_SIZE){STATSTG stat{};return SUCCEEDED(i.stream->Stat(&stat,STATFLAG_NONAME))?int64_t(stat.cbSize.QuadPart):AVERROR(EIO);}
        origin&=~AVSEEK_FORCE;if(origin<0||origin>2)return AVERROR(EINVAL);
        LARGE_INTEGER move;move.QuadPart=offset;ULARGE_INTEGER result;
        return SUCCEEDED(i.stream->Seek(move,DWORD(origin),&result))?int64_t(result.QuadPart):AVERROR(EIO);
    }
};
HRESULT decode(IStream *stream,UINT size,HBITMAP *bitmap,WTS_ALPHATYPE *alpha){
    Decoder d;if(!d.load())return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    Input input{stream,GetTickCount64()+15000};LARGE_INTEGER zero{};
    if(FAILED(stream->Seek(zero,STREAM_SEEK_SET,nullptr)))return E_FAIL;
    auto *buffer=static_cast<unsigned char*>(d.av_malloc(65536));if(!buffer)return E_OUTOFMEMORY;
    auto *io=d.avio_alloc_context(buffer,65536,0,&input,Input::read,nullptr,Input::seek);
    if(!io){d.av_free(buffer);return E_OUTOFMEMORY;}
    auto *format=d.avformat_alloc_context();AVCodecContext *codec=nullptr;AVPacket *packet=nullptr;AVFrame *frame=nullptr;SwsContext *scaler=nullptr;
    HRESULT result=E_FAIL;
    do {
        if(!format)break;
        format->pb=io;format->flags|=AVFMT_FLAG_CUSTOM_IO;format->interrupt_callback={Input::interrupt,&input};
        format->probesize=4*1024*1024;format->max_analyze_duration=AV_TIME_BASE;
        AVDictionary *options=nullptr;d.av_dict_set(&options,"protocol_whitelist","",0);
        const int opened=d.avformat_open_input(&format,nullptr,nullptr,&options);d.av_dict_free(&options);
        if(opened<0)break;
        // JPEG XL probing decodes the whole image. Decode it only once below.
        const bool jpegxl=format->nb_streams==1&&format->streams[0]->codecpar->codec_id==AV_CODEC_ID_JPEGXL;
        if(!jpegxl&&d.avformat_find_stream_info(format,nullptr)<0)break;
        const int index=d.av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);if(index<0)break;
        const auto *parameters=format->streams[index]->codecpar;
        if((!jpegxl&&(parameters->width<=0||parameters->height<=0))||int64_t(parameters->width)*parameters->height>67108864)break;
        const auto *decoder=d.avcodec_find_decoder(parameters->codec_id);if(!decoder)break;
        codec=d.avcodec_alloc_context3(decoder);if(!codec||d.avcodec_parameters_to_context(codec,parameters)<0)break;
        codec->thread_count=2;codec->max_pixels=67108864;
        if(d.avcodec_open2(codec,decoder,nullptr)<0)break;
        if(format->duration>2*AV_TIME_BASE){
            const auto target=std::min<int64_t>(10*AV_TIME_BASE,format->duration/10);
            d.av_seek_frame(format,-1,(format->start_time==AV_NOPTS_VALUE?0:format->start_time)+target,AVSEEK_FLAG_BACKWARD);
        }
        packet=d.av_packet_alloc();frame=d.av_frame_alloc();if(!packet||!frame)break;
        bool decoded=false;
        for(int n=0;n<4096&&!Input::interrupt(&input);++n){
            const int read=d.av_read_frame(format,packet);
            if(read<0){d.avcodec_send_packet(codec,nullptr);decoded=d.avcodec_receive_frame(codec,frame)==0;break;}
            if(packet->stream_index==index){
                const int sent=d.avcodec_send_packet(codec,packet);
                if(sent>=0 || sent==AVERROR(EAGAIN))decoded=d.avcodec_receive_frame(codec,frame)==0;
            }
            d.av_packet_unref(packet);if(decoded)break;
        }
        if(!decoded||frame->width<=0||frame->height<=0||int64_t(frame->width)*frame->height>67108864)break;
        const double factor=std::min(1.,double(size)/std::max(frame->width,frame->height));
        const int width=std::max(1,int(frame->width*factor)),height=std::max(1,int(frame->height*factor));
        scaler=d.sws_getContext(frame->width,frame->height,AVPixelFormat(frame->format),width,height,AV_PIX_FMT_BGRA,SWS_BILINEAR,nullptr,nullptr,nullptr);if(!scaler)break;
        const int matrix=frame->colorspace==AVCOL_SPC_BT709?SWS_CS_ITU709:frame->colorspace==AVCOL_SPC_BT2020_NCL?SWS_CS_BT2020:SWS_CS_DEFAULT;
        d.sws_setColorspaceDetails(scaler,d.sws_getCoefficients(matrix),frame->color_range==AVCOL_RANGE_JPEG,d.sws_getCoefficients(matrix),1,0,1<<16,1<<16);
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void *pixels=nullptr;auto image=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!image)break;
        uint8_t *output[]{static_cast<uint8_t*>(pixels),nullptr,nullptr,nullptr};int stride[]{width*4,0,0,0};
        if(d.sws_scale(scaler,frame->data,frame->linesize,0,frame->height,output,stride)!=height){DeleteObject(image);break;}
        // Shell expects premultiplied BGRA.
        auto *p=static_cast<uint8_t*>(pixels);for(int n=0;n<width*height;++n,p+=4)for(int c=0;c<3;++c)p[c]=uint8_t((unsigned(p[c])*p[3]+127)/255);
        *bitmap=image;*alpha=WTSAT_ARGB;result=S_OK;
    }while(false);
    if(scaler)d.sws_freeContext(scaler);d.av_frame_free(&frame);d.av_packet_free(&packet);d.avcodec_free_context(&codec);d.avformat_close_input(&format);d.av_freep(&io->buffer);d.avio_context_free(&io);
    return result;
}
}
