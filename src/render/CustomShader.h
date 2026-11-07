#pragma once
// Cache format shared with ShaderPackage.cpp. This runtime needs no converter or Python.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vsr::shader {
using Microsoft::WRL::ComPtr;
class Pipeline {
    struct Pass {std::string hook,save,width,height,when;std::vector<std::string> bindings;std::vector<char> bytecode;};
    struct Surface {
        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11RenderTargetView> target;
        ComPtr<ID3D11ShaderResourceView> view;
        unsigned width=0,height=0;
    };
    std::vector<Pass> passes_;
    std::vector<Surface> surfaces_;
    std::vector<ComPtr<ID3D11PixelShader>> shaders_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11SamplerState> sampler_;
    ComPtr<ID3D11Buffer> constants_,lumaConstants_;
    ComPtr<ID3D11PixelShader> extractLuma_,mergeLuma_;
    ID3D11Device* device_=nullptr;
    Surface* processed_=nullptr;
    unsigned outputWidth_=0,outputHeight_=0;
    std::string error_;

    bool surface(Surface& s,unsigned width,unsigned height) {
        if(s.texture && s.width==width && s.height==height)return true;
        s={};D3D11_TEXTURE2D_DESC d{};d.Width=width;d.Height=height;d.MipLevels=d.ArraySize=1;
        d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        if(FAILED(device_->CreateTexture2D(&d,nullptr,&s.texture)) || FAILED(device_->CreateRenderTargetView(s.texture.Get(),nullptr,&s.target)) ||
           FAILED(device_->CreateShaderResourceView(s.texture.Get(),nullptr,&s.view))){error_="Cannot allocate custom shader FP16 texture.";return false;}
        s.width=width;s.height=height;return true;
    }
    bool initialize(ID3D11Device* device) {
        if(device_==device && vertex_)return true;
        reset();if(!device || passes_.empty())return false;
        constexpr char source[]=R"(struct V{float2 uv:TEXCOORD0;float4 p:SV_Position;};
V main(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;})";
        ComPtr<ID3DBlob> code;
        if(FAILED(D3DCompile(source,sizeof(source),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&code,nullptr)) ||
           FAILED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vertex_))){error_="Cannot create custom shader vertex stage.";return false;}
        constexpr char lumaSource[]=R"(Texture2D<float4> tex:register(t0);Texture2D<float4> luma:register(t1);SamplerState smp:register(s0);
cbuffer C:register(b0){float4 coefficients;float4 range;};
float4 extract(float4 p:SV_Position,float2 uv:TEXCOORD0):SV_Target{return float4(dot(tex.SampleLevel(smp,uv,0).rgb,coefficients.rgb)*range.x+coefficients.w,0,0,1);}
float4 merge(float4 p:SV_Position,float2 uv:TEXCOORD0):SV_Target{float4 c=tex.SampleLevel(smp,uv,0);float oldY=dot(c.rgb,coefficients.rgb)*range.x+coefficients.w;c.rgb+=(luma.SampleLevel(smp,uv,0).r-oldY)/range.x;return c;})";
        const auto compile=[&](const char* entry,ComPtr<ID3D11PixelShader>& shader){ComPtr<ID3DBlob> blob;return SUCCEEDED(D3DCompile(lumaSource,sizeof(lumaSource),nullptr,nullptr,nullptr,entry,"ps_5_0",0,0,&blob,nullptr)) && SUCCEEDED(device->CreatePixelShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&shader));};
        D3D11_BUFFER_DESC lb{};lb.ByteWidth=32;lb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(!compile("extract",extractLuma_) || !compile("merge",mergeLuma_) || FAILED(device->CreateBuffer(&lb,nullptr,&lumaConstants_))){error_="Cannot create luma stages.";reset();return false;}
        D3D11_SAMPLER_DESC s{};s.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;s.MaxLOD=D3D11_FLOAT32_MAX;
        D3D11_BUFFER_DESC b{};b.ByteWidth=32;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(device->CreateSamplerState(&s,&sampler_)) || FAILED(device->CreateBuffer(&b,nullptr,&constants_))){error_="Cannot create custom shader constants/sampler.";return false;}
        shaders_.resize(passes_.size());surfaces_.resize(passes_.size()+3);
        for(size_t i=0;i<passes_.size();++i)if(FAILED(device->CreatePixelShader(passes_[i].bytecode.data(),passes_[i].bytecode.size(),nullptr,&shaders_[i]))){error_="Invalid or unsupported custom pixel shader bytecode.";reset();return false;}
        device_=device;return true;
    }
    bool eval(const std::string &expression,const std::unordered_map<std::string,Surface*> &saved,unsigned outputWidth,unsigned outputHeight,float &value) {
        std::istringstream in(expression);std::string token;std::vector<float> stack;
        while(in>>token) {
            if(token=="!"){if(stack.empty())return false;stack.back()=stack.back()==0;}
            else if(token=="+" || token=="-" || token=="*" || token=="/" || token==">" || token=="<" || token=="=") {
                if(stack.size()<2)return false;const float b=stack.back();stack.pop_back();float& a=stack.back();
                if(token=="+")a+=b;else if(token=="-")a-=b;else if(token=="*")a*=b;else if(token=="/"){if(b==0)return false;a/=b;}else if(token==">")a=a>b;else if(token=="<")a=a<b;else a=a==b;
            } else {
                const auto dot=token.find('.');const auto suffix=dot==std::string::npos?std::string():token.substr(dot+1);
                if(suffix=="w" || suffix=="h" || suffix=="width" || suffix=="height"){
                    const auto name=token.substr(0,dot);const bool width=suffix=="w" || suffix=="width";
                    if(name=="OUTPUT")stack.push_back(float(width?outputWidth:outputHeight));
                    else if(name=="NATIVE")stack.push_back(float(width?surfaces_.back().width:surfaces_.back().height));
                    else {const auto it=saved.find(name);if(it==saved.end() || !it->second)return false;stack.push_back(float(width?it->second->width:it->second->height));}
                } else {char* end=nullptr;const float number=std::strtof(token.c_str(),&end);if(end==token.c_str() || *end || !std::isfinite(number))return false;stack.push_back(number);}
            }
            if(!stack.empty() && !std::isfinite(stack.back()))return false;
        }
        if(stack.size()!=1)return false;value=stack.back();return true;
    }
