#include "graph/VpyScriptBuilder.h"

#include "graph/FilterCatalog.h"
#include "graph/FilterGraph.h"

#include <QLocale>
#include <QSet>

namespace vsr {
namespace {

QString number(const QVariantMap &p, const QString &key)
{
    return QString::number(p.value(key).toInt());
}

QString realNumber(const QVariantMap &p, const QString &key)
{
    return QLocale::c().toString(p.value(key).toDouble(), 'g', 12);
}

QString booleanValue(const QVariantMap &p, const QString &key)
{
    return p.value(key).toBool() ? QStringLiteral("True") : QStringLiteral("False");
}

int choiceCode(const QVariantMap &p, const QString &key)
{
    return p.value(key).toString().section(' ', 0, 0).toInt();
}

QString choiceNumber(const QVariantMap &p, const QString &key)
{
    return QString::number(choiceCode(p, key));
}

QString interpolationField(const QVariantMap &p)
{
    const int mode = choiceCode(p, QStringLiteral("mode"));
    return QString::number(mode < 4 ? mode : mode - 4);
}

QString interpolationDoubleHeight(const QVariantMap &p)
{
    return choiceCode(p, QStringLiteral("mode")) >= 4 ? QStringLiteral("True") : QStringLiteral("False");
}

QString emitNode(const FilterNode &node)
{
    const auto &p = node.parameters;
    if (node.definitionId == "trim")
        return QString("clip = core.std.Trim(clip, first=%1, last=%2)").arg(number(p, "first"), number(p, "last"));
    if (node.definitionId == "crop")
        return QString("clip = core.std.Crop(clip, left=%1, right=%2, top=%3, bottom=%4)")
            .arg(number(p, "left"), number(p, "right"), number(p, "top"), number(p, "bottom"));
    if (node.definitionId == "transpose")
        return QStringLiteral("clip = core.std.Transpose(clip)");
    if (node.definitionId == "assume_fps")
        return QString("clip = core.std.AssumeFPS(clip, fpsnum=%1, fpsden=%2)")
            .arg(number(p, "fpsnum"), number(p, "fpsden"));
    if (node.definitionId == "depth") {
        const int dmode = p.value("dmode_ordered").toBool() ? 1 : 3;
        return QString("clip = core.fmtc.bitdepth(clip, bits=%1, dmode=%2)")
            .arg(p.value("bits").toString()).arg(dmode);
    }
    if (node.definitionId == "resize") {
        const QString kernel = p.value("kernel").toString();
        const QString suffix = kernel == "Lanczos" ? QString(", taps=%1").arg(number(p, "taps")) : QString();
        return QString("clip = core.resize.%1(clip, width=%2, height=%3%4)")
            .arg(kernel, number(p, "width"), number(p, "height"), suffix);
    }
    if (node.definitionId == "remove_grain")
        return QString("clip = core.rgvs.RemoveGrain(clip, mode=%1)").arg(number(p, "mode"));
    if (node.definitionId == "bilateral")
        return QString("clip = core.vszip.Bilateral(clip, sigmaS=%1, sigmaR=%2)")
            .arg(realNumber(p, "sigmaS"), realNumber(p, "sigmaR"));
    if (node.definitionId == "nlmeans")
        return QString("clip = core.nlm_ispc.NLMeans(clip, d=%1, wmode=%2, h=%3)")
            .arg(number(p, "d"), number(p, "wmode"), number(p, "h"));
    if (node.definitionId == "ttempsmooth")
        return QString("clip = core.zsmooth.TTempSmooth(clip, maxr=%1, thresh=[%2, %3, %3], mdiff=[%4, %5, %5], strength=%6, scthresh=0.0, fp=%7)")
            .arg(number(p, "maxr"), number(p, "thresh_y"), number(p, "thresh_c"), number(p, "mdiff_y"),
                 number(p, "mdiff_c"), number(p, "strength"), booleanValue(p, "fp"));
    if (node.definitionId == "deband")
        return QString("clip = core.vszip.Deband(clip, range=%1, thr=[%2 / 255.0, %3 / 255.0, %4 / 255.0], grain=[%5 / 255.0, %6 / 255.0])\n"
                       "clip = core.fmtc.bitdepth(clip, bits=%7, dmode=3)")
            .arg(number(p, "range"), number(p, "y"), number(p, "cb"), number(p, "cr"),
                 number(p, "grainy"), number(p, "grainc"), p.value("output_depth").toString());
    if (node.definitionId == "deblock")
        return QString("clip = core.deblock.Deblock(clip, quant=%1, aoffset=%2, boffset=%3)")
            .arg(number(p, "quant"), number(p, "aoffset"), number(p, "boffset"));
    if (node.definitionId == "cas")
        return QString("clip = core.cas.CAS(clip, sharpness=%1)").arg(realNumber(p, "sharpness"));
    if (node.definitionId == "znedi3")
        return QString("clip = core.znedi3.nnedi3(clip, field=%1, dh=%2, nsize=%3, nns=%4, qual=%5, pscrn=%6)")
            .arg(interpolationField(p), interpolationDoubleHeight(p), choiceNumber(p, "nsize"),
                 choiceNumber(p, "nns"), choiceNumber(p, "qual"), choiceNumber(p, "pscrn"));
    if (node.definitionId == "eedi3")
        return QString("clip = core.eedi3m.EEDI3(clip, field=%1, dh=%2, alpha=%3, beta=%4, gamma=%5, nrad=%6, mdis=%7)")
            .arg(interpolationField(p), interpolationDoubleHeight(p), realNumber(p, "alpha"), realNumber(p, "beta"),
                 realNumber(p, "gamma"), number(p, "nrad"), number(p, "mdis"));
    if (node.definitionId == "sangnom")
        return QString("clip = core.sangnom.SangNom(clip, order=%1, dh=%2, aa=[%3, %4, %4])")
            .arg(choiceNumber(p, "order"), booleanValue(p, "dh"), number(p, "aa_y"), number(p, "aa_c"));
    if (node.definitionId == "bwdif")
        return QString("clip = core.bwdif.Bwdif(clip, field=%1)").arg(choiceNumber(p, "field"));
    if (node.definitionId == "vivtc")
        return QString("clip = core.vivtc.VFM(clip, order=%1, field=2, mode=%2, mchroma=%3, cthresh=%4, mi=%5)\n"
                       "clip = core.vivtc.VDecimate(clip, cycle=%6, dupthresh=%7)")
            .arg(choiceNumber(p, "order"), choiceNumber(p, "mode"), booleanValue(p, "mchroma"),
                 number(p, "cthresh"), number(p, "mi"), number(p, "cycle"), realNumber(p, "dupthresh"));
    if (node.definitionId == "grain_add")
        return QString("clip = core.grain.Add(clip, var=%1, constant=%2)")
            .arg(realNumber(p, "var"), booleanValue(p, "constant"));
    return {};
}

}

ScriptBuildResult VpyScriptBuilder::build(const QString &sourcePath, SourceFilter sourceFilter,
                                          const FilterGraph &graph)
{
    ScriptBuildResult result;
    if (sourcePath.trimmed().isEmpty()) {
        result.errors.append(QStringLiteral("尚未选择源视频。"));
        return result;
    }

    QSet<QString> namespaces;
    namespaces.insert(sourceFilter == SourceFilter::Lsmas ? QStringLiteral("lsmas") : QStringLiteral("ffms2"));

    QStringList body;
    body << QStringLiteral("# Generated by VS Renderer. Edit the graph instead of this cache file.")
         << QStringLiteral("import vapoursynth as vs")
         << QStringLiteral("core = vs.core")
         << QString();

    const QString path = pythonString(sourcePath);
    if (sourceFilter == SourceFilter::Lsmas)
        body << QString("src = core.lsmas.LWLibavSource(source=%1)").arg(path);
    else
        body << QString("src = core.ffms2.Source(source=%1)").arg(path);
    body << QStringLiteral("clip = src");

    int emitted = 0;
    for (const auto &node : graph.nodes()) {
        if (!node.enabled)
            continue;
        const auto *definition = FilterCatalog::find(node.definitionId);
        if (!definition) {
            result.errors.append(QString("未知滤镜节点：%1").arg(node.definitionId));
            continue;
        }
        namespaces.insert(definition->pluginNamespace);
        if (node.definitionId == QStringLiteral("deband"))
            namespaces.insert(QStringLiteral("fmtc"));
        const QString line = emitNode(node);
        if (line.isEmpty()) {
            result.errors.append(QString("节点尚无脚本映射：%1").arg(definition->name));
            continue;
        }
        body << QString() << QString("# %1. %2").arg(++emitted).arg(definition->name) << line;
    }

    body << QString() << QStringLiteral("src.set_output(1)") << QStringLiteral("clip.set_output(0)");
    result.requiredNamespaces = namespaces.values();
    result.requiredNamespaces.sort(Qt::CaseInsensitive);
    body.insert(1, QString("# Required namespaces: %1").arg(result.requiredNamespaces.join(", ")));
    result.script = body.join('\n') + '\n';
    return result;
}

QString VpyScriptBuilder::pythonString(const QString &value)
{
    QString escaped;
    escaped.reserve(value.size() + 8);
    for (const QChar ch : value) {
        switch (ch.unicode()) {
        case '\\': escaped += QStringLiteral("\\\\"); break;
        case '"': escaped += QStringLiteral("\\\""); break;
        case '\n': escaped += QStringLiteral("\\n"); break;
        case '\r': escaped += QStringLiteral("\\r"); break;
        case '\t': escaped += QStringLiteral("\\t"); break;
        default: escaped += ch; break;
        }
    }
    return '"' + escaped + '"';
}

}
