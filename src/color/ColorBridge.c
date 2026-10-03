#define COBJMACROS
#include "ColorBridge.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libplacebo/d3d11.h>
#include <libplacebo/renderer.h>
#include <libplacebo/shaders/custom.h>
#include <libplacebo/utils/libav.h>
#include <libavutil/hwcontext.h>

typedef struct ColorContext {
    ID3D11Device *device;
    ID3D11DeviceContext *immediate;
    ID3D11Texture2D *cache;
    pl_log log;
    pl_d3d11 d3d;
    pl_renderer renderer;
    pl_tex planes[3], target;
    void *planeResources[3];
    struct pl_frame source;
    struct pl_dovi_metadata dovi;
    AVFrame *metadata;
    VsrColorSettings settings;
    VsrColorStatus status;
    uint64_t generation, cachedGeneration;
    VsrColorDraw cachedDraw;
    pl_icc_object icc;
    struct pl_custom_lut *lut;
    const struct pl_hook *scrgb;
    HMONITOR profileMonitor;
    int profileValid;
    char error[1024];
} ColorContext;

static void message(void *priv, enum pl_log_level level, const char *text) {
    ColorContext *c = priv;
    if (level <= PL_LOG_ERR) snprintf(c->error, sizeof(c->error), "%s", text);
}

static void fail(ColorContext *c, const char *text) {
    c->status.activeEngine = 0;
    snprintf(c->status.fallback, sizeof(c->status.fallback), "%s", text);
}

static wchar_t *wide(const char *text) {
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    if (!count) return NULL;
    wchar_t *out = calloc(count, sizeof(wchar_t));
    if (out) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, out, count);
    return out;
}

static char *read_file(const char *path, size_t *length) {
    wchar_t *name = wide(path);
    FILE *file = name ? _wfopen(name, L"rb") : NULL;
    free(name);
    if (!file) return NULL;
    if (_fseeki64(file, 0, SEEK_END)) { fclose(file); return NULL; }
    int64_t size = _ftelli64(file);
    if (size <= 0 || size > 128 * 1024 * 1024) { fclose(file); return NULL; }
    rewind(file);
    char *data = malloc((size_t)size + 1);
    if (!data || fread(data, 1, (size_t)size, file) != (size_t)size) { free(data); fclose(file); return NULL; }
    fclose(file); data[size] = 0; *length = (size_t)size;
    return data;
}

static void release_planes(ColorContext *c) {
    for (int i = 0; i < 3; ++i) { pl_tex_destroy(c->d3d->gpu, &c->planes[i]); c->planeResources[i] = NULL; }
}

__declspec(dllexport) void *vsr_color_create(void *device) {
    if (!device) return NULL;
    ColorContext *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->settings = VsrColorDefaultSettings();
    c->status.size = sizeof(c->status); c->status.version = 1;
    c->log = pl_log_create(PL_API_VER, &(struct pl_log_params) { .log_cb = message, .log_priv = c, .log_level = PL_LOG_WARN });
    c->device = device; ID3D11Device_AddRef(c->device);
    ID3D11Device_GetImmediateContext(c->device, &c->immediate);
    c->d3d = pl_d3d11_create(c->log, &(struct pl_d3d11_params) { .device = device });
    if (!c->d3d) { vsr_color_destroy(c); return NULL; }
    c->renderer = pl_renderer_create(c->log, c->d3d->gpu);
    c->metadata = av_frame_alloc();
    /* libplacebo 360 linear RGB uses 203 nits; Windows scRGB uses 80 nits.
       This hook changes units only, after the library's tone/gamut mapping. */
    const char hook[] = "//!HOOK OUTPUT\n//!BIND HOOKED\n//!DESC Windows scRGB units\nvec4 hook(){vec4 c=HOOKED_tex(HOOKED_pos);c.rgb*=2.5375;return c;}\n";
    c->scrgb = pl_mpv_user_shader_parse(c->d3d->gpu, hook, sizeof(hook) - 1);
    if (!c->renderer || !c->metadata || !c->scrgb) { vsr_color_destroy(c); return NULL; }
    snprintf(c->status.engine, sizeof(c->status.engine), "libplacebo %s / D3D11", pl_version());
    return c;
}

