#include "player/PlayerWindow.h"
#include "player/PlayerMediaMatching.h"
#include "player/PlayerAssociations.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QJsonArray>
#include <QSet>

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
    static const QRegularExpression number("^\\d{1,3}$");QSet<QString> an,bn;
    for(const auto &word:aw)if(number.match(word).hasMatch())an.insert(QString::number(word.toInt()));
    for(const auto &word:bw)if(number.match(word).hasMatch())bn.insert(QString::number(word.toInt()));
    if(!an.isEmpty() && !bn.isEmpty() && an!=bn)return 0;
    return common>=4 && similarity>=.65?similarity:0;
}
void PlayerWindow::matchExternalTracks() {
    if(matchedTracks_ || imageMode_ || networkSource())return;matchedTracks_=true;
    bool video=false;for(const auto &entry:media_.value("streams").toArray())if(entry.toObject().value("type").toString()=="video")video=true;
    if(!video)return;
    QString best[2];double scores[2]{};bool ambiguous[2]{};
    for(const auto &file:QFileInfo(source_).absoluteDir().entryInfoList(QDir::Files,QDir::Name|QDir::IgnoreCase)) {
        const auto suffix=file.suffix().toLower();const int kind=QStringList{"ass","ssa","srt","sup","vtt"}.contains(suffix)?1:playerAudioExtensions().contains(suffix)?0:-1;
        if(kind<0 || file.absoluteFilePath()==source_)continue;
        const auto score=playerMediaMatchScore(source_,file.absoluteFilePath());
        if(score>scores[kind]){scores[kind]=score;best[kind]=file.absoluteFilePath();ambiguous[kind]=false;}
        else if(score>0 && qAbs(score-scores[kind])<.0001)ambiguous[kind]=true;
    }
    if(!best[0].isEmpty() && !ambiguous[0] && externalAudio_.isEmpty())attachAudio(best[0]);
    if(!best[1].isEmpty() && !ambiguous[1] && externalSubtitle_.isEmpty())attachSubtitle(best[1]);
}
}
