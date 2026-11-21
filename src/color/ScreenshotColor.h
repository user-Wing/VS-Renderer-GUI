#pragma once
#include <QColorSpace>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QRect>
#include <algorithm>
#include <array>
#include <cmath>

namespace vsr::screenshot {
enum class DisplayClass { Unknown, LegacySdr, AdvancedSdr, Hdr };
enum class Primaries { Unknown, Rec709, P3D65, Rec2020, CustomIcc };
enum class Transfer { Unknown, Srgb, Linear, Pq, Hlg, CustomIcc };
enum class Units { Relative, ScRgb80Nits };
enum class Range { Unknown, Full };
enum class Preview { Identity, DesktopWhite, Photographic };

struct DisplayEvidence {
    QString identity;
    QString modeSource, gamutSource;
    bool modeKnown=false, advancedActive=false, hdrActive=false, wideActive=false;
    int outputBits=0; // Scan-out depth is not source precision.
    QRect nativeBounds;
    int rotation=0;
    std::array<float,8> nativeChromaticities{}; // Rxy, Gxy, Bxy, Wxy; zero means unavailable.
    double sdrWhiteNits=0; // Zero means unmeasured; fallback is a decision, not evidence.
    DisplayClass classification() const {
        if(hdrActive)return DisplayClass::Hdr;
        if(wideActive || (modeKnown && advancedActive))return DisplayClass::AdvancedSdr;
        return modeKnown?DisplayClass::LegacySdr:DisplayClass::Unknown;
    }
    bool needsFloatCapture() const { return classification()!=DisplayClass::LegacySdr; }
};

struct FrameColor {
    Primaries primaries=Primaries::Unknown;
    Transfer transfer=Transfer::Unknown;
    Units units=Units::Relative;
    Range range=Range::Unknown;
    int sourceBits=0, effectiveBits=0; // FP16 = storage, effectiveBits=0 = unmeasured.
    bool floating=false, desktop=false, advancedSdr=false;
    double sdrWhiteNits=0;
    QString sourceIdentity, frameIdentity, backend, degradation;
};
struct ColorPlan {
    FrameColor source;
    QColorSpace target;
    Preview preview=Preview::Identity;
    bool scRgbToPq=false, scRgbToWideSdr=false;
    int containerBits=16, cicpPrimaries=0, cicpTransfer=0;
    int rgbMatrix=0, yuvMatrix=0; // RGB PNG/JXL versus AVIF YUV.
    double whiteNits=0;
    QString degradation;
};

inline double finite(double v) { return std::isfinite(v)?v:0.; }
inline double encodePqNits(double nits) {
    const double p=std::pow(std::clamp(finite(nits)/10000.,0.,1.),2610./16384.);
    return std::pow((3424./4096.+2413./128.*p)/(1.+2392./128.*p),2523./32.);
}
inline double srgb(double v) {v=std::clamp(finite(v),0.,1.);return v<=.0031308?12.92*v:1.055*std::pow(v,1./2.4)-.055;}
inline std::array<double,3> toRec2020(double r,double g,double b) {
    // Transform signed scRGB first. Clipping before this matrix destroys wide-gamut colors.
    r=finite(r);g=finite(g);b=finite(b);
    return {.6274039*r+.3292830*g+.0433131*b,
            .0690973*r+.9195404*g+.0113623*b,
            .0163914*r+.0880133*g+.8955953*b};
}
inline void desktopMap(std::array<double,3> &rgb) {
    // Desktop saturation policy follows TrueToneCap ToneMapper.cs at a855dc21
    // (Apache-2.0). Adapted for signed double RGB and non-finite input, 2026-11-17.
    // See third_party/truetonecap/LICENSE. This is saturation, not highlight recovery.
    for(auto &v:rgb)v=finite(v);
    const double peak=std::max({1.,rgb[0],rgb[1],rgb[2]});
    for(auto &v:rgb)v/=peak; // SDR identity below white, ratio-preserving saturation above white.
}
inline QJsonObject json(const FrameColor &f) {
    return {{"primaries",int(f.primaries)},{"transfer",int(f.transfer)},{"units",int(f.units)},
        {"range",int(f.range)},{"sourceBits",f.sourceBits},{"effectiveBits",f.effectiveBits},
        {"floating",f.floating},{"desktop",f.desktop},{"advancedSdr",f.advancedSdr},
        {"sdrWhiteNits",f.sdrWhiteNits},{"sourceIdentity",f.sourceIdentity},{"frameIdentity",f.frameIdentity},
        {"backend",f.backend},{"degradation",f.degradation}};
}
inline void attach(QImage &image,const FrameColor &f) {
    image.setText("vsrColorEvidence",QString::fromUtf8(QJsonDocument(json(f)).toJson(QJsonDocument::Compact)));
}
inline FrameColor evidence(const QImage &image) {
    FrameColor f;
    const auto o=QJsonDocument::fromJson(image.text("vsrColorEvidence").toUtf8()).object();
    if(!o.isEmpty()) {
        f.primaries=Primaries(o["primaries"].toInt());f.transfer=Transfer(o["transfer"].toInt());f.units=Units(o["units"].toInt());f.range=Range(o["range"].toInt());
        f.sourceBits=o["sourceBits"].toInt();f.effectiveBits=o["effectiveBits"].toInt();f.floating=o["floating"].toBool();f.desktop=o["desktop"].toBool();f.advancedSdr=o["advancedSdr"].toBool();
        f.sdrWhiteNits=o["sdrWhiteNits"].toDouble();f.sourceIdentity=o["sourceIdentity"].toString();f.frameIdentity=o["frameIdentity"].toString();f.backend=o["backend"].toString();f.degradation=o["degradation"].toString();
        return f;
    }
    const auto space=image.colorSpace();
    f.primaries=space.primaries()==QColorSpace::Primaries::SRgb?Primaries::Rec709:space.primaries()==QColorSpace::Primaries::DciP3D65?Primaries::P3D65:space.primaries()==QColorSpace::Primaries::Bt2020?Primaries::Rec2020:space.isValid()?Primaries::CustomIcc:Primaries::Unknown;
    f.transfer=space.transferFunction()==QColorSpace::TransferFunction::SRgb?Transfer::Srgb:space.transferFunction()==QColorSpace::TransferFunction::Linear?Transfer::Linear:space.transferFunction()==QColorSpace::TransferFunction::St2084?Transfer::Pq:space.transferFunction()==QColorSpace::TransferFunction::Hlg?Transfer::Hlg:space.isValid()?Transfer::CustomIcc:Transfer::Unknown;
    const auto format=image.format();f.floating=format==QImage::Format_RGBA32FPx4 || format==QImage::Format_RGBX32FPx4 || format==QImage::Format_RGBA16FPx4 || format==QImage::Format_RGBX16FPx4 || format==QImage::Format_RGBA32FPx4_Premultiplied || format==QImage::Format_RGBA16FPx4_Premultiplied;
    f.sourceBits=image.text("sourceBitDepth").toInt();if(!f.sourceBits)f.sourceBits=f.floating?image.depth()/4:image.depth()>32?16:8;
    if(image.text("capHdr")=="scRGB-FP16") {f.primaries=Primaries::Rec709;f.transfer=Transfer::Linear;f.units=Units::ScRgb80Nits;f.desktop=true;f.sourceBits=16;f.range=Range::Full;}
    const double white=image.text("capSdrWhiteNits").toDouble();if(std::isfinite(white) && white>0)f.sdrWhiteNits=white;
    return f;
}
inline void standardDeclaration(ColorPlan &p) {
    const auto s=p.target;
    p.cicpPrimaries=s.primaries()==QColorSpace::Primaries::SRgb?1:s.primaries()==QColorSpace::Primaries::DciP3D65?12:s.primaries()==QColorSpace::Primaries::Bt2020?9:0;
    p.cicpTransfer=s.transferFunction()==QColorSpace::TransferFunction::SRgb?13:s.transferFunction()==QColorSpace::TransferFunction::Linear?8:s.transferFunction()==QColorSpace::TransferFunction::St2084?16:s.transferFunction()==QColorSpace::TransferFunction::Hlg?18:s.transferFunction()==QColorSpace::TransferFunction::Bt2020?1:0;
    if(!p.cicpPrimaries || !p.cicpTransfer)p.cicpPrimaries=p.cicpTransfer=0;
    p.yuvMatrix=p.cicpPrimaries==9?9:1;
}
inline QColorSpace spaceFromCicp(int primaries,int transfer) {
    const auto p=primaries==1?QColorSpace::Primaries::SRgb:primaries==9?QColorSpace::Primaries::Bt2020:primaries==12?QColorSpace::Primaries::DciP3D65:QColorSpace::Primaries::Custom;
    const auto t=transfer==13?QColorSpace::TransferFunction::SRgb:transfer==8?QColorSpace::TransferFunction::Linear:transfer==16?QColorSpace::TransferFunction::St2084:transfer==18?QColorSpace::TransferFunction::Hlg:transfer==1 || transfer==14 || transfer==15?QColorSpace::TransferFunction::Bt2020:QColorSpace::TransferFunction::Custom;
    return p==QColorSpace::Primaries::Custom || t==QColorSpace::TransferFunction::Custom?QColorSpace():QColorSpace(p,t);
}
inline ColorPlan exportPlan(const QImage &image) {
    ColorPlan p;p.source=evidence(image);p.target=image.colorSpace();
    if(p.source.units==Units::ScRgb80Nits) {
        p.scRgbToWideSdr=p.source.advancedSdr;p.scRgbToPq=!p.scRgbToWideSdr;
        p.target=p.scRgbToPq?QColorSpace(QColorSpace::Bt2100Pq):QColorSpace(QColorSpace::Primaries::Bt2020,QColorSpace::TransferFunction::SRgb);
        p.whiteNits=p.source.sdrWhiteNits>0?p.source.sdrWhiteNits:80.;
    } else if(!p.target.isValid()){
        p.target=QColorSpace::SRgb;p.degradation=QStringLiteral("Unspecified source color; sRGB compatibility assumption");
    }
    standardDeclaration(p);return p;
}
inline ColorPlan previewPlan(const QImage &image) {
    auto p=exportPlan(image);
    if(p.source.units==Units::ScRgb80Nits || p.source.transfer==Transfer::Pq || p.source.transfer==Transfer::Hlg) {
        p.preview=p.source.desktop || p.source.sdrWhiteNits>0?Preview::DesktopWhite:Preview::Photographic;
        p.whiteNits=p.source.sdrWhiteNits>0?p.source.sdrWhiteNits:p.source.desktop?80.:203.;
        if(p.source.desktop && p.source.sdrWhiteNits<=0)p.degradation=QStringLiteral("SDR white unmeasured; using 80 nit reference");
        p.target=QColorSpace::SRgb;
    }
    standardDeclaration(p);return p;
}
inline QImage exportPixels(const QImage &image,const ColorPlan &plan) {
    if(!plan.scRgbToPq && !plan.scRgbToWideSdr){
        auto output=image.convertToFormat(QImage::Format_RGBA64);output.setColorSpace(plan.target);return output;
    }
    const auto input=image.convertToFormat(QImage::Format_RGBA32FPx4);QImage out(image.size(),QImage::Format_RGBA64);
    if(out.isNull())return {};
    const auto pack=[](double v){return quint16(std::lround(std::clamp(finite(v),0.,1.)*65535.));};
    for(int y=0;y<input.height();++y) {
        const auto *src=reinterpret_cast<const float*>(input.constScanLine(y));auto *dst=reinterpret_cast<QRgba64*>(out.scanLine(y));
        for(int x=0;x<input.width();++x) {
            const auto rgb=toRec2020(src[x*4],src[x*4+1],src[x*4+2]);std::array<quint16,3> code{};
            for(int c=0;c<3;++c)code[c]=pack(plan.scRgbToPq?encodePqNits(rgb[c]*80.):srgb(rgb[c]*80./plan.whiteNits));
            dst[x]=QRgba64::fromRgba64(code[0],code[1],code[2],pack(src[x*4+3]));
        }
    }
    out.setColorSpace(plan.target);
    if(plan.scRgbToPq && (plan.source.desktop || plan.source.sdrWhiteNits>0)){
        out.setText("capSdrWhiteNits",QString::number(plan.whiteNits,'g',17));
        out.setText("capSdrWhiteBasis",plan.source.sdrWhiteNits>0?"display-query":"80-nit-reference-fallback");
    }
    auto facts=plan.source;facts.primaries=Primaries::Rec2020;facts.transfer=plan.scRgbToPq?Transfer::Pq:Transfer::Srgb;facts.units=Units::Relative;
    attach(out,facts);out.setText("sourceBitDepth",QString::number(plan.source.sourceBits));return out;
}

// No mutable access or external QImage storage survives this boundary. Copies are COW.
class FrozenFrame {
    const QImage pixels_;
    const FrameColor color_;
public:
    explicit FrozenFrame(const QImage &image):pixels_(image.copy()),color_(evidence(image)){}
    const QImage &pixels() const {return pixels_;}
    const FrameColor &color() const {return color_;}
};
}
