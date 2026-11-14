// Private VS source: no colour conversion, no integer/float quantization.
// Opening indexes decoded frames once. Backward requests reopen and decode by
// ordinal rather than guessing a seek timestamp from the nominal frame rate.
#include <VapourSynth4.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/pixdesc.h>
}
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Stamp { std::int64_t pts, duration; };
struct Source {
    std::string path;
    AVFormatContext* format = nullptr;
    AVCodecContext* codec = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    int stream = -1, ordinal = -1;
    bool draining = false;
    AVRational timeBase{}, rate{};
    std::vector<Stamp> stamps;
    VSVideoInfo vi{};
    ~Source() { close(); }
    void close() {
        av_frame_free(&frame); av_packet_free(&packet);
        avcodec_free_context(&codec); avformat_close_input(&format);
    }
    void open() {
        close(); ordinal = -1; draining = false;
        if (avformat_open_input(&format, path.c_str(), nullptr, nullptr) < 0 ||
            avformat_find_stream_info(format, nullptr) < 0)
            throw std::runtime_error("Source could not open media");
        stream = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (stream < 0) throw std::runtime_error("Source has no video stream");
        auto* st = format->streams[stream];
        if (const auto* dv = av_packet_side_data_get(st->codecpar->coded_side_data,
                st->codecpar->nb_coded_side_data, AV_PKT_DATA_DOVI_CONF)) {
            if (dv->size < sizeof(AVDOVIDecoderConfigurationRecord))
                throw std::runtime_error("Invalid Dolby Vision base-layer configuration");
            const auto* config = reinterpret_cast<const AVDOVIDecoderConfigurationRecord*>(dv->data);
            const auto compatibility = config->dv_bl_signal_compatibility_id;
            if (!config->bl_present_flag || (compatibility != 1 && compatibility != 2 && compatibility != 4))
                throw std::runtime_error("Dolby Vision source has no supported HDR10/SDR/HLG base-layer fallback");
        }
        auto* decoder = st->codecpar->codec_id == AV_CODEC_ID_AV1 ? avcodec_find_decoder_by_name("libdav1d") : nullptr;
        if (!decoder) decoder = avcodec_find_decoder(st->codecpar->codec_id);
        codec = decoder ? avcodec_alloc_context3(decoder) : nullptr;
        if (!codec || avcodec_parameters_to_context(codec, st->codecpar) < 0)
            throw std::runtime_error("Source decoder allocation failed");
        codec->thread_count = 0;
        if (avcodec_open2(codec, decoder, nullptr) < 0)
            throw std::runtime_error("Source decoder could not open");
        packet = av_packet_alloc(); frame = av_frame_alloc();
        if (!packet || !frame) throw std::runtime_error("Source frame allocation failed");
        timeBase = st->time_base; rate = av_guess_frame_rate(format, st, nullptr);
    }
    bool next() {
        for (;;) {
            av_frame_unref(frame);
            int result = avcodec_receive_frame(codec, frame);
            if (result == 0) {
                const auto* parameters = format->streams[stream]->codecpar;
                if ((av_frame_get_side_data(frame, AV_FRAME_DATA_DOVI_METADATA) ||
                     av_frame_get_side_data(frame, AV_FRAME_DATA_DOVI_RPU_BUFFER)) &&
                    !av_packet_side_data_get(parameters->coded_side_data, parameters->nb_coded_side_data, AV_PKT_DATA_DOVI_CONF))
                    throw std::runtime_error("Dolby Vision base-layer compatibility could not be verified");
                ++ordinal; return true;
            }
            if (result == AVERROR_EOF) return false;
            if (result != AVERROR(EAGAIN)) throw std::runtime_error("Source video decode failed");
            if (draining) throw std::runtime_error("Source decoder did not drain");
            do {
                av_packet_unref(packet);
                result = av_read_frame(format, packet);
            } while (result >= 0 && packet->stream_index != stream);
            if (result == AVERROR_EOF) {
                draining = true;
                if (avcodec_send_packet(codec, nullptr) < 0)
                    throw std::runtime_error("Source decoder flush failed");
            } else if (result < 0) {
                throw std::runtime_error("Source media read failed");
            } else if (avcodec_send_packet(codec, packet) < 0) {
                throw std::runtime_error("Source decoder rejected a video packet");
            }
        }
    }
};

