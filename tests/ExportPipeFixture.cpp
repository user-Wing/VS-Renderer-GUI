#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
int main(int argc,char **argv){
#ifdef _WIN32
_setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
if(std::strstr(argv[0],"vspipe")){argv[1]=const_cast<char*>("produce");argc=2;}
int at=1;while(at<argc&&(std::strcmp(argv[at],"-y")==0||std::strcmp(argv[at],"-n")==0))++at;
if(at>=argc)return 2;
unsigned char block[512];
if(std::strcmp(argv[at],"produce")==0){for(int i=0;i<60;++i){std::memset(block,i,sizeof(block));if(std::fwrite(block,1,sizeof(block),stdout)!=sizeof(block))return 3;std::fflush(stdout);std::this_thread::sleep_for(std::chrono::milliseconds(40));}return 0;}
if(at+1>=argc)return 2;const char *output=std::strcmp(argv[at],"consume")==0?argv[at+1]:argv[argc-1];FILE *out=std::fopen(output,"wb");if(!out)return 4;int n=0;while(auto size=std::fread(block,1,sizeof(block),stdin)){std::fwrite(block,1,size,out);++n;std::printf("frame=%d\nout_time_us=%d\n",n,n*512000);std::fflush(stdout);}std::fclose(out);return 0;
}
