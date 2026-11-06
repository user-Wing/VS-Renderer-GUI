#pragma once
#include <QSettings>
#include <QStringList>
#include <QVariant>

namespace vsr {
struct PlayerShortcut {QString key,action;double value;};
inline QList<PlayerShortcut> playerShortcuts(QSettings &settings){
    return {{"Space","play",0},{"Up","volume",5},{"Down","volume",-5},
        {"Ctrl+Up","rate",.05},{"Ctrl+Down","rate",-.05},{"M","mute",0},{"L","list",0},
        {"J","jinc",0},{"D","d3d11",0},{"A","auto",0},
        {"Left",settings.value("playback/arrows","seconds").toString()=="frame"?"frame":settings.value("playback/arrows").toString()=="keyframe"?"keyframe":"seek",-1},
        {"Right",settings.value("playback/arrows","seconds").toString()=="frame"?"frame":settings.value("playback/arrows").toString()=="keyframe"?"keyframe":"seek",1},
        {"Ctrl+Left","seek",-settings.value("playback/ctrlSeconds",10).toDouble()},
        {"Ctrl+Right","seek",settings.value("playback/ctrlSeconds",10).toDouble()},
        {"Ctrl+Alt+Left","seek",-settings.value("playback/ctrlAltSeconds",60).toDouble()},
        {"Ctrl+Alt+Right","seek",settings.value("playback/ctrlAltSeconds",60).toDouble()},
        {"Ctrl+Shift+Left","chapter",-1},{"Ctrl+Shift+Right","chapter",1},
        {"Ctrl+C","clipboard",0},{".","minimal",0}};
}
inline QString shortcutPrefix(int row){return QString("shortcuts/%1/").arg(row);}
}
