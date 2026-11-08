#pragma once
#include <QStringList>
#include <QList>
namespace vsr {
inline QStringList builtinAnimeFiles(){return {"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy","Anime-8-A-CNN-Enhanced.vpy","Anime-9-A-CNN.vpy","MoeSR-50K.vpy"};}
inline QList<int> builtinAnimeOrder(){return {10,8,9,0,1,2,3,6,7,4,5};}
inline QStringList defaultAnimeStages(){return {"8","0","6","4","5"};}
}
