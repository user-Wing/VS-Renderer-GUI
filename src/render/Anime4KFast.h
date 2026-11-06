#pragma once
// D3D11 execution of the bundled Anime4K presets. Shader weights/operations are
// generated from the original GLSL; this class only schedules its mpv hooks.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vsr::anime4k {
using Microsoft::WRL::ComPtr;
struct Anime4KPass {
    const unsigned char* bytes; size_t size;
    const char *hook, *save, *width, *height, *when;
    const char* bindings[8]; unsigned bindingCount, program;
};
#include "Anime4KFastShaders.h"

class Pipeline {
    struct Surface {
        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11RenderTargetView> target;
        ComPtr<ID3D11ShaderResourceView> view;
        unsigned width=0, height=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    };
    static constexpr unsigned passCount=unsigned(std::size(anime4kPasses));
    std::array<Surface,passCount+3> surfaces_;
    std::array<ComPtr<ID3D11PixelShader>,passCount> shaders_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11PixelShader> copy_;
    ComPtr<ID3D11PixelShader> extractLuma_,mergeLuma_;
    ComPtr<ID3D11Buffer> lumaConstants_;
    ComPtr<ID3D11SamplerState> sampler_;
    ID3D11Device* device_=nullptr;
    Surface* processed_=nullptr;

    bool surface(ID3D11Device* device, Surface& s, unsigned w, unsigned h,DXGI_FORMAT format=DXGI_FORMAT_R16G16B16A16_FLOAT) {
        if(s.texture && s.width==w && s.height==h && s.format==format)return true;
        s={};
        D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=1;
        d.Format=format;d.SampleDesc.Count=1;
        d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        if(FAILED(device->CreateTexture2D(&d,nullptr,&s.texture)) ||
           FAILED(device->CreateRenderTargetView(s.texture.Get(),nullptr,&s.target)) ||
           FAILED(device->CreateShaderResourceView(s.texture.Get(),nullptr,&s.view)))return false;
        s.width=w;s.height=h;s.format=format;return true;
    }
    bool initialize(ID3D11Device* device) {
        if(device_==device && vertex_)return true;
        reset();
        constexpr char source[]=R"(
struct V {float2 uv:TEXCOORD0;float4 p:SV_Position;};
V vs(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
Texture2D<float4> tex:register(t0);SamplerState smp:register(s0);
Texture2D<float4> luma:register(t1);
cbuffer LumaSettings:register(b0){float4 Coefficients;float4 Range;};
float4 ps(V v):SV_Target{return tex.SampleLevel(smp,v.uv,0);}
float4 extract(V v):SV_Target{return float4(dot(tex.SampleLevel(smp,v.uv,0).rgb,Coefficients.rgb)*Range.x+Coefficients.w,0,0,1);}
float4 merge(V v):SV_Target{float4 p=tex.SampleLevel(smp,v.uv,0);float oldY=dot(p.rgb,Coefficients.rgb)*Range.x+Coefficients.w;p.rgb+=(luma.SampleLevel(smp,v.uv,0).r-oldY)/Range.x;return p;}
)";
        ComPtr<ID3DBlob> vs,ps;
        if(FAILED(D3DCompile(source,sizeof(source),nullptr,nullptr,nullptr,"vs","vs_5_0",0,0,&vs,nullptr)) ||
           FAILED(D3DCompile(source,sizeof(source),nullptr,nullptr,nullptr,"ps","ps_5_0",0,0,&ps,nullptr)) ||
           FAILED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_)) ||
           FAILED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&copy_))) {reset();return false;}
        const auto compile=[&](const char* entry,ComPtr<ID3D11PixelShader>& shader){
            ComPtr<ID3DBlob> blob;
            return SUCCEEDED(D3DCompile(source,sizeof(source),nullptr,nullptr,nullptr,entry,"ps_5_0",0,0,&blob,nullptr)) &&
                SUCCEEDED(device->CreatePixelShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,shader.GetAddressOf()));
        };
        if(!compile("extract",extractLuma_) || !compile("merge",mergeLuma_)){reset();return false;}
        D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=32;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(device->CreateBuffer(&buffer,nullptr,&lumaConstants_))){reset();return false;}
        D3D11_SAMPLER_DESC d{};d.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        d.AddressU=d.AddressV=d.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;d.MaxLOD=D3D11_FLOAT32_MAX;
        if(FAILED(device->CreateSamplerState(&d,&sampler_))){reset();return false;}
        for(unsigned i=0;i<shaders_.size();++i)
            if(FAILED(device->CreatePixelShader(anime4kPasses[i].bytes,anime4kPasses[i].size,nullptr,&shaders_[i]))){reset();return false;}
        device_=device;return true;
    }
    void draw(ID3D11DeviceContext* c, ID3D11RenderTargetView* target,
              ID3D11PixelShader* shader, ID3D11ShaderResourceView** views,
              unsigned count, float x,float y,float w,float h) {
        c->OMSetRenderTargets(1,&target,nullptr);
        c->OMSetBlendState(nullptr,nullptr,0xffffffff);
        D3D11_VIEWPORT viewport{x,y,w,h,0,1};c->RSSetViewports(1,&viewport);
        c->IASetInputLayout(nullptr);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(vertex_.Get(),nullptr,0);c->PSSetShader(shader,nullptr,0);
        ID3D11SamplerState* samplers[8];for(auto& s:samplers)s=sampler_.Get();
        c->PSSetSamplers(0,8,samplers);c->PSSetShaderResources(0,count,views);c->Draw(3,0);
        ID3D11ShaderResourceView* empty[8]{};c->PSSetShaderResources(0,8,empty);
        c->OMSetRenderTargets(0,nullptr,nullptr);
    }
