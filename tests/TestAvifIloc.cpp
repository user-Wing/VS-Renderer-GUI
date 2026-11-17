#define PCH_H
#include <windows.h>
#include <string>
#include <vector>
#include <QtTest>
#include "3FP/Core/AvifIlocShim.cpp"
namespace {
void number(std::vector<uint8_t> &v,uint32_t n,int width){for(int i=width-1;i>=0;--i)v.push_back(uint8_t(n>>(i*8)));}
std::vector<uint8_t> box(const char *name,const std::vector<uint8_t> &payload){std::vector<uint8_t> v;number(v,uint32_t(payload.size()+8),4);v.insert(v.end(),name,name+4);v.insert(v.end(),payload.begin(),payload.end());return v;}
std::vector<uint8_t> fixture(bool zero=false){
    std::vector<uint8_t> v=box("ftyp",{'a','v','i','f',0,0,0,0});
    std::vector<uint8_t> iloc{0,0,0,0,uint8_t(zero?0:0x44),0};
    number(iloc,1,2);number(iloc,1,2);number(iloc,0,2);number(iloc,zero?65535:2,2);
    if(!zero){number(iloc,0,4);number(iloc,2,4);number(iloc,2,4);number(iloc,2,4);}
    auto child=box("iloc",iloc);std::vector<uint8_t> meta(4,0);meta.insert(meta.end(),child.begin(),child.end());auto b=box("meta",meta);v.insert(v.end(),b.begin(),b.end());return v;
}
}
class TestAvifIloc:public QObject {
    Q_OBJECT
private slots:
    void extendedBox(){
        auto bytes=fixture();
        // Convert ftyp to the ISO BMFF 64-bit box header, keeping its payload.
        bytes[3]=1;
        bytes.insert(bytes.begin()+8,{0,0,0,0,0,0,0,24});
        std::string detail;
        QVERIFY2(AvifIlocShim::RepairInPlace(bytes,detail),detail.c_str());
    }
    void budgetsAndControl(){
        std::string detail;auto zero=fixture(true),original=zero;
        QVERIFY(!AvifIlocShim::RepairInPlace(zero,detail));QVERIFY(zero==original);
        for(uint8_t widths:{uint8_t(0x04),uint8_t(0x40)}){auto single=fixture(true);single[40]=widths;auto before=single;QVERIFY(!AvifIlocShim::RepairInPlace(single,detail));QVERIFY(single==before);}
        auto good=fixture();QVERIFY(AvifIlocShim::RepairInPlace(good,detail));
        auto truncated=fixture();truncated.pop_back();original=truncated;
        QVERIFY(!AvifIlocShim::RepairInPlace(truncated,detail));QVERIFY(truncated==original);
    }
};
QTEST_APPLESS_MAIN(TestAvifIloc)
#include "TestAvifIloc.moc"
