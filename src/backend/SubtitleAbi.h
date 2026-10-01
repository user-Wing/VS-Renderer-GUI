#pragma once
#include <cstdint>
namespace vsr {
struct SubtitleBitmap { uint32_t size=sizeof(SubtitleBitmap), version=1, flags=0,reserved=0; int64_t start=0,end=0; int32_t canvasWidth=0,canvasHeight=0,x=0,y=0,width=0,height=0,stride=0; uint32_t pixelBytes=0; int64_t sequence=0; };
struct TimedTextCommand { uint32_t size=sizeof(TimedTextCommand),version=1,type=2,flags=0; float x=0,y=0,width=0,height=0; uint32_t foreground=0xffffffff,outline=0; float fontSize=0,outlineWidth=0; uint32_t horizontal=0,vertical=0; const char *text=nullptr,*font=nullptr; const void *bitmap=nullptr; uint32_t bitmapWidth=0,bitmapHeight=0,bitmapStride=0,bitmapBytes=0; uint64_t contentId=0; uint32_t shadow=0; float shadowX=0,shadowY=0; uint32_t reserved=0; };
struct TimedTextLayer { uint32_t size=sizeof(TimedTextLayer),version=1,width=0,height=0,count=0,slot=0; uint64_t sequence=0; const TimedTextCommand *commands=nullptr; float rate=30; uint32_t reserved=0; float blur=0; uint32_t passes=0,downsample=0,tint=0; float region=0,lyrics=0,left=0,top=0,right=0; };
}
