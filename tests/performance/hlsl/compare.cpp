#include <DirectXPackedVector.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdint>
std::vector<uint16_t> read(const char*p){std::ifstream f(p,std::ios::binary|std::ios::ate);std::vector<uint16_t> v(size_t(f.tellg())/2);f.seekg(0);f.read((char*)v.data(),v.size()*2);return v;}
int main(int argc,char**argv){if(argc<3)return 2;auto a=read(argv[1]),b=read(argv[2]);if(a.size()!=b.size())return 3;double mx=0,mae=0,mse=0,sum=0,sq=0;size_t different=0,bad=0,n=0,nonfinite=0;float lo=1e9,hi=-1e9;for(size_t i=0;i<a.size();i++){float x=DirectX::PackedVector::XMConvertHalfToFloat(a[i]),y=DirectX::PackedVector::XMConvertHalfToFloat(b[i]);if(!std::isfinite(x)||!std::isfinite(y)){nonfinite++;continue;}double e=std::abs(x-y);mx=std::max(mx,e);mae+=e;mse+=e*e;sum+=y;sq+=y*y;different+=a[i]!=b[i];bad+=e>1./255;n++;lo=std::min(lo,y);hi=std::max(hi,y);}std::cout<<"{\"maxAbs\":"<<mx<<",\"mae\":"<<mae/n<<",\"rmse\":"<<sqrt(mse/n)<<",\"psnr\":"<<(mse?10*log10(n/mse):999)<<",\"differentFraction\":"<<double(different)/n<<",\"above1over255Fraction\":"<<double(bad)/n<<",\"nonfinite\":"<<nonfinite<<",\"range\":["<<lo<<","<<hi<<"],\"stddev\":"<<sqrt(sq/n-(sum/n)*(sum/n))<<"}\n";
 if(argc>3){int w=960,h=540;std::vector<unsigned char> bmp(54+w*h*3,0);bmp[0]='B';bmp[1]='M';auto set=[&](int pos,uint32_t v){for(int j=0;j<4;j++)bmp[pos+j]=(v>>(j*8))&255;};set(2,bmp.size());set(10,54);set(14,40);set(18,w);set(22,h);bmp[26]=1;bmp[28]=24;for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<3;c++)bmp[54+((h-1-y)*w+x)*3+2-c]=(unsigned char)std::clamp(int(DirectX::PackedVector::XMConvertHalfToFloat(b[(size_t(y*4)*3840+x*4)*4+c])*255+.5f),0,255);std::ofstream f(argv[3],std::ios::binary);f.write((char*)bmp.data(),bmp.size());}return 0;}
