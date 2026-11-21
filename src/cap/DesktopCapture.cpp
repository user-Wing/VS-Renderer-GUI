#include "DesktopCapture.h"
#include "image/HdrPreview.h"
#include "image/ScreenshotPng.h"
#include "WgcBridge.h"
#include "CaptureFallback.h"
#include <QJsonArray>
#include <QLibrary>
#include <QCoreApplication>
#include <QDir>
#include <QUuid>
#include <QScreen>
#include <QWidget>
#include <QtGui/qscreen_platform.h>
#include <QPixmap>
#include <QColorSpace>
#include <QTransform>
#include <QElapsedTimer>
#include <QtCore/qfloat16.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace cap {
bool placeCaptureWindow(QWidget *window,const QRect &nativeBounds)
{
    if(nativeBounds.isEmpty())return false;
    return SetWindowPos(reinterpret_cast<HWND>(window->winId()),nullptr,nativeBounds.x(),nativeBounds.y(),nativeBounds.width(),nativeBounds.height(),SWP_NOZORDER|SWP_NOACTIVATE);
}
// Layout from Windows SDK 26100. Use the query's numeric ID on older MinGW headers.
struct AdvancedColorInfo2 {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 flags,colorEncoding,bitsPerColorChannel,activeColorMode;
};

vsr::screenshot::DisplayEvidence displayEvidence(QScreen *screen)
{
    vsr::screenshot::DisplayEvidence result;
    if(!screen)return result;
    const auto *native=screen->nativeInterface<QNativeInterface::QWindowsScreen>();
    MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);
    if(!native || !GetMonitorInfoW(native->handle(),reinterpret_cast<MONITORINFO*>(&monitor)))return result;
    result.identity=QString::fromWCharArray(monitor.szDevice);
    result.nativeBounds=QRect(monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top);
    bool outputColorKnown=false;
    ComPtr<IDXGIFactory1> factory;
    if(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        ComPtr<IDXGIAdapter1> adapter;
        for(UINT a=0;factory->EnumAdapters1(a,&adapter)==S_OK;++a,adapter.Reset()) {
            ComPtr<IDXGIOutput> output;
            for(UINT o=0;adapter->EnumOutputs(o,&output)==S_OK;++o,output.Reset()) {
                DXGI_OUTPUT_DESC desc{};if(FAILED(output->GetDesc(&desc)) || desc.Monitor!=native->handle())continue;
                result.rotation=int(desc.Rotation);
                ComPtr<IDXGIOutput6> advanced;DXGI_OUTPUT_DESC1 color{};
                if(SUCCEEDED(output.As(&advanced)) && SUCCEEDED(advanced->GetDesc1(&color))) {
                    outputColorKnown=true;result.gamutSource=QStringLiteral("DXGI_OUTPUT_DESC1 (driver reported, not measured)");
                    result.outputBits=color.BitsPerColor;
                    result.hdrActive=color.ColorSpace==DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                    result.nativeChromaticities={color.RedPrimary[0],color.RedPrimary[1],color.GreenPrimary[0],color.GreenPrimary[1],color.BluePrimary[0],color.BluePrimary[1],color.WhitePoint[0],color.WhitePoint[1]};
                }
            }
        }
    }
    for(int attempt=0;attempt<3;++attempt){
        UINT32 pathCount=0,modeCount=0;
        if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&pathCount,&modeCount)!=ERROR_SUCCESS)break;
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
        const auto status=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&pathCount,paths.data(),&modeCount,modes.data(),nullptr);
        if(status==ERROR_INSUFFICIENT_BUFFER)continue;
        if(status!=ERROR_SUCCESS)break;
        for(UINT32 i=0;i<pathCount;++i){
            const auto &path=paths[i];DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
            source.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(source),path.sourceInfo.adapterId,path.sourceInfo.id};
            if(DisplayConfigGetDeviceInfo(&source.header)!=ERROR_SUCCESS || QString::fromWCharArray(source.viewGdiDeviceName)!=QString::fromWCharArray(monitor.szDevice))continue;
            result.identity+=QString(":%1:%2:%3").arg(quint32(path.targetInfo.adapterId.HighPart)).arg(path.targetInfo.adapterId.LowPart).arg(path.targetInfo.id);
            AdvancedColorInfo2 info2{};info2.header={static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(15),sizeof(info2),path.targetInfo.adapterId,path.targetInfo.id};
            if(DisplayConfigGetDeviceInfo(&info2.header)==ERROR_SUCCESS) {
                result.modeSource=QStringLiteral("DisplayConfig advanced color v2");
                result.modeKnown=true;result.advancedActive=(info2.flags&2)!=0;
                result.hdrActive=info2.activeColorMode==2;result.wideActive=info2.activeColorMode==1;
                result.outputBits=info2.bitsPerColorChannel;
            }else{
                DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO info{};info.header={DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO,sizeof(info),path.targetInfo.adapterId,path.targetInfo.id};
                if(DisplayConfigGetDeviceInfo(&info.header)==ERROR_SUCCESS){
                    result.modeKnown=!info.advancedColorEnabled || outputColorKnown;result.advancedActive=info.advancedColorEnabled;result.outputBits=info.bitsPerColorChannel;
                    result.modeSource=result.modeKnown?QStringLiteral("DisplayConfig v1 + DXGI"):QStringLiteral("DisplayConfig v1; active mode unresolved");
                }
            }
            DISPLAYCONFIG_SDR_WHITE_LEVEL white{};
            white.header={DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,sizeof(white),path.targetInfo.adapterId,path.targetInfo.id};
            if(DisplayConfigGetDeviceInfo(&white.header)==ERROR_SUCCESS && white.SDRWhiteLevel>0)result.sdrWhiteNits=white.SDRWhiteLevel*80.0/1000.0;
            return result;
        }
        break;
    }
    return result;
}

