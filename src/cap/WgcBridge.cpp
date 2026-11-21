#include "WgcBridge.h"
#include <d3d11.h>
#include <dxgi1_6.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <chrono>
#include <thread>
#include <string>

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

static void capture(HMONITOR monitor,VsrWgcReceiver receiver,void *user)
{
    if(!GraphicsCaptureSession::IsSupported())throw hresult_error(E_NOTIMPL,L"Windows Graphics Capture unavailable");
    com_ptr<IDXGIFactory1> factory;check_hresult(CreateDXGIFactory1(__uuidof(IDXGIFactory1),factory.put_void()));
    com_ptr<IDXGIAdapter1> adapter,selected;
    for(UINT a=0;factory->EnumAdapters1(a,adapter.put())==S_OK;++a) {
        com_ptr<IDXGIOutput> output;
        for(UINT o=0;adapter->EnumOutputs(o,output.put())==S_OK;++o) {
            DXGI_OUTPUT_DESC desc{};check_hresult(output->GetDesc(&desc));if(desc.Monitor==monitor){selected=adapter;break;}
        }
        if(selected)break;
    }
    if(!selected)throw hresult_error(DXGI_ERROR_NOT_FOUND,L"Capture monitor/adapter disappeared");
    com_ptr<ID3D11Device> device;com_ptr<ID3D11DeviceContext> context;
    check_hresult(D3D11CreateDevice(selected.get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.put(),nullptr,context.put()));
    com_ptr<IInspectable> inspectable;check_hresult(CreateDirect3D11DeviceFromDXGIDevice(device.as<IDXGIDevice>().get(),inspectable.put()));
    const auto directDevice=inspectable.as<IDirect3DDevice>();
    auto interop=get_activation_factory<GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
    GraphicsCaptureItem item{nullptr};check_hresult(interop->CreateForMonitor(monitor,guid_of<GraphicsCaptureItem>(),put_abi(item)));
    auto size=item.Size();
    auto pool=Direct3D11CaptureFramePool::CreateFreeThreaded(directDevice,DirectXPixelFormat::R16G16B16A16Float,1,size);
    struct CloseCapture {
        Direct3D11CaptureFramePool pool;
        GraphicsCaptureSession session{nullptr};
        ~CloseCapture(){try{if(session)session.Close();}catch(...){}try{pool.Close();}catch(...){}}
    } close{pool};
    auto session=pool.CreateCaptureSession(item);
    close.session=session;
    // Cursor behavior matches the existing desktop screenshot. The OS owns capture indication.
    if(auto s=session.try_as<IGraphicsCaptureSession2>())s.IsCursorCaptureEnabled(false);
    session.StartCapture();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(1500);
    bool received=false;
    while(std::chrono::steady_clock::now()<deadline) {
        auto frame=pool.TryGetNextFrame();
        if(!frame){std::this_thread::sleep_for(std::chrono::milliseconds(5));continue;}
        const auto content=frame.ContentSize();
        if(content.Width<=0 || content.Height<=0){frame.Close();continue;}
        if(content.Width!=size.Width || content.Height!=size.Height) {
            frame.Close();size=content;pool.Recreate(directDevice,DirectXPixelFormat::R16G16B16A16Float,1,size);continue;
        }
        auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        com_ptr<ID3D11Texture2D> texture;check_hresult(access->GetInterface(__uuidof(ID3D11Texture2D),texture.put_void()));
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
        if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || UINT(content.Width)>desc.Width || UINT(content.Height)>desc.Height)throw hresult_error(E_UNEXPECTED,L"Unexpected WGC surface");
        desc.Width=content.Width;desc.Height=content.Height;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        com_ptr<ID3D11Texture2D> staging;check_hresult(device->CreateTexture2D(&desc,nullptr,staging.put()));
        const D3D11_BOX box{0,0,0,desc.Width,desc.Height,1};context->CopySubresourceRegion(staging.get(),0,0,0,0,texture.get(),0,&box);
        D3D11_MAPPED_SUBRESOURCE mapped{};check_hresult(context->Map(staging.get(),0,D3D11_MAP_READ,0,&mapped));
        receiver(static_cast<const uint16_t *>(mapped.pData),desc.Width,desc.Height,mapped.RowPitch,user);
        context->Unmap(staging.get(),0);frame.Close();received=true;break;
    }
    if(!received)throw hresult_error(DXGI_ERROR_WAIT_TIMEOUT,L"Timed out waiting for a frozen WGC frame");
}

extern "C" __declspec(dllexport) HRESULT __cdecl vsrCaptureWgc(HMONITOR monitor,VsrWgcReceiver receiver,void *user,wchar_t *error,uint32_t capacity)
{
    if(!monitor || !receiver)return E_INVALIDARG;
    HRESULT result=S_OK;std::wstring message;
    // Qt/OLE may already own an STA. Keep WinRT and all GPU objects in this short-lived MTA.
    std::thread worker([&]{
        bool initialized=false;
        try {init_apartment(apartment_type::multi_threaded);initialized=true;capture(monitor,receiver,user);}
        catch(const hresult_error &e){result=e.code();message=e.message();}
        catch(...){result=E_FAIL;message=L"WGC capture failed";}
        if(initialized)uninit_apartment();
    });
    worker.join();
    if(error && capacity)wcsncpy_s(error,capacity,message.c_str(),_TRUNCATE);
    return result;
}
