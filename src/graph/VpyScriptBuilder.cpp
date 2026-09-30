#include "graph/VpyScriptBuilder.h"

#include "graph/FilterCatalog.h"
#include "graph/FilterGraph.h"

#include <QLocale>
#include <QSet>

#include <algorithm>

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

QString preserveDoubleHeightAspect(const QVariantMap &p)
{
    return choiceCode(p, QStringLiteral("mode")) >= 4
        ? QStringLiteral("\nclip = core.resize.Spline36(clip, width=clip.width * 2, height=clip.height)")
        : QString();
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
    if (node.definitionId == "anime4k") {
        const int scale = std::clamp(p.value("scale").toString().section(QChar(0x00d7), 0, 0).toInt(), 1, 4);
        return QString("clip = core.resize.Spline36(clip, format=vs.YUV420P16)\n"
                       "clip = core.placebo.Shader(clip, shader=%1, width=clip.width * %2, height=clip.height * %2)")
            .arg(VpyScriptBuilder::pythonString(p.value("shader").toString())).arg(scale);
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
    if (node.definitionId == "dering")
        return QString("_blur = core.std.Convolution(clip, matrix=[1,2,1,2,4,2,1,2,1])\n"
                       "_lo = core.std.Minimum(_blur)\n_hi = core.std.Maximum(_blur)\n"
                       "_limited = core.std.Expr([clip, _lo, _hi], expr='x y max z min')\n"
                       "clip = core.std.Merge(clip, _limited, weight=%1)").arg(realNumber(p, "strength"));
    if (node.definitionId == "thin_edges")
        return QString("_thin = core.std.Maximum(clip, planes=[0])\n"
                       "clip = core.std.Merge(clip, _thin, weight=[%1, 0, 0] if clip.format.num_planes == 3 else [%1])")
            .arg(realNumber(p, "strength"));
    if (node.definitionId == "sharpen_edges" || node.definitionId == "crispen_edges" || node.definitionId == "enhance_detail") {
        QString expression;
        if (node.definitionId == "sharpen_edges")
            expression = QString("x y - abs %1 {scale} * > x x y - %2 * + x ?")
                .arg(realNumber(p, "threshold"), realNumber(p, "strength"));
        else if (node.definitionId == "crispen_edges")
            expression = QString("x x y - %1 * +").arg(realNumber(p, "strength"));
        else
            expression = QString("x x y - y z - 0.5 * + %1 * +").arg(realNumber(p, "strength"));
        const QString matrix = node.definitionId == "crispen_edges"
            ? QStringLiteral("[0,1,0,1,4,1,0,1,0]") : QStringLiteral("[1,2,1,2,4,2,1,2,1]");
        return QString("_blur = core.std.Convolution(clip, matrix=%1, planes=[0])\n"
                       "_wide = core.std.Convolution(_blur, matrix=[1,2,1,2,4,2,1,2,1], planes=[0])\n"
                       "_scale = 1 / 255 if clip.format.sample_type == vs.FLOAT else (1 << max(0, clip.format.bits_per_sample - 8))\n"
                       "_expr = %2.replace('{scale}', str(_scale))\n"
                       "_sharp = core.std.Expr([clip, _blur, _wide], expr=[_expr, '', ''] if clip.format.num_planes == 3 else [_expr])\n"
                       "_lo = core.std.Minimum(clip, planes=[0])\n_hi = core.std.Maximum(clip, planes=[0])\n"
                       "clip = core.std.Expr([_sharp, _lo, _hi], expr=['x y max z min', '', ''] if clip.format.num_planes == 3 else ['x y max z min'])")
            .arg(matrix, VpyScriptBuilder::pythonString(expression));
    }
    if (node.definitionId == "cas")
        return QString("clip = core.cas.CAS(clip, sharpness=%1)").arg(realNumber(p, "sharpness"));
    if (node.definitionId == "znedi3")
        return QString("clip = core.znedi3.nnedi3(clip, field=%1, dh=%2, nsize=%3, nns=%4, qual=%5, pscrn=%6)%7")
            .arg(interpolationField(p), interpolationDoubleHeight(p), choiceNumber(p, "nsize"),
                 choiceNumber(p, "nns"), choiceNumber(p, "qual"), choiceNumber(p, "pscrn"),
                 preserveDoubleHeightAspect(p));
    if (node.definitionId == "eedi3")
        return QString("clip = core.eedi3m.EEDI3(clip, field=%1, dh=%2, alpha=%3, beta=%4, gamma=%5, nrad=%6, mdis=%7)%8")
            .arg(interpolationField(p), interpolationDoubleHeight(p), realNumber(p, "alpha"), realNumber(p, "beta"),
                 realNumber(p, "gamma"), number(p, "nrad"), number(p, "mdis"), preserveDoubleHeightAspect(p));
    if (node.definitionId == "sangnom")
        return QString("clip = core.sangnom.SangNom(clip, order=%1, dh=%2, aa=[%3, %4, %4])%5")
            .arg(choiceNumber(p, "order"), booleanValue(p, "dh"), number(p, "aa_y"), number(p, "aa_c"),
                 p.value("dh").toBool()
                    ? QStringLiteral("\nclip = core.resize.Spline36(clip, width=clip.width * 2, height=clip.height)")
                    : QString());
    if (node.definitionId == "bwdif")
        return QString("clip = core.bwdif.Bwdif(clip, field=%1)").arg(choiceNumber(p, "field"));
    if (node.definitionId == "vivtc")
        return QString("clip = core.vivtc.VFM(clip, order=%1, field=2, mode=%2, mchroma=%3, cthresh=%4, mi=%5)\n"
                       "clip = core.vivtc.VDecimate(clip, cycle=%6, dupthresh=%7)")
            .arg(choiceNumber(p, "order"), choiceNumber(p, "mode"), booleanValue(p, "mchroma"),
                 number(p, "cthresh"), number(p, "mi"), number(p, "cycle"), realNumber(p, "dupthresh"));
    if (node.definitionId == "temporal_median")
        return QString("clip = core.zsmooth.TemporalMedian(clip, radius=%1, scenechange=True)").arg(number(p,"radius"));
    if (node.definitionId == "flux_t")
        return QString("clip = core.zsmooth.FluxSmoothT(clip, temporal_threshold=[%1], scalep=True)").arg(realNumber(p,"threshold"));
    if (node.definitionId == "flux_st")
        return QString("clip = core.zsmooth.FluxSmoothST(clip, temporal_threshold=[%1], spatial_threshold=[%2], scalep=True)").arg(realNumber(p,"temporal"),realNumber(p,"spatial"));
    if (node.definitionId == "smart_median")
        return QString("clip = core.zsmooth.SmartMedian(clip, radius=[%1], threshold=[%2], scalep=True)").arg(number(p,"radius"),realNumber(p,"threshold"));
    if (node.definitionId == "iq_mean")
        return QString("clip = core.zsmooth.InterQuartileMean(clip, radius=[%1])").arg(number(p,"radius"));
    if (node.definitionId == "degrain_median")
        return QString("clip = core.zsmooth.DegrainMedian(clip, limit=[%1], mode=[%2], scalep=True)").arg(realNumber(p,"limit"),number(p,"mode"));
    if (node.definitionId == "cnr4")
        return QString("clip = core.zsmooth.Cnr4(clip, radius=%1, str=[0,%2,%2])").arg(number(p,"radius"),number(p,"strength"));
    if (node.definitionId == "ccd")
        return QString("clip = core.resize.Bicubic(clip, format=vs.RGBS, matrix_in_s='709')\n"
                       "clip = core.zsmooth.CCD(clip, threshold=%1, temporal_radius=%2, scale=max(1.0, clip.height / 240.0))\n"
                       "clip = core.resize.Bicubic(clip, format=vs.YUV444P16, matrix_s='709')").arg(realNumber(p,"threshold"),number(p,"radius"));
    if (node.definitionId == "dct_filter")
        return QString("clip = core.zsmooth.DCTFilter(clip, factors=[1,1,1,1,1,%1,%1,%1])").arg(realNumber(p,"high"));
    if (node.definitionId == "temporal_soften")
        return QString("clip = core.zsmooth.TemporalSoften(clip, radius=%1, threshold=[%2], scenechange=0, scalep=True)").arg(number(p,"radius"),realNumber(p,"threshold"));
    if (node.definitionId == "vertical_cleaner")
        return QString("clip = core.zsmooth.VerticalCleaner(clip, mode=[%1])").arg(choiceNumber(p,"mode"));
    if (node.definitionId == "clahe")
        return QString("_clahe_src = core.resize.Point(clip, format=vs.YUV444P8)\n"
                       "_clahe_y = core.std.ShufflePlanes(_clahe_src, planes=0, colorfamily=vs.GRAY)\n"
                       "_clahe_y = core.vszip.CLAHE(_clahe_y, limit=%1, tiles=[%2,%2])\n"
                       "clip = core.std.ShufflePlanes([_clahe_y,_clahe_src], planes=[0,1,2], colorfamily=vs.YUV)").arg(number(p,"limit"),number(p,"tiles"));
    if (node.definitionId == "descale")
        return QString("_descale_y = core.std.ShufflePlanes(clip, planes=0, colorfamily=vs.GRAY)\n"
                       "_descale_y = core.resize.Point(_descale_y, format=vs.GRAYS)\n"
                       "_descale_y = core.descale.%1(_descale_y, width=%2, height=%3)\n"
                       "_descale_y = core.resize.Point(_descale_y, format=vs.GRAY16)\n"
                       "_descale_uv = core.resize.Spline36(clip, width=%2, height=%3, format=vs.YUV444P16)\n"
                       "clip = core.std.ShufflePlanes([_descale_y,_descale_uv], planes=[0,1,2], colorfamily=vs.YUV)").arg(p.value("kernel").toString(),number(p,"width"),number(p,"height"));
    if (node.definitionId == "rife")
        return QString("import os\n"
                       "_model = os.path.join(os.path.dirname(vs.__file__), 'plugins', 'models', %1)\n"
                       "clip = core.resize.Bicubic(clip, format=vs.RGBS, matrix_in_s='709')\n"
                       "clip = core.rife.RIFE(clip, model_path=_model, factor_num=%2, gpu_id=%3, gpu_thread=%4, sc=%5)\n"
                       "clip = core.resize.Bicubic(clip, format=vs.YUV444P16, matrix_s='709')")
            .arg(VpyScriptBuilder::pythonString(p.value("model").toString()=="4.26 Heavy"?"rife-v4.26-heavy":"rife-v4.26"),
                 number(p,"factor"),number(p,"gpu"),number(p,"threads"),booleanValue(p,"scene"));
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
    body << QStringLiteral(
        "def _vsr_scene_detect(c):\n"
        "    nxt = c[1:] + c[-1] if c.num_frames > 1 else c\n"
        "    prv = c[0] + c[:-1] if c.num_frames > 1 else c\n"
        "    next_stats = core.std.PlaneStats(c, nxt, prop='Next')\n"
        "    prev_stats = core.std.PlaneStats(c, prv, prop='Prev')\n"
        "    def mark(n, f):\n"
        "        out = f[0].copy()\n"
        "        out.props['_SceneChangeNext'] = int(f[1].props['NextDiff'] > 0.1)\n"
        "        out.props['_SceneChangePrev'] = int(f[2].props['PrevDiff'] > 0.1)\n"
        "        return out\n"
        "    return core.std.ModifyFrame(c, clips=[c, next_stats, prev_stats], selector=mark)\n");
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
        QString line = emitNode(node);
        if (node.definitionId == "temporal_median" || node.definitionId == "cnr4" ||
            (node.definitionId == "rife" && node.parameters.value("scene").toBool()))
            line.prepend(QStringLiteral("clip = _vsr_scene_detect(clip)\n"));
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