double sdrWhiteNits(const QImage &image)
{
    const double value=image.text("capSdrWhiteNits").toDouble();
    return std::isfinite(value) && value>0?value:80.0;
}

static QImage captureDxgi(QScreen *screen, QString *error)
{
    ComPtr<IDXGIFactory1> factory;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT a=0; factory->EnumAdapters1(a,&adapter)==S_OK; ++a,adapter.Reset()) {
            ComPtr<IDXGIOutput> output;
            for (UINT o=0; adapter->EnumOutputs(o,&output)==S_OK; ++o,output.Reset()) {
                DXGI_OUTPUT_DESC desc{};
                output->GetDesc(&desc);
                const auto *native=screen->nativeInterface<QNativeInterface::QWindowsScreen>();
                if (!native || desc.Monitor!=native->handle()) continue;
                ComPtr<IDXGIOutput6> output6;
                DXGI_OUTPUT_DESC1 color{};
                // The returned texture format, not HDR capability or scan-out bits, proves FP16 storage.
                if(SUCCEEDED(output.As(&output6)))output6->GetDesc1(&color);
                // DuplicateOutput1 preserves the desktop's FP16 scRGB surface.
                // The older DuplicateOutput always converts to 8-bit BGRA.
                ComPtr<ID3D11Device> device;
                ComPtr<ID3D11DeviceContext> context;
                ComPtr<IDXGIOutput5> output5;
                ComPtr<IDXGIOutputDuplication> duplicate;
                const DXGI_FORMAT format=DXGI_FORMAT_R16G16B16A16_FLOAT;
                HRESULT hr=D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
                if (SUCCEEDED(hr)) hr=output.As(&output5);
                if (SUCCEEDED(hr)) hr=output5->DuplicateOutput1(device.Get(),0,1,&format,&duplicate);
                DXGI_OUTDUPL_FRAME_INFO info{};
                ComPtr<IDXGIResource> resource;
                if (SUCCEEDED(hr)) {
                    QElapsedTimer wait;wait.start();
                    do {
                        hr=duplicate->AcquireNextFrame(UINT(qMax<qint64>(1,1000-wait.elapsed())),&info,&resource);
                        if (FAILED(hr) || info.LastPresentTime.QuadPart!=0) break;
                        // A pointer-only notification can precede the first desktop
                        // image. Its texture is not a screenshot (often all black).
                        duplicate->ReleaseFrame();resource.Reset();
                        hr=DXGI_ERROR_WAIT_TIMEOUT;
                    } while (wait.elapsed()<1000);
                }
                if (FAILED(hr)) {
                    *error=QStringLiteral("HDR 桌面采集失败（0x%1）；请检查显示会话和显卡驱动。").arg(quint32(hr),8,16,QChar('0'));
                    return {};
                }
                ComPtr<ID3D11Texture2D> texture,staging;
                hr=resource.As(&texture);
                D3D11_TEXTURE2D_DESC td{};
                if (SUCCEEDED(hr)) texture->GetDesc(&td);
                if (SUCCEEDED(hr) && td.Format!=format) hr=E_FAIL;
                td.Usage=D3D11_USAGE_STAGING;
                td.BindFlags=0;td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
                if (SUCCEEDED(hr)) hr=device->CreateTexture2D(&td,nullptr,&staging);
                D3D11_MAPPED_SUBRESOURCE mapped{};
                if (SUCCEEDED(hr)) {
                    context->CopyResource(staging.Get(),texture.Get());
                    hr=context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped);
                }
                QImage image;
                if (SUCCEEDED(hr)) {
                    image=QImage(td.Width,td.Height,QImage::Format_RGBA32FPx4);
                    for (UINT y=0;y<td.Height;++y) {
                        const auto *src=reinterpret_cast<const qfloat16*>(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch);
                        auto *dst=reinterpret_cast<float*>(image.scanLine(y));
                        for (UINT x=0;x<td.Width;++x) {
                            for (int c=0;c<3;++c) dst[x*4+c]=float(src[x*4+c]);
                            dst[x*4+3]=1.0f;
                        }
                    }
                    context->Unmap(staging.Get(),0);
                    int angle=0;
                    if (desc.Rotation==DXGI_MODE_ROTATION_ROTATE90) angle=90;
                    if (desc.Rotation==DXGI_MODE_ROTATION_ROTATE180) angle=180;
                    if (desc.Rotation==DXGI_MODE_ROTATION_ROTATE270) angle=270;
                    if (angle) image=image.transformed(QTransform().rotate(angle));
                    image.setColorSpace(QColorSpace::SRgbLinear);
                    image.setText("capHdr","scRGB-FP16");
                    image.setText("capSdrWhiteNits",QString::number(displayEvidence(screen).sdrWhiteNits,'g',17));
                }
                duplicate->ReleaseFrame();
                if (image.isNull()) *error=QStringLiteral("HDR 桌面未提供可读取的 FP16 图像。");
                return image;
            }
        }
    }
    if(error->isEmpty())*error=QStringLiteral("DXGI output unavailable");
    return {};
}

