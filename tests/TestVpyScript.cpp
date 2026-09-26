#include "graph/FilterGraph.h"
#include "graph/VpyScriptBuilder.h"

#include <QTest>

using namespace vsr;

class TestVpyScript final : public QObject {
    Q_OBJECT
private slots:
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
            {"anime4k", "placebo", "core.placebo.Shader"},
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
        QVERIFY(result.script.contains(QStringLiteral("format=vs.YUV420P16")));
        QVERIFY(result.script.contains(QStringLiteral("shader=\"D:\\\\Shaders\\\\Anime4K.glsl\"")));
        QVERIFY(result.script.contains(QStringLiteral("width=clip.width * 3, height=clip.height * 3")));
    }
};

QTEST_APPLESS_MAIN(TestVpyScript)

#include "TestVpyScript.moc"
