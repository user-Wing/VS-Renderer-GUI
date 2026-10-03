#define COBJMACROS
#include "color/ColorBridge.h"
#include <windows.h>
#include <d3d11.h>
#include <libavutil/frame.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/dovi_meta.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
static float half(uint16_t h) {
    const int exponent=(h>>10)&31;const float fraction=(h&1023)/1024.0f;
    float value=exponent?ldexpf(1+fraction,exponent-15):ldexpf(fraction,-14);return h&32768?-value:value;
}
static float pixel(ID3D11DeviceContext *immediate, ID3D11Texture2D *target, ID3D11Texture2D *readback) {
    D3D11_MAPPED_SUBRESOURCE mapped;
    ID3D11DeviceContext_CopyResource(immediate,(ID3D11Resource*)readback,(ID3D11Resource*)target);
    if(FAILED(ID3D11DeviceContext_Map(immediate,(ID3D11Resource*)readback,0,D3D11_MAP_READ,0,&mapped)))return NAN;
    const float value=half(((uint16_t*)mapped.pData)[0]);ID3D11DeviceContext_Unmap(immediate,(ID3D11Resource*)readback,0);return value;
}
int main(void) {
    wchar_t path[32768];GetModuleFileNameW(NULL,path,32768);wchar_t *end=wcsrchr(path,L'\\');wcscpy(end+1,L"color\\vsr-color.dll");
    HMODULE module=LoadLibraryExW(path,NULL,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);CHECK(module);
    void*(*create)(void*)=(void*)GetProcAddress(module,"vsr_color_create");
    void(*destroy)(void*)=(void*)GetProcAddress(module,"vsr_color_destroy");
    int(*configure)(void*,const VsrColorSettings*)=(void*)GetProcAddress(module,"vsr_color_configure");
    int(*frame)(void*,const void*,int,uint32_t,uint32_t)=(void*)GetProcAddress(module,"vsr_color_frame");
    int(*draw)(void*,const VsrColorDraw*)=(void*)GetProcAddress(module,"vsr_color_draw");
    int(*status)(void*,VsrColorStatus*)=(void*)GetProcAddress(module,"vsr_color_status");
    CHECK(create&&destroy&&configure&&frame&&draw&&status);
    ID3D11Device *device=NULL;ID3D11DeviceContext *immediate=NULL;
    CHECK(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,D3D11_CREATE_DEVICE_BGRA_SUPPORT,NULL,0,D3D11_SDK_VERSION,&device,NULL,&immediate)));
    void *context=create(device);CHECK(context);VsrColorSettings settings=VsrColorDefaultSettings();settings.engine=1;settings.icc=0;settings.peakDetect=0;settings.tone=4;CHECK(configure(context,&settings));
    uint16_t pixels[32*16*4];
    const double power=pow(203.0/10000.0,2610.0/16384.0);
    const uint16_t pq=(uint16_t)lround(pow((3424.0/4096+2413.0/128*power)/(1+2392.0/128*power),2523.0/32)*65535);
    for(int i=0;i<32*16;++i){pixels[i*4]=pixels[i*4+1]=pixels[i*4+2]=pq;pixels[i*4+3]=65535;}
    D3D11_TEXTURE2D_DESC desc={.Width=32,.Height=16,.MipLevels=1,.ArraySize=1,.Format=DXGI_FORMAT_R16G16B16A16_UNORM,.SampleDesc={1,0},.Usage=D3D11_USAGE_DEFAULT,.BindFlags=D3D11_BIND_SHADER_RESOURCE};
    D3D11_SUBRESOURCE_DATA input={.pSysMem=pixels,.SysMemPitch=32*8};ID3D11Texture2D *texture=NULL,*target=NULL,*readback=NULL;
    CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,&input,&texture)));
    desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&target)));
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&readback)));
    AVFrame *source=av_frame_alloc();CHECK(source);source->width=32;source->height=16;source->format=AV_PIX_FMT_RGBA64LE;
    source->colorspace=AVCOL_SPC_RGB;source->color_primaries=AVCOL_PRI_BT2020;source->color_trc=AVCOL_TRC_SMPTE2084;source->color_range=AVCOL_RANGE_JPEG;
    AVMasteringDisplayMetadata *master=av_mastering_display_metadata_create_side_data(source);CHECK(master);master->has_luminance=1;master->max_luminance=(AVRational){1000,1};master->min_luminance=(AVRational){0,1};
    CHECK(frame(context,source,source->format,0,1));
    VsrColorDraw render={.textures={texture},.targetTexture=target,.hdr=1,.outputBits=16,.targetPeak=1000,.crop={0,0,1,1},.destination={0,0,32,16}};
    CHECK(draw(context,&render));ID3D11DeviceContext_CopyResource(immediate,(ID3D11Resource*)readback,(ID3D11Resource*)target);
    D3D11_MAPPED_SUBRESOURCE mapped;CHECK(SUCCEEDED(ID3D11DeviceContext_Map(immediate,(ID3D11Resource*)readback,0,D3D11_MAP_READ,0,&mapped)));
    const float white=half(((uint16_t*)mapped.pData)[0]);ID3D11DeviceContext_Unmap(immediate,(ID3D11Resource*)readback,0);
    printf("203-nit PQ -> scRGB: %.4f (expected 2.5375)\n",white);CHECK(fabsf(white-2.5375f)<0.05f);
    VsrColorStatus info={.size=sizeof(info),.version=1};CHECK(status(context,&info));CHECK(info.masteringPeak==1000);
    size_t plusSize=0;AVDynamicHDRPlus *plus=av_dynamic_hdr_plus_alloc(&plusSize);CHECK(plus);
    plus->application_version=1;plus->num_windows=1;plus->targeted_system_display_maximum_luminance=(AVRational){100,1};
    plus->params[0].maxscl[0]=plus->params[0].maxscl[1]=plus->params[0].maxscl[2]=(AVRational){1,10};
    plus->params[0].average_maxrgb=(AVRational){1,50};plus->params[0].tone_mapping_flag=1;
    plus->params[0].knee_point_x=(AVRational){1,4};plus->params[0].knee_point_y=(AVRational){1,2};
    plus->params[0].num_bezier_curve_anchors=1;plus->params[0].bezier_curve_anchors[0]=(AVRational){3,4};
    AVFrameSideData *side=av_frame_new_side_data(source,AV_FRAME_DATA_DYNAMIC_HDR_PLUS,plusSize);CHECK(side);memcpy(side->data,plus,plusSize);av_free(plus);
    settings.tone=0;render.hdr=0;render.outputBits=8;render.targetPeak=100;
    CHECK(configure(context,&settings));CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));CHECK(status(context,&info));CHECK(info.hdr10plus);CHECK(info.tone==2);
    const float dynamicA=pixel(immediate,target,readback);
    ((AVDynamicHDRPlus*)side->data)->params[0].knee_point_y=(AVRational){1,10};
    ((AVDynamicHDRPlus*)side->data)->params[0].bezier_curve_anchors[0]=(AVRational){1,10};
    CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));const float dynamicB=pixel(immediate,target,readback);
    printf("HDR10+ changed knee / anchor: %.4f -> %.4f\n",dynamicA,dynamicB);CHECK(isfinite(dynamicA)&&isfinite(dynamicB));CHECK(fabsf(dynamicA-dynamicB)>.001f);
    ((AVDynamicHDRPlus*)side->data)->params[0].num_bezier_curve_anchors=255;CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));CHECK(status(context,&info));CHECK(!info.hdr10plus);
    // A truncated DV packet must stay a reported base-layer fallback.
    side=av_frame_new_side_data(source,AV_FRAME_DATA_DOVI_METADATA,8);CHECK(side);memset(side->data,0xff,8);
    CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));CHECK(status(context,&info));CHECK(info.doviDetected && !info.doviActive);
    CHECK(source->color_trc==AVCOL_TRC_SMPTE2084);CHECK(av_frame_get_side_data(source,AV_FRAME_DATA_DOVI_METADATA)->size==8);
    // Synthetic residual-free DV metadata exercises the real reshaping shader.
    av_frame_remove_side_data(source,AV_FRAME_DATA_DOVI_METADATA);av_frame_remove_side_data(source,AV_FRAME_DATA_DYNAMIC_HDR_PLUS);
    size_t dvSize=0;AVDOVIMetadata *dv=av_dovi_metadata_alloc(&dvSize);CHECK(dv);
    AVDOVIRpuDataHeader *header=av_dovi_get_header(dv);header->bl_bit_depth=16;header->coef_log2_denom=16;header->disable_residual_flag=1;
    AVDOVIDataMapping *mapping=av_dovi_get_mapping(dv);
    for(int component=0;component<3;++component){AVDOVIReshapingCurve *curve=&mapping->curves[component];curve->num_pivots=2;curve->pivots[1]=65535;curve->poly_order[0]=1;curve->poly_coef[0][1]=65536;}
    AVDOVIColorMetadata *color=av_dovi_get_color(dv);color->source_max_pq=3079;
    for(int i=0;i<9;++i){color->ycc_to_rgb_matrix[i]=(AVRational){i%4==0,1};color->rgb_to_lms_matrix[i]=(AVRational){i%4==0,1};}
    for(int i=0;i<3;++i)color->ycc_to_rgb_offset[i]=(AVRational){0,1};
    side=av_frame_new_side_data(source,AV_FRAME_DATA_DOVI_METADATA,dvSize);CHECK(side);memcpy(side->data,dv,dvSize);av_free(dv);
    CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));CHECK(status(context,&info));CHECK(info.doviDetected && info.doviActive);CHECK(isfinite(pixel(immediate,target,readback)));
    header=av_dovi_get_header((AVDOVIMetadata*)side->data);header->disable_residual_flag=0;
    CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));CHECK(status(context,&info));CHECK(info.doviDetected && !info.doviActive);
    // Quantization should distribute adjacent levels without shifting the mean.
    av_frame_unref(source);source->width=32;source->height=16;source->format=AV_PIX_FMT_RGBA64LE;
    source->colorspace=AVCOL_SPC_RGB;source->color_primaries=AVCOL_PRI_BT709;source->color_trc=AVCOL_TRC_IEC61966_2_1;source->color_range=AVCOL_RANGE_JPEG;
    for(int i=0;i<32*16;++i){pixels[i*4]=pixels[i*4+1]=pixels[i*4+2]=31744;pixels[i*4+3]=65535;}
    ID3D11DeviceContext_UpdateSubresource(immediate,(ID3D11Resource*)texture,0,NULL,pixels,32*8,0);
    ID3D11Texture2D_Release(target);ID3D11Texture2D_Release(readback);
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=0;
    CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&target)));
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;CHECK(SUCCEEDED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&readback)));
    render.targetTexture=target;settings.tone=4;
    for(unsigned dither=0;dither<2;++dither){
        settings.dither=dither;CHECK(configure(context,&settings));CHECK(frame(context,source,source->format,0,1));CHECK(draw(context,&render));
        ID3D11DeviceContext_CopyResource(immediate,(ID3D11Resource*)readback,(ID3D11Resource*)target);CHECK(SUCCEEDED(ID3D11DeviceContext_Map(immediate,(ID3D11Resource*)readback,0,D3D11_MAP_READ,0,&mapped)));
        unsigned levels[256]={0},unique=0;double mean=0;
        for(int y=0;y<16;++y)for(int x=0;x<32;++x){const uint8_t *rgb=(const uint8_t*)mapped.pData+y*mapped.RowPitch+x*4;levels[rgb[0]]++;mean+=rgb[0];CHECK(abs(rgb[0]-rgb[1])<=1 && abs(rgb[0]-rgb[2])<=1);}
        ID3D11DeviceContext_Unmap(immediate,(ID3D11Resource*)readback,0);for(int i=0;i<256;++i)unique+=levels[i]!=0;mean/=512;
        printf("8-bit dither %u: %u levels, mean %.4f (expected %.4f)\n",dither,unique,mean,31744.0/65535*255);
        CHECK(dither?unique>=2:unique==1);CHECK(fabs(mean-31744.0/65535*255)<1.0);
    }
    av_frame_free(&source);destroy(context);ID3D11Texture2D_Release(readback);ID3D11Texture2D_Release(target);ID3D11Texture2D_Release(texture);ID3D11DeviceContext_Release(immediate);ID3D11Device_Release(device);FreeLibrary(module);
    puts("PASS: scRGB units, HDR10+ curve, DV reshape / FEL fallback, dithering, malformed side data, source immutability");return 0;
}
