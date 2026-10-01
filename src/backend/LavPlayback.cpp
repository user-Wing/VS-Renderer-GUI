#include "backend/LavPlayback.h"
#include <QCoreApplication>
#include <QDir>
#include <windows.h>
#include <dshow.h>
#include <cmath>
#include <vector>
#include <QProcess>
#include <QFileInfo>
#include <QHash>

namespace vsr {
namespace {
// Vendor COM interfaces have no implementation in this translation unit.
// Call the documented slot directly, avoiding MinGW pure-virtual devirtualization.
HRESULT setOsd(IUnknown *object,HBITMAP bitmap) {
    using Method=HRESULT (STDMETHODCALLTYPE *)(IUnknown*,const char*,HBITMAP,HBITMAP,COLORREF,int,int,bool,int,DWORD,DWORD,void*,void*,void*);
    auto method=reinterpret_cast<Method>((*reinterpret_cast<void ***>(object))[3]);
    return method(object,"VSPlayer.Subtitles",bitmap,nullptr,0,0,0,false,1,0,0,nullptr,nullptr,nullptr);
}
template<class T> void release(T *&value) { if (value) { value->Release(); value = nullptr; } }
GUID guid(const wchar_t *text) { GUID id{}; CLSIDFromString(text, &id); return id; }

}
struct LavPlayback::Impl {
    IGraphBuilder *graph = nullptr;
    IMediaControl *control = nullptr;
    IMediaSeeking *seeking = nullptr;
    IBasicAudio *audio = nullptr;
    IVideoWindow *window = nullptr;
    IBaseFilter *renderer = nullptr;
    IUnknown *osd = nullptr;
    HBITMAP subtitleBitmap = nullptr;
    std::vector<HMODULE> modules;
    QString error;
    bool initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    ~Impl() { clear(); if (initialized) CoUninitialize(); }
    void clear() {
        if (osd) setOsd(osd,nullptr);
        release(osd); if(subtitleBitmap) DeleteObject(subtitleBitmap); subtitleBitmap=nullptr;
        if (control) control->Stop(); release(window); release(renderer); release(audio); release(seeking); release(control); release(graph);
        for (auto module : modules) FreeLibrary(module); modules.clear();
    }
    bool check(HRESULT result, const QString &operation) {
        if (SUCCEEDED(result)) return true;
        error = operation + QStringLiteral(" (0x%1)").arg(static_cast<quint32>(result), 8, 16, QChar('0')); return false;
    }
    IBaseFilter *filter(const QString &file, const wchar_t *clsid, const QString &folder = "LAVFilters64") {
        const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(folder + "/" + file);
        static QHash<QString,HMODULE> madModules;
        if(folder=="madVR09217") for(const auto &dependency:QStringList{"madHcNet64.dll","mvrSettings64.dll"}) {
            const auto dependencyPath=QDir(QFileInfo(path).absolutePath()).filePath(dependency);
            if(madModules.contains(dependencyPath))continue;
            HMODULE loaded=LoadLibraryExW(reinterpret_cast<LPCWSTR>(dependencyPath.utf16()),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if(!loaded) {error=QCoreApplication::translate("LavPlayback","madVR 依赖加载失败：%1（Windows 错误 %2）").arg(dependencyPath).arg(GetLastError());return nullptr;}// madVR keeps global worker callbacks; its helper DLLs stay loaded for this process.
            madModules.insert(dependencyPath,loaded);
        }
        HMODULE module = folder=="madVR09217"?madModules.value(path):nullptr;
        if(!module)module = LoadLibraryExW(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!module) { error = QCoreApplication::translate("LavPlayback","无法加载随附组件：%1（Windows 错误 %2）").arg(path).arg(GetLastError()); return nullptr; }
        if(folder!="madVR09217") modules.push_back(module);
        else madModules.insert(path,module);
        using Factory = HRESULT (WINAPI *)(REFCLSID, REFIID, void **);
        auto factory = reinterpret_cast<Factory>(GetProcAddress(module, "DllGetClassObject"));
        IClassFactory *object = nullptr; IBaseFilter *result = nullptr;
        if (!factory || !check(factory(guid(clsid), IID_IClassFactory, reinterpret_cast<void **>(&object)), "LAV factory")) return nullptr;
        check(object->CreateInstance(nullptr, IID_IBaseFilter, reinterpret_cast<void **>(&result)), "LAV filter"); release(object);
        return result;
    }
};
LavPlayback::LavPlayback() : impl_(std::make_unique<Impl>()) {}
LavPlayback::~LavPlayback() = default;
bool LavPlayback::open(const QString &path, void *videoWindow, bool enableVideo, bool enableAudio) {
    auto &i = *impl_; i.clear(); i.error.clear();
    if (!i.check(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<void **>(&i.graph)), "DirectShow graph")) return false;
    auto *source = i.filter("LAVSplitter.ax", L"{B98D13E7-55DB-4385-A33D-09FD1BA26338}");
    auto *video = enableVideo ? i.filter("LAVVideo.ax", L"{EE30215D-164F-4A92-A4EB-9D4C13390F9F}") : nullptr;
    auto *audio = enableAudio ? i.filter("LAVAudio.ax", L"{E8E73B6B-4CB3-44A4-BE99-4F7BCB96E491}") : nullptr;
    if (!source || (enableVideo && !video) || (enableAudio && !audio)) { release(source); release(video); release(audio); return false; }
    i.graph->AddFilter(source, L"Bundled LAV Splitter Source"); if(video)i.graph->AddFilter(video, L"Bundled LAV Video Decoder"); if(audio)i.graph->AddFilter(audio, L"Bundled LAV Audio Decoder");
    if (videoWindow) {
        i.renderer = i.filter("madVR64.ax", L"{E1A8B82A-32CE-4B0D-BE0D-AA68C772E423}", "madVR09217");
        if (!i.renderer || !i.check(i.graph->AddFilter(i.renderer, L"madshi video renderer"), "madVR filter")) { release(source); release(video); release(audio); return false; }
    }
    IFileSourceFilter *input = nullptr;
    HRESULT result = source->QueryInterface(IID_IFileSourceFilter, reinterpret_cast<void **>(&input));
    if (SUCCEEDED(result)) result = input->Load(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr); release(input);
    if (SUCCEEDED(result)) {
        IEnumPins *pins = nullptr; source->EnumPins(&pins); IPin *pin = nullptr; bool rendered = false;
        while (pins && pins->Next(1, &pin, nullptr) == S_OK) {
            PIN_DIRECTION direction; pin->QueryDirection(&direction);
            if (direction == PINDIR_OUTPUT) {
                IEnumMediaTypes *types=nullptr; AM_MEDIA_TYPE *type=nullptr; GUID major{};
                if(SUCCEEDED(pin->EnumMediaTypes(&types)) && types->Next(1,&type,nullptr)==S_OK) {
                    major=type->majortype; if(type->cbFormat)CoTaskMemFree(type->pbFormat); if(type->pUnk)type->pUnk->Release();CoTaskMemFree(type);
                }
                release(types);
                if(((enableVideo && major==MEDIATYPE_Video) || (enableAudio && major==MEDIATYPE_Audio)) && SUCCEEDED(i.graph->Render(pin)))rendered=true;
            }
            release(pin);
        }
        release(pins); if (!rendered) result = VFW_E_CANNOT_RENDER;
    }
    release(source); release(video); release(audio);
    if (!i.check(result, QCoreApplication::translate("LavPlayback","LAV 打开媒体"))) return false;
    i.graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&i.control));
    i.graph->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void **>(&i.seeking));
    i.graph->QueryInterface(IID_IBasicAudio, reinterpret_cast<void **>(&i.audio));
    if (i.renderer) {
        if (!i.check(i.renderer->QueryInterface(IID_IVideoWindow, reinterpret_cast<void **>(&i.window)), "madVR window")) return false;
        IEnumPins *pins = nullptr; i.renderer->EnumPins(&pins); IPin *pin = nullptr; bool connected = false;
        while (pins && pins->Next(1,&pin,nullptr) == S_OK) { IPin *peer = nullptr; if (SUCCEEDED(pin->ConnectedTo(&peer))) connected = true; release(peer); release(pin); }
        release(pins); if (!connected) { i.error = QCoreApplication::translate("LavPlayback","madVR 未连接到视频输出。"); return false; }
        i.window->put_Owner(reinterpret_cast<OAHWND>(videoWindow)); i.window->put_MessageDrain(reinterpret_cast<OAHWND>(videoWindow));
        i.window->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN); i.window->put_AutoShow(OAFALSE); i.window->put_Visible(OATRUE);
        i.renderer->QueryInterface(guid(L"{3AE03A88-F613-4BBA-AD3E-EE236976BF9A}"),reinterpret_cast<void **>(&i.osd));
        const auto controller = QDir(QCoreApplication::applicationDirPath()).filePath("madVR09217/madHcCtrl.exe");
        static bool controllerStarted=false; if(!controllerStarted)controllerStarted=QProcess::startDetached(controller, {}, QFileInfo(controller).absolutePath());
    } else {
        IVideoWindow *window = nullptr;
        if (SUCCEEDED(i.graph->QueryInterface(IID_IVideoWindow, reinterpret_cast<void **>(&window)))) { window->put_AutoShow(OAFALSE); window->put_Visible(OAFALSE); release(window); }
    }
    return i.control && i.seeking && i.check(i.control->Pause(), "LAV pause");
}
bool LavPlayback::play() { return impl_->control && impl_->check(impl_->control->Run(), "LAV play"); }
bool LavPlayback::pause() { return impl_->control && impl_->check(impl_->control->Pause(), "LAV pause"); }
bool LavPlayback::seek(std::int64_t position) { LONGLONG target = position; return impl_->seeking && impl_->check(impl_->seeking->SetPositions(&target, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning), "LAV seek"); }
std::int64_t LavPlayback::position() const { LONGLONG value = 0; if (impl_->seeking) impl_->seeking->GetCurrentPosition(&value); return value; }
std::int64_t LavPlayback::duration() const { LONGLONG value = 0; if (impl_->seeking) impl_->seeking->GetDuration(&value); return value; }
void LavPlayback::volume(float value, bool muted) { if (impl_->audio) impl_->audio->put_Volume(muted || value <= 0 ? -10000 : static_cast<long>(2000 * std::log10(value))); }
QString LavPlayback::error() const { return impl_->error; }
void LavPlayback::resizeVideo(int width, int height) { if (impl_->window) impl_->window->SetWindowPosition(0,0,width,height); }
bool LavPlayback::madvrActive() const { return impl_->renderer && impl_->window; }
void LavPlayback::setSubtitle(const QImage &image) {
    auto &i=*impl_; if(!i.osd) return; HBITMAP next=nullptr;
    if(!image.isNull()) { BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=image.width();info.bmiHeader.biHeight=-image.height();info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        void *pixels=nullptr;next=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0); if(next) for(int y=0;y<image.height();++y) memcpy(static_cast<char *>(pixels)+y*image.width()*4,image.constScanLine(y),image.width()*4);
    }
    setOsd(i.osd,next);
    if(i.subtitleBitmap) DeleteObject(i.subtitleBitmap); i.subtitleBitmap=next;
}
bool LavPlayback::setRate(double rate) { return impl_->seeking && impl_->check(impl_->seeking->SetRate(rate), "DirectShow rate"); }
void LavPlayback::showVideoSettings(void *owner) {
    IBaseFilter *video = nullptr; ISpecifyPropertyPages *pages = nullptr; CAUUID identifiers{};
    if (impl_->graph) impl_->graph->FindFilterByName(L"Bundled LAV Video Decoder",&video);
    if(!video) video=impl_->filter("LAVVideo.ax",L"{EE30215D-164F-4A92-A4EB-9D4C13390F9F}");
    if (video && SUCCEEDED(video->QueryInterface(IID_ISpecifyPropertyPages,reinterpret_cast<void **>(&pages)))) {
        if (SUCCEEDED(pages->GetPages(&identifiers))) { IUnknown *object = video; OleCreatePropertyFrame(static_cast<HWND>(owner),0,0,L"LAV Video Decoder",1,&object,identifiers.cElems,identifiers.pElems,0,0,nullptr); CoTaskMemFree(identifiers.pElems); }
    }
    release(pages); release(video);
}
QImage LavPlayback::capture() const {
    IBasicVideo *video = nullptr; QImage image; impl_->error.clear();
    if(impl_->renderer) {
        IUnknown *grab=nullptr; void *dib=nullptr;
        if(SUCCEEDED(impl_->renderer->QueryInterface(guid(L"{B0F34BA5-5EFD-4762-A07F-FF9046B4566C}"),reinterpret_cast<void **>(&grab)))) {
            using Grab=HRESULT (STDMETHODCALLTYPE *)(IUnknown*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,void**,void*);
            auto method=reinterpret_cast<Grab>((*reinterpret_cast<void ***>(grab))[3]);
            const auto grabResult=method(grab,0,3,0,0,0,0,&dib,nullptr);
            if(FAILED(grabResult)) impl_->error=QString("madVR GrabFrame: 0x%1").arg(static_cast<quint32>(grabResult),8,16,QChar('0'));
            if(SUCCEEDED(grabResult) && dib) {
                const auto *header=static_cast<BITMAPINFOHEADER *>(dib);
                if(header->biWidth>0 && header->biHeight && (header->biBitCount==24 || header->biBitCount==32)) {
                    const int stride=((header->biWidth*header->biBitCount+31)/32)*4;
                    if(header->biWidth<=20000 && std::abs(header->biHeight)<=20000) {
                        image=QImage(header->biWidth,std::abs(header->biHeight),QImage::Format_RGB32);
                        for(int y=0;y<image.height();++y) {const auto *row=static_cast<const uchar *>(dib)+header->biSize+(header->biHeight>0?image.height()-1-y:y)*stride;auto *out=reinterpret_cast<QRgb *>(image.scanLine(y));for(int x=0;x<image.width();++x){const auto *pixel=row+x*(header->biBitCount/8);out[x]=qRgb(pixel[2],pixel[1],pixel[0]);}}
                    }
                }
                LocalFree(dib);
            } release(grab);
        }
        if(!image.isNull()) return image;
    }
    if (impl_->renderer && SUCCEEDED(impl_->renderer->QueryInterface(IID_IBasicVideo,reinterpret_cast<void **>(&video)))) {
        long size = 0;
        if (SUCCEEDED(video->GetCurrentImage(&size,nullptr)) && size > 0) {
            std::vector<char> data(size); if (SUCCEEDED(video->GetCurrentImage(&size,reinterpret_cast<long *>(data.data())))) {
                auto *header = reinterpret_cast<BITMAPINFOHEADER *>(data.data());
                if(header->biBitCount==32 && header->biWidth>0 && header->biHeight && header->biSize+static_cast<qint64>(header->biWidth)*std::abs(header->biHeight)*4<=size) {
                    image=QImage(header->biWidth,std::abs(header->biHeight),QImage::Format_RGB32);
                    for(int y=0;y<image.height();++y)memcpy(image.scanLine(y),data.data()+header->biSize+(header->biHeight>0?image.height()-1-y:y)*header->biWidth*4,header->biWidth*4);
                }
            }
        }
    }
    release(video); return image;
}
void LavPlayback::showAudioSettings(void *owner) {
    IBaseFilter *audio=nullptr;ISpecifyPropertyPages *pages=nullptr;CAUUID identifiers{};
    if(impl_->graph)impl_->graph->FindFilterByName(L"Bundled LAV Audio Decoder",&audio);
    if(!audio)audio=impl_->filter("LAVAudio.ax",L"{E8E73B6B-4CB3-44A4-BE99-4F7BCB96E491}");
    if(audio && SUCCEEDED(audio->QueryInterface(IID_ISpecifyPropertyPages,reinterpret_cast<void **>(&pages))))
        if(SUCCEEDED(pages->GetPages(&identifiers))) {IUnknown *object=audio;OleCreatePropertyFrame(static_cast<HWND>(owner),0,0,L"LAV Audio Decoder",1,&object,identifiers.cElems,identifiers.pElems,0,0,nullptr);CoTaskMemFree(identifiers.pElems);}
    release(pages);release(audio);
}
}