__declspec(dllexport) void vsr_color_destroy(void *context) {
    ColorContext *c = context;
    if (!c) return;
    if (c->d3d) { release_planes(c); pl_tex_destroy(c->d3d->gpu, &c->target); }
    if (c->cache) ID3D11Texture2D_Release(c->cache);
    pl_mpv_user_shader_destroy(&c->scrgb);
    pl_lut_free(&c->lut); pl_icc_close(&c->icc);
    pl_renderer_destroy(&c->renderer); pl_d3d11_destroy(&c->d3d); pl_log_destroy(&c->log);
    av_frame_free(&c->metadata);
    if (c->immediate) ID3D11DeviceContext_Release(c->immediate);
    if (c->device) ID3D11Device_Release(c->device);
    free(c);
}

__declspec(dllexport) int vsr_color_configure(void *context, const VsrColorSettings *settings) {
    ColorContext *c = context;
    if (!c || !settings || settings->size != sizeof(*settings) || settings->version != 1 ||
        settings->engine > 1 || settings->output > 2 || settings->quality > 1 ||
        settings->tone > 5 || settings->gamut > 4 || settings->icc > 2 ||
        settings->peakDetect > 1 || settings->dither > 1 || settings->inverseTone > 1 ||
        !isfinite(settings->sdrPeak) || settings->sdrPeak <= 0 || settings->sdrPeak > 10000 ||
        !isfinite(settings->displayPeak) || settings->displayPeak < 0 || settings->displayPeak > 10000 ||
        !isfinite(settings->paperWhite) || settings->paperWhite <= 0 || settings->paperWhite > 10000 ||
        !isfinite(settings->contrastRecovery) || settings->contrastRecovery < 0 || settings->contrastRecovery > 1 ||
        !memchr(settings->iccPath, 0, sizeof(settings->iccPath)) || !memchr(settings->lutPath, 0, sizeof(settings->lutPath))) return 0;
    c->settings = *settings; ++c->generation; c->profileValid = 0;
    c->status.requestedEngine = settings->engine;
    pl_lut_free(&c->lut);
    if (settings->lutPath[0]) {
        size_t size; char *data = read_file(settings->lutPath, &size);
        if (data) { c->lut = pl_lut_parse_cube(c->log, data, size); free(data); }
        if (!c->lut) { fail(c, "Custom .cube LUT cannot be read or parsed; using original 3FP."); return 0; }
    }
    return 1;
}

/* Reject truncated side data before the mature library sees it. The original
   decoder/VS AVFrame is never mutated. */
static void validate_side_data(AVFrame *frame, enum AVFrameSideDataType type, size_t minimum) {
    AVFrameSideData *sd = av_frame_get_side_data(frame, type);
    if (sd && sd->size < minimum) av_frame_remove_side_data(frame, type);
}

