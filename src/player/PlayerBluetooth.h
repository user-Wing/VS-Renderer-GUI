#pragma once
#include <QSettings>
#include <QStringList>
#include <QUuid>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <mmdeviceapi.h>
namespace vsr {
struct BluetoothAudio {
    // 0 none, 1 A2DP, 2 HFP, 3 LE Audio, 4 unknown Bluetooth.
    int protocol=0, channels=0, rate=0, bits=0;
    bool connected=false;
    QString name;
};
inline QString bluetoothContainer(const QVariant &value){
    if(value.metaType().id()==QMetaType::QByteArray){const auto b=value.toByteArray();if(b.size()>=16)return QUuid::fromBytes(b.last(16).constData(),QSysInfo::LittleEndian).toString().toUpper();}
    return value.toString().trimmed().toUpper();
}
inline QString bluetoothText(const QVariant &value){
    if(value.metaType().id()==QMetaType::QByteArray){const auto b=value.toByteArray();if(b.size()>8)return QString::fromUtf16(reinterpret_cast<const char16_t *>(b.constData()+8),(b.size()-8)/2).split(QChar(0)).first();}
    return value.toString();
}
inline BluetoothAudio detectBluetoothAudio(){
    BluetoothAudio result;
    const HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    IMMDeviceEnumerator *enumerator=nullptr;IMMDevice *device=nullptr;LPWSTR id=nullptr;
    if(SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void **>(&enumerator)))){
        if(SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender,eMultimedia,&device)))device->GetId(&id);
    }
    QString endpoint=id?QString::fromWCharArray(id):QString();CoTaskMemFree(id);
    if(device)device->Release();if(enumerator)enumerator->Release();if(SUCCEEDED(init))CoUninitialize();
    if(endpoint.isEmpty())return result;
    endpoint=endpoint.mid(endpoint.lastIndexOf('{'));
    const QString enumRoot="HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Enum/";
    QSettings key("HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Windows/CurrentVersion/MMDevices/Audio/Render/"+endpoint,QSettings::NativeFormat);
    result.connected=key.value("DeviceState").toInt()==1;key.beginGroup("Properties");
    result.name=bluetoothText(key.value("{a45c254e-df1c-4efd-8020-67d146a850e0},2"));
    const auto container=bluetoothContainer(key.value("{8c7ed206-3f8a-4827-b3ab-ae9e1faefc6c},2"));
    const auto format=key.value("{f19f064d-082c-4e27-bc73-6882a1bb8e4c},0").toByteArray();
    if(format.size()>=26){const auto *p=reinterpret_cast<const uchar *>(format.constData())+8;result.channels=qFromLittleEndian<quint16>(p+2);result.rate=qFromLittleEndian<quint32>(p+4);result.bits=qFromLittleEndian<quint16>(p+14);if(format.size()>=48 && qFromLittleEndian<quint16>(p)==65534)result.bits=std::max(result.bits,int(qFromLittleEndian<quint16>(p+18)));}
    auto parent=bluetoothText(key.value("{4340a6c5-93fa-4706-972c-7b648008a5a7},8",key.value("{b3f8fa53-0004-438e-9003-51a46e139bfc},39"))).toUpper();
    auto protocol=[](QString path,QString service,QString hardware){const auto all=(path+service+hardware).toUpper();if(all.contains("BTHLE"))return 3;if(all.contains("BTHA2DP") || all.contains("0000110B-"))return 1;if(all.contains("BTHHF") || all.contains("BTHHSP") || all.contains("0000111E-") || all.contains("00001112-") || all.contains("00001108-"))return 2;return 4;};
    if(parent.contains("BTHENUM") || parent.contains("BTHLE")){result.protocol=protocol(parent,{},{});}
    if(!container.isEmpty())for(const auto &root:QStringList{"BTHENUM","BTHLE"}){
        QSettings tree(enumRoot+root,QSettings::NativeFormat);
        for(const auto &model:tree.childGroups()){
            tree.beginGroup(model);const auto instances=tree.childGroups();tree.endGroup();
            for(const auto &instance:instances){QSettings node(enumRoot+root+"/"+model+"/"+instance,QSettings::NativeFormat);
                auto c=bluetoothContainer(node.value("ContainerID"));if(c!=container)continue;
                const int candidate=protocol(root+model,node.value("Service").toString(),node.value("HardwareID").toStringList().join(' '));
                if(result.protocol!=1 && (candidate==1 || result.protocol!=3))result.protocol=candidate;
            }
        }
    }
    return result;
}
inline qint64 bluetoothCompensation(const BluetoothAudio &audio,int codec,double refresh,int advance){
    if(!audio.connected || !audio.protocol)return 0;
    static constexpr int ranges[][2]={{0,0},{190,210},{140,160},{110,130},{120,140},{40,40},{120,140},{110,130},{80,100},{30,100}};
    double latency=audio.protocol==3?65:audio.protocol==2?175:225;
    if(codec>0 && codec<10 && (audio.protocol==4 || (audio.protocol==1 && codec!=9) || (audio.protocol==3 && codec==9)))latency=(ranges[codec][0]+ranges[codec][1])/2.;
    latency+=(audio.channels>2?8:0)+(audio.rate>=88200?6:0)+(audio.bits>=24?3:0);
    if(refresh<24 || refresh>500)refresh=60;
    return std::llround((std::clamp(2000./refresh+5.,10.,100.)-latency-std::clamp(advance,-100,100))*10000.);
}
}


