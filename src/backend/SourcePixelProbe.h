#pragma once
#include <cstdint>

// Local optional ABI; original decoded samples, before colour processing.
struct VsrSourcePixelProbe {
    std::uint32_t size = sizeof(VsrSourcePixelProbe), version = 1;
    std::uint32_t coordinates = 0; // 0: source pixel; 1: physical client pixel
    std::int32_t x = 0, y = 0;
    std::int32_t surfaceX = 0, surfaceY = 0;
    std::uint32_t width = 0, height = 0, bitDepth = 0;
    std::uint32_t family = 0; // 1: YUV; 2: RGB; 3: gray
    std::uint32_t values[3]{};
    std::uint32_t chromaShiftX = 0, chromaShiftY = 0;
};
