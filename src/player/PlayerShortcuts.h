#pragma once
#include <QSettings>
#include <QSet>
#include <QStringList>
#include <QVariant>

namespace vsr {
struct PlayerShortcut {QString key,action;double value;};
inline QList<PlayerShortcut> playerShortcuts(QSettings &settings){
    return {{"Space","play",0},{"Up","volume",5},{"Down","volume",-5},
        {"Ctrl+Up","rate",.05},{"Ctrl+Down","rate",-.05},{"M","mute",0},{"L","list",0},
        {"Left",settings.value("playback/arrows","seconds").toString()=="frame"?"frame":settings.value("playback/arrows").toString()=="keyframe"?"keyframe":"seek",-1},
        {"Right",settings.value("playback/arrows","seconds").toString()=="frame"?"frame":settings.value("playback/arrows").toString()=="keyframe"?"keyframe":"seek",1},
        {"Ctrl+Left","seek",-settings.value("playback/ctrlSeconds",10).toDouble()},
        {"Ctrl+Right","seek",settings.value("playback/ctrlSeconds",10).toDouble()},
        {"Ctrl+Alt+Left","seek",-settings.value("playback/ctrlAltSeconds",60).toDouble()},
        {"Ctrl+Alt+Right","seek",settings.value("playback/ctrlAltSeconds",60).toDouble()},
        {"Ctrl+Shift+Left","chapter",-1},{"Ctrl+Shift+Right","chapter",1},
        {"Ctrl+C","clipboard",0},{".","minimal",0},
        {"3","moesr",0},
        {"4","animeA",0},
        {"5","animeFast",0},
        {"6","anime1x",0},
        {"7","openngu",0},
        {"8","jinc",0},
        {"9","lanczos",0},
        {"0","d3d11",0},
        {"-","auto",0},
        {"Z","videoTracks",0},
        {"X","audioTracks",0},
        {"C","subtitleTracks",0},
        {"Q","jxl",0},
        {"W","avif",0},
        {"E","png",0},
        {"Ctrl+Q","jxlFolder",0},
        {"Ctrl+W","avifFolder",0},
        {"Ctrl+E","pngFolder",0}};
}
inline QString shortcutPrefix(int row){return QString("shortcuts/%1/").arg(row);}
inline constexpr int legacyShortcutLimit = 512;
// Leave space for the 18 defaults added since the original 20-row layout.
inline constexpr int maximumPlayerShortcuts = legacyShortcutLimit + 18;
inline int playerShortcutCount(const QSettings &settings, int defaultCount)
{
    return qBound(defaultCount, settings.value("shortcuts/rowCount", defaultCount).toInt(), maximumPlayerShortcuts);
}
inline void migratePlayerShortcuts(QSettings &settings)
{
    if(settings.value("shortcuts/layoutWithoutJda").toBool())return;
    const auto bindings=playerShortcuts(settings);
    const int defaults=bindings.size();
    const int oldDefaults=settings.value("shortcuts/layout120").toBool() ? 38 : 20;
    const int count=qBound(oldDefaults,settings.value("shortcuts/rowCount").toInt(),legacyShortcutLimit);
    QSet<QString> assigned;
    QList<QVariantMap> rows(defaults), extras;
    const QStringList oldKeys{"J","D","A"},oldActions{"jinc","d3d11","auto"};
    for(int i=0;i<count;++i) {
        QVariantMap row;
        const auto prefix=shortcutPrefix(i);
        for(const auto *field:{"key","action","value","enabled"})
            if(settings.contains(prefix+field))row.insert(field,settings.value(prefix+field));
        if(row.value("enabled",true).toBool())assigned.insert(row.value("key").toString());
        if(i>=7 && i<=9) {
            const auto key=row.value("key",oldKeys[i-7]).toString();
            const auto action=row.value("action",oldActions[i-7]).toString();
            if(key!=oldKeys[i-7] || action!=oldActions[i-7]) {
                row.insert("key",key);row.insert("action",action);
                if(row.value("enabled",true).toBool())assigned.insert(key);
                extras.append(row);
            }
        } else if(i>=oldDefaults) extras.append(row);
        else rows[i<7 ? i : i-3]=row;
    }
    for(int i=oldDefaults-3;i<defaults;++i)
        if(assigned.contains(bindings[i].key))rows[i].insert("key",QString());
    rows.append(extras);
    for(int i=0;i<count;++i)settings.remove(shortcutPrefix(i).chopped(1));
    for(int i=0;i<rows.size();++i)
        for(auto it=rows[i].cbegin();it!=rows[i].cend();++it)
            settings.setValue(shortcutPrefix(i)+it.key(),it.value());
    settings.setValue("shortcuts/rowCount",rows.size());
    settings.setValue("shortcuts/layout120",true);
    settings.setValue("shortcuts/layoutWithoutJda",true);
}
inline bool isPlayerFullscreenKey(int key)
{
    return key == Qt::Key_Return || key == Qt::Key_Enter;
}
}
