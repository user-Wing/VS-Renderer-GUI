#pragma once
#include <array>
#include <cstdint>
namespace vsr_native {
// Try the source rate before the Windows shared-mode mix format. Do not
// force 192 kHz merely because it appears first in a generic fallback list.
inline std::array<std::uint32_t,8> exclusiveSampleRates(std::uint32_t source,std::uint32_t mix) noexcept {
    return {source,mix,48000,44100,96000,88200,192000,176400};
}
}
