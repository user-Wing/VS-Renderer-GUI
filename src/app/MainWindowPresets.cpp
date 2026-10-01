#include "app/MainWindow.h"
#include "graph/PresetStore.h"
#include "ui/PresetDialog.h"
#include <QComboBox>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>

namespace vsr {
ScriptBuildResult MainWindow::currentScript(const QString &source) const
{
    const QString input = source.isEmpty() ? sourcePath_->text() : source;
    if (!activePreset_.isEmpty()) return PresetStore::load(activePreset_, input);
    return VpyScriptBuilder::build(input, static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), graph_);
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
        return PresetStore::create(graph_, static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), sourcePath_->text(), note);
    }, [this](const QString &path) { loadPreset(path); }, [this] {
        activePreset_.clear(); graph_ = FilterGraph(); refreshPipeline();
    }, this);
    dialog.exec();
}
void MainWindow::loadPreset(const QString &path)
{
    const auto result = PresetStore::load(path);
    if (!result.errors.isEmpty()) { setStatus(result.errors.join('\n'), true); return; }
    const auto data = PresetStore::metadata(result.script);
    activePreset_ = path;
    if (!data.isEmpty()) {
        graph_ = PresetStore::graph(data);
        sourceFilter_->setCurrentIndex(sourceFilter_->findData(data.value("sourceFilter").toInt()));
        refreshPipeline(graph_.nodes().isEmpty() ? -1 : 0);
        if (sourcePath_->text().isEmpty() && QFileInfo::exists(data.value("sourcePath").toString()))
            { loadSource(data.value("sourcePath").toString()); return; }
    }
    setStatus(QStringLiteral("已载入 VPY：%1").arg(QFileInfo(path).fileName()));
    if (!sourcePath_->text().isEmpty()) validateScript();
}
}
