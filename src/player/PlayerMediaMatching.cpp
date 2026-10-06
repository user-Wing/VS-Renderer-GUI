#include "player/PlayerWindow.h"
#include "player/PlayerMediaMatching.h"
#include "player/PlayerAssociations.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QJsonArray>
#include <QSet>
#include <algorithm>

namespace vsr {
namespace {
QString episode(const QString &name) {
    static const QRegularExpression pattern(QString("(?:s\\d{1,2}e\\d{1,3}|(?:ep|episode|")+QChar(0x7b2c)+")[ ._-]*\\d{1,3})",QRegularExpression::CaseInsensitiveOption);
    return pattern.match(name).captured().toLower().remove(QRegularExpression("[ ._-]"));
}
QStringList words(QString name) {
    name=name.toLower();name.remove(QRegularExpression(R"(\[[^\]]*\])"));
    name.remove(QRegularExpression(R"(\b(?:crf|x|h)\d+\b|\b\d{3,4}p\b)"));
    return name.split(QRegularExpression("[^\\p{L}\\p{N}]+"),Qt::SkipEmptyParts);
}
QSet<QString> numbers(const QStringList &parts) {
    static const QRegularExpression number("^\\d{1,3}$");QSet<QString> result;
    for(const auto &part:parts)if(number.match(part).hasMatch())result.insert(QString::number(part.toInt()));
    return result;
}
}
double playerMediaMatchScore(const QString &video,const QString &track) {
    const auto a=QFileInfo(video).completeBaseName().toLower(),b=QFileInfo(track).completeBaseName().toLower();
    if(a==b)return 2;
    const auto ae=episode(a),be=episode(b);if(!ae.isEmpty() && !be.isEmpty() && ae!=be)return 0;
    const auto aw=words(a),bw=words(b);int common=0,total=0;
    for(const auto &word:bw){total+=word.size();if(aw.contains(word))common+=word.size();}
    int videoTotal=0;for(const auto &word:aw)videoTotal+=word.size();
    const double similarity=double(common)/qMax(1,qMin(total,videoTotal));
    if(!ae.isEmpty() && ae==be)return 1+similarity*.5;
    // Bare episode numbers must agree as well; resolution and encoder numbers were removed above.
    const auto an=numbers(aw),bn=numbers(bw);
    if(!an.isEmpty() && !bn.isEmpty() && an!=bn)return 0;
    return common>=4 && similarity>=.65?similarity:0;
}
QStringList playerMatchingSubtitles(const QString &video) {
    const auto stem=QFileInfo(video).completeBaseName().toLower();
    const auto ae=episode(stem);const auto an=numbers(words(stem));
    QList<QPair<double,QString>> candidates;
    for(const auto &file:QFileInfo(video).absoluteDir().entryInfoList(QDir::Files,QDir::Name|QDir::IgnoreCase)) {
        if(!QStringList{"ass","ssa","srt","sup","vtt","mks"}.contains(file.suffix().toLower()))continue;
        const auto name=file.completeBaseName().toLower();
        const auto be=episode(name);const auto parts=words(name);const auto bn=numbers(parts);
        if((!ae.isEmpty() && !be.isEmpty() && ae!=be) || (!an.isEmpty() && !bn.isEmpty() && an!=bn))continue;
        double score=playerMediaMatchScore(video,file.absoluteFilePath());
        // A full stem followed by language suffixes also matches short names.
        if(name.startsWith(stem) && name.size()>stem.size() && QString(". _-([").contains(name.at(stem.size())))score=qMax(score,1.75);
        if(score==0)for(const auto &part:parts){
            const bool longEnough=part.size()>=3 || (part.size()>=2 && part.front().unicode()>127);
            static const QRegularExpression letter("\\p{L}");
            if(longEnough && letter.match(part).hasMatch() && !QStringList{"jpn","eng","chs","cht","chi","zho","the","and"}.contains(part) && stem.contains(part))score=.5;
        }
        if(score>0)candidates.append({score,file.absoluteFilePath()});
    }
    std::stable_sort(candidates.begin(),candidates.end(),[](const auto &a,const auto &b){return a.first>b.first;});
    QStringList files;for(const auto &candidate:candidates)files<<candidate.second;
    return files;
}
void PlayerWindow::matchExternalTracks() {
    if(matchedTracks_ || imageMode_ || audioMode_ || networkSource())return;matchedTracks_=true;
    bool video=false;for(const auto &entry:media_.value("streams").toArray())if(entry.toObject().value("type").toString()=="video")video=true;
    if(!video)return;
    QString best;double bestScore=0;bool ambiguous=false;
    for(const auto &file:QFileInfo(source_).absoluteDir().entryInfoList(QDir::Files,QDir::Name|QDir::IgnoreCase)) {
        if(!playerAudioExtensions().contains(file.suffix().toLower()) || file.absoluteFilePath()==source_)continue;
        const auto score=playerMediaMatchScore(source_,file.absoluteFilePath());
        if(score>bestScore){bestScore=score;best=file.absoluteFilePath();ambiguous=false;}
        else if(score>0 && qAbs(score-bestScore)<.0001)ambiguous=true;
    }
    if(!best.isEmpty() && !ambiguous && externalAudio_.isEmpty())attachAudio(best);
    const auto subtitles=playerMatchingSubtitles(source_);
    if(!subtitles.isEmpty() && externalSubtitle_.isEmpty())attachSubtitle(subtitles.first());
}
}
