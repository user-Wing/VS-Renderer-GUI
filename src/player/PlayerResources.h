#pragma once
#include <memory>
#include <QString>
namespace vsr {
struct ResourceUsage { double cpu = -1, processCpu = -1, gpu = -1, ram = -1, vram = -1; double cpuWatts=-1,gpuWatts=-1;QString cpuPowerStatus="N/A"; unsigned long long memoryMiB = 0, totalMemoryMiB = 0; };
class PlayerResources final {
public:
    PlayerResources(); ~PlayerResources();
    ResourceUsage sample(bool detailed = true);
private:
    struct Impl; std::unique_ptr<Impl> impl_;
};
}
