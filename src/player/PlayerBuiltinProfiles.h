#pragma once
#include <QStringList>
#include <QList>
namespace vsr {
// Existing IDs remain stable; 2/3 are retired and never offered as active stages.
inline QStringList builtinAnimeFiles(){return {"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy",{}, {},"Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-AFast-1X-Enhanced.vpy","Anime-7-AFast-1X.vpy","Anime-8-A-CNN-Enhanced.vpy","Anime-9-A-CNN.vpy","MoeSR-50K.vpy"};}
inline QList<int> builtinAnimeOrder(){return {10,8,9,0,1,6,7,4,5};}
inline QStringList defaultAnimeStages(){return {"8","0","7","4","5"};}
inline int legacyAnimeStage(int stage){return stage==2?6:stage==3 || stage==6 || stage==7?7:stage;}
inline int builtinAnimeStage(const QString &name){
    if(name=="Anime-2-No-CNN-Enhanced.vpy")return 6;
    if(name=="Anime-3-No-CNN.vpy" || name=="Anime-6-1X-Jinc.vpy" || name=="Anime-7-1X-D3D11.vpy")return 7;
    return name.isEmpty()?-1:builtinAnimeFiles().indexOf(name);
}
}
