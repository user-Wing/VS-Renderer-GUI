#include "image/ImageHdrSurface.h"
#include <QColorSpace>
#include <QColorTransform>
#include <QElapsedTimer>
#include <QPaintEngine>
#include <algorithm>
#include <cmath>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

// Microsoft Advanced Color guidance: FP16 flip swap chains use linear scRGB; 1.0 is 80 nits.
// https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
// https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgioutput6-getdesc1

namespace vsr {
#ifdef Q_OS_WIN
namespace {
static constexpr char HdrShader[]=R"(
cbuffer Settings : register(b0) { float4 imageRect; float4 view; };
Texture2D<float4> imageTexture : register(t0);
Texture2D<float4> overlayTexture : register(t1);
SamplerState imageSampler : register(s0);
struct Vertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };
Vertex vs(uint id : SV_VertexID) { Vertex o; o.uv=float2((id<<1)&2,id&2); o.position=float4(o.uv.x*2-1,1-o.uv.y*2,0,1); return o; }
float4 ps(Vertex v) : SV_Target {
    float2 p=v.position.xy;
    float3 color=float3(.025,.025,.025)*view.z;
    float2 imageUv=(p-imageRect.xy)/imageRect.zw;
    if(all(imageUv>=0) && all(imageUv<=1)) {
        float checker=fmod(floor(p.x/12)+floor(p.y/12),2)>0 ? .08 : .045;
        float4 native=imageTexture.Sample(imageSampler,imageUv);
        color=lerp(checker.xxx*view.z,native.rgb*view.w,saturate(native.a));
    }
    float4 overlay=overlayTexture.Sample(imageSampler,p/view.xy);
    color=lerp(color,overlay.rgb*view.z,saturate(overlay.a));
    return float4(color,1);
}
)";
}
#endif
struct ImageHdrSurface::Impl {
    ImageHdrSurface *owner;
    bool active = false;
    QString state = QStringLiteral("SDR preview (HDR output disabled)");
    explicit Impl(ImageHdrSurface *surface):owner(surface){}
#ifdef Q_OS_WIN
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain3> swap;
    ComPtr<ID3D11RenderTargetView> backBuffer;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11ShaderResourceView> imageView,overlayView;
    QSize bufferSize;
    qint64 imageKey = 0,overlayKey = 0;
    QElapsedTimer displayCheck;
    bool failure(const QString &message,HRESULT hr=S_OK){
        state=QStringLiteral("SDR fallback: ")+message;if(FAILED(hr))state+=QStringLiteral(" (0x%1)").arg(quint32(hr),8,16,QChar('0'));
        active=false;owner->hide();return false;
    }
    bool outputSupportsHdr(){
        ComPtr<IDXGIFactory1> factory;
        auto hr=CreateDXGIFactory1(__uuidof(IDXGIFactory1),reinterpret_cast<void **>(factory.GetAddressOf()));
        if(FAILED(hr))return failure(QStringLiteral("Unable to query display HDR capabilities"),hr);
        const auto monitor=MonitorFromWindow(reinterpret_cast<HWND>(owner->winId()),MONITOR_DEFAULTTONEAREST);
        for(UINT adapterIndex=0;;++adapterIndex){
            ComPtr<IDXGIAdapter1> adapter;hr=factory->EnumAdapters1(adapterIndex,adapter.GetAddressOf());if(hr==DXGI_ERROR_NOT_FOUND)break;if(FAILED(hr))continue;
            for(UINT outputIndex=0;;++outputIndex){
                ComPtr<IDXGIOutput> output;hr=adapter->EnumOutputs(outputIndex,output.GetAddressOf());if(hr==DXGI_ERROR_NOT_FOUND)break;if(FAILED(hr))continue;
                DXGI_OUTPUT_DESC desc{};if(FAILED(output->GetDesc(&desc)) || desc.Monitor!=monitor)continue;
                ComPtr<IDXGIOutput6> advanced;DXGI_OUTPUT_DESC1 desc1{};
                if(FAILED(output.As(&advanced)) || FAILED(advanced->GetDesc1(&desc1)))return failure(QStringLiteral("The display does not expose Advanced Color"));
                if(desc1.ColorSpace!=DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)return failure(QStringLiteral("Windows HDR is not enabled on this display"));
                return true;
            }
        }
        return failure(QStringLiteral("Unable to find the display containing the editor"));
    }
    bool makeBackBuffer(){
        ComPtr<ID3D11Texture2D> buffer;auto hr=swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void **>(buffer.GetAddressOf()));
        if(FAILED(hr))return failure(QStringLiteral("Unable to acquire HDR swap-chain buffer"),hr);
        hr=device->CreateRenderTargetView(buffer.Get(),nullptr,backBuffer.ReleaseAndGetAddressOf());
        return SUCCEEDED(hr) || failure(QStringLiteral("Unable to create HDR render target"),hr);
    }
    bool initialize(){
        if(!outputSupportsHdr())return false;
        const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0};D3D_FEATURE_LEVEL feature{};
        auto hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,2,D3D11_SDK_VERSION,device.ReleaseAndGetAddressOf(),&feature,context.ReleaseAndGetAddressOf());
        if(hr==E_INVALIDARG)hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels+1,1,D3D11_SDK_VERSION,device.ReleaseAndGetAddressOf(),&feature,context.ReleaseAndGetAddressOf());
        if(FAILED(hr))return failure(QStringLiteral("Unable to initialize D3D11 HDR output"),hr);
        ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<IDXGIFactory2> factory;
        hr=device.As(&dxgi);if(SUCCEEDED(hr))hr=dxgi->GetAdapter(adapter.GetAddressOf());if(SUCCEEDED(hr))hr=adapter->GetParent(__uuidof(IDXGIFactory2),reinterpret_cast<void **>(factory.GetAddressOf()));
        if(FAILED(hr))return failure(QStringLiteral("Unable to obtain the HDR swap-chain factory"),hr);
        bufferSize=(owner->size()*owner->devicePixelRatioF()).expandedTo({1,1});
        DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=bufferSize.width();desc.Height=bufferSize.height();desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.Scaling=DXGI_SCALING_STRETCH;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;desc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
        ComPtr<IDXGISwapChain1> first;hr=factory->CreateSwapChainForHwnd(device.Get(),reinterpret_cast<HWND>(owner->winId()),&desc,nullptr,nullptr,first.GetAddressOf());
        if(SUCCEEDED(hr))hr=first.As(&swap);if(FAILED(hr))return failure(QStringLiteral("Unable to create an FP16 HDR swap chain"),hr);
        UINT support=0;hr=swap->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,&support);
        if(FAILED(hr) || !(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))return failure(QStringLiteral("The swap chain does not support scRGB presentation"),hr);
        hr=swap->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);if(FAILED(hr))return failure(QStringLiteral("Unable to select the scRGB output color space"),hr);
        if(!makeBackBuffer())return false;

        ComPtr<ID3DBlob> vs,ps,errors;hr=D3DCompile(HdrShader,sizeof(HdrShader)-1,"ImageHdrSurface",nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,vs.GetAddressOf(),errors.GetAddressOf());
        if(FAILED(hr))return failure(QStringLiteral("Unable to compile HDR vertex shader"),hr);
        hr=D3DCompile(HdrShader,sizeof(HdrShader)-1,"ImageHdrSurface",nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,ps.GetAddressOf(),errors.ReleaseAndGetAddressOf());
        if(FAILED(hr))return failure(QStringLiteral("Unable to compile HDR pixel shader"),hr);
        hr=device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,vertexShader.ReleaseAndGetAddressOf());if(SUCCEEDED(hr))hr=device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,pixelShader.ReleaseAndGetAddressOf());
        if(FAILED(hr))return failure(QStringLiteral("Unable to create HDR shaders"),hr);
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=32;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        hr=device->CreateBuffer(&cb,nullptr,constants.ReleaseAndGetAddressOf());if(FAILED(hr))return failure(QStringLiteral("Unable to allocate HDR settings"),hr);
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
        hr=device->CreateSamplerState(&sd,sampler.ReleaseAndGetAddressOf());if(FAILED(hr))return failure(QStringLiteral("Unable to create HDR image sampler"),hr);
        displayCheck.start();active=true;state=QStringLiteral("HDR active: D3D11 FP16 scRGB (1.0 = 80 nits)");return true;
    }
    bool upload(const QImage &image,DXGI_FORMAT format,ComPtr<ID3D11ShaderResourceView> &view){
        if(image.isNull() || image.width()>D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || image.height()>D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)return failure(QStringLiteral("HDR preview texture is empty or exceeds the D3D11 size limit"));
        D3D11_TEXTURE2D_DESC td{};td.Width=image.width();td.Height=image.height();td.MipLevels=td.ArraySize=1;td.Format=format;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=image.constBits();data.SysMemPitch=image.bytesPerLine();ComPtr<ID3D11Texture2D> texture;
        auto hr=device->CreateTexture2D(&td,&data,texture.GetAddressOf());if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture.Get(),nullptr,view.ReleaseAndGetAddressOf());
        return SUCCEEDED(hr) || failure(QStringLiteral("Unable to upload HDR preview texture"),hr);
    }
    void reset(){
        if(context)context->ClearState();
        overlayView.Reset();imageView.Reset();sampler.Reset();constants.Reset();pixelShader.Reset();vertexShader.Reset();backBuffer.Reset();swap.Reset();
        if(context)context->Flush();
        context.Reset();device.Reset();imageKey=overlayKey=0;bufferSize={};
    }
