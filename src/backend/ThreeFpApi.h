#pragma once
#include "color/ColorBridge.h"

#include <QLibrary>
#include <QString>
#include <QImage>
#include "backend/SubtitleAbi.h"

#include <cstdint>

namespace vsr {

enum class ThreeFpResult : std::int32_t {
    Success = 0,
    InvalidArgument = -1,
    InvalidState = -2,
    BufferTooSmall = -3,
    NativeFailure = -4,
    FfmpegFailure = -5,
    DeviceFailure = -6,
    NotSupported = -7
};

enum class ThreeFpState : std::uint32_t {
    Idle = 0,
    Opening = 1,
    Ready = 2,
    Playing = 3,
    Paused = 4,
    Ended = 5,
    Failed = 6,
    Closed = 7
};

enum class ThreeFpScalingAlgorithm : std::uint32_t {
    Nearest = 0,
    Bilinear = 1,
    Bicubic = 2,
    Lanczos3 = 3,
    Jinc2 = 4,
    Spline36 = 5,
    SuperXbrSinglePass = 6,
    D3D11Native = 7,
    Lanczos4 = 8
};

struct ThreeFpConfiguration {
    std::uint32_t size;
    std::uint32_t version;
    void *outputWindow;
    std::uint32_t decodeMode;
    std::uint32_t colorMode;
    float sdrPeakNits;
    float hdrPeakNits;
    float sdrPaperWhiteNits;
    const char *audioEndpointIdUtf8;
    void *eventCallback;
    void *eventCallbackContext;
    std::uint32_t videoScalingQuality;
    std::uint32_t forceHdrOutput;
    std::int32_t preferredAdapterIndex = -1;
    std::uint32_t sdrScRgbMode = 0;
};

struct ThreeFpSnapshot {
    std::uint32_t size;
    std::uint32_t version;
    ThreeFpState state;
    std::uint32_t decodeMode;
    std::uint32_t requestedColorMode;
    std::uint32_t actualColorMode;
    std::int64_t position100ns;
    std::int64_t duration100ns;
    std::int64_t frameIndex;
    std::int64_t framePts;
    std::int32_t frameTimeBaseNumerator;
    std::int32_t frameTimeBaseDenominator;
    std::int32_t selectedVideoStream;
    std::int32_t selectedAudioStream;
    std::uint32_t videoWidth;
    std::uint32_t videoHeight;
    std::uint32_t isHdrSource;
    std::uint32_t isExternalAudio;
    std::int64_t externalAudioOffset100ns;
    std::uint64_t decodedVideoFrames;
    std::uint64_t presentedVideoFrames;
    std::uint64_t droppedVideoFrames;
    std::uint32_t queuedVideoFrames;
    std::uint32_t sourcePeakNits;
    std::uint64_t decodedAudioFrames;
    std::int64_t audioPosition100ns;
    std::int64_t bufferedAudio100ns;
    std::uint64_t audioUnderruns;
    std::uint64_t audioTimestampJitterFrames;
    std::uint64_t audioDiscontinuities;
    std::uint64_t audioInsertedSilenceFrames;
    std::uint64_t audioDroppedOverlapFrames;
    std::uint64_t coalescedVideoFrames;
    std::uint64_t audioRejectedFrames;
    std::uint64_t swapChainPresents;
    std::uint64_t presentWait100ns;
    std::uint64_t deviceLockWait100ns;
    std::uint64_t hardwareTransfer100ns;
    std::uint64_t softwareConvert100ns;
    std::uint64_t videoBitRate;
    std::uint64_t audioBitRate;
    std::uint32_t videoOutputBitDepth;
    std::uint32_t videoScalingMode;
    std::uint64_t timelineGeneration;
    std::uint32_t hdrFormat;
    std::uint32_t compatibleHdrFormats;
    std::uint32_t hdrProcessingPath;
    std::uint32_t dolbyVisionProfile;
    std::uint32_t dolbyVisionLevel;
    std::uint32_t hasDolbyVisionRpu;
    std::uint32_t hasDolbyVisionEnhancementLayer;
    std::uint32_t dolbyVisionEnhancementLayer;
    std::uint32_t dynamicHdrMetadataActive;
    std::uint32_t hdrFallbackActive;
    std::uint32_t displayMinLuminanceMilliNits;
    std::uint32_t displayPeakNits;
    std::uint32_t displayFullFramePeakNits;
    std::uint32_t effectiveTargetPeakNits;
};

struct ThreeFpPixelProbe {
    std::uint32_t size;
    std::uint32_t version;
    std::uint32_t x;
    std::uint32_t y;
    float red;
    float green;
    float blue;
    float alpha;
    std::uint32_t scalingMode;
    std::uint32_t outputBitDepth;
    std::uint32_t colorMode;
    std::uint32_t reserved;
};

enum class ThreeFpExternalPixelFormat : std::uint32_t {
    Gray8 = 1,
    Gray16 = 2,
    Yuv420P8 = 10,
    Yuv420P10 = 11,
    Yuv420P12 = 12,
    Yuv420P16 = 13,
    Yuv422P8 = 20,
    Yuv422P10 = 21,
    Yuv422P12 = 22,
    Yuv422P16 = 23,
    Yuv444P8 = 30,
    Yuv444P10 = 31,
    Yuv444P12 = 32,
    Yuv444P16 = 33,
    GbrP8 = 40,
    GbrP10 = 41,
    GbrP12 = 42,
    GbrP16 = 43
};

struct ThreeFpExternalVideoFrame {
    std::uint32_t size;
    std::uint32_t version;
    std::uint32_t width;
    std::uint32_t height;
    ThreeFpExternalPixelFormat format;
    std::uint32_t colorRange;
    std::uint32_t colorPrimaries;
    std::uint32_t colorTransfer;
    std::uint32_t colorMatrix;
    std::uint32_t chromaLocation;
    const void *data[4];
    std::int32_t linesize[4];
    std::int64_t frameIndex;
    std::int64_t duration100ns;
    std::int64_t totalFrames;
};

class ThreeFpApi final {
public:
    ThreeFpApi();

