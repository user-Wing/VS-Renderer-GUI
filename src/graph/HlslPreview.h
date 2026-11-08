#pragma once
#include "graph/FilterGraph.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSize>
#include <algorithm>

namespace vsr {
struct HlslPreviewPlan {
    FilterGraph graph;
    QVariantMap parameters;
    QList<QVariantMap> enhancements;
    QString shaderPath, error;
};
inline HlslPreviewPlan hlslPreviewPlan(const FilterGraph &graph) {
    HlslPreviewPlan plan;plan.graph=graph;int last=-1,shader=-1;
    for(int i=0;i<graph.nodes().size();++i){const auto &node=graph.nodes()[i];if(!node.enabled)continue;last=i;
        if(node.definitionId=="anime4k" && (node.parameters.value("hlsl",true).toBool() || QFileInfo(node.parameters.value("shader").toString()).suffix().compare("hlsl",Qt::CaseInsensitive)==0))shader=i;
    }
    if(shader<0)return plan;
    for(int i=shader+1;i<=last;++i){const auto &node=graph.nodes()[i];if(!node.enabled)continue;
        if(!QStringList{"sharpen_edges","enhance_detail","crispen_edges"}.contains(node.definitionId)){plan.error=QStringLiteral("着色器后的滤镜不支持 HLSL，保留原有 GLSL 顺序。");return plan;}
        auto parameters=node.parameters;parameters.insert("mode",node.definitionId);plan.enhancements.append(parameters);
    }
    plan.parameters=graph.nodes()[shader].parameters;
    plan.shaderPath=plan.parameters.value("shader").toString();
    if(plan.shaderPath.isEmpty())plan.shaderPath=QDir(QCoreApplication::applicationDirPath()).filePath("shaders/"+plan.parameters.value("mode").toString());
    for(int i=last;i>=shader;--i)plan.graph.remove(i);return plan;
}
inline bool hlslAspectCompatible(const QVariantMap &parameters,const QSize &input,const QSize &output) {
    return parameters.value("output_mode").toString()!=QStringLiteral("指定分辨率") || parameters.value("keep_aspect",true).toBool() ||
        qint64(input.width())*output.height()==qint64(input.height())*output.width();
}
inline QSize hlslOutputSize(const QVariantMap &parameters,const QSize &input) {
    if(input.isEmpty())return {};
    QSize result;
    if(parameters.value("output_mode").toString()==QStringLiteral("指定分辨率")){
        const int w=std::clamp(parameters.value("width",1920).toInt(),2,65536),h=std::clamp(parameters.value("height",1080).toInt(),2,65536);
        result=QSize(w,h);
        if(parameters.value("keep_aspect",true).toBool())result=parameters.value("dimension_axis","width").toString()=="height"
            ?QSize(qMax(2,qRound(double(h)*input.width()/input.height())),h)
            :QSize(w,qMax(2,qRound(double(w)*input.height()/input.width())));
    }else{
        const double scale=std::clamp(parameters.value("scale","2×").toString().section(QChar(0x00d7),0,0).toDouble(),.5,4.0);
        result=QSize(qMax(2,int(input.width()*scale)),qMax(2,int(input.height()*scale)));
    }
    return result.width()>16384 || result.height()>16384?QSize():result;
}
}