public:
    void reset(){surfaces_={};shaders_={};vertex_.Reset();copy_.Reset();extractLuma_.Reset();mergeLuma_.Reset();lumaConstants_.Reset();sampler_.Reset();device_=nullptr;processed_=nullptr;}
    // Native colour conversion renders once at source resolution into this target.
    ID3D11RenderTargetView* input(ID3D11Device* device,unsigned w,unsigned h) {
        return initialize(device) && surface(device,surfaces_[passCount],w,h)?surfaces_[passCount].target.Get():nullptr;
    }
    bool process(ID3D11DeviceContext* c,unsigned w,unsigned h,unsigned algorithm=9,
                 float kr=0.2126f,float kb=0.0722f,float yOffset=16.0f/255,float yScale=219.0f/255) {
        if(!device_ || !surfaces_[passCount].view || !w || !h || algorithm<9 || algorithm>14)return false;
        auto& input=surfaces_[passCount];
        if(algorithm>=13){w=input.width;h=input.height;}
        const bool enhanced=algorithm==10 || algorithm==11;
        const unsigned program=algorithm==11 || algorithm==12?1:0;
        std::unordered_map<std::string,Surface*> saved{{"MAIN",&input}, {"NATIVE",&input}};
        if(enhanced){
            auto& luma=surfaces_[passCount+1];if(!surface(device_,luma,input.width,input.height,DXGI_FORMAT_R16_FLOAT))return false;
            const float coefficients[]{kr,1-kr-kb,kb,yOffset,yScale,0,0,0};
            c->UpdateSubresource(lumaConstants_.Get(),0,nullptr,coefficients,0,0);
            auto* buffer=lumaConstants_.Get();c->PSSetConstantBuffers(0,1,&buffer);
            auto* view=input.view.Get();draw(c,luma.target.Get(),extractLuma_.Get(),&view,1,0,0,float(input.width),float(input.height));
            saved["LUMA"]=&luma;
        }
        // The bundle uses mpv's reverse-Polish size/condition expressions.
        const auto eval=[&](const char* expression) {
            std::istringstream in(expression);std::string token;std::vector<float> stack;
            while(in>>token){
                if(token=="*" || token=="/" || token==">" || token=="<"){
                    if(stack.size()<2)return 0.0f;
                    float b=stack.back();stack.pop_back();float& a=stack.back();
                    if(token=="*")a*=b;else if(token=="/")a/=b;else if(token==">")a=a>b;else a=a<b;
                }else if(auto dot=token.find('.');dot!=std::string::npos && (token.back()=='w' || token.back()=='h')){
                    auto name=token.substr(0,dot);bool width=token.back()=='w';
                    if(name=="OUTPUT")stack.push_back(float(width?w:h));
                    else {auto it=saved.find(name);if(it==saved.end())return 0.0f;stack.push_back(float(width?it->second->width:it->second->height));}
                }else stack.push_back(std::strtof(token.c_str(),nullptr));
            }
            return stack.size()==1?stack[0]:0.0f;
        };
        for(const char* hook:{"LUMA","MAIN","PREKERNEL"}){
          if(std::string(hook)=="LUMA" && !enhanced)continue;
          for(unsigned i=0;i<shaders_.size();++i){
            const auto& p=anime4kPasses[i];if(p.program!=(std::string(hook)=="LUMA"?2:program) || std::string(p.hook)!=hook || !eval(p.when))continue;
            saved["HOOKED"]=saved[std::string(hook)=="LUMA"?"LUMA":"MAIN"];
            const auto width=unsigned(std::lround(eval(p.width))),height=unsigned(std::lround(eval(p.height)));
            if(!width || !height || width>16384 || height>16384 || !surface(device_,surfaces_[i],width,height,std::string(p.hook)=="LUMA" && std::string(p.save)=="LUMA"?DXGI_FORMAT_R16_FLOAT:DXGI_FORMAT_R16G16B16A16_FLOAT))return false;
            ID3D11ShaderResourceView* views[8]{};
            for(unsigned b=0;b<p.bindingCount;++b){auto it=saved.find(p.bindings[b]);if(it==saved.end())return false;views[b]=it->second->view.Get();}
            draw(c,surfaces_[i].target.Get(),shaders_[i].Get(),views,p.bindingCount,0,0,float(width),float(height));
            saved[p.save]=&surfaces_[i];
          }
          if(std::string(hook)=="LUMA"){
            auto& rgb=surfaces_[passCount+2];if(!surface(device_,rgb,input.width,input.height))return false;
            ID3D11ShaderResourceView* views[]{input.view.Get(),saved["LUMA"]->view.Get()};
            draw(c,rgb.target.Get(),mergeLuma_.Get(),views,2,0,0,float(input.width),float(input.height));saved["MAIN"]=&rgb;
          }
        }
        processed_=saved["MAIN"];
        return true;
    }
    ID3D11ShaderResourceView* output() const{return processed_?processed_->view.Get():nullptr;}
    unsigned width() const{return processed_?processed_->width:0;}
    unsigned height() const{return processed_?processed_->height:0;}
    bool render(ID3D11DeviceContext* c,ID3D11RenderTargetView* target,float x,float y,unsigned w,unsigned h,unsigned algorithm=9){
        if(!process(c,w,h,algorithm))return false;
        auto* view=output();draw(c,target,copy_.Get(),&view,1,x,y,float(w),float(h));return true;
    }
};
}
