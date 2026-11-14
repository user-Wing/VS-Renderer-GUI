#pragma once
#include <stdint.h>

// Versioned tail of the VSP external frame. Missing fields describe unknown
// metadata, never metadata inherited from a previous submitted frame.
struct VsrFrameColorMetadata {
    uint32_t flags = 0; // 1: mastering primaries, 2: mastering luminance, 4: CLL
    uint32_t maxCll = 0, maxFall = 0, fieldBased = 0;
    double primaryX[3]{}, primaryY[3]{};
    double whiteX = 0, whiteY = 0, minLuminance = 0, maxLuminance = 0;
    int64_t presentationTime100ns = -1;
    int32_t sarNumerator = 0, sarDenominator = 0;
    const void* sideData = nullptr;
    uint32_t sideDataBytes = 0, streamHdr10Plus = 0;
};

static_assert(sizeof(VsrFrameColorMetadata) == 128);
