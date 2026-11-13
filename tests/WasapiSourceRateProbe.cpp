#include "pch.h"
#include "3FP/Audio/WasapiRenderer.h"
#include <iostream>
int main(){
    int failures=0;
    for(unsigned rate:{44100u,48000u,96000u}) {
        PlayerAudioRuntimeState state;
        PlayerWasapiRenderer renderer(L"",true,&state,{},rate);
        const auto result=renderer.Start();
        std::cout<<"requested="<<rate<<" result="<<static_cast<int>(result)<<" output="<<renderer.OutputSampleRate()<<" bits="<<renderer.OutputValidBitsPerSample()<<"\n";
        if(result!=FFFResult::Success){std::cout<<renderer.LastError()<<"\n";++failures;}
        else if(renderer.OutputSampleRate()!=rate){std::cout<<"DEVICE_FALLBACK\n";}
        renderer.Stop();
    }
    return failures?1:0;
}