__declspec(dllexport) int vsr_color_frame(void *context, const void *avframe, int softwareFormat,
                                        uint32_t convertedRgb, uint32_t sourceKind) {
    ColorContext *c = context; const AVFrame *frame = avframe;
    if (!c || !frame) return 0;
    av_frame_unref(c->metadata);
    if (av_frame_copy_props(c->metadata, frame) < 0) { fail(c, "Color frame properties could not be retained."); return 0; }
    c->metadata->format = softwareFormat;
    c->metadata->width = frame->width; c->metadata->height = frame->height;
    c->status.sourceMatrix = frame->colorspace; c->status.sourcePrimaries = frame->color_primaries;
    c->status.sourceTransfer = frame->color_trc; c->status.sourceRange = frame->color_range;
    c->status.sourceChroma = frame->chroma_location; c->status.sourceKind = sourceKind;
    c->status.explicitFields = (frame->colorspace != AVCOL_SPC_UNSPECIFIED ? 1 : 0) |
        (frame->color_primaries != AVCOL_PRI_UNSPECIFIED ? 2 : 0) |
        (frame->color_trc != AVCOL_TRC_UNSPECIFIED ? 4 : 0) |
        (frame->color_range != AVCOL_RANGE_UNSPECIFIED ? 8 : 0) |
        (frame->chroma_location != AVCHROMA_LOC_UNSPECIFIED ? 16 : 0);
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(softwareFormat);
    if (!desc) { fail(c, "Unsupported color frame pixel representation."); return 0; }
    c->status.sourceBits = desc->comp[0].depth;
    if (convertedRgb) {
        c->metadata->format = c->status.sourceBits > 8 ? AV_PIX_FMT_RGBA64LE : AV_PIX_FMT_BGRA;
        c->metadata->colorspace = AVCOL_SPC_RGB; c->metadata->color_range = AVCOL_RANGE_JPEG;
    }
    validate_side_data(c->metadata, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA, sizeof(AVMasteringDisplayMetadata));
    validate_side_data(c->metadata, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL, sizeof(AVContentLightMetadata));
    validate_side_data(c->metadata, AV_FRAME_DATA_DYNAMIC_HDR_PLUS, sizeof(AVDynamicHDRPlus));
    AVFrameSideData *dynamic = av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DYNAMIC_HDR_PLUS);
    if (dynamic) {
        const AVDynamicHDRPlus *plus = (const AVDynamicHDRPlus *)dynamic->data;
        if (!plus->num_windows || plus->num_windows > 3 ||
            plus->params[0].num_distribution_maxrgb_percentiles > 15 ||
            plus->params[0].num_bezier_curve_anchors > 15)
            av_frame_remove_side_data(c->metadata, AV_FRAME_DATA_DYNAMIC_HDR_PLUS);
    }
    pl_frame_from_avframe(&c->source, c->metadata);
    // D3D11 BGRA has a sampled channel order; AVFrame's generic helper does
    // not sort packed component offsets (its upload helper normally does).
    if (c->metadata->format == AV_PIX_FMT_BGRA) {
        c->source.planes[0].component_mapping[0] = 2;
        c->source.planes[0].component_mapping[2] = 0;
    }
    c->status.hdr10plus = av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DYNAMIC_HDR_PLUS) != NULL;
    c->status.vividDetected = av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DYNAMIC_HDR_VIVID) != NULL;
    c->status.doviDetected = av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DOVI_METADATA) != NULL ||
        av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DOVI_RPU_BUFFER) != NULL;
    c->status.doviActive = 0;
    AVFrameSideData *sd = av_frame_get_side_data(c->metadata, AV_FRAME_DATA_DOVI_METADATA);
    if (!convertedRgb && sd && sd->size >= sizeof(AVDOVIMetadata)) {
        AVDOVIMetadata *dv = (AVDOVIMetadata *)sd->data;
        if (sizeof(AVDOVIRpuDataHeader) <= sd->size && sizeof(AVDOVIDataMapping) <= sd->size &&
            sizeof(AVDOVIColorMetadata) <= sd->size &&
            dv->header_offset <= sd->size - sizeof(AVDOVIRpuDataHeader) &&
            dv->mapping_offset <= sd->size - sizeof(AVDOVIDataMapping) &&
            dv->color_offset <= sd->size - sizeof(AVDOVIColorMetadata) &&
            (!dv->num_ext_blocks || (dv->ext_block_offset <= sd->size &&
                dv->ext_block_size >= sizeof(AVDOVIDmData) &&
                dv->num_ext_blocks <= (sd->size - dv->ext_block_offset) / dv->ext_block_size))) {
            const AVDOVIRpuDataHeader *header = av_dovi_get_header(dv);
            const AVDOVIDataMapping *mapping = av_dovi_get_mapping(dv);
            int valid = header->bl_bit_depth >= 8 && header->bl_bit_depth <= 16 &&
                header->coef_log2_denom <= 30;
            for (int component = 0; component < 3; ++component) {
                const AVDOVIReshapingCurve *curve = &mapping->curves[component];
                if (curve->num_pivots < 2 || curve->num_pivots > 9) { valid = 0; break; }
                for (int i = 0; i + 1 < curve->num_pivots; ++i)
                    if (curve->mapping_idc[i] > 1 || curve->mmr_order[i] > 3 || curve->poly_order[i] > 2) valid = 0;
            }
            if (valid) {
                pl_map_avdovi_metadata(&c->source.color, &c->source.repr, &c->dovi, dv);
                c->status.doviActive = c->source.repr.sys == PL_COLOR_SYSTEM_DOLBYVISION;
            }
        }
    }
    c->status.masteringPeak = c->source.color.hdr.max_luma; c->status.masteringBlack = c->source.color.hdr.min_luma;
    c->status.maxCll = c->source.color.hdr.max_cll; c->status.maxFall = c->source.color.hdr.max_fall;
    if (!c->source.repr.sys) c->source.repr.sys = convertedRgb ? PL_COLOR_SYSTEM_RGB :
        frame->width >= 1280 ? PL_COLOR_SYSTEM_BT_709 : PL_COLOR_SYSTEM_BT_601;
    if (!c->source.repr.levels) c->source.repr.levels = convertedRgb ? PL_COLOR_LEVELS_FULL : PL_COLOR_LEVELS_LIMITED;
    if (!c->source.color.primaries) c->source.color.primaries = pl_color_primaries_guess(frame->width, frame->height);
    if (!c->source.color.transfer) c->source.color.transfer = PL_COLOR_TRC_BT_1886;
    pl_color_space_infer(&c->source.color);
    if (!pl_color_space_is_hdr(&c->source.color)) c->source.color.hdr.max_luma = c->settings.sdrPeak;
    if (!c->source.color.hdr.max_luma || !isfinite(c->source.color.hdr.max_luma))
        c->source.color.hdr.max_luma = pl_color_space_is_hdr(&c->source.color) ? 1000 : c->settings.sdrPeak;
    ++c->generation;
    return 1;
}