public:
    bool active() const{return !passes_.empty();}
    const std::string& error() const{return error_;}
    void reset(){surfaces_.clear();shaders_.clear();vertex_.Reset();sampler_.Reset();constants_.Reset();lumaConstants_.Reset();extractLuma_.Reset();mergeLuma_.Reset();device_=nullptr;processed_=nullptr;}
    void clear(){reset();passes_.clear();outputWidth_=outputHeight_=0;error_.clear();}
    // Parse/instantiate in a temporary pipeline so a failed replacement preserves the old one.
    bool load(const char* path,unsigned width,unsigned height,ID3D11Device* device) {
        if(!path || !*path){clear();return true;}
        if(width>16384 || height>16384 || bool(width)!=bool(height)){error_="Invalid custom shader output size.";return false;}
        const std::u8string utf8(path,path+std::strlen(path));
        std::ifstream file(std::filesystem::path(utf8),std::ios::binary|std::ios::ate);
        const auto size=file?file.tellg():std::streampos(-1);
        if(size<12 || size>64*1024*1024){error_="Cannot read custom shader package.";return false;}
        std::vector<char> bytes(static_cast<size_t>(size));file.seekg(0);if(!file.read(bytes.data(),std::streamsize(bytes.size()))){error_="Cannot read custom shader package.";return false;}
        size_t at=8;
        const auto integer=[&](uint32_t &n){if(at+4>bytes.size())return false;const auto p=reinterpret_cast<const unsigned char*>(bytes.data()+at);n=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);at+=4;return true;};
        const auto text=[&](std::string& out){uint32_t n=0;if(!integer(n) || n>4096 || at+n>bytes.size())return false;out.assign(bytes.data()+at,n);at+=n;return out.find('\0')==std::string::npos;};
        uint32_t count=0;Pipeline next;bool valid=std::memcmp(bytes.data(),"VSRHLSL1",8)==0 && integer(count) && count>0 && count<=128;
        for(uint32_t i=0;valid && i<count;++i){Pass p;uint32_t bindings=0,length=0;
            valid=text(p.hook) && (p.hook=="LUMA" || p.hook=="MAIN" || p.hook=="PREKERNEL") && text(p.save) && text(p.width) && text(p.height) && text(p.when) && integer(bindings) && bindings<=D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
            for(uint32_t j=0;valid && j<bindings;++j){std::string name;valid=text(name);p.bindings.push_back(name);}
            valid=valid && integer(length) && length>=4 && length<=16*1024*1024 && at+length<=bytes.size();
            if(valid){p.bytecode.assign(bytes.data()+at,bytes.data()+at+length);valid=std::memcmp(p.bytecode.data(),"DXBC",4)==0;at+=length;next.passes_.push_back(std::move(p));}
        }
        if(!valid || at!=bytes.size()){error_="Invalid custom shader package format.";return false;}
        next.outputWidth_=width;next.outputHeight_=height;
        if(device && !next.initialize(device)){error_=next.error_;return false;}
        *this=std::move(next);return true;
    }
    ID3D11RenderTargetView* input(ID3D11Device* device,unsigned width,unsigned height) {
        return initialize(device) && width && height && width<=16384 && height<=16384 && surface(surfaces_.back(),width,height)?surfaces_.back().target.Get():nullptr;
    }
    bool process(ID3D11DeviceContext* context,unsigned targetWidth,unsigned targetHeight,float kr=.2126f,float kb=.0722f,float yOffset=16.f/255,float yScale=219.f/255) {
        if(!device_ || surfaces_.empty() || !surfaces_.back().view)return false;
        if(outputWidth_){targetWidth=outputWidth_;targetHeight=outputHeight_;}
        auto& input=surfaces_.back();std::unordered_map<std::string,Surface*> saved{{"MAIN",&input}};
        const float coefficients[]{kr,1-kr-kb,kb,yOffset,yScale,0,0,0};
        context->UpdateSubresource(lumaConstants_.Get(),0,nullptr,coefficients,0,0);
        const auto lumaDraw=[&](Surface &target,ID3D11PixelShader *shader,ID3D11ShaderResourceView *a,ID3D11ShaderResourceView *b){
            auto *rt=target.target.Get();context->OMSetRenderTargets(1,&rt,nullptr);context->OMSetBlendState(nullptr,nullptr,0xffffffff);
            const D3D11_VIEWPORT viewport{0,0,float(target.width),float(target.height),0,1};context->RSSetViewports(1,&viewport);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->VSSetShader(vertex_.Get(),nullptr,0);context->PSSetShader(shader,nullptr,0);auto *cb=lumaConstants_.Get();context->PSSetConstantBuffers(0,1,&cb);
            auto *sampler=sampler_.Get();context->PSSetSamplers(0,1,&sampler);ID3D11ShaderResourceView *views[]{a,b};context->PSSetShaderResources(0,2,views);context->Draw(3,0);ID3D11ShaderResourceView *empty[2]{};context->PSSetShaderResources(0,2,empty);context->OMSetRenderTargets(0,nullptr,nullptr);
        };
        bool lumaActive=false,prekernel=false;
        const auto merge=[&](){if(!lumaActive)return true;auto *luma=saved["LUMA"];auto &rgb=surfaces_[passes_.size()+1];if(!surface(rgb,luma->width,luma->height))return false;lumaDraw(rgb,mergeLuma_.Get(),input.view.Get(),luma->view.Get());saved["MAIN"]=&rgb;lumaActive=false;return true;};

        for(size_t i=0;i<passes_.size();++i){const auto &p=passes_[i];
            if(p.hook!="LUMA" && !merge())return false;
            if(p.hook=="LUMA" && !lumaActive){auto &luma=surfaces_[passes_.size()];if(!surface(luma,input.width,input.height))return false;lumaDraw(luma,extractLuma_.Get(),input.view.Get(),nullptr);saved["LUMA"]=&luma;lumaActive=true;}
            if(p.hook=="PREKERNEL" && !prekernel){saved["PREKERNEL"]=saved["MAIN"];prekernel=true;}
            saved["HOOKED"]=saved[p.hook];
            float condition=0,fw=0,fh=0;
            if(!eval(p.when,saved,targetWidth,targetHeight,condition)){error_="Cannot evaluate custom shader WHEN.";return false;}
            if(condition==0)continue;
            if(!eval(p.width,saved,targetWidth,targetHeight,fw) || !eval(p.height,saved,targetWidth,targetHeight,fh) || fw<1 || fh<1 || fw>16384 || fh>16384){error_="Invalid custom shader output dimensions.";return false;}
            const unsigned width=unsigned(std::lround(fw)),height=unsigned(std::lround(fh));
            if(!surface(surfaces_[i],width,height))return false;
            ID3D11ShaderResourceView* views[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]{};
            for(size_t b=0;b<p.bindings.size();++b){const auto it=saved.find(p.bindings[b]);if(it==saved.end()){error_="Unavailable custom shader texture: "+p.bindings[b];return false;}views[b]=it->second->view.Get();}
            auto* target=surfaces_[i].target.Get();context->OMSetRenderTargets(1,&target,nullptr);context->OMSetBlendState(nullptr,nullptr,0xffffffff);
            const D3D11_VIEWPORT viewport{0,0,float(width),float(height),0,1};context->RSSetViewports(1,&viewport);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->VSSetShader(vertex_.Get(),nullptr,0);context->PSSetShader(shaders_[i].Get(),nullptr,0);
            const auto* hooked=saved["HOOKED"];const float constants[]{float(hooked->width),float(hooked->height),1.0f/hooked->width,1.0f/hooked->height,float(width),float(height),1.0f/width,1.0f/height};
            context->UpdateSubresource(constants_.Get(),0,nullptr,constants,0,0);auto* cb=constants_.Get();context->PSSetConstantBuffers(0,1,&cb);
            ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];for(auto& sampler:samplers)sampler=sampler_.Get();context->PSSetSamplers(0,D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,samplers);context->PSSetShaderResources(0,D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,views);context->Draw(3,0);
            ID3D11ShaderResourceView* empty[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]{};context->PSSetShaderResources(0,D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,empty);context->OMSetRenderTargets(0,nullptr,nullptr);saved[p.save]=&surfaces_[i];
        }
        if(!merge())return false;
        processed_=saved[prekernel?"PREKERNEL":"MAIN"];error_.clear();return true;
    }
    ID3D11ShaderResourceView* output() const{return processed_?processed_->view.Get():nullptr;}
    unsigned width() const{return processed_?processed_->width:0;}
    unsigned height() const{return processed_?processed_->height:0;}
};
}
