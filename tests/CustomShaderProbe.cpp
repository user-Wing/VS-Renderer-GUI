// Headless D3D11 probe: package, input W/H, output W/H, RGBA16 input/output.
#include "render/CustomShader.h"
#include <iostream>
int main(int argc,char **argv) {
    if(argc!=8)return 2;
    const unsigned w=std::stoul(argv[2]),h=std::stoul(argv[3]),ow=std::stoul(argv[4]),oh=std::stoul(argv[5]);
    Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)))return 3;
    vsr::shader::Pipeline pipeline;
    if(!pipeline.load(argv[1],0,0,device.Get())){std::cerr<<pipeline.error();return 4;}
    auto *input=pipeline.input(device.Get(),w,h);if(!input)return 5;
    std::vector<char> pixels(size_t(w)*h*8);std::ifstream source(argv[6],std::ios::binary);source.read(pixels.data(),pixels.size());if(!source)return 6;
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;input->GetResource(&resource);context->UpdateSubresource(resource.Get(),0,nullptr,pixels.data(),w*8,0);
    if(!pipeline.process(context.Get(),ow,oh)){std::cerr<<pipeline.error();return 7;}
    pipeline.output()->GetResource(&resource);
    D3D11_TEXTURE2D_DESC desc{};desc.Width=pipeline.width();desc.Height=pipeline.height();desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> readback;if(FAILED(device->CreateTexture2D(&desc,nullptr,&readback)))return 8;
    context->CopyResource(readback.Get(),resource.Get());D3D11_MAPPED_SUBRESOURCE mapped{};if(FAILED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)))return 9;
    std::ofstream result(argv[7],std::ios::binary);
    for(unsigned y=0;y<desc.Height;++y)result.write(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch,desc.Width*8);
    context->Unmap(readback.Get(),0);std::cout<<desc.Width<<" "<<desc.Height<<"\n";return result?0:10;
}
