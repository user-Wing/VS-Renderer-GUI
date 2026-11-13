#include "render/CustomShader.h"
#include "render/Anime4KFast.h"
#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
using Microsoft::WRL::ComPtr;
int main(int argc,char**argv){try{
 if(argc<5)return 2;const int count=std::stoi(argv[3]);const unsigned w=argc>5?std::stoul(argv[5]):1920,h=argc>6?std::stoul(argv[6]):1080,ow=argc>7?std::stoul(argv[7]):3840,oh=argc>8?std::stoul(argv[8]):2160;
 ComPtr<IDXGIFactory1> factory;CreateDXGIFactory1(IID_PPV_ARGS(&factory));ComPtr<IDXGIAdapter1> adapter;
 for(UINT i=0;factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;i++){DXGI_ADAPTER_DESC1 d;adapter->GetDesc1(&d);if(d.VendorId==0x1002 && !(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))break;adapter.Reset();}if(!adapter)return 3;
 DXGI_ADAPTER_DESC1 ad{};adapter->GetDesc1(&ad);std::wcerr<<ad.Description<<"\n";
 ComPtr<ID3D11Device> dev;ComPtr<ID3D11DeviceContext> ctx;D3D_FEATURE_LEVEL level;
 if(FAILED(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,&level,&ctx)))return 4;
 D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT minp{};dev->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT,&minp,sizeof(minp));std::cerr<<"pixelMinPrecision="<<minp.PixelShaderMinPrecision<<"\n";
 ComPtr<ID3D12Device> d12;if(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&d12)))){D3D12_FEATURE_DATA_SHADER_MODEL sm{D3D_SHADER_MODEL_6_7};d12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&sm,sizeof(sm));D3D12_FEATURE_DATA_D3D12_OPTIONS4 o4{};d12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4,&o4,sizeof(o4));D3D12_FEATURE_DATA_D3D12_OPTIONS1 o1{};d12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1,&o1,sizeof(o1));std::cerr<<"D12 SM="<<int(sm.HighestShaderModel)<<" native16="<<o4.Native16BitShaderOpsSupported<<" wave="<<o1.WaveOps<<" lanes="<<o1.WaveLaneCountMin<<".."<<o1.WaveLaneCountMax<<"\n";}
 std::vector<uint16_t> bits(size_t(w)*h*4);std::ifstream f(argv[2],std::ios::binary);f.read((char*)bits.data(),bits.size()*2);if(!f)return 5;for(auto &v:bits)v=DirectX::PackedVector::XMConvertFloatToHalf(float(v)/65535.f);
 vsr::shader::Pipeline custom;vsr::anime4k::Pipeline fast;int algo=std::string(argv[1])=="fast"?9:std::string(argv[1])=="fast+"?10:0;
 ID3D11RenderTargetView *rt=nullptr;if(algo)rt=fast.input(dev.Get(),w,h);else{if(!custom.load(argv[1],0,0,dev.Get())){std::cerr<<custom.error();return 6;}rt=custom.input(dev.Get(),w,h);}if(!rt)return 7;
 ComPtr<ID3D11Resource> resource;rt->GetResource(&resource);ctx->UpdateSubresource(resource.Get(),0,nullptr,bits.data(),w*8,0);
 auto render=[&](){bool ok=algo?fast.process(ctx.Get(),ow,oh,algo):custom.process(ctx.Get(),ow,oh);if(!ok)throw std::runtime_error(custom.error());};
 ComPtr<ID3D11Query> begin,end,disjoint;D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP,0};dev->CreateQuery(&q,&begin);dev->CreateQuery(&q,&end);q.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;dev->CreateQuery(&q,&disjoint);
 auto wait=[&](auto* query,auto &out){while(ctx->GetData(query,&out,sizeof(out),0)==S_FALSE)Sleep(0);};std::vector<double> times,cpu;
 for(int i=(count<5?-1:-8);i<count;i++){ctx->Begin(disjoint.Get());ctx->End(begin.Get());auto t=std::chrono::steady_clock::now();render();auto u=std::chrono::steady_clock::now();ctx->End(end.Get());ctx->End(disjoint.Get());ctx->Flush();D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{};wait(disjoint.Get(),d);UINT64 a,b;wait(begin.Get(),a);wait(end.Get(),b);if(i>=0&&!d.Disjoint){times.push_back(double(b-a)*1000/d.Frequency);cpu.push_back(std::chrono::duration<double,std::milli>(u-t).count());}}
 auto* srv=algo?fast.output():custom.output();srv->GetResource(&resource);D3D11_TEXTURE2D_DESC desc{};desc.Width=ow;desc.Height=oh;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> readback;if(FAILED(dev->CreateTexture2D(&desc,nullptr,&readback)))return 8;ctx->CopyResource(readback.Get(),resource.Get());D3D11_MAPPED_SUBRESOURCE map{};if(FAILED(ctx->Map(readback.Get(),0,D3D11_MAP_READ,0,&map)))return 9;std::ofstream out(argv[4],std::ios::binary);for(unsigned y=0;y<oh;y++)out.write((char*)map.pData+y*map.RowPitch,ow*8);ctx->Unmap(readback.Get(),0);
 auto sorted=times;std::sort(sorted.begin(),sorted.end());double avg=0;for(auto v:times)avg+=v;avg/=times.size();double cavg=0;for(auto v:cpu)cavg+=v;cavg/=cpu.size();std::cout<<std::setprecision(8)<<"{\"api\":\"d3d11\",\"n\":"<<times.size()<<",\"gpuMedianMs\":"<<sorted[sorted.size()/2]<<",\"gpuMeanMs\":"<<avg<<",\"gpuP95Ms\":"<<sorted[size_t(sorted.size()*.95)]<<",\"cpuSubmitMeanMs\":"<<cavg<<",\"gpuOnlyFps\":"<<1000/avg<<",\"samplesMs\":[";for(size_t i=0;i<times.size();i++)std::cout<<(i?",":"")<<times[i];std::cout<<"]}\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what();return 10;}}
