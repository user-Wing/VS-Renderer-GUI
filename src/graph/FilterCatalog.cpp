#include "graph/FilterCatalog.h"

#include <QSet>

namespace vsr {
namespace {

ParameterDefinition integer(QString id, QString label, int value, int minimum, int maximum, int step = 1)
{
    return {std::move(id), std::move(label), ParameterType::Integer, value,
            static_cast<double>(minimum), static_cast<double>(maximum), static_cast<double>(step), {}};
}

ParameterDefinition real(QString id, QString label, double value, double minimum, double maximum, double step)
{
    return {std::move(id), std::move(label), ParameterType::Real, value, minimum, maximum, step, {}};
}

ParameterDefinition boolean(QString id, QString label, bool value)
{
    return {std::move(id), std::move(label), ParameterType::Boolean, value, 0, 1, 1, {}};
}

ParameterDefinition choice(QString id, QString label, QString value, QStringList choices)
{
    return {std::move(id), std::move(label), ParameterType::Choice, std::move(value), 0, 0, 0, std::move(choices)};
}

ParameterDefinition file(QString id, QString label, QString value)
{
    return {std::move(id), std::move(label), ParameterType::File, std::move(value), 0, 0, 0, {}};
}

const QList<FilterDefinition> kCatalog = {
    {"trim", "截取 Trim", "基础操作", "std", "按闭区间保留帧。",
     {integer("first", "起始帧", 0, 0, 100000000), integer("last", "结束帧", 239, 0, 100000000)}},
    {"crop", "裁切 Crop", "基础操作", "std", "从四边裁掉像素。",
     {integer("left", "左", 0, 0, 8192, 2), integer("right", "右", 0, 0, 8192, 2),
      integer("top", "上", 0, 0, 8192, 2), integer("bottom", "下", 0, 0, 8192, 2)}},
    {"transpose", "转置 Transpose", "基础操作", "std", "交换画面宽高。", {}},
    {"assume_fps", "指定帧率 AssumeFPS", "基础操作", "std", "只修改时间基，不插帧。",
     {integer("fpsnum", "分子", 24000, 1, 1000000), integer("fpsden", "分母", 1001, 1, 1000000)}},
    {"depth", "位深转换 Depth", "格式与缩放", "fmtc", "使用 fmtc 转换位深。",
     {choice("bits", "目标位深", "16", {"8", "10", "12", "16", "32"}), boolean("dmode_ordered", "有序抖动", false)}},
    {"resize", "尺寸缩放 Resize", "格式与缩放", "resize", "使用 VapourSynth core resizer。",
     {integer("width", "宽度", 1920, 16, 16384, 2), integer("height", "高度", 1080, 16, 16384, 2),
      choice("kernel", "算法", "Spline36", {"Point", "Bicubic", "Lanczos", "Spline36"}),
      integer("taps", "Lanczos taps", 3, 2, 16)}},
    {"anime4k", "Anime4K GLSL", "GPU 超分与着色器", "placebo",
     "由 vs-placebo 在 VapourSynth 中直接执行 mpv/libplacebo GLSL；GPU 实时性取决于 shader、倍率、分辨率与显卡。输入会转成 16-bit YUV，输出为 YUV444P16。",
     {file("shader", "GLSL 文件", "C:\\PortableSoft\\FFmpegFreeUI ReadyToRun x64\\libplacebo\\anime4k-v4-a.glsl"),
      choice("scale", "输出倍率", "2×", {"1×", "2×", "3×", "4×"})}},
    {"remove_grain", "RemoveGrain", "降噪", "rgvs", "VCB 教程中的基础空间降噪。",
     {integer("mode", "模式", 20, 0, 28)}},
    {"bilateral", "Bilateral", "降噪", "vszip", "VSZip 双边滤波，注意纹理损失。",
     {real("sigmaS", "空间 sigma", 3.0, 0.1, 100.0, 0.1), real("sigmaR", "范围 sigma", 0.02, 0.001, 1.0, 0.001)}},
    {"nlmeans", "Reduce random noise · NLMeans", "去伪影 Artifact removal", "nlm_ispc", "非局部均值降噪。",
     {integer("d", "时域半径", 0, 0, 10), integer("wmode", "权重模式", 3, 0, 4), integer("h", "强度", 3, 0, 100)}},
    {"ttempsmooth", "Zsmooth TTempSmooth", "降噪", "zsmooth",
     "运动自适应时域平滑，适合稳定动画噪声且保细节；半径和阈值越高越慢，运动区域越可能拖影，跨镜头处建议分段使用。",
     {integer("maxr", "最大时域半径", 3, 1, 7), integer("thresh_y", "Y 运动阈值", 4, 1, 256),
      integer("thresh_c", "色度运动阈值", 5, 1, 256), integer("mdiff_y", "Y 满权重差值", 2, 0, 255),
      integer("mdiff_c", "色度满权重差值", 3, 0, 255), integer("strength", "时域权重强度", 2, 1, 8),
      boolean("fp", "运动区回补中心帧", true)}},
    {"deband", "Reduce banding · VSZip", "去伪影 Artifact removal", "vszip", "API 4 去色带，可串联多次。",
     {integer("range", "范围", 12, 1, 64), integer("y", "Y 阈值", 96, 0, 255),
      integer("cb", "Cb 阈值", 48, 0, 255), integer("cr", "Cr 阈值", 48, 0, 255),
      integer("grainy", "Y grain", 0, 0, 64), integer("grainc", "C grain", 0, 0, 64),
      choice("output_depth", "输出位深", "16", {"8", "10", "12", "16"})}},
    {"deblock", "Reduce compression · Deblock", "去伪影 Artifact removal", "deblock",
     "直接削弱 H.264/MPEG 类块边界，教程称其强但通常有效；quant 与偏移越高越平滑，也越容易损失真实边缘。",
     {integer("quant", "基础强度 quant", 25, 0, 60), integer("aoffset", "边缘检测偏移", 0, -24, 24),
      integer("boffset", "去块强度偏移", 0, -24, 24)}},
    {"dering", "Reduce ringing 去振铃", "去伪影 Artifact removal", "std",
     "将超出平滑邻域包络的过冲限制回局部范围；开源 VS 原语实现，不是 madVR 私有滤镜。",
     {real("strength", "强度", 0.5, 0.0, 1.0, 0.05)}},
    {"sharpen_edges", "Sharpen edges 边缘锐化", "图像增强 Image enhancements", "std",
     "阈值控制的反锐化遮罩，只增强明显边缘，避免放大平坦区域噪声。",
     {real("strength", "强度", 0.5, 0.0, 2.0, 0.05), real("threshold", "边缘阈值（8-bit）", 2.0, 0.0, 32.0, 0.5)}},
    {"crispen_edges", "Crispen edges 边缘清晰度", "图像增强 Image enhancements", "std",
     "四邻域拉普拉斯高频增强，限幅抑制过冲；细线素材建议使用较低强度。",
     {real("strength", "强度", 0.25, 0.0, 1.0, 0.05)}},
    {"thin_edges", "Thin edges 细化暗线", "图像增强 Image enhancements", "std",
     "亮度平面的形态学最大值与原图混合，收窄暗色线条；不适合所有实拍素材。",
     {real("strength", "强度", 0.25, 0.0, 1.0, 0.05)}},
    {"enhance_detail", "Enhance detail 细节增强", "图像增强 Image enhancements", "std",
     "小尺度与中尺度高频的加权增强，局部限幅避免明显亮暗光晕。",
     {real("strength", "强度", 0.3, 0.0, 1.0, 0.05)}},
    {"cas", "CAS 锐化", "锐化", "cas", "Contrast Adaptive Sharpening。",
     {real("sharpness", "锐度", 0.5, 0.0, 1.0, 0.01)}},
    {"znedi3", "ZNEDI3 插值", "抗锯齿与插值", "znedi3",
     "CPU 优化的神经网络边缘插值，可反交错或纵向 2×；网络越大越慢，输入场序选错会造成明显抖动。",
     {choice("mode", "输出与场序", "3 - 双倍帧率，上场起始",
             {"0 - 单倍帧率，保留下场", "1 - 单倍帧率，保留上场", "2 - 双倍帧率，下场起始",
              "3 - 双倍帧率，上场起始", "4 - 纵向 2×，保留下场", "5 - 纵向 2×，保留上场"}),
      choice("nsize", "邻域", "0 - 8×6（放大推荐）",
             {"0 - 8×6（放大推荐）", "1 - 16×6", "2 - 32×6", "3 - 48×6", "4 - 8×4（锐利）", "5 - 16×4", "6 - 32×4"}),
      choice("nns", "神经元", "2 - 64", {"0 - 16", "1 - 32", "2 - 64", "3 - 128", "4 - 256"}),
      choice("qual", "预测质量", "2 - 双网络高质量", {"1 - 单网络快速", "2 - 双网络高质量"}),
      choice("pscrn", "预筛选", "2 - 新版 level 0", {"0 - 禁用（最慢）", "1 - 旧版", "2 - 新版 level 0", "3 - 新版 level 1", "4 - 新版 level 2"})}},
    {"eedi3", "EEDI3 边缘插值", "抗锯齿与插值", "eedi3m",
     "高质量边缘导向插值，比 NNEDI3 更强但慢得多；连接半径和相似度权重过高会拉出错误线条。",
     {choice("mode", "输出与场序", "3 - 双倍帧率，上场起始",
             {"0 - 单倍帧率，保留下场", "1 - 单倍帧率，保留上场", "2 - 双倍帧率，下场起始",
              "3 - 双倍帧率，上场起始", "4 - 纵向 2×，保留下场", "5 - 纵向 2×，保留上场"}),
      real("alpha", "邻域相似权重 alpha", 0.2, 0.0, 1.0, 0.01), real("beta", "垂直差异权重 beta", 0.25, 0.0, 1.0, 0.01),
      real("gamma", "方向平滑惩罚 gamma", 20.0, 0.0, 1000.0, 1.0), integer("nrad", "邻域半径", 2, 0, 3),
      integer("mdis", "最大连接距离", 20, 1, 40)}},
    {"sangnom", "SangNom 单场抗锯齿", "抗锯齿与插值", "sangnom",
     "快速且很强的单场边缘插值，适合严重锯齿；教程明确警告其破坏性，强度过高会吞掉细线与纹理。",
     {choice("order", "保留场", "1 - 上场", {"1 - 上场", "2 - 下场"}), boolean("dh", "纵向 2×", false),
      integer("aa_y", "Y 抗锯齿强度", 48, 0, 128), integer("aa_c", "色度抗锯齿强度", 0, 0, 128)}},
    {"bwdif", "Bwdif 反交错", "反交错与 IVTC", "bwdif",
     "运动自适应反交错，速度和质量均衡；双倍帧率更流畅但计算量与输出帧数也翻倍，场序必须正确。",
     {choice("field", "输出与场序", "3 - 双倍帧率，上场起始",
             {"0 - 单倍帧率，保留下场", "1 - 单倍帧率，保留上场", "2 - 双倍帧率，下场起始", "3 - 双倍帧率，上场起始"})}},
    {"vivtc", "VIVTC 逆胶片化", "反交错与 IVTC", "vivtc",
     "VFM 场匹配后由 VDecimate 删除重复帧，适合规则胶片转电视源；不含残余梳齿后处理，混合场或坏剪辑需更复杂脚本。",
     {choice("order", "源场序", "1 - 上场优先", {"0 - 下场优先", "1 - 上场优先"}),
      choice("mode", "匹配策略", "1 - 两路匹配，梳齿时试第三路",
             {"0 - 安全两路匹配", "1 - 两路匹配，梳齿时试第三路", "2 - 同场序第三路", "3 - 最多五路", "4 - 固定三路", "5 - 三路并在梳齿时扩展"}),
      integer("cthresh", "梳齿可见阈值", 9, -1, 255), integer("mi", "梳齿像素门限", 80, 0, 256),
      boolean("mchroma", "匹配时包含色度", true), integer("cycle", "每周期帧数", 5, 2, 100),
      real("dupthresh", "重复帧阈值 %", 1.1, 0.0, 100.0, 0.1)}},
    {"grain_add", "添加颗粒 Grain", "颗粒", "grain", "加入动态或静态颗粒保护细节。",
     {real("var", "强度", 0.5, 0.0, 100.0, 0.1), boolean("constant", "静态颗粒", false)}},
};

}

const QList<FilterDefinition> &FilterCatalog::all()
{
    return kCatalog;
}

const FilterDefinition *FilterCatalog::find(const QString &id)
{
    for (const auto &definition : kCatalog) {
        if (definition.id == id)
            return &definition;
    }
    return nullptr;
}

QStringList FilterCatalog::categories()
{
    QStringList result;
    QSet<QString> seen;
    for (const auto &definition : kCatalog) {
        if (!seen.contains(definition.category)) {
            seen.insert(definition.category);
            result.append(definition.category);
        }
    }
    return result;
}

}
