// Standalone MSVC GPU probe: input is RGBA float16, output RGBA float32.
#include "render/Anime4KFast.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=7 && argc!=8)return 2;
    const unsigned w=std::stoul(argv[1]),h=std::stoul(argv[2]),ow=std::stoul(argv[3]),oh=std::stoul(argv[4]);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)))return 3;
    vsr::anime4k::Pipeline pipeline;auto* input=pipeline.input(device.Get(),w,h);if(!input)return 4;
    std::vector<char> pixels(size_t(w)*h*8);std::ifstream source(argv[5],std::ios::binary);source.read(pixels.data(),pixels.size());if(!source)return 5;
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;input->GetResource(&resource);
    context->UpdateSubresource(resource.Get(),0,nullptr,pixels.data(),w*8,0);
    D3D11_TEXTURE2D_DESC d{};d.Width=ow;d.Height=oh;d.MipLevels=d.ArraySize=1;
    d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_RENDER_TARGET;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> output,readback;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
    if(FAILED(device->CreateTexture2D(&d,nullptr,&output)) || FAILED(device->CreateRenderTargetView(output.Get(),nullptr,&target)))return 6;
    if(!pipeline.render(context.Get(),target.Get(),0,0,ow,oh,argc==8?std::stoul(argv[7]):9))return 7;
    d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if(FAILED(device->CreateTexture2D(&d,nullptr,&readback)))return 8;
    context->CopyResource(readback.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
    if(FAILED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)))return 9;
    std::ofstream result(argv[6],std::ios::binary);
    for(unsigned y=0;y<oh;++y)result.write(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch,ow*16);
    context->Unmap(readback.Get(),0);
    std::cout<<"Anime4K Fast D3D11 "<<w<<"x"<<h<<" -> "<<ow<<"x"<<oh<<"\n";
    return result?0:10;
}