static void profile(ColorContext *c, HMONITOR monitor, int hdr) {
    if (c->profileValid && c->profileMonitor == monitor && c->status.outputHdr == (uint32_t)hdr) return;
    c->profileValid = 1; c->profileMonitor = monitor; c->status.profile[0] = 0;
    pl_icc_close(&c->icc); c->status.iccState = 0;
    /* SDR display ICC curves are not valid for Windows HDR/scRGB targets. */
    if (!c->settings.icc) return;
    if (hdr) { c->status.iccState = 2; return; }
    char path[1024] = {0};
    if (c->settings.icc == 2) snprintf(path, sizeof(path), "%s", c->settings.iccPath);
    else {
        MONITORINFOEXW info = { .cbSize = sizeof(info) };
        if (GetMonitorInfoW(monitor, (MONITORINFO *)&info)) {
            HDC dc = CreateDCW(info.szDevice, NULL, NULL, NULL);
            if (dc) {
                wchar_t file[1024]; DWORD length = 1024;
                if (GetICMProfileW(dc, &length, file)) WideCharToMultiByte(CP_UTF8, 0, file, -1, path, sizeof(path), NULL, NULL);
                DeleteDC(dc);
            }
        }
    }
    if (!path[0]) { c->status.iccState = c->settings.icc == 2 ? 3 : 4; return; }
    size_t size; char *data = read_file(path, &size);
    if (data) {
        struct pl_icc_profile p = { .data = data, .len = size };
        pl_icc_profile_compute_signature(&p);
        struct pl_icc_params params = pl_icc_default_params;
        params.max_luma = c->settings.sdrPeak;
        if (c->settings.quality) params.size_r = params.size_g = params.size_b = 65;
        c->icc = pl_icc_open(c->log, &p, &params);
        free(data);
    }
    c->status.iccState = c->icc ? 1 : 3;
    snprintf(c->status.profile, sizeof(c->status.profile), "%s", path);
}