    bool available() const;
    QString libraryPath() const;
    QString errorString() const;
    std::uint32_t apiVersion() const;

    ThreeFpResult create(const ThreeFpConfiguration *configuration, void **handle) const;
    ThreeFpResult open(void *handle, const char *pathUtf8) const;
    ThreeFpResult play(void *handle) const;
    ThreeFpResult pause(void *handle) const;
    ThreeFpResult setClockOnly(void *handle, bool enabled) const;
    ThreeFpResult stop(void *handle) const;
    ThreeFpResult seek(void *handle, std::int64_t position100ns) const;
    ThreeFpResult seekFrame(void *handle, std::int64_t frame) const;
    ThreeFpResult stepFrame(void *handle, std::int32_t direction) const;
    ThreeFpResult stepKeyframe(void *handle, int direction) const;
    ThreeFpResult setPlaybackRate(void *handle, double rate) const;
    QString mediaInfo(void *handle) const;
    ThreeFpResult selectAudio(void *handle, int stream) const;
    ThreeFpResult loadExternalAudio(void *handle, const char *path) const;
    ThreeFpResult clearExternalAudio(void *handle) const;
    ThreeFpResult setAudioEffects(void *handle, bool enabled, const float *gains, float wave, qint64 delay) const;
    ThreeFpResult setVolume(void *handle, float volume, std::uint32_t muted) const;
    ThreeFpResult setPresentConfig(void *handle, bool enabled) const;
    ThreeFpResult setPacingConfig(void *handle, bool enabled) const;
    ThreeFpResult setViewTransform(void *handle, float zoom, float panX, float panY) const;
    ThreeFpResult setScalingAlgorithms(void *handle, ThreeFpScalingAlgorithm upscale,
                                       ThreeFpScalingAlgorithm downscale) const;
    ThreeFpResult snapshot(void *handle, ThreeFpSnapshot *snapshot) const;
    ThreeFpResult setColorSettings(void *handle, const VsrColorSettings *settings) const;
    ThreeFpResult colorStatus(void *handle, VsrColorStatus *status) const;
    ThreeFpResult readPixel(void *handle, ThreeFpPixelProbe *probe) const;
    ThreeFpResult submitExternalVideoFrame(void *handle, const ThreeFpExternalVideoFrame *frame) const;
    ThreeFpResult setExternalOutputFormat(void *handle, const char *format) const;
    ThreeFpResult setSubtitleLayer(void *handle, const TimedTextLayer *layer) const;
    QImage capture(void *handle, int width, int height) const;
    ThreeFpResult redraw(void *handle) const;
    void destroy(void *handle) const;