#endif
};
ImageHdrSurface::ImageHdrSurface(QWidget *parent):QWidget(parent),d(std::make_unique<Impl>(this)){
    setAttribute(Qt::WA_NativeWindow);setAttribute(Qt::WA_PaintOnScreen);setAttribute(Qt::WA_NoSystemBackground);setAttribute(Qt::WA_TransparentForMouseEvents);setFocusPolicy(Qt::NoFocus);hide();
}
ImageHdrSurface::~ImageHdrSurface(){
#ifdef Q_OS_WIN
    d->reset();
#endif
}
bool ImageHdrSurface::setHdrEnabled(bool enabled){
    hide();d->active=false;
#ifdef Q_OS_WIN
    d->reset();
    if(enabled && d->initialize()){show();raise();return true;}
    if(!enabled)d->state=QStringLiteral("SDR preview (HDR output disabled)");
#else
    d->state=enabled?QStringLiteral("SDR fallback: native HDR output is supported on Windows only"):QStringLiteral("SDR preview (HDR output disabled)");
#endif
    return false;
}
bool ImageHdrSurface::hdrActive() const{return d->active;}
QString ImageHdrSurface::status() const{return d->state;}
bool ImageHdrSurface::validateShaders(QString *error){
#ifdef Q_OS_WIN
    for(const auto *entry:{"vs","ps"}){
        Microsoft::WRL::ComPtr<ID3DBlob> blob,errors;const auto profile=entry[0]=='v'?"vs_5_0":"ps_5_0";
        const auto hr=D3DCompile(HdrShader,sizeof(HdrShader)-1,"ImageHdrSurface",nullptr,nullptr,entry,profile,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,blob.GetAddressOf(),errors.GetAddressOf());
        if(FAILED(hr)){if(error)*error=errors?QString::fromUtf8(static_cast<const char *>(errors->GetBufferPointer()),int(errors->GetBufferSize())):QStringLiteral("Unable to compile HDR shader (0x%1)").arg(quint32(hr),8,16,QChar('0'));return false;}
    }
    return true;
#else
    if(error)*error=QStringLiteral("Native HDR output is supported on Windows only");return false;
#endif
}
QPaintEngine *ImageHdrSurface::paintEngine() const{return nullptr;}
void ImageHdrSurface::paintEvent(QPaintEvent *){}
bool ImageHdrSurface::nativeEvent(const QByteArray &type,void *message,qintptr *result){
#ifdef Q_OS_WIN
    const auto *event=static_cast<MSG *>(message);if(event && event->message==WM_NCHITTEST){*result=HTTRANSPARENT;return true;}
#endif
    return QWidget::nativeEvent(type,message,result);
}
void ImageHdrSurface::present(const QImage &native,const QRectF &target,const QImage &overlay,double exposure,double paperWhiteNits){
#ifdef Q_OS_WIN
    if(!d->active)return;
    if(!std::isfinite(exposure) || !std::isfinite(paperWhiteNits) || paperWhiteNits<=0 || target.isEmpty())return;
    if(d->displayCheck.elapsed()>1000){if(!d->outputSupportsHdr())return;d->displayCheck.restart();}
    const QSize pixels=(size()*devicePixelRatioF()).expandedTo({1,1});
    if(pixels!=d->bufferSize){d->context->OMSetRenderTargets(0,nullptr,nullptr);d->backBuffer.Reset();const auto hr=d->swap->ResizeBuffers(0,pixels.width(),pixels.height(),DXGI_FORMAT_UNKNOWN,0);if(FAILED(hr)){d->failure(QStringLiteral("Unable to resize HDR swap chain"),hr);return;}d->bufferSize=pixels;if(!d->makeBackBuffer())return;}
    if(d->imageKey!=native.cacheKey() || !d->imageView){
        QImage image=native.convertToFormat(QImage::Format_RGBA32FPx4);
        if(native.colorSpace().isValid())image=image.colorTransformed(native.colorSpace().transformationToColorSpace(QColorSpace::SRgbLinear),QImage::Format_RGBA32FPx4);
        else {
            const auto format=native.format();const bool floating=format==QImage::Format_RGBA32FPx4 || format==QImage::Format_RGBA32FPx4_Premultiplied || format==QImage::Format_RGBX32FPx4 || format==QImage::Format_RGBA16FPx4 || format==QImage::Format_RGBA16FPx4_Premultiplied || format==QImage::Format_RGBX16FPx4;
            if(!floating){image.setColorSpace(QColorSpace::SRgb);image=image.colorTransformed(image.colorSpace().transformationToColorSpace(QColorSpace::SRgbLinear),QImage::Format_RGBA32FPx4);}
        }
        if(!d->upload(image,DXGI_FORMAT_R32G32B32A32_FLOAT,d->imageView))return;d->imageKey=native.cacheKey();
    }
    if(d->overlayKey!=overlay.cacheKey() || !d->overlayView){
        QImage image;if(overlay.isNull()){image=QImage(1,1,QImage::Format_RGBA8888);image.fill(Qt::transparent);}else image=overlay.convertToFormat(QImage::Format_RGBA8888);
        if(!d->upload(image,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,d->overlayView))return;d->overlayKey=overlay.cacheKey();
    }
    const float ratio=devicePixelRatioF(),white=paperWhiteNits/80.;
    // Qt 6.10 maps PQ's 10000 nit endpoint to 64 linear units. PQ is absolute;
    // ordinary linear/SDR input uses the user-selected relative paper white.
    const float imageWhite=native.colorSpace().transferFunction()==QColorSpace::TransferFunction::St2084?10000.f/(64*80):white;
    const float settings[]={float(target.x())*ratio,float(target.y())*ratio,float(target.width())*ratio,float(target.height())*ratio,float(pixels.width()),float(pixels.height()),white,float(std::exp2(std::clamp(exposure,-32.,32.)))*imageWhite};
    d->context->UpdateSubresource(d->constants.Get(),0,nullptr,settings,0,0);
    D3D11_VIEWPORT viewport{};viewport.Width=pixels.width();viewport.Height=pixels.height();viewport.MaxDepth=1;d->context->RSSetViewports(1,&viewport);
    auto *back=d->backBuffer.Get();d->context->OMSetRenderTargets(1,&back,nullptr);d->context->IASetInputLayout(nullptr);d->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);d->context->VSSetShader(d->vertexShader.Get(),nullptr,0);d->context->PSSetShader(d->pixelShader.Get(),nullptr,0);
    auto *cb=d->constants.Get();d->context->PSSetConstantBuffers(0,1,&cb);auto *sampler=d->sampler.Get();d->context->PSSetSamplers(0,1,&sampler);ID3D11ShaderResourceView *views[]={d->imageView.Get(),d->overlayView.Get()};d->context->PSSetShaderResources(0,2,views);d->context->Draw(3,0);
    const auto hr=d->swap->Present(1,0);if(FAILED(hr))d->failure(QStringLiteral("HDR presentation failed"),hr);
#else
    Q_UNUSED(native);Q_UNUSED(target);Q_UNUSED(overlay);Q_UNUSED(exposure);Q_UNUSED(paperWhiteNits);
#endif
}
}
