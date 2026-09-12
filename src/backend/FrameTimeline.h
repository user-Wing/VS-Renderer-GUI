#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace vsr {

inline std::int64_t frameAtPosition100ns(std::int64_t position100ns, std::int64_t totalFrames,
                                         std::int64_t fpsNumerator, std::int64_t fpsDenominator)
{
    if (position100ns < 0 || totalFrames <= 0 || fpsNumerator <= 0 || fpsDenominator <= 0)
        return -1;
    const long double frame = static_cast<long double>(position100ns) * fpsNumerator /
        (10000000.0L * fpsDenominator);
    return std::clamp<std::int64_t>(static_cast<std::int64_t>(std::llround(frame)), 0, totalFrames - 1);
}

}
