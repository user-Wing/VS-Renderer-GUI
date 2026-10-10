#include "backend/VapourSynthFrameServer.h"
#include "backend/LavPlayback.h"

// Shared UI calls these interfaces; the Lite binary contains no external
// decoder loaders, DirectShow graphs or VapourSynth processing threads.
namespace vsr {
struct VapourSynthFrameServer::Impl {};
VapourSynthFrameServer::VapourSynthFrameServer(QObject *parent, bool) : QObject(parent) {}
VapourSynthFrameServer::~VapourSynthFrameServer() = default;
void VapourSynthFrameServer::initialize() { emit initialized(false); }
bool VapourSynthFrameServer::available() const { return false; }
bool VapourSynthFrameServer::initializing() const { return false; }
QString VapourSynthFrameServer::libraryPath() const { return {}; }
QString VapourSynthFrameServer::errorString() const { return QStringLiteral("VapourSynth is unavailable in Lite."); }
void VapourSynthFrameServer::loadScript(const QString &, const QString &) { emit errorOccurred(errorString()); }
void VapourSynthFrameServer::unloadScript() { emit scriptUnloaded(); }
void VapourSynthFrameServer::requestFrame(int, int, bool) {}
void VapourSynthFrameServer::setResourceLimits(int, int) {}

struct LavPlayback::Impl {};
LavPlayback::LavPlayback() = default;
LavPlayback::~LavPlayback() = default;
bool LavPlayback::open(const QString &, void *, bool, bool) { return false; }
bool LavPlayback::play() { return false; }
bool LavPlayback::pause() { return false; }
bool LavPlayback::seek(std::int64_t) { return false; }
std::int64_t LavPlayback::position() const { return 0; }
std::int64_t LavPlayback::duration() const { return 0; }
void LavPlayback::volume(float, bool) {}
QString LavPlayback::error() const { return QStringLiteral("LAV is unavailable in Lite."); }
void LavPlayback::resizeVideo(int, int) {}
void LavPlayback::showVideoSettings(void *) {}
void LavPlayback::showAudioSettings(void *) {}
bool LavPlayback::setRate(double) { return false; }
QImage LavPlayback::capture() const { return {}; }
bool LavPlayback::madvrActive() const { return false; }
void LavPlayback::setSubtitle(const QImage &) {}
}
