#include "render/ShaderPackage.h"
#include <QTest>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QProcess>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

using namespace vsr;
namespace {
bool writeFile(const QString &path,const QByteArray &data) {
    QFile file(path);return file.open(QIODevice::WriteOnly) && file.write(data)==data.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();
}
const QByteArray identity=R"(Texture2D<float4> image:register(t0);SamplerState sampleState:register(s0);
float4 main(float2 uv:TEXCOORD0):SV_Target{return image.Sample(sampleState,uv);})";
}

class TestShaderPackage final : public QObject {
    Q_OBJECT
private slots:
    void bundledArchiveUsesPrecompiledPackage(){
        QTemporaryDir directory;QDir().mkpath(directory.filePath("mpv-shaders"));const auto source=directory.filePath("mpv-shaders/identity.hlsl");QVERIFY(writeFile(source,identity));
        const auto compiled=ShaderPackage::prepare(source,directory.filePath("compile"));QVERIFY2(compiled.error.isEmpty(),qPrintable(compiled.error));const auto bytes=readFile(compiled.packagePath);QVERIFY(writeFile(source+".vsrhlsl",bytes));
        QVERIFY(writeFile(source+".vsrhlsl.json",QJsonDocument(QJsonObject{{"semantics","luma-main-prekernel-v7"},{"sha256",QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex())}}).toJson()));
        QProcess archive;archive.setWorkingDirectory(directory.path());archive.start(QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/7z.exe"),{"a","mpv-shaders.7z","mpv-shaders","-bso0"});QVERIFY(archive.waitForFinished(30000));QCOMPARE(archive.exitCode(),0);
        for(const auto &suffix:QStringList{"",".vsrhlsl",".vsrhlsl.json"})QVERIFY(QFile::remove(source+suffix));
        const auto loaded=ShaderPackage::prepare(source,directory.filePath("import"));QVERIFY2(loaded.error.isEmpty(),qPrintable(loaded.error));QVERIFY(loaded.packagePath.contains("/bundled/"));QCOMPARE(readFile(loaded.packagePath),bytes);
    }
    void nativeMultipassHlsl() {
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const auto source=directory.filePath("multipass.hlsl");
        QVERIFY(writeFile(source,R"(//!HOOK MAIN
//!BIND MAIN
//!SAVE FEATURES
Texture2D<float4> image:register(t0);SamplerState s:register(s0);
float4 main(float2 uv:TEXCOORD0):SV_Target{return image.SampleLevel(s,uv,0)*0.5;}
//!HOOK MAIN
//!BIND FEATURES
//!WIDTH MAIN.w 2 *
//!HEIGHT MAIN.h 2 *
Texture2D<float4> image:register(t0);SamplerState s:register(s0);
float4 main(float2 uv:TEXCOORD0):SV_Target{return image.SampleLevel(s,uv,0)*2;}
)"));
        const auto result=ShaderPackage::prepare(source,directory.filePath("cache"));
        QVERIFY2(result.error.isEmpty(),qPrintable(result.error));
        const auto bytes=readFile(result.packagePath);QVERIFY(bytes.startsWith("VSRHLSL1"));
        QCOMPARE(bytes.mid(8,4),QByteArray::fromHex("02000000"));
    }
    void cacheReuseInvalidationAndRepair() {
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const auto source=directory.filePath("自定义 shader.hlsl"),cache=directory.filePath("cache");
        QVERIFY(writeFile(source,identity));const auto first=ShaderPackage::prepare(source,cache);
        QVERIFY2(first.error.isEmpty(),qPrintable(first.error));QVERIFY(!first.packagePath.isEmpty());
        const auto original=readFile(first.packagePath);QVERIFY(original.startsWith("VSRHLSL1"));
        const auto modified=QFileInfo(first.packagePath).lastModified();QTest::qWait(20);
        const auto again=ShaderPackage::prepare(source,cache);QCOMPARE(again.packagePath,first.packagePath);
        QCOMPARE(QFileInfo(again.packagePath).lastModified(),modified);
        QVERIFY(writeFile(source,identity+"\n// changed source\n"));
        const auto changed=ShaderPackage::prepare(source,cache);QVERIFY2(changed.error.isEmpty(),qPrintable(changed.error));
        QVERIFY(changed.packagePath!=first.packagePath);
        QVERIFY(writeFile(source,identity));QVERIFY(writeFile(first.packagePath,original.left(13)));
        const auto repaired=ShaderPackage::prepare(source,cache);QVERIFY2(repaired.error.isEmpty(),qPrintable(repaired.error));
        QCOMPARE(repaired.packagePath,first.packagePath);QCOMPARE(readFile(repaired.packagePath),original);
    }
    void glslMainPrekernelAndComponents() {
        QTemporaryDir directory;QVERIFY(directory.isValid());const auto source=directory.filePath("multipass.glsl");
        QVERIFY(writeFile(source,R"(//!HOOK MAIN
//!BIND HOOKED
//!SAVE MONO
//!COMPONENTS 1
//!WIDTH HOOKED.width
//!HEIGHT HOOKED.height
//!WHEN OUTPUT.w 0 >
vec4 hook(){return vec4(HOOKED_tex(HOOKED_pos).r,0.7,0.8,0.4);}
//!HOOK PREKERNEL
//!BIND MONO
//!SAVE PREKERNEL
vec4 hook(){return vec4(MONO_tex(MONO_pos).rgb,1);}
)"));
        const auto result=ShaderPackage::prepare(source,directory.filePath("cache"));
        QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY(!result.packagePath.isEmpty());
    }
    void bundledAnimeShadersCompile() {
        QTemporaryDir directory;QVERIFY(directory.isValid());
        for(const auto &name:{"../assets/anime4k-a-fast.glsl","../assets/anime4k-no-cnn.glsl"}){
            const auto path=QTest::qFindTestData(name,__FILE__,__LINE__);QVERIFY(!path.isEmpty());
            const auto result=ShaderPackage::prepare(path,directory.filePath("cache"));
            QVERIFY2(result.error.isEmpty(),qPrintable(result.error));QVERIFY(!result.packagePath.isEmpty());
        }
    }
    void rejectsUnsupportedContracts_data() {
        QTest::addColumn<QByteArray>("source");QTest::addColumn<QString>("extension");QTest::addColumn<QString>("reason");
        const QByteArray shader="//!HOOK MAIN\n//!BIND HOOKED\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}\n";
        auto luma=shader;luma.replace("HOOK MAIN","HOOK CHROMA");QTest::newRow("chroma")<<luma<<QString("glsl")<<QString("CHROMA");
        QTest::newRow("compute")<<(QByteArray("//!COMPUTE 8 8\n")+shader)<<QString("glsl")<<QString("COMPUTE");
        QTest::newRow("offset")<<(QByteArray("//!OFFSET 1 0\n")+shader)<<QString("glsl")<<QString("OFFSET");
        QTest::newRow("native-pixels")<<QByteArray("//!HOOK MAIN\n//!BIND NATIVE\nvec4 hook(){return NATIVE_tex(NATIVE_pos);}\n")<<QString("glsl")<<QString("NATIVE");
        QTest::newRow("include")<<(QByteArray("#include \"extra.hlsl\"\n")+identity)<<QString("hlsl")<<QString("#include");
        QTest::newRow("uint-texture")<<QByteArray(
            "Texture2D<uint4> image:register(t0);float4 main(float2 uv:TEXCOORD0):SV_Target{return float4(image.Load(int3(uv*64,0)));}")
            <<QString("hlsl")<<QString("bindings");
        QTest::newRow("wrong-constants")<<QByteArray(
            "cbuffer Sizes:register(b0){float4 sourceSize;uint4 outputSize;};float4 main(float2 uv:TEXCOORD0):SV_Target{return sourceSize+float4(outputSize);}")
            <<QString("hlsl")<<QString("bindings");
        QTest::newRow("comparison-sampler")<<QByteArray(
            "Texture2D<float> image:register(t0);SamplerComparisonState s:register(s0);float4 main(float2 uv:TEXCOORD0):SV_Target{return image.SampleCmpLevelZero(s,uv,0.5).xxxx;}")
            <<QString("hlsl")<<QString("bindings");
    }
    void rejectsUnsupportedContracts() {
        QFETCH(QByteArray,source);QFETCH(QString,extension);QFETCH(QString,reason);
        QTemporaryDir directory;QVERIFY(directory.isValid());const auto path=directory.filePath("unsupported."+extension);
        QVERIFY(writeFile(path,source));const auto result=ShaderPackage::prepare(path,directory.filePath("cache"));
        QVERIFY(result.packagePath.isEmpty());QVERIFY2(result.error.contains(reason),qPrintable(result.error));
    }
};
QTEST_GUILESS_MAIN(TestShaderPackage)
#include "TestShaderPackage.moc"
