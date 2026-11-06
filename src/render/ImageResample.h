#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstring>
namespace vsr::image {
inline bool resample(const float* source,unsigned width,unsigned height,unsigned stride,float* output,unsigned outWidth,unsigned outHeight,unsigned outStride,unsigned algorithm){
    using Microsoft::WRL::ComPtr;
    if(!source || !output || !width || !height || !outWidth || !outHeight || width>16384 || height>16384 || outWidth>16384 || outHeight>16384 || stride<width*16 || outStride<outWidth*16 || algorithm>4)return false;
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)) && FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)))return false;
    constexpr char shader[]=R"(
Texture2D<float4> image:register(t0);SamplerState smp:register(s0);
cbuffer C:register(b0){float2 inputSize;float2 outputSize;uint algorithm;float3 padding;};
struct V{float4 p:SV_Position;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float sinc(float x){return abs(x)<1e-5?1:sin(3.14159265359*x)/(3.14159265359*x);}
float jinc(float x){x*=3.14159265359;if(abs(x)<1e-5)return 1;float term=x*.5,sum=term;[unroll]for(int k=1;k<=16;++k){term*=(-x*x*.25)/(k*(k+1));sum+=term;}return 2*sum/x;}
float4 ps(V v):SV_Target{
 if(algorithm==0)return image.Load(int3(clamp(int2(v.uv*inputSize),int2(0,0),int2(inputSize)-1),0));
 if(algorithm==1)return image.SampleLevel(smp,v.uv,0);
 float2 center=v.uv*inputSize-.5,scale=max(inputSize/outputSize,1);float radius=algorithm==2?2:algorithm==3?3:4;
 float4 sum=0;float weightSum=0;int2 first=int2(ceil(center-radius*scale)),last=int2(floor(center+radius*scale));
 for(int y=first.y;y<=last.y;++y)for(int x=first.x;x<=last.x;++x){float2 d=(float2(x,y)-center)/scale;float r=length(d);float w=algorithm==2?(r<radius?jinc(r)*jinc(r/radius):0):(abs(d.x)<radius && abs(d.y)<radius?sinc(d.x)*sinc(d.x/radius)*sinc(d.y)*sinc(d.y/radius):0);sum+=w*image.Load(int3(clamp(int2(x,y),int2(0,0),int2(inputSize)-1),0));weightSum+=w;}
 return sum/max(weightSum,1e-7);
})";
    ComPtr<ID3DBlob> vsCode,psCode;ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;
    if(FAILED(D3DCompile(shader,sizeof(shader),nullptr,nullptr,nullptr,"vs","vs_5_0",0,0,&vsCode,nullptr)) || FAILED(D3DCompile(shader,sizeof(shader),nullptr,nullptr,nullptr,"ps","ps_5_0",0,0,&psCode,nullptr)) || FAILED(device->CreateVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),nullptr,&vs)) || FAILED(device->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps)))return false;
    D3D11_TEXTURE2D_DESC description{};description.Width=width;description.Height=height;description.MipLevels=description.ArraySize=1;description.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;description.SampleDesc.Count=1;description.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{source,stride,0};ComPtr<ID3D11Texture2D> current;ComPtr<ID3D11ShaderResourceView> view;
    if(FAILED(device->CreateTexture2D(&description,&initial,&current)) || FAILED(device->CreateShaderResourceView(current.Get(),nullptr,&view)))return false;
    D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=32;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ComPtr<ID3D11Buffer> constants;if(FAILED(device->CreateBuffer(&buffer,nullptr,&constants)))return false;
    D3D11_SAMPLER_DESC sampling{};sampling.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sampling.AddressU=sampling.AddressV=sampling.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampling.MaxLOD=D3D11_FLOAT32_MAX;ComPtr<ID3D11SamplerState> sampler;if(FAILED(device->CreateSamplerState(&sampling,&sampler)))return false;
    do {
        // Bounded reduction stages avoid a single shader invocation with an enormous footprint.
        const unsigned nextWidth=width>outWidth*4?(width+1)/2:outWidth,nextHeight=height>outHeight*4?(height+1)/2:outHeight;
        description.Width=nextWidth;description.Height=nextHeight;description.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rt;ComPtr<ID3D11ShaderResourceView> nextView;
        if(FAILED(device->CreateTexture2D(&description,nullptr,&target)) || FAILED(device->CreateRenderTargetView(target.Get(),nullptr,&rt)) || FAILED(device->CreateShaderResourceView(target.Get(),nullptr,&nextView)))return false;
        struct Settings{float input[2],output[2];unsigned algorithm,padding[3];} settings{{float(width),float(height)},{float(nextWidth),float(nextHeight)},algorithm,{0,0,0}};
        context->UpdateSubresource(constants.Get(),0,nullptr,&settings,0,0);auto *cb=constants.Get();auto *targetView=rt.Get();auto *sourceView=view.Get();auto *sample=sampler.Get();context->OMSetRenderTargets(1,&targetView,nullptr);const D3D11_VIEWPORT viewport{0,0,float(nextWidth),float(nextHeight),0,1};context->RSSetViewports(1,&viewport);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);context->PSSetConstantBuffers(0,1,&cb);context->PSSetShaderResources(0,1,&sourceView);context->PSSetSamplers(0,1,&sample);context->Draw(3,0);ID3D11ShaderResourceView *empty=nullptr;context->PSSetShaderResources(0,1,&empty);context->OMSetRenderTargets(0,nullptr,nullptr);
        current=target;view=nextView;width=nextWidth;height=nextHeight;
    }while(width!=outWidth || height!=outHeight);
    description.BindFlags=0;description.Usage=D3D11_USAGE_STAGING;description.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;if(FAILED(device->CreateTexture2D(&description,nullptr,&staging)))return false;context->CopyResource(staging.Get(),current.Get());D3D11_MAPPED_SUBRESOURCE mapped{};if(FAILED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)))return false;
    for(unsigned y=0;y<outHeight;++y)std::memcpy(reinterpret_cast<char*>(output)+y*outStride,static_cast<const char*>(mapped.pData)+y*mapped.RowPitch,outWidth*16);context->Unmap(staging.Get(),0);return true;
}
}
