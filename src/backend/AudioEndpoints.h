#pragma once
#include <QList>
#include <QString>
#include <windows.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <functiondiscoverykeys_devpkey.h>

namespace vsr {
struct AudioEndpoint { QString id; QString name; };
// The native API accepts stable MMDevice IDs but has no enumeration API.
inline QList<AudioEndpoint> audioEndpoints() {
    QList<AudioEndpoint> result;
    const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    IMMDeviceEnumerator *enumerator=nullptr;
    IMMDeviceCollection *collection=nullptr;
    if(SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),reinterpret_cast<void **>(&enumerator)))) {
        if(SUCCEEDED(enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,&collection))) {
            UINT count=0;collection->GetCount(&count);
            for(UINT n=0;n<count;++n) {
                IMMDevice *device=nullptr;LPWSTR id=nullptr;IPropertyStore *properties=nullptr;
                if(SUCCEEDED(collection->Item(n,&device))) {
                    if(SUCCEEDED(device->GetId(&id))) {
                        QString name=QString::fromWCharArray(id);
                        if(SUCCEEDED(device->OpenPropertyStore(STGM_READ,&properties))) {
                            PROPVARIANT value;PropVariantInit(&value);
                            if(SUCCEEDED(properties->GetValue(PROPERTYKEY{{0xa45c254e,0xdf1c,0x4efd,{0x80,0x20,0x67,0xd1,0x46,0xa8,0x50,0xe0}},14},&value)) && value.vt==VT_LPWSTR)
                                name=QString::fromWCharArray(value.pwszVal);
                            PropVariantClear(&value);properties->Release();
                        }
                        result.append({QString::fromWCharArray(id),name});CoTaskMemFree(id);
                    }
                    device->Release();
                }
            }
            collection->Release();
        }
        enumerator->Release();
    }
    if(SUCCEEDED(initialized))CoUninitialize();
    return result;
}
}
