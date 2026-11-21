#include <VapourSynth4.h>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
struct Cube {
    VSNode *clip = nullptr;
    const VSAPI *api;
    AVFilterGraph *graph = nullptr;
    AVFilterContext *input = nullptr, *output = nullptr;
    AVFrame *source = nullptr, *result = nullptr;
    explicit Cube(const VSAPI *vs) : api(vs) {}
    ~Cube() {
        if (clip) api->freeNode(clip);
        av_frame_free(&source);
        av_frame_free(&result);
        avfilter_graph_free(&graph);
    }
};

const VSFrame *VS_CC cubeFrame(int n, int reason, void *opaque, void **, VSFrameContext *context,
                              VSCore *core, const VSAPI *api) {
    auto &cube = *static_cast<Cube *>(opaque);
    if (reason == arInitial) { api->requestFrameFilter(n, cube.clip, context); return nullptr; }
    if (reason != arAllFramesReady) return nullptr;
    const auto *input = api->getFrameFilter(n, cube.clip, context);
    if (!input) return nullptr;
    const auto width = api->getFrameWidth(input, 0), height = api->getFrameHeight(input, 0);
    VSFrame *output = nullptr;
    if (av_frame_make_writable(cube.source) >= 0) {
        // VapourSynth planes are RGB; FFmpeg planar float uses GBR order.
        constexpr std::array<int, 3> ffPlane{2, 0, 1};
        for (int p = 0; p < 3; ++p)
            for (int y = 0; y < height; ++y)
                std::memcpy(cube.source->data[ffPlane[p]] + y * cube.source->linesize[ffPlane[p]],
                            api->getReadPtr(input, p) + y * api->getStride(input, p), width * sizeof(float));
        cube.source->pts = n;
        if (av_buffersrc_add_frame_flags(cube.input, cube.source, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0 &&
            av_buffersink_get_frame(cube.output, cube.result) >= 0) {
            output = api->newVideoFrame(api->getVideoFrameFormat(input), width, height, input, core);
            if (output) {
                for (int p = 0; p < 3; ++p)
                    for (int y = 0; y < height; ++y)
                        std::memcpy(api->getWritePtr(output, p) + y * api->getStride(output, p),
                                    cube.result->data[ffPlane[p]] + y * cube.result->linesize[ffPlane[p]], width * sizeof(float));
            }
            av_frame_unref(cube.result);
        }
    }
    api->freeFrame(input);
    if (!output) api->setFilterError("FFmpeg lut3d failed to process an RGB float frame", context);
    return output;
}

void VS_CC freeCube(void *opaque, VSCore *, const VSAPI *) { delete static_cast<Cube *>(opaque); }

void VS_CC createCube(const VSMap *in, VSMap *out, void *, VSCore *core, const VSAPI *api) {
    try {
        auto cube = std::make_unique<Cube>(api);
        int error = 0;
        cube->clip = api->mapGetNode(in, "clip", 0, &error);
        if (error || !cube->clip) throw std::runtime_error("Cube requires a video clip");
        const auto *path = api->mapGetData(in, "file", 0, &error);
        if (error || !path || !*path) throw std::runtime_error("Cube requires a local .cube file");
        const auto *info = api->getVideoInfo(cube->clip);
        if (info->width <= 0 || info->height <= 0 || info->format.colorFamily != cfRGB ||
            info->format.sampleType != stFloat || info->format.bitsPerSample != 32)
            throw std::runtime_error("Cube requires constant-dimension RGBS input");
        cube->graph = avfilter_graph_alloc();
        if (!cube->graph) throw std::bad_alloc();
        cube->graph->nb_threads = 2;
        const auto bufferArgs = "video_size=" + std::to_string(info->width) + "x" + std::to_string(info->height) +
            ":pix_fmt=gbrpf32le:time_base=1/1:pixel_aspect=1/1";
        auto *lut = avfilter_graph_alloc_filter(cube->graph, avfilter_get_by_name("lut3d"), "cube");
        if (!lut || av_opt_set(lut, "file", path, AV_OPT_SEARCH_CHILDREN) < 0 ||
            av_opt_set(lut, "interp", "trilinear", AV_OPT_SEARCH_CHILDREN) < 0 || avfilter_init_str(lut, nullptr) < 0)
            throw std::runtime_error("FFmpeg could not load the 3D Cube LUT");
        if (avfilter_graph_create_filter(&cube->input, avfilter_get_by_name("buffer"), "input",
                bufferArgs.c_str(), nullptr, cube->graph) < 0 ||
            avfilter_graph_create_filter(&cube->output, avfilter_get_by_name("buffersink"), "output",
                nullptr, nullptr, cube->graph) < 0 ||
            avfilter_link(cube->input, 0, lut, 0) < 0 || avfilter_link(lut, 0, cube->output, 0) < 0 ||
            avfilter_graph_config(cube->graph, nullptr) < 0 || av_buffersink_get_format(cube->output) != AV_PIX_FMT_GBRPF32LE)
            throw std::runtime_error("FFmpeg could not initialize the Cube filter");
        cube->source = av_frame_alloc();
        cube->result = av_frame_alloc();
        if (!cube->source || !cube->result) throw std::bad_alloc();
        cube->source->format = AV_PIX_FMT_GBRPF32LE;
        cube->source->width = info->width;
        cube->source->height = info->height;
        if (av_frame_get_buffer(cube->source, 32) < 0) throw std::bad_alloc();
        const VSFilterDependency dependency{cube->clip, rpStrictSpatial};
        api->createVideoFilter(out, "Cube", info, cubeFrame, freeCube, fmFrameState, &dependency, 1, cube.get(), core);
        cube.release();
    } catch (const std::exception &e) {
        api->mapSetError(out, e.what());
    }
}
}

void registerNativeCube(VSPlugin *plugin, const VSPLUGINAPI *api) {
    api->registerFunction("Cube", "clip:vnode;file:data;", "clip:vnode;", createCube, nullptr, plugin);
}