static struct pl_filter_config filter(uint32_t value) {
    switch (value & 255) {
    case 0: return pl_filter_nearest;
    case 1: case 7: return pl_filter_bilinear;
    case 2: return pl_filter_bicubic;
    case 3: return pl_filter_lanczos;
    case 5: return pl_filter_spline36;
    case 8: { struct pl_filter_config result = pl_filter_lanczos; result.radius = 4; return result; }
    default: return pl_filter_ewa_jinc;
    }
}

static void monitor_gamut(ColorContext *c, HMONITOR monitor, struct pl_color_space *target) {
    IDXGIDevice *dxgi = NULL; IDXGIAdapter *adapter = NULL;
    if (FAILED(ID3D11Device_QueryInterface(c->device, &IID_IDXGIDevice, (void **)&dxgi))) return;
    if (SUCCEEDED(IDXGIDevice_GetAdapter(dxgi, &adapter))) {
        for (UINT i = 0;; ++i) {
            IDXGIOutput *output = NULL; IDXGIOutput6 *advanced = NULL;
            if (FAILED(IDXGIAdapter_EnumOutputs(adapter, i, &output))) break;
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(IDXGIOutput_GetDesc(output, &desc)) && desc.Monitor == monitor &&
                SUCCEEDED(IDXGIOutput_QueryInterface(output, &IID_IDXGIOutput6, (void **)&advanced))) {
                DXGI_OUTPUT_DESC1 info;
                if (SUCCEEDED(IDXGIOutput6_GetDesc1(advanced, &info)) && info.RedPrimary[0] > 0 && info.GreenPrimary[1] > 0 && info.WhitePoint[1] > 0)
                    target->hdr.prim = (struct pl_raw_primaries) { .red = {info.RedPrimary[0], info.RedPrimary[1]},
                        .green = {info.GreenPrimary[0], info.GreenPrimary[1]}, .blue = {info.BluePrimary[0], info.BluePrimary[1]},
                        .white = {info.WhitePoint[0], info.WhitePoint[1]} };
                IDXGIOutput6_Release(advanced);
            }
            IDXGIOutput_Release(output);
        }
        IDXGIAdapter_Release(adapter);
    }
    IDXGIDevice_Release(dxgi);
}

