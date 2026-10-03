#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Separate, optional C ABI: the existing 3FP snapshot/frame ABI stays intact. */
typedef struct VsrColorSettings {
    uint32_t size, version;
    uint32_t engine;       /* 0: original 3FP, 1: libplacebo with explicit fallback */
    uint32_t output;       /* 0: display auto, 1: SDR, 2: HDR developer override */
    uint32_t tone;         /* auto, spline, ST2094-40, BT.2390, clip, linear */
    uint32_t gamut;        /* auto, perceptual, softclip, relative, clip */
    uint32_t quality;      /* balanced / high quality (color only) */
    uint32_t peakDetect, dither, inverseTone;
    uint32_t icc;          /* off / monitor auto / custom */
    float sdrPeak, displayPeak, paperWhite, contrastRecovery;
    char iccPath[1024];
    char lutPath[1024];
} VsrColorSettings;

static inline VsrColorSettings VsrColorDefaultSettings(void) {
    VsrColorSettings c = {0};
    c.size = sizeof(c); c.version = 1;
    c.quality = 1; c.peakDetect = 1; c.dither = 1; c.icc = 1;
    c.sdrPeak = 100; c.paperWhite = 203;
    return c;
}

typedef struct VsrColorStatus {
    uint32_t size, version;
    uint32_t requestedEngine, activeEngine, outputHdr, outputBits;
    uint32_t sourceMatrix, sourcePrimaries, sourceTransfer, sourceRange, sourceChroma;
    uint32_t sourceBits, explicitFields, sourceKind; /* 1: decoded frame, 2: VS props */
    uint32_t streamFields; /* Fields supplemented from decoded-stream metadata, never reapplied to VS. */
    uint32_t tone, gamut, dither;
    uint32_t hdr10plus, doviDetected, doviActive, vividDetected, iccState, lutActive;
    float masteringPeak, masteringBlack, maxCll, maxFall;
    float targetPeak, targetBlack, paperWhite, renderSubmitMs;
    uint64_t renderedFrames, cachedPresents;
    char engine[128], fallback[1024], profile[1024];
} VsrColorStatus;

typedef struct VsrColorDraw {
    void *textures[3];
    void *targetTexture;
    uint32_t layout, upscale, downscale, hdr, outputBits;
    float targetPeak, targetBlack;
    float crop[4], destination[4];
    void *monitor;
} VsrColorDraw;

/* All calls for one object are serialized by 3FP's existing device mutex. */
void *vsr_color_create(void *device);
void vsr_color_destroy(void *context);
int vsr_color_configure(void *context, const VsrColorSettings *settings);
int vsr_color_frame(void *context, const void *avframe, int softwareFormat,
                    uint32_t convertedRgb, uint32_t sourceKind);
int vsr_color_draw(void *context, const VsrColorDraw *draw);
int vsr_color_status(void *context, VsrColorStatus *status);

#ifdef __cplusplus
}
#endif
