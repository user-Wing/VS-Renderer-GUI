#pragma once
#include <cstdint>
namespace vsr {
struct AudioTapConfig {std::uint32_t size=sizeof(AudioTapConfig),version=1,ringCapacityBytes=1024*1024,flags=0;};
struct AudioTapBlock {
    std::uint32_t size=sizeof(AudioTapBlock),version=1,bytesWritten=0,requiredBytes=0;
    std::uint64_t generation=0;std::int64_t position100ns=0;
    std::uint32_t frames=0,sampleRate=0,channels=0;
    std::int32_t positions[24]{},sampleFormat=0;
};
static_assert(sizeof(AudioTapConfig)==16 && sizeof(AudioTapBlock)==144);
struct VideoEnhancementStatus {
    std::uint32_t size=sizeof(VideoEnhancementStatus),version=20,requested=0,nvidiaAdapter=0,active=0,driverRejected=0,reason=0;
    std::uint32_t sourceWidth=0,sourceHeight=0,targetWidth=0,targetHeight=0;
};
static_assert(sizeof(VideoEnhancementStatus)==44);
}
