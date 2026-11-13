// GPU correctness regression for the built-in Mode A reconstruction fusion.
// Hardware opt-in: baseline is the pinned pre-fusion DXBC, not regenerated math.
#include "render/CustomShader.h"
#include <QTest>
#include <QFile>
#include <QTemporaryDir>
#include <qfloat16.h>
#include <cmath>
#include <dxgi1_2.h>
class TestAnime4KFusion final:public QObject {
    Q_OBJECT
private slots:
    void equivalent_data(){QTest::addColumn<QSize>("input");QTest::addColumn<QSize>("target");QTest::newRow("odd-2x")<<QSize(63,35)<<QSize(126,70);QTest::newRow("identity")<<QSize(64,36)<<QSize(64,36);QTest::newRow("1.5x")<<QSize(64,36)<<QSize(96,54);QTest::newRow("2.5x")<<QSize(64,36)<<QSize(160,90);QTest::newRow("4x")<<QSize(64,36)<<QSize(256,144);}
    void equivalent(){
        if(!qEnvironmentVariableIsSet("VSR_TEST_GPU"))QSKIP("Set VSR_TEST_GPU=1 for explicit bounded GPU correctness tests");
        QFETCH(QSize,input);QFETCH(QSize,target);
        const auto reference=QTest::qFindTestData("fixtures/anime4k-mode-a-reference.vsrhlsl",__FILE__,__LINE__);
        auto candidate=QTest::qFindTestData("../assets/builtin-shaders/anime4k-mode-a.glsl.vsrhlsl",__FILE__,__LINE__);QVERIFY(!reference.isEmpty()&&!candidate.isEmpty());if(qEnvironmentVariableIsSet("VSR_FUSION_REPEAT_BASE"))candidate=reference;
        Microsoft::WRL::ComPtr<IDXGIFactory1> factory;QVERIFY(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;for(UINT i=0;factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;i++){DXGI_ADAPTER_DESC1 d{};adapter->GetDesc1(&d);if(!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE) && (qEnvironmentVariableIntValue("VSR_TEST_AMD")==0 || d.VendorId==0x1002))break;adapter.Reset();}QVERIFY(adapter);DXGI_ADAPTER_DESC1 ad{};adapter->GetDesc1(&ad);qInfo()<<QString::fromWCharArray(ad.Description);Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;QVERIFY(SUCCEEDED(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
        std::vector<qfloat16> pixels(input.width()*input.height()*4);for(int y=0;y<input.height();y++)for(int x=0;x<input.width();x++)for(int c=0;c<4;c++)pixels[(y*input.width()+x)*4+c]=qfloat16(c==3?1.f:float((x*13+y*7+c*41)%251)/250.f);
        QByteArray results[2];int j=0;
        for(const auto &path:{reference,candidate}){
            // Pipelines share a context; isolate binding state and allow queued GPU work to settle.
            context->ClearState();vsr::shader::Pipeline pipeline;QVERIFY2(pipeline.load(path.toUtf8().constData(),0,0,device.Get()),pipeline.error().c_str());auto *rt=pipeline.input(device.Get(),input.width(),input.height());QVERIFY(rt);
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;rt->GetResource(&resource);context->UpdateSubresource(resource.Get(),0,nullptr,pixels.data(),input.width()*8,0);QVERIFY2(pipeline.process(context.Get(),target.width(),target.height()),pipeline.error().c_str());for(int warm=0;warm<8;warm++){QVERIFY(pipeline.process(context.Get(),target.width(),target.height()));context->Flush();QTest::qWait(10);}
            pipeline.output()->GetResource(resource.ReleaseAndGetAddressOf());D3D11_TEXTURE2D_DESC d{};d.Width=pipeline.width();d.Height=pipeline.height();d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;QVERIFY(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&staging)));context->CopyResource(staging.Get(),resource.Get());D3D11_MAPPED_SUBRESOURCE mapped{};QVERIFY(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));auto &bytes=results[j++];for(unsigned y=0;y<d.Height;y++)bytes.append(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch,d.Width*8);context->Unmap(staging.Get(),0);
        }
        QCOMPARE(results[0].size(),results[1].size());double maximum=0,mean=0,rgbMax=0,alphaMax=0,rgbMean=0;const auto *a=reinterpret_cast<const qfloat16*>(results[0].constData()),*b=reinterpret_cast<const qfloat16*>(results[1].constData());const int n=results[0].size()/2;for(int i=0;i<n;i++){const double delta=std::abs(float(a[i])-float(b[i]));QVERIFY(std::isfinite(delta));maximum=std::max(maximum,delta);mean+=delta;if(i%4!=3){rgbMax=std::max(rgbMax,delta);rgbMean+=delta;}else alphaMax=std::max(alphaMax,delta);}
        qInfo()<<"Fusion GPU error"<<maximum<<mean/n<<"RGB"<<rgbMax<<rgbMean/(n*.75)<<"alpha"<<alphaMax<<"bitwise"<<(results[0]==results[1]);QVERIFY(maximum<=1.0/1024);QVERIFY(mean/n<=0.00001);
    }
};
QTEST_GUILESS_MAIN(TestAnime4KFusion)
#include "TestAnime4KFusion.moc"