__declspec(dllexport) int vsr_color_draw(void *context, const VsrColorDraw *draw) {
    ColorContext *c = context;
    if (!c || !draw || !draw->targetTexture || !c->generation) return 0;
    D3D11_TEXTURE2D_DESC desc; ID3D11Texture2D_GetDesc((ID3D11Texture2D *)draw->targetTexture, &desc);
    VsrColorDraw key = *draw; key.targetTexture = NULL; // Flip-model buffers rotate independently of video changes.
    profile(c, draw->monitor, draw->hdr);
    if (c->status.iccState == 3) { fail(c, "Display ICC cannot be read or parsed; using original 3FP."); return 0; }
    D3D11_TEXTURE2D_DESC cached = {0}; if (c->cache) ID3D11Texture2D_GetDesc(c->cache, &cached);
    if (c->cache && cached.Width == desc.Width && cached.Height == desc.Height && cached.Format == desc.Format &&
        c->cachedGeneration == c->generation && !memcmp(&c->cachedDraw, &key, sizeof(key))) {
        ID3D11DeviceContext_CopyResource(c->immediate, draw->targetTexture, (ID3D11Resource *)c->cache);
        ++c->status.cachedPresents; return 1;
    }
    LARGE_INTEGER start, end, frequency; QueryPerformanceCounter(&start); QueryPerformanceFrequency(&frequency);
    D3D11_TEXTURE2D_DESC old = {0}; if (c->cache) ID3D11Texture2D_GetDesc(c->cache, &old);
    if (!c->cache || old.Width != desc.Width || old.Height != desc.Height || old.Format != desc.Format) {
        pl_tex_destroy(c->d3d->gpu, &c->target);
        if (c->cache) { ID3D11Texture2D_Release(c->cache); c->cache = NULL; }
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = 0; desc.Usage = D3D11_USAGE_DEFAULT; desc.CPUAccessFlags = 0;
        if (FAILED(ID3D11Device_CreateTexture2D(c->device, &desc, NULL, &c->cache))) { fail(c, "Color render target allocation failed."); return 0; }
        c->target = pl_d3d11_wrap(c->d3d->gpu, &(struct pl_d3d11_wrap_params) { .tex = (ID3D11Resource *)c->cache });
        if (!c->target) { fail(c, "Color render target import failed."); return 0; }
    }
    struct pl_frame image = c->source;
    for (int i = 0; i < image.num_planes; ++i) {
        ID3D11Texture2D *texture = draw->textures[i];
        if (!texture && draw->layout == 2) texture = draw->textures[0];
        if (!texture || i >= 3) { fail(c, "Color source plane is unavailable."); return 0; }
        if (c->planeResources[i] != texture) {
            pl_tex_destroy(c->d3d->gpu, &c->planes[i]);
            D3D11_TEXTURE2D_DESC source; ID3D11Texture2D_GetDesc(texture, &source);
            struct pl_d3d11_wrap_params p = { .tex = (ID3D11Resource *)texture };
            if (source.Format == DXGI_FORMAT_NV12 || source.Format == DXGI_FORMAT_P010 || source.Format == DXGI_FORMAT_P016) {
                p.fmt = source.Format == DXGI_FORMAT_NV12 ? (i ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8_UNORM) :
                    (i ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16_UNORM);
                p.w = i ? (source.Width + 1) / 2 : source.Width; p.h = i ? (source.Height + 1) / 2 : source.Height;
            }
            c->planes[i] = pl_d3d11_wrap(c->d3d->gpu, &p); c->planeResources[i] = texture;
        }
        if (!c->planes[i]) { fail(c, "D3D11 source plane import failed; using original 3FP."); return 0; }
        image.planes[i].texture = c->planes[i];
    }
    image.crop = (pl_rect2df) { draw->crop[0] * image.planes[0].texture->params.w,
        draw->crop[1] * image.planes[0].texture->params.h, draw->crop[2] * image.planes[0].texture->params.w,
        draw->crop[3] * image.planes[0].texture->params.h };
    struct pl_frame target = { .num_planes = 1, .planes = {{ .texture = c->target, .components = 4,
        .component_mapping = {0, 1, 2, 3} }}, .repr = pl_color_repr_rgb, .color = pl_color_space_srgb,
        .crop = { draw->destination[0], draw->destination[1], draw->destination[2], draw->destination[3] } };
    target.repr.alpha = PL_ALPHA_NONE;
    target.repr.bits.sample_depth = target.repr.bits.color_depth = draw->hdr ? 0 : draw->outputBits;
    target.color.hdr.max_luma = draw->hdr ? draw->targetPeak : c->settings.sdrPeak;
    target.color.hdr.min_luma = draw->targetBlack;
    if (draw->hdr) { target.color.primaries = PL_COLOR_PRIM_BT_709; target.color.transfer = PL_COLOR_TRC_LINEAR; monitor_gamut(c, draw->monitor, &target.color); }
    else if (c->icc) { target.icc = c->icc; target.color = c->icc->csp; }
    struct pl_render_params params = c->settings.quality ? pl_render_high_quality_params : pl_render_default_params;
    params.deband_params = NULL; params.sigmoid_params = NULL; params.color_adjustment = NULL;
    struct pl_filter_config upscale = filter(draw->upscale), downscale = filter(draw->downscale);
    params.upscaler = &upscale; params.downscaler = &downscale;
    params.antiringing_strength = (draw->upscale & 65536) ? 0.5f : 0;
    struct pl_color_map_params map = pl_color_map_default_params;
    const struct pl_tone_map_function *tones[] = { &pl_tone_map_spline, &pl_tone_map_spline, &pl_tone_map_st2094_40,
        &pl_tone_map_bt2390, &pl_tone_map_clip, &pl_tone_map_linear };
    const struct pl_gamut_map_function *gamuts[] = { &pl_gamut_map_perceptual, &pl_gamut_map_perceptual,
        &pl_gamut_map_softclip, &pl_gamut_map_relative, &pl_gamut_map_clip };
    const bool sourceHdr = pl_color_space_is_hdr(&image.color);
    const uint32_t tone = c->settings.tone ? c->settings.tone :
        !sourceHdr && !draw->hdr ? 4 : c->status.hdr10plus ? 2 : 1;
    const uint32_t gamut = c->settings.gamut ? c->settings.gamut :
        !sourceHdr && !pl_color_primaries_is_wide_gamut(image.color.primaries) ? 3 : 1;
    map.tone_mapping_function = tones[tone];
    map.gamut_mapping = gamuts[gamut]; map.gamut_expansion = false;
    map.contrast_recovery = c->settings.contrastRecovery; map.inverse_tone_mapping = c->settings.inverseTone != 0;
    params.color_map_params = &map;
    params.peak_detect_params = c->settings.peakDetect ? &pl_peak_detect_default_params : NULL;
    params.dither_params = c->settings.dither && !draw->hdr ? &pl_dither_default_params : NULL;
    params.hooks = draw->hdr ? &c->scrgb : NULL; params.num_hooks = draw->hdr ? 1 : 0;
    /* .cube LUT is display-signal referred; SDR LUTs must not clip HDR scRGB. */
    if (c->lut && !draw->hdr) { target.lut = c->lut; target.lut_type = PL_LUT_NATIVE; }
    if (!pl_render_image(c->renderer, &image, &target, &params)) { fail(c, c->error[0] ? c->error : "libplacebo rendering failed; using original 3FP."); return 0; }
    pl_gpu_flush(c->d3d->gpu);
    ID3D11DeviceContext_CopyResource(c->immediate, draw->targetTexture, (ID3D11Resource *)c->cache);
    c->cachedDraw = key; c->cachedGeneration = c->generation;
    c->status.activeEngine = 1; c->status.outputHdr = draw->hdr; c->status.outputBits = draw->outputBits;
    c->status.targetPeak = target.color.hdr.max_luma; c->status.targetBlack = target.color.hdr.min_luma;
    c->status.paperWhite = c->settings.paperWhite; c->status.lutActive = c->lut && !draw->hdr;
    c->status.tone = tone; c->status.gamut = gamut; c->status.dither = c->settings.dither && !draw->hdr;
    c->status.fallback[0] = 0;
    if (c->status.vividDetected) snprintf(c->status.fallback, sizeof(c->status.fallback), "HDR Vivid detected; PQ/HLG target mapping, dynamic Vivid curves unavailable.");
    else if (c->status.doviDetected && !c->status.doviActive) snprintf(c->status.fallback, sizeof(c->status.fallback), "Dolby Vision detected; base-layer fallback (FEL reconstruction unavailable).");
    QueryPerformanceCounter(&end); c->status.renderSubmitMs = (float)(1000.0 * (end.QuadPart - start.QuadPart) / frequency.QuadPart);
    ++c->status.renderedFrames;
    return 1;
}

__declspec(dllexport) int vsr_color_status(void *context, VsrColorStatus *status) {
    ColorContext *c = context;
    if (!c || !status || status->size != sizeof(*status) || status->version != 1) return 0;
    *status = c->status; return 1;
}
