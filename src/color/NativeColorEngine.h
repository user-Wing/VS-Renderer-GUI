#pragma once
#include "ColorBridge.h"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <string>
extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
}

// Loaded only when requested; the original renderer has no new DLL requirement.
class NativeColorEngine final {
public:
    VsrColorSettings settings = VsrColorDefaultSettings();
    ~NativeColorEngine() { resetDevice(); av_frame_free(&metadata_); avcodec_parameters_free(&stream_); if (module_) FreeLibrary(module_); }
    void stream(const AVCodecParameters* source) {
        avcodec_parameters_free(&stream_);
        if(source) {stream_=avcodec_parameters_alloc();if(stream_)avcodec_parameters_copy(stream_,source);}
    }
    void resetDevice() { if (context_) destroy_(context_); context_ = nullptr; prepared_ = false; }
    bool configure(const VsrColorSettings& value) {
        settings = value; prepared_ = false;
        if (!settings.engine) return true;
        if (context_) return prepare();
        return true;
    }
    void frame(const AVFrame* frame, int format, bool rgb, unsigned kind) {
        if (!metadata_) metadata_ = av_frame_alloc();
        if (!metadata_) { failure("Color properties allocation failed."); return; }
        av_frame_unref(metadata_);
        if (av_frame_copy_props(metadata_, frame) < 0) { failure("Color properties copy failed."); return; }
        metadata_->width = frame->width; metadata_->height = frame->height; metadata_->format = format;
        streamFields_ = 0;
        if (kind == 1 && stream_) {
            if(metadata_->colorspace==AVCOL_SPC_UNSPECIFIED && stream_->color_space!=AVCOL_SPC_UNSPECIFIED){metadata_->colorspace=stream_->color_space;streamFields_|=1;}
            if(metadata_->color_primaries==AVCOL_PRI_UNSPECIFIED && stream_->color_primaries!=AVCOL_PRI_UNSPECIFIED){metadata_->color_primaries=stream_->color_primaries;streamFields_|=2;}
            if(metadata_->color_trc==AVCOL_TRC_UNSPECIFIED && stream_->color_trc!=AVCOL_TRC_UNSPECIFIED){metadata_->color_trc=stream_->color_trc;streamFields_|=4;}
            if(metadata_->color_range==AVCOL_RANGE_UNSPECIFIED && stream_->color_range!=AVCOL_RANGE_UNSPECIFIED){metadata_->color_range=stream_->color_range;streamFields_|=8;}
            if(metadata_->chroma_location==AVCHROMA_LOC_UNSPECIFIED && stream_->chroma_location!=AVCHROMA_LOC_UNSPECIFIED){metadata_->chroma_location=stream_->chroma_location;streamFields_|=16;}
            if(metadata_->color_trc==AVCOL_TRC_SMPTE2084 || metadata_->color_trc==AVCOL_TRC_ARIB_STD_B67) {
                supplement(AV_PKT_DATA_MASTERING_DISPLAY_METADATA,AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
                supplement(AV_PKT_DATA_CONTENT_LIGHT_LEVEL,AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
            }
        }
        format_ = format; rgb_ = rgb; kind_ = kind;
        if (context_ && settings.engine && configured_) prepared_ = frame_(context_, metadata_, format_, rgb_, kind_) != 0;
    }
    bool draw(void* device, const VsrColorDraw& draw) {
        if (!settings.engine) return false;
        bypass_.clear();
        if (!context_) {
            if (!load()) return false;
            context_ = create_(device);
            if (!context_) { failure("libplacebo D3D11 initialization failed; using original 3FP."); return false; }
            if (!prepare()) return false;
        }
        return prepared_ && draw_(context_, &draw);
    }
    void bypass(const char* reason) { bypass_ = reason; }
    VsrColorStatus status() const {
        VsrColorStatus value{}; value.size = sizeof(value); value.version = 1;
        value.requestedEngine = settings.engine;
        std::snprintf(value.engine, sizeof(value.engine), "3FP native");
        if (settings.engine && context_) status_(context_, &value);
        value.streamFields = streamFields_;
        if (settings.engine && !error_.empty()) {
            value.activeEngine = 0; std::snprintf(value.fallback, sizeof(value.fallback), "%s", error_.c_str());
        }
        if (settings.engine && !bypass_.empty()) {
            value.activeEngine = 0; std::snprintf(value.engine, sizeof(value.engine), "3FP native");
            std::snprintf(value.fallback, sizeof(value.fallback), "%s", bypass_.c_str());
        }
        if (!value.activeEngine) std::snprintf(value.engine, sizeof(value.engine), "3FP native");
        return value;
    }
private:
    void supplement(AVPacketSideDataType packetType, AVFrameSideDataType frameType) {
        if(av_frame_get_side_data(metadata_,frameType))return;
        const auto* source=av_packet_side_data_get(stream_->coded_side_data,stream_->nb_coded_side_data,packetType);
        if(source && source->size)if(auto* target=av_frame_new_side_data(metadata_,frameType,source->size))std::memcpy(target->data,source->data,source->size);
    }
    bool prepare() {
        error_.clear();
        configured_ = configure_(context_, &settings) != 0;
        prepared_ = configured_ && metadata_ &&
            frame_(context_, metadata_, format_, rgb_, kind_);
        return prepared_;
    }
    void failure(const char* error) { error_ = error; prepared_ = false; }
    bool load() {
        if (module_) return create_ && destroy_ && configure_ && frame_ && draw_ && status_;
        HMODULE own = nullptr; wchar_t path[32768]{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&moduleAnchor), &own) || !GetModuleFileNameW(own, path, 32768)) return false;
        std::wstring name(path); name.resize(name.find_last_of(L"\\/") + 1); name += L"color\\vsr-color.dll";
        module_ = LoadLibraryExW(name.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!module_) { failure("Optional color/vsr-color.dll or its dependencies are unavailable; using original 3FP."); return false; }
        create_ = reinterpret_cast<decltype(create_)>(GetProcAddress(module_, "vsr_color_create"));
        destroy_ = reinterpret_cast<decltype(destroy_)>(GetProcAddress(module_, "vsr_color_destroy"));
        configure_ = reinterpret_cast<decltype(configure_)>(GetProcAddress(module_, "vsr_color_configure"));
        frame_ = reinterpret_cast<decltype(frame_)>(GetProcAddress(module_, "vsr_color_frame"));
        draw_ = reinterpret_cast<decltype(draw_)>(GetProcAddress(module_, "vsr_color_draw"));
        status_ = reinterpret_cast<decltype(status_)>(GetProcAddress(module_, "vsr_color_status"));
        if (!create_ || !destroy_ || !configure_ || !frame_ || !draw_ || !status_) {
            failure("The optional color engine ABI is incompatible; using original 3FP."); return false;
        }
        return true;
    }
    static void moduleAnchor() {}
    HMODULE module_ = nullptr;
    void* context_ = nullptr;
    AVFrame* metadata_ = nullptr;
    AVCodecParameters* stream_ = nullptr;
    unsigned streamFields_ = 0;
    int format_ = 0;
    unsigned rgb_ = 0, kind_ = 1;
    bool prepared_ = false;
    bool configured_ = false;
    std::string error_;
    std::string bypass_;
    decltype(&vsr_color_create) create_ = nullptr;
    decltype(&vsr_color_destroy) destroy_ = nullptr;
    decltype(&vsr_color_configure) configure_ = nullptr;
    decltype(&vsr_color_frame) frame_ = nullptr;
    decltype(&vsr_color_draw) draw_ = nullptr;
    decltype(&vsr_color_status) status_ = nullptr;
};