VSVideoFormat videoFormat(const AVFrame* f, VSCore* core, const VSAPI* api) {
    const auto* d = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(f->format));
    if (!d || (d->flags & (AV_PIX_FMT_FLAG_HWACCEL | AV_PIX_FMT_FLAG_BE | AV_PIX_FMT_FLAG_PAL | AV_PIX_FMT_FLAG_ALPHA)))
        throw std::runtime_error("Source pixel format is not supported by the lossless VS bridge");
    const bool rgb = (d->flags & AV_PIX_FMT_FLAG_RGB) != 0;
    const bool floating = (d->flags & AV_PIX_FMT_FLAG_FLOAT) != 0;
    const int depth = d->comp[0].depth;
    const int planes = av_pix_fmt_count_planes(static_cast<AVPixelFormat>(f->format));
    const int family = d->nb_components == 1 ? cfGray : rgb ? cfRGB : cfYUV;
    if ((d->nb_components != 1 && d->nb_components != 3) ||
        (planes != d->nb_components && !(rgb && planes == 1 && d->nb_components == 3)) ||
        (floating ? (depth != 16 && depth != 32) : (depth != 8 && depth != 10 && depth != 12 && depth != 16)) ||
        (!floating && family == cfGray && depth != 8 && depth != 16) ||
        (floating && family == cfYUV) || (!rgb && (d->log2_chroma_w > 1 || d->log2_chroma_h > 1 ||
                                               (d->log2_chroma_w == 0 && d->log2_chroma_h != 0))))
        throw std::runtime_error("Source requires planar Gray/YUV420/422/444 or three-component RGB in 8/10/12/16-bit integer or 16/32-bit float");
    for (int i = 0; i < d->nb_components; ++i)
        if (d->comp[i].depth != depth || d->comp[i].shift != 0 || d->comp[i].offset < 0 ||
            d->comp[i].step < (depth + 7) / 8 ||
            d->comp[i].offset + (depth + 7) / 8 > d->comp[i].step)
            throw std::runtime_error("Source pixel samples are not directly representable in VS");
    VSVideoFormat result{};
    if (!api->queryVideoFormat(&result, family, floating ? stFloat : stInteger, depth,
                              family == cfYUV ? d->log2_chroma_w : 0,
                              family == cfYUV ? d->log2_chroma_h : 0, core))
        throw std::runtime_error("Source format is unavailable in this VS core");
    return result;
}

const AVFrameSideData* sideData(Source& source, AVFrameSideDataType type, AVPacketSideDataType streamType) {
    if (const auto* found = av_frame_get_side_data(source.frame, type)) return found;
    const auto* parameters = source.format->streams[source.stream]->codecpar;
    const auto* blob = av_packet_side_data_get(parameters->coded_side_data, parameters->nb_coded_side_data, streamType);
    if (blob && blob->size) {
        auto* target = av_frame_new_side_data(source.frame, type, blob->size);
        if (!target) throw std::runtime_error("Source metadata allocation failed");
        std::memcpy(target->data, blob->data, blob->size); return target;
    }
    return nullptr;
}