    static QString resultText(ThreeFpResult result);

private:
    template<typename T> bool resolve(T &target, const char *name);

    QLibrary library_;
    QString error_;

    using GetApiVersionFn = std::uint32_t (*)();
    using CreateFn = ThreeFpResult (*)(const ThreeFpConfiguration *, void **);
    using HandleFn = ThreeFpResult (*)(void *);
    using OpenFn = ThreeFpResult (*)(void *, const char *);
    using SeekFn = ThreeFpResult (*)(void *, std::int64_t);
    using StepFn = ThreeFpResult (*)(void *, std::int32_t);
    using RateFn = ThreeFpResult (*)(void *, double);
    using InfoFn = ThreeFpResult (*)(void *, char *, std::uint32_t, std::uint32_t *);
    using ExternalAudioFn = ThreeFpResult (*)(void *, const char *, int, std::int64_t);
    using EffectsFn = ThreeFpResult (*)(void *, std::uint32_t, const float *, float, std::int64_t);
    StepFn selectAudio_ = nullptr;
    StepFn setClockOnly_ = nullptr;
    ExternalAudioFn loadExternalAudio_ = nullptr;
    HandleFn clearExternalAudio_ = nullptr;
    EffectsFn setAudioEffects_ = nullptr;
    RateFn setPlaybackRate_ = nullptr;
    StepFn stepKeyframe_ = nullptr;
    InfoFn mediaInfo_ = nullptr;
    OpenFn setExternalOutputFormat_ = nullptr;
    using LayerFn = ThreeFpResult (*)(void *, const TimedTextLayer *);
    using RegionFn = ThreeFpResult (*)(void *, uint32_t, uint32_t, uint32_t, uint32_t, float *, uint32_t, uint32_t *);
    LayerFn setSubtitleLayer_ = nullptr;
    RegionFn readRegion_ = nullptr;
    using VolumeFn = ThreeFpResult (*)(void *, float, std::uint32_t);
    using ToggleFn = ThreeFpResult (*)(void *, std::uint32_t);
    using ViewFn = ThreeFpResult (*)(void *, float, float, float);
    using ScalingFn = ThreeFpResult (*)(void *, ThreeFpScalingAlgorithm, ThreeFpScalingAlgorithm);
    using SnapshotFn = ThreeFpResult (*)(void *, ThreeFpSnapshot *);
    using PixelFn = ThreeFpResult (*)(void *, ThreeFpPixelProbe *);
    using ExternalFrameFn = ThreeFpResult (*)(void *, const ThreeFpExternalVideoFrame *);
    using DestroyFn = void (*)(void *);

    GetApiVersionFn getApiVersion_ = nullptr;
    CreateFn create_ = nullptr;
    OpenFn open_ = nullptr;
    HandleFn play_ = nullptr;
    HandleFn pause_ = nullptr;
    HandleFn stop_ = nullptr;
    SeekFn seek_ = nullptr;
    SeekFn seekFrame_ = nullptr;
    StepFn stepFrame_ = nullptr;
    VolumeFn setVolume_ = nullptr;
    ToggleFn setPresentConfig_ = nullptr;
    ToggleFn setPacingConfig_ = nullptr;
    ViewFn setViewTransform_ = nullptr;
    ScalingFn setScalingAlgorithms_ = nullptr;
    SnapshotFn snapshot_ = nullptr;
    using ColorSetFn = ThreeFpResult (*)(void *, const VsrColorSettings *);
    using ColorStatusFn = ThreeFpResult (*)(void *, VsrColorStatus *);
    ColorSetFn setColorSettings_ = nullptr;
    ColorStatusFn colorStatus_ = nullptr;
    PixelFn readPixel_ = nullptr;
    ExternalFrameFn submitExternalVideoFrame_ = nullptr;
    HandleFn redraw_ = nullptr;
    DestroyFn destroy_ = nullptr;
};

}
