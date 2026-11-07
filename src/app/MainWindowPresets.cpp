#include "app/MainWindow.h"
#include "graph/PresetStore.h"
#include "ui/PresetDialog.h"
#include "ui/PresetCommunityDialog.h"
#include <QComboBox>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>

namespace vsr {
void MainWindow::showCommunity(){PresetCommunityDialog dialog([this](const QString &path,bool current){loadPreset(path,current);},this);dialog.exec();}
ScriptBuildResult MainWindow::currentScript(const QString &source) const
{
    const QString input = source.isEmpty() ? sourcePath_->text() : source;
    if (!activePreset_.isEmpty()) return PresetStore::load(activePreset_, input);
    return VpyScriptBuilder::buildGroups(input, static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), currentTimeGroups());
}
void MainWindow::showPresets()
{
    PresetDialog dialog([this](const QString &note) {
        if (!activePreset_.isEmpty()) {
            QFile file(activePreset_); if (!file.open(QIODevice::ReadOnly)) return QString();
            QString text = QString::fromUtf8(file.readAll()); auto metadata = PresetStore::metadata(text);
            if (!metadata.isEmpty()) {
                metadata.insert("note", note); QStringList lines = text.split('\n');
                for (auto &line : lines) if (line.startsWith("# VSR_PRESET ")) line = "# VSR_PRESET " + QString::fromLatin1(QJsonDocument(metadata).toJson(QJsonDocument::Compact).toBase64());
                text = lines.join('\n');
            }
            return text;
        }
        return PresetStore::create(currentTimeGroups(), static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), sourcePath_->text(), note);
    }, [this](const QString &path) { loadPreset(path); }, [this] {
        activePreset_.clear(); replaceTimeGroups({{QStringLiteral("全片"),0,{}}});
    }, this, [this](const QString &path){loadPreset(path,true);}, [this](const QString &path){if(addRoute())loadPreset(path);});
    dialog.exec();
}
void MainWindow::loadPreset(const QString &path, bool currentGroup)
{
    const auto result = PresetStore::load(path);
    if (!result.errors.isEmpty()) { setStatus(result.errors.join('\n'), true); return; }
    const auto data = PresetStore::metadata(result.script);
    QFile original(path);FilterGraph verifiedGraph;SourceFilter verifiedFilter=SourceFilter::Ffms2;
    const bool generated=original.open(QIODevice::ReadOnly) && PresetStore::verifiedGraph(QString::fromUtf8(original.readAll()),&verifiedGraph,&verifiedFilter);
    auto groups=PresetStore::timeGroups(data);
    if(groups.size()==1 && result.script.contains("# VSR_PLAYER_PROFILE "))groups[0].script=result.script.mid(result.script.indexOf("# VSR_PLAYER_PROFILE "));
    if(currentGroup){
        if(data.isEmpty()){setStatus(QStringLiteral("当前时间组导入需要含滤镜元数据的 VSR 预设，外部 VPY 请导入所有时间组。"),true);return;}
        if(groups.size()!=1 || groups.first().startSeconds!=0){setStatus(QStringLiteral("导入当前时间组仅支持从开头到结尾的单组预设；多组预设请导入所有时间组。"),true);return;}
        if(!generated && groups.first().script.isEmpty()){setStatus(QStringLiteral("VPY 正文已修改，不能仅导入滤镜图；请导入所有时间组以保留 Python。"),true);return;}
        timeGroups_=currentTimeGroups();timeGroups_[timeGroupIndex_].graph=graph_=groups.first().graph;timeGroups_[timeGroupIndex_].script=groups.first().script;activePreset_.clear();refreshPipeline(graph_.nodes().isEmpty()?-1:0);
        setStatus(QStringLiteral("已导入当前时间组，时间边界保持不变。"));return;
    }
    activePreset_ = path;
    if (!data.isEmpty()) {
        replaceTimeGroups(groups);
        if(generated)activePreset_.clear();
        sourceFilter_->setCurrentIndex(sourceFilter_->findData(data.value("sourceFilter").toInt()));
        refreshPipeline(graph_.nodes().isEmpty() ? -1 : 0);
        if (sourcePath_->text().isEmpty() && QFileInfo::exists(data.value("sourcePath").toString()))
            { loadSource(data.value("sourcePath").toString()); return; }
    }
    setStatus(QStringLiteral("已载入 VPY：%1").arg(QFileInfo(path).fileName()));
}
}