QImage captureWgcScreen(QScreen *screen,QString *error)
{
    const auto *native=screen->nativeInterface<QNativeInterface::QWindowsScreen>();
    if(!native){*error=QStringLiteral("No native monitor handle");return {};}
    QLibrary bridge(QDir(QCoreApplication::applicationDirPath()).filePath("vsr-wgc.dll"));
    const auto capture=reinterpret_cast<VsrWgcCapture>(bridge.resolve("vsrCaptureWgc"));
    if(!capture){*error=QStringLiteral("WGC helper unavailable: ")+bridge.errorString();return {};}
    QImage image;wchar_t message[512]{};
    const auto receiver=[](const uint16_t *pixels,uint32_t w,uint32_t h,uint32_t pitch,void *user){
        auto &out=*static_cast<QImage*>(user);out=QImage(int(w),int(h),QImage::Format_RGBA32FPx4);
        if(out.isNull())return;
        for(uint32_t y=0;y<h;++y){const auto *src=reinterpret_cast<const qfloat16*>(reinterpret_cast<const char*>(pixels)+y*pitch);auto *dst=reinterpret_cast<float*>(out.scanLine(int(y)));
            for(uint32_t x=0;x<w;++x){for(int c=0;c<3;++c)dst[x*4+c]=float(src[x*4+c]);dst[x*4+3]=1.f;}}
    };
    const auto hr=capture(native->handle(),receiver,&image,message,512);
    if(FAILED(hr)){*error=QString("WGC 0x%1: %2").arg(quint32(hr),8,16,QChar('0')).arg(QString::fromWCharArray(message));return {};}
    image.setColorSpace(QColorSpace::SRgbLinear);return image;
}

