#pragma once
#include "color/ScreenshotColor.h"
#include <functional>
namespace cap {
using CaptureAttempt=std::function<QImage(QString *)>;
struct CaptureResult {QImage image;QString backend,degradation;bool floating=false;};
// No permanent negative capability cache: a failed request cannot disable the next one.
inline CaptureResult captureWithFallback(const vsr::screenshot::DisplayEvidence &display,
    const CaptureAttempt &wgc,const CaptureAttempt &dxgi,const CaptureAttempt &legacy) {
    CaptureResult result;
    if(display.needsFloatCapture()) {
        result.image=wgc(&result.degradation);result.backend="WGC-FP16";
        if(result.image.isNull()){
            QString error;result.image=dxgi(&error);result.backend="DXGI-FP16";
            if(result.image.isNull())result.degradation+="; "+error;
        }
        result.floating=!result.image.isNull();
    }
    if(!result.floating){QString error;result.image=legacy(&error);result.backend="QScreen-GDI8";if(!error.isEmpty())result.degradation+="; "+error;}
    return result;
}
}
