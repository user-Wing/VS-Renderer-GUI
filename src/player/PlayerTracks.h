#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <limits>

namespace vsr {
inline QString playerTrackLabel(const QJsonObject &stream, int ordinal) {
    auto tags=stream.value("tags").toObject();const auto metadata=stream.value("metadata").toObject();
    for(auto it=metadata.begin();it!=metadata.end();++it)tags.insert(it.key(),it.value());
    QString name=tags.value("title").toString();if(name.isEmpty())name=stream.value("codec").toString();
    QString language=tags.value("language-ietf").toString();if(language.isEmpty())language=tags.value("language").toString();if(language.isEmpty())language="und";
    QStringList flags;if(stream.value("default").toBool())flags<<"Default";if(stream.value("forced").toBool())flags<<"Forced";
    return QString("Track 0:%1:%2 - %3 - %4 - %5").arg(stream.value("type").toString()=="audio"?"a":"s").arg(ordinal).arg(name,language,flags.isEmpty()?QString("None"):flags.join('+'));
}
inline int playerDefaultTrack(const QJsonArray &streams, const QString &type) {
    int first=std::numeric_limits<int>::max(),selected=first;
    for(const auto &entry:streams){const auto stream=entry.toObject();if(stream.value("type").toString()!=type)continue;const int index=stream.value("index").toInt();first=qMin(first,index);if(stream.value("default").toBool())selected=qMin(selected,index);}
    if(selected!=std::numeric_limits<int>::max())return selected;
    return first!=std::numeric_limits<int>::max()?first:-1;
}
}