void properties(Source& source, VSMap* props, const VSAPI* api) {
    const auto* f = source.frame;
    const auto* stream = source.format->streams[source.stream]->codecpar;
    const auto matrix = f->colorspace == AVCOL_SPC_UNSPECIFIED ? stream->color_space : f->colorspace;
    const auto primaries = f->color_primaries == AVCOL_PRI_UNSPECIFIED ? stream->color_primaries : f->color_primaries;
    const auto transfer = f->color_trc == AVCOL_TRC_UNSPECIFIED ? stream->color_trc : f->color_trc;
    const auto range = f->color_range == AVCOL_RANGE_UNSPECIFIED ? stream->color_range : f->color_range;
    auto chroma = f->chroma_location == AVCHROMA_LOC_UNSPECIFIED ? stream->chroma_location : f->chroma_location;
    if (chroma == AVCHROMA_LOC_UNSPECIFIED && (f->format == AV_PIX_FMT_YUVJ420P ||
        f->format == AV_PIX_FMT_YUVJ422P || f->format == AV_PIX_FMT_YUVJ444P)) chroma = AVCHROMA_LOC_CENTER;
    api->mapSetInt(props, "_Matrix", matrix, maReplace);
    api->mapSetInt(props, "_Primaries", primaries, maReplace);
    api->mapSetInt(props, "_Transfer", transfer, maReplace);
    if (range != AVCOL_RANGE_UNSPECIFIED) api->mapSetInt(props, "_ColorRange", range == AVCOL_RANGE_JPEG ? 0 : 1, maReplace);
    if (chroma != AVCHROMA_LOC_UNSPECIFIED) api->mapSetInt(props, "_ChromaLocation", chroma - 1, maReplace);
    const std::int64_t cicp[] = {matrix, primaries, transfer,
        range == AVCOL_RANGE_UNSPECIFIED ? -1 : range == AVCOL_RANGE_JPEG ? 0 : 1,
        chroma == AVCHROMA_LOC_UNSPECIFIED ? -1 : chroma - 1};
    api->mapSetIntArray(props, "_VsrSourceCICP", cicp, 5);
    api->mapSetInt(props, "_FieldBased", f->flags & AV_FRAME_FLAG_INTERLACED ? (f->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST ? 2 : 1) : 0, maReplace);
    if (f->sample_aspect_ratio.num > 0 && f->sample_aspect_ratio.den > 0) {
        api->mapSetInt(props, "_SARNum", f->sample_aspect_ratio.num, maReplace);
        api->mapSetInt(props, "_SARDen", f->sample_aspect_ratio.den, maReplace);
    }
    const auto stamp = source.stamps.at(source.ordinal);
    if (stamp.duration > 0) {
        if (stamp.duration > INT64_MAX / source.timeBase.num) throw std::runtime_error("Source frame duration overflow");
        const std::int64_t num = stamp.duration * source.timeBase.num, den = source.timeBase.den;
        const auto divisor = std::gcd(num, den);
        api->mapSetInt(props, "_DurationNum", num / divisor, maReplace);
        api->mapSetInt(props, "_DurationDen", den / divisor, maReplace);
    }
    if (stamp.pts != AV_NOPTS_VALUE) api->mapSetFloat(props, "_AbsoluteTime", (stamp.pts - (source.stamps.front().pts == AV_NOPTS_VALUE ? 0 : source.stamps.front().pts)) * av_q2d(source.timeBase), maReplace);
    api->mapSetInt(props, "_VsrSourceFrame", source.ordinal, maReplace);
    const bool streamPlus = av_packet_side_data_get(stream->coded_side_data, stream->nb_coded_side_data,
        AV_PKT_DATA_DYNAMIC_HDR10_PLUS) != nullptr;
    api->mapSetInt(props, "_VsrStreamHDR10Plus", streamPlus ? 1 : 0, maReplace);
    const auto* master = sideData(source, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA, AV_PKT_DATA_MASTERING_DISPLAY_METADATA);
    const auto* light = sideData(source, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL, AV_PKT_DATA_CONTENT_LIGHT_LEVEL);
    if (master && master->size >= sizeof(AVMasteringDisplayMetadata)) {
        const auto* m = reinterpret_cast<const AVMasteringDisplayMetadata*>(master->data);
        if (m->has_primaries) {
            double x[3], y[3]; for (int i = 0; i < 3; ++i) { x[i] = av_q2d(m->display_primaries[i][0]); y[i] = av_q2d(m->display_primaries[i][1]); }
            api->mapSetFloatArray(props, "MasteringDisplayPrimariesX", x, 3); api->mapSetFloatArray(props, "MasteringDisplayPrimariesY", y, 3);
            api->mapSetFloat(props, "MasteringDisplayWhitePointX", av_q2d(m->white_point[0]), maReplace);
            api->mapSetFloat(props, "MasteringDisplayWhitePointY", av_q2d(m->white_point[1]), maReplace);
        }
        if (m->has_luminance) {
            api->mapSetFloat(props, "MasteringDisplayMinLuminance", av_q2d(m->min_luminance), maReplace);
            api->mapSetFloat(props, "MasteringDisplayMaxLuminance", av_q2d(m->max_luminance), maReplace);
        }
    }
    if (light && light->size >= sizeof(AVContentLightMetadata)) {
        const auto* l = reinterpret_cast<const AVContentLightMetadata*>(light->data);
        api->mapSetInt(props, "ContentLightLevelMax", l->MaxCLL, maReplace);
        api->mapSetInt(props, "ContentLightLevelAverage", l->MaxFALL, maReplace);
    }
    // Private process-local bytes. Receiver must require this exact avutil
    // version before interpreting ABI structures; never a portable file format.
    std::vector<std::uint8_t> bytes;
    const auto append = [&bytes](const void* data, std::size_t size) { const auto* p = static_cast<const std::uint8_t*>(data); bytes.insert(bytes.end(), p, p + size); };
    const std::uint32_t version = 1, avutil = avutil_version(); append(&version, 4); append(&avutil, 4);
    for (const auto type : {AV_FRAME_DATA_MASTERING_DISPLAY_METADATA, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL, AV_FRAME_DATA_DYNAMIC_HDR_PLUS, AV_FRAME_DATA_DYNAMIC_HDR_VIVID}) {
        const auto* sd = av_frame_get_side_data(source.frame, type);
        if (!sd || !sd->size) continue;
        if (sd->size > 16384) throw std::runtime_error("Source metadata exceeds private bridge bound");
        const auto tag = static_cast<std::uint32_t>(type), size = static_cast<std::uint32_t>(sd->size);
        append(&tag, 4); append(&size, 4); append(sd->data, size);
    }
    api->mapSetData(props, "_VsrFrameSideData", reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()), dtBinary, maReplace);
}