QImage captureScreen(QScreen *screen,QString *error)
{
    QString failure;if(error)error->clear();if(!screen){if(error)*error=QStringLiteral("No capture display");return {};}
    // Stateless sessions retry each request, including access-lost, adapter and mode changes.
    for(int attempt=0;attempt<2;++attempt) {
        const auto display=displayEvidence(screen);const auto geometry=screen->geometry();const auto dpr=screen->devicePixelRatio();
        auto result=captureWithFallback(display,[&](QString *e){return captureWgcScreen(screen,e);},[&](QString *e){return captureDxgi(screen,e);},[&](QString *){auto i=screen->grabWindow(0).toImage();i.setColorSpace(QColorSpace::SRgb);return i;});
        auto image=std::move(result.image);const auto backend=result.backend;const bool floating=result.floating;failure=result.degradation;
        const auto current=displayEvidence(screen);
        if(geometry!=screen->geometry() || dpr!=screen->devicePixelRatio() || current.identity!=display.identity || current.classification()!=display.classification() || current.sdrWhiteNits!=display.sdrWhiteNits || current.nativeBounds!=display.nativeBounds || current.rotation!=display.rotation || (!image.isNull() && !current.nativeBounds.isEmpty() && image.size()!=current.nativeBounds.size())){failure=QStringLiteral("Display changed during capture; retry required");continue;}
        if(image.isNull()){if(error)*error=failure.isEmpty()?QStringLiteral("Desktop capture returned no pixels"):failure;return {};}
        vsr::screenshot::FrameColor facts;facts.desktop=true;facts.backend=backend;facts.sourceIdentity=display.identity;facts.frameIdentity=QUuid::createUuid().toString(QUuid::WithoutBraces);
        facts.sourceBits=floating?16:8;facts.floating=floating;facts.range=vsr::screenshot::Range::Full;facts.sdrWhiteNits=display.sdrWhiteNits;
        facts.advancedSdr=display.classification()==vsr::screenshot::DisplayClass::AdvancedSdr;
        facts.primaries=floating?vsr::screenshot::Primaries::Rec709:vsr::screenshot::Primaries::Unknown;
        facts.transfer=floating?vsr::screenshot::Transfer::Linear:vsr::screenshot::Transfer::Unknown;
        facts.units=floating?vsr::screenshot::Units::ScRgb80Nits:vsr::screenshot::Units::Relative;
        facts.degradation=failure;
        if(!floating)facts.degradation+=QStringLiteral("; legacy 8-bit capture; sRGB assumed, original display precision/color unmeasured");
        if(!display.modeKnown && !display.hdrActive)facts.degradation+=QStringLiteral("; active display color mode unknown");
        image.setDevicePixelRatio(1);image.setText("capHdr",floating?"scRGB-FP16":"SDR");
        image.setText("capSdrWhiteNits",QString::number(display.sdrWhiteNits,'g',17));
        image.setText("capDisplayClass",QString::number(int(display.classification())));image.setText("capOutputBits",QString::number(display.outputBits));
        QJsonArray gamut;for(float v:display.nativeChromaticities)gamut.append(v);
        const QJsonObject diagnostics{{"identity",display.identity},{"modeKnown",display.modeKnown},{"modeSource",display.modeSource},{"gamutSource",display.gamutSource},{"advancedActive",display.advancedActive},{"hdrActive",display.hdrActive},{"wideActive",display.wideActive},{"outputBits",display.outputBits},{"nativeChromaticities",gamut},{"rotation",display.rotation},{"width",display.nativeBounds.width()},{"height",display.nativeBounds.height()},{"sdrWhiteNits",display.sdrWhiteNits}};
        image.setText("capDisplayEvidence",QString::fromUtf8(QJsonDocument(diagnostics).toJson(QJsonDocument::Compact)));
        image.setText("sourceBitDepth",QString::number(facts.sourceBits));image.setText("sourcePixelFormat",backend);
        vsr::screenshot::attach(image,facts);return image;
    }
    if(error)*error=failure;return {};
}

QImage pngPixels(const QImage &image)
{
    return vsr::screenshot::exportPixels(image,vsr::screenshot::exportPlan(image));
}

QImage preview(const QImage &image)
{
    const auto plan=vsr::screenshot::previewPlan(image);
    const bool scRgb=plan.source.units==vsr::screenshot::Units::ScRgb80Nits;
    const auto desktopWhite=plan.preview==vsr::screenshot::Preview::DesktopWhite?plan.whiteNits:0;
    auto output=vsr::imageForSdrDisplay(image,scRgb,desktopWhite,false);
    if(output.colorSpace().isValid() && output.colorSpace()!=QColorSpace(QColorSpace::SRgb))output=output.convertedToColorSpace(QColorSpace::SRgb,QImage::Format_RGBA64);
    return output;
}

bool savePng(const QImage &image,const QString &path)
{
    const auto plan=vsr::screenshot::exportPlan(image);
    return vsr::screenshot::writePng(path,vsr::screenshot::exportPixels(image,plan),plan).isEmpty();
}
}
