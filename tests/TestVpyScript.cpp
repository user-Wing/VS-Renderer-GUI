#include "graph/FilterGraph.h"
#include "graph/VpyScriptBuilder.h"
#include "graph/FilterCatalog.h"
#include "graph/PresetStore.h"
#include "graph/HlslPreview.h"

#include <QTest>
#include <QFile>

using namespace vsr;

class TestVpyScript final : public QObject {
    Q_OBJECT
private slots:
    void hlslWithPostEnhancement(){
        FilterGraph graph;graph.add("crop");const int shader=graph.add("anime4k");graph.setParameter(shader,"shader","test.hlsl");graph.add("sharpen_edges");graph.add("enhance_detail");
        const auto plan=hlslPreviewPlan(graph);QVERIFY(plan.error.isEmpty());QCOMPARE(plan.shaderPath,QString("test.hlsl"));QCOMPARE(plan.enhancements.size(),2);QCOMPARE(plan.graph.nodes().size(),1);QCOMPARE(plan.graph.nodes().first().definitionId,QString("crop"));
        graph.add("resize");QVERIFY(hlslPreviewPlan(graph).shaderPath.isEmpty());
    }
    void rejectsInvalidParameterValues(){
        FilterGraph graph;const int row=graph.add("resize");
        QVERIFY(!graph.setParameter(row,"kernel","Bicubic(clip); __import__('os').system('bad') #"));
        QVERIFY(!graph.setParameter(row,"width",-1));QVERIFY(!graph.setParameter(row,"width",4.5));QVERIFY(!graph.setParameter(row,"width","nan"));
        QVERIFY(graph.setParameter(row,"width",640));QVERIFY(graph.setParameter(row,"kernel","Bicubic"));
        QCOMPARE(graph.nodes()[row].parameters.value("kernel").toString(),QString("Bicubic"));
    }
    void hlslTailPreservesOrderAndDimensions(){
        FilterGraph graph;graph.add("crop");const int shader=graph.add("anime4k");graph.setParameter(shader,"mode","anime4k-a-fast.glsl");
        auto plan=hlslPreviewPlan(graph);QVERIFY(!plan.shaderPath.isEmpty());QCOMPARE(plan.graph.nodes().size(),1);QCOMPARE(plan.graph.nodes().first().definitionId,QString("crop"));
        QCOMPARE(hlslOutputSize(plan.parameters,QSize(1920,1080)),QSize(3840,2160));
        graph.setParameter(shader,"output_mode","指定分辨率");graph.setParameter(shader,"dimension_axis","height");graph.setParameter(shader,"height",720);
        plan=hlslPreviewPlan(graph);QCOMPARE(hlslOutputSize(plan.parameters,QSize(1920,1080)),QSize(1280,720));
        graph.setParameter(shader,"keep_aspect",false);graph.setParameter(shader,"width",1000);plan=hlslPreviewPlan(graph);QVERIFY(!hlslAspectCompatible(plan.parameters,QSize(1920,1080),hlslOutputSize(plan.parameters,QSize(1920,1080))));
        graph.add("resize");plan=hlslPreviewPlan(graph);QVERIFY(plan.shaderPath.isEmpty());QVERIFY(!plan.error.isEmpty());QCOMPARE(plan.graph.nodes().size(),3);
        graph.setParameter(shader,"hlsl",false);plan=hlslPreviewPlan(graph);QVERIFY(plan.shaderPath.isEmpty());QVERIFY(plan.error.isEmpty());
    }
    void hlslRejectsModifiedPythonAndTimeGroups(){
        FilterGraph graph;graph.add("anime4k");FilterGraph checked;SourceFilter filter=SourceFilter::Lsmas;
        const auto script=PresetStore::create(graph,SourceFilter::Ffms2,{},{});
        QVERIFY(PresetStore::verifiedGraph(script,&checked,&filter));QCOMPARE(filter,SourceFilter::Ffms2);QCOMPARE(checked.nodes().size(),1);
        QVERIFY(!PresetStore::verifiedGraph(script+"\nclip = core.std.Transpose(clip)\n",&checked,&filter));
        const auto groups=PresetStore::create(QList<FilterTimeGroup>{{"All",0,graph}},SourceFilter::Ffms2,{},{});
        QVERIFY(PresetStore::verifiedGraph(groups,&checked,&filter));
        const auto timed=PresetStore::create(QList<FilterTimeGroup>{{"First",0,graph},{"Next",10,graph}},SourceFilter::Ffms2,{},{});
        QVERIFY(!PresetStore::verifiedGraph(timed,&checked,&filter));
        QVERIFY(!PresetStore::verifiedGraph("import vapoursynth as vs\nclip.set_output()\n",&checked,&filter));
    }
    void timeGroupBoundaries(){
        FilterGraph first,second;first.add("transpose");second.add("resize");
        const auto result=VpyScriptBuilder::buildGroups("input.mkv",SourceFilter::Ffms2,{{"Opening",0,first},{"Main",12,second}});
        QVERIFY(result.errors.isEmpty());QVERIFY(result.script.contains("def _vsr_group_0"));QVERIFY(result.script.contains("def _vsr_group_1"));
        QVERIFY(result.script.contains("12 * src.fps_num"));QVERIFY(result.script.contains("core.std.Splice"));
        QVERIFY(!VpyScriptBuilder::buildGroups("x",SourceFilter::Ffms2,{{"a",2,{}}}).errors.isEmpty());
        QVERIFY(!VpyScriptBuilder::buildGroups("x",SourceFilter::Ffms2,{{"a",0,{}},{"b",0,{}}}).errors.isEmpty());
    }
    void shaderCategoriesAndHardware() {
        QCOMPARE(FilterCatalog::shaderCategory("NVScaler.glsl"),QString("超分"));
        QCOMPARE(FilterCatalog::shaderCategory("NVSharpen.glsl"),QString("基础锐化"));
        QCOMPARE(FilterCatalog::shaderCategory("ArtCNN_C4F16_DS.glsl"),QString("超分"));
        QCOMPARE(FilterCatalog::shaderCategory("mpv/ds/CuNNy-4x16-DS.glsl"),QString("超分"));
        QCOMPARE(FilterCatalog::shaderCategory("SSimDownscaler.glsl"),QString("缩小"));
        QCOMPARE(FilterCatalog::shaderCategory("CfL_Prediction.glsl"),QString("色度重建"));
        QCOMPARE(FilterCatalog::shaderCategory("mpv-shaders/test/AA/SMAA.glsl"),QString("线条增强与抗锯齿"));
        QVERIFY(FilterCatalog::shaderHardwareLabel("NVScaler.glsl").contains("非 N 卡独占"));
        QCOMPARE(FilterCatalog::shaderHardwareLabel("mpv/dp4a/CuNNy-Q.glsl"),QString("需要 DP4A"));
        QVERIFY(FilterCatalog::shaderHardwareLabel("FSR.glsl").isEmpty());
    }
    void shaderOutputResolution() {
        FilterGraph graph;const int node=graph.add("anime4k");graph.setParameter(node,"output_mode","指定分辨率");graph.setParameter(node,"width",2560);graph.setParameter(node,"height",1440);
        auto script=VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph).script;
        QVERIFY(script.contains("width=2560, height=max(2, int(2560 * clip.height / clip.width + 0.5))"));
        QVERIFY(script.contains(".lstrip('\\ufeff')"));
        graph.setParameter(node,"dimension_axis","height");script=VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph).script;
        QVERIFY(script.contains("width=max(2, int(1440 * clip.width / clip.height + 0.5)), height=1440"));
        graph.setParameter(node,"keep_aspect",false);script=VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph).script;
        QVERIFY(script.contains("width=2560, height=1440"));
        graph.setParameter(node,"width",102);graph.setParameter(node,"height",60);
        for(const auto &mode:QStringList{"width","height","unlocked"}){
            graph.setParameter(node,"dimension_axis",mode);graph.setParameter(node,"keep_aspect",mode!="unlocked");
            QFile file(QCoreApplication::applicationDirPath()+"/shader-output-"+mode+"-1.0.4.vpy");QVERIFY(file.open(QIODevice::WriteOnly));file.write(VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph).script.toUtf8());
        }
    }
    void bundledMpvShaderAndDownscale() {
        const auto *definition=FilterCatalog::find("anime4k");QVERIFY(definition);
        QStringList choices;for(const auto &p:definition->parameters)if(p.id=="mode")choices=p.choices;
        QVERIFY(choices.contains("anime4k-a-fast.glsl"));QVERIFY(choices.contains("自定义 GLSL"));
        FilterGraph graph;const int node=graph.add("anime4k");QVERIFY(!graph.setParameter(node,"mode","mpv-shaders/gist--test/downscale.hook"));QVERIFY(graph.setParameter(node,"mode","自定义 GLSL"));QVERIFY(graph.setParameter(node,"shader","mpv-shaders/gist--test/downscale.hook"));graph.setParameter(node,"scale","0.5×");
        const auto result=VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph);QVERIFY(result.errors.isEmpty());QVERIFY(result.script.contains("mpv-shaders/gist--test/downscale.hook"));QVERIFY(result.script.contains("int(clip.width * 0.5)"));
    }
    void rejectsMissingSource()
    {
        FilterGraph graph;
        const auto result = VpyScriptBuilder::build({}, SourceFilter::Lsmas, graph);
        QVERIFY(!result.errors.isEmpty());
        QVERIFY(result.script.isEmpty());
    }

    void escapesWindowsPath()
    {
        QCOMPARE(VpyScriptBuilder::pythonString(QStringLiteral("C:\\video\\a\"b.mkv")),
                 QStringLiteral("\"C:\\\\video\\\\a\\\"b.mkv\""));
    }

    void gpuSharpenAndLegacyUpgrade() {
        FilterGraph graph;graph.add("sharpen_edges");graph.add("crispen_edges");graph.add("enhance_detail");const auto script=VpyScriptBuilder::build("input.mkv",SourceFilter::Ffms2,graph);
        QVERIFY(script.script.contains("def _vsr_sharpen_shader"));QVERIFY(script.script.contains("clip = _vsr_sharpen_chain(clip, [(\"sharpen_edges\", 0.5, 2), (\"crispen_edges\", 0.25, 0), (\"enhance_detail\", 0.3, 0)])"));QVERIFY(script.requiredNamespaces.contains("placebo"));
        const auto network=VpyScriptBuilder::build("https://example.org/video.mkv?token=value",SourceFilter::Lsmas,FilterGraph());QVERIFY(network.script.contains("def _vsr_network_source"));QVERIFY(network.script.contains("src = _vsr_network_source"));QVERIFY(network.script.contains("cache=False"));
    }

    void preservesOrderAndParameters()
    {
        FilterGraph graph;
        const int crop = graph.add(QStringLiteral("crop"));
        const int deband = graph.add(QStringLiteral("deband"));
        QVERIFY(crop >= 0);
        QVERIFY(deband >= 0);
        QVERIFY(graph.setParameter(crop, QStringLiteral("left"), 12));
        QVERIFY(graph.setParameter(deband, QStringLiteral("range"), 24));

        const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Lsmas, graph);
        QVERIFY2(result.errors.isEmpty(), qPrintable(result.errors.join('\n')));
        const int cropAt = result.script.indexOf(QStringLiteral("core.std.Crop"));
        const int debandAt = result.script.indexOf(QStringLiteral("core.vszip.Deband"));
        QVERIFY(cropAt > 0);
        QVERIFY(debandAt > cropAt);
        QVERIFY(result.script.contains(QStringLiteral("left=12")));
        QVERIFY(result.script.contains(QStringLiteral("range=24")));
        QVERIFY(result.script.contains(QStringLiteral("src.set_output(1)")));
        QVERIFY(result.script.contains(QStringLiteral("clip.set_output(0)")));
        QVERIFY(result.requiredNamespaces.contains(QStringLiteral("vszip")));
        QVERIFY(result.requiredNamespaces.contains(QStringLiteral("fmtc")));
    }

    void skipsDisabledNodes()
    {
        FilterGraph graph;
        const int row = graph.add(QStringLiteral("cas"));
        QVERIFY(graph.setEnabled(row, false));
        const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Ffms2, graph);
        QVERIFY(!result.script.contains(QStringLiteral("core.cas.CAS")));
        QVERIFY(!result.requiredNamespaces.contains(QStringLiteral("cas")));
    }

    void mapsGuidePlugins()
    {
        struct PluginCase {
            const char *id;
            const char *pluginNamespace;
            const char *scriptFragment;
        };
        const PluginCase cases[] = {
            {"ttempsmooth", "zsmooth", "core.zsmooth.TTempSmooth"},
            {"deblock", "deblock", "core.deblock.Deblock"},
            {"znedi3", "znedi3", "core.znedi3.nnedi3"},
            {"eedi3", "eedi3m", "core.eedi3m.EEDI3"},
            {"sangnom", "sangnom", "core.sangnom.SangNom"},
            {"bwdif", "bwdif", "core.bwdif.Bwdif"},
            {"vivtc", "vivtc", "core.vivtc.VFM"},
            {"anime4k", "placebo", "getattr(core, 'vsrplacebo', core.placebo).Shader"},
        };

        for (const auto &plugin : cases) {
            FilterGraph graph;
            QVERIFY2(graph.add(QString::fromLatin1(plugin.id)) >= 0, plugin.id);
            const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Lsmas, graph);
            QVERIFY2(result.errors.isEmpty(), qPrintable(result.errors.join('\n')));
            QVERIFY2(result.script.contains(QString::fromLatin1(plugin.scriptFragment)), plugin.scriptFragment);
            QVERIFY2(result.requiredNamespaces.contains(QString::fromLatin1(plugin.pluginNamespace)), plugin.pluginNamespace);
        }

        FilterGraph ivtc;
        QVERIFY(ivtc.add(QStringLiteral("vivtc")) >= 0);
        const auto ivtcResult = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Lsmas, ivtc);
        QVERIFY(ivtcResult.script.contains(QStringLiteral("core.vivtc.VDecimate")));
    }

    void convertsFriendlyChoicesToPluginValues()
    {
        FilterGraph graph;
        const int nnedi = graph.add(QStringLiteral("znedi3"));
        QVERIFY(nnedi >= 0);
        QVERIFY(graph.setParameter(nnedi, QStringLiteral("mode"), QStringLiteral("4 - 纵向 2×，保留下场")));
        QVERIFY(graph.setParameter(nnedi, QStringLiteral("nns"), QStringLiteral("4 - 256")));

        const int smooth = graph.add(QStringLiteral("ttempsmooth"));
        QVERIFY(smooth >= 0);
        QVERIFY(graph.setParameter(smooth, QStringLiteral("thresh_y"), 6));
        QVERIFY(graph.setParameter(smooth, QStringLiteral("thresh_c"), 9));

        const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Lsmas, graph);
        QVERIFY2(result.errors.isEmpty(), qPrintable(result.errors.join('\n')));
        QVERIFY(result.script.contains(QStringLiteral("field=0, dh=True")));
        QVERIFY(result.script.contains(QStringLiteral("width=clip.width * 2")));
        QVERIFY(result.script.contains(QStringLiteral("nns=4")));
        QVERIFY(result.script.contains(QStringLiteral("thresh=[6, 9, 9]")));
    }

    void anime4kUsesShaderPathAndUniformScale()
    {
        FilterGraph graph;
        const int row = graph.add(QStringLiteral("anime4k"));
        QVERIFY(row >= 0);
        QVERIFY(graph.setParameter(row, QStringLiteral("shader"), QStringLiteral("D:\\Shaders\\Anime4K.glsl")));
        QVERIFY(graph.setParameter(row, QStringLiteral("scale"), QStringLiteral("3×")));

        const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\src.mkv"), SourceFilter::Lsmas, graph);
        QVERIFY2(result.errors.isEmpty(), qPrintable(result.errors.join('\n')));
        QVERIFY(result.requiredNamespaces.contains(QStringLiteral("placebo")));
        QVERIFY(result.script.contains(QStringLiteral("clip.format.replace(bits_per_sample=16, sample_type=vs.INTEGER)")));
        QVERIFY(!result.script.contains(QStringLiteral("format=vs.YUV420P16")));
        QVERIFY(result.script.contains(QStringLiteral("_shader_path = \"D:\\\\Shaders\\\\Anime4K.glsl\"")));
        QVERIFY(result.script.contains(QStringLiteral("open(_shader_path, encoding='utf-8')")));
        QVERIFY(result.script.contains(QStringLiteral("width=max(2, int(clip.width * 3)), height=max(2, int(clip.height * 3))")));
    }

    void interpolationMapsParametersAndKeepsOriginalBranch()
    {
        FilterGraph graph;
        const int mv = graph.add(QStringLiteral("mvtools"));
        QVERIFY(mv >= 0);
        QVERIFY(graph.setParameter(mv, QStringLiteral("block"), QStringLiteral("32")));
        QVERIFY(graph.setParameter(mv, QStringLiteral("overlap"), true));
        const int rife = graph.add(QStringLiteral("rife"));
        QVERIFY(rife >= 0);
        QVERIFY(graph.setParameter(rife, QStringLiteral("inference_scale"), QStringLiteral("2 - 半宽半高")));
        const auto result = VpyScriptBuilder::build(QStringLiteral("D:\\source.mkv"), SourceFilter::Lsmas, graph);
        QVERIFY(result.errors.isEmpty());
        QVERIFY(result.requiredNamespaces.contains(QStringLiteral("mv")));
        QVERIFY(result.requiredNamespaces.contains(QStringLiteral("rife")));
        QVERIFY(result.script.contains(QStringLiteral("block=32, pel=1, overlap=True")));
        QVERIFY(result.script.contains(QStringLiteral("core.mv.Super(work, pel=pel, chroma=True)")));
        QVERIFY(result.script.contains(QStringLiteral("scene=True, scale=2")));
        QVERIFY(result.script.contains(QStringLiteral("branches = [c]")));
        QVERIFY(result.script.contains(QStringLiteral("middle[:-1] + c[-1]")));
    }
};

QTEST_GUILESS_MAIN(TestVpyScript)

#include "TestVpyScript.moc"