const VSFrame* VS_CC getFrame(int n, int reason, void* data, void**, VSFrameContext* context, VSCore* core, const VSAPI* api) {
    if (reason != arInitial) return nullptr;
    auto& s = *static_cast<Source*>(data);
    VSFrame* output = nullptr;
    try {
        if (n < 0 || n >= static_cast<int>(s.stamps.size())) throw std::runtime_error("Source frame index is out of bounds");
        if (n < s.ordinal) s.open();
        while (s.ordinal < n) if (!s.next()) throw std::runtime_error("Source ended before indexed frame");
        const auto fmt = videoFormat(s.frame, core, api);
        output = api->newVideoFrame(&fmt, s.frame->width, s.frame->height, nullptr, core);
        if (!output) throw std::runtime_error("VS source frame allocation failed");
        const auto* descriptor = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(s.frame->format));
        const auto* stream = s.format->streams[s.stream]->codecpar;
        const bool needsDigest = sideData(s, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA, AV_PKT_DATA_MASTERING_DISPLAY_METADATA) ||
            sideData(s, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL, AV_PKT_DATA_CONTENT_LIGHT_LEVEL) ||
            av_frame_get_side_data(s.frame, AV_FRAME_DATA_DYNAMIC_HDR_PLUS) ||
            av_frame_get_side_data(s.frame, AV_FRAME_DATA_DYNAMIC_HDR_VIVID) ||
            av_packet_side_data_get(stream->coded_side_data, stream->nb_coded_side_data, AV_PKT_DATA_DYNAMIC_HDR10_PLUS);
        std::uint64_t digest = UINT64_C(14695981039346656037);
        for (int plane = 0; plane < fmt.numPlanes; ++plane) {
            // Descriptor components are logical R/G/B (even when the planes
            // are GBR) or Y/U/V. Packed RGB is only deinterleaved byte-for-byte.
            const auto component = descriptor->comp[plane];
            const int input = component.plane, rows = api->getFrameHeight(output, plane);
            const int samples = api->getFrameWidth(output, plane), width = samples * fmt.bytesPerSample;
            auto* destination = api->getWritePtr(output, plane); const auto stride = api->getStride(output, plane);
            const std::int64_t sourceBytes = component.offset + std::int64_t(samples - 1) * component.step + fmt.bytesPerSample;
            if (!s.frame->data[input] || std::abs(std::int64_t(s.frame->linesize[input])) < sourceBytes) throw std::runtime_error("Source plane stride is invalid");
            for (int y = 0; y < rows; ++y) {
                const auto* row = s.frame->data[input] + y * s.frame->linesize[input] + component.offset;
                auto* target = destination + y * stride;
                if (component.step == fmt.bytesPerSample) std::memcpy(target, row, width);
                else for (int x = 0; x < samples; ++x)
                    std::memcpy(target + x * fmt.bytesPerSample, row + x * component.step, fmt.bytesPerSample);
                if (needsDigest) for (int x = 0; x < width; ++x) { digest ^= target[x]; digest *= UINT64_C(1099511628211); }
            }
        }
        auto* props = api->getFramePropertiesRW(output);
        properties(s, props, api);
        if (needsDigest) api->mapSetData(props, "_VsrSourcePixelDigest", reinterpret_cast<const char*>(&digest), sizeof(digest), dtBinary, maReplace);
        return output;
    } catch (const std::exception& e) {
        if (output) api->freeFrame(output); api->setFilterError(e.what(), context); return nullptr;
    }
}
void VS_CC freeSource(void* data, VSCore*, const VSAPI*) { delete static_cast<Source*>(data); }
void VS_CC create(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* api) {
    try {
        int error = 0; const char* path = api->mapGetData(in, "source", 0, &error);
        if (error || !path) throw std::runtime_error("Source requires a UTF-8 path");
        auto s = std::make_unique<Source>(); s->path = path; s->open();
        bool first = true;
        while (s->next()) {
            const auto fmt = videoFormat(s->frame, core, api);
            if (first) { s->vi.format = fmt; s->vi.width = s->frame->width; s->vi.height = s->frame->height; first = false; }
            else if (s->vi.format.colorFamily != fmt.colorFamily || s->vi.format.sampleType != fmt.sampleType || s->vi.format.bitsPerSample != fmt.bitsPerSample || s->vi.format.subSamplingW != fmt.subSamplingW || s->vi.format.subSamplingH != fmt.subSamplingH || s->vi.width != s->frame->width || s->vi.height != s->frame->height)
                throw std::runtime_error("Source format or dimensions change midstream");
            if (s->stamps.size() == INT_MAX) throw std::runtime_error("Source exceeds VS frame count limit");
            s->stamps.push_back({s->frame->best_effort_timestamp, s->frame->duration});
        }
        if (s->stamps.empty()) throw std::runtime_error("Source decoded no video frames");
        for (std::size_t i = 0; i + 1 < s->stamps.size(); ++i)
            if (s->stamps[i].pts != AV_NOPTS_VALUE && s->stamps[i + 1].pts > s->stamps[i].pts) s->stamps[i].duration = s->stamps[i + 1].pts - s->stamps[i].pts;
        s->vi.numFrames = static_cast<int>(s->stamps.size());
        if (s->rate.num > 0 && s->rate.den > 0) { s->vi.fpsNum = s->rate.num; s->vi.fpsDen = s->rate.den; }
        else { s->vi.fpsNum = 0; s->vi.fpsDen = 0; }
        s->open();
        api->createVideoFilter(out, "Source", &s->vi, getFrame, freeSource, fmUnordered, nullptr, 0, s.get(), core);
        s.release();
    } catch (const std::exception& e) { api->mapSetError(out, e.what()); }
}
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* api) {
    api->configPlugin("com.vsrenderer.native-source", "vsrsource", "VS Renderer FFmpeg source", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
    api->registerFunction("Source", "source:data;", "clip:vnode;", create, nullptr, plugin);
}
