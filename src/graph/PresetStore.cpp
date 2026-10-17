#include "graph/PresetStore.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace vsr {
QString PresetStore::directory() { return QDir(QCoreApplication::applicationDirPath()).filePath("vpy"); }
QJsonObject PresetStore::metadata(const FilterGraph &graph, SourceFilter filter, const QString &source, const QString &note)
{
    QJsonArray nodes;
    for (const auto &node : graph.nodes())
        nodes.append(QJsonObject{{"id", node.definitionId}, {"enabled", node.enabled}, {"parameters", QJsonObject::fromVariantMap(node.parameters)}});
    return {{"schema", 1}, {"nodes", nodes}, {"sourceFilter", static_cast<int>(filter)}, {"sourcePath", source}, {"note", note}};
}
QString PresetStore::create(const FilterGraph &graph, SourceFilter filter, const QString &source, const QString &note)
{
    const auto data = QJsonDocument(metadata(graph, filter, source, note)).toJson(QJsonDocument::Compact).toBase64();
    return "# VSR_PRESET " + QString::fromLatin1(data) + QStringLiteral("\nimport os\n_vsr_directory = globals().get('_vsr_directory', os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))\n") + VpyScriptBuilder::build(source.isEmpty() ? QStringLiteral("__VSR_INPUT__") : source, filter, graph).script;
}
QJsonObject PresetStore::metadata(const QString &script)
{
    for (const auto &line : script.split('\n'))
        if (line.startsWith("# VSR_PRESET "))
            return QJsonDocument::fromJson(QByteArray::fromBase64(line.mid(13).toLatin1())).object();
    return {};
}
QJsonObject PresetStore::metadata(const QList<FilterTimeGroup> &groups,SourceFilter filter,const QString &source,const QString &note) {
    auto data=metadata(groups.isEmpty()?FilterGraph():groups.first().graph,filter,source,note);
    QJsonArray entries;for(const auto &group:groups){auto entry=metadata(group.graph,filter,{},{});entry.insert("name",group.name);entry.insert("startSeconds",group.startSeconds);if(!group.script.isEmpty())entry.insert("script",group.script);entries<<entry;}
    data.insert("schema",2);data.insert("timeGroups",entries);return data;
}
QString PresetStore::create(const QList<FilterTimeGroup> &groups,SourceFilter filter,const QString &source,const QString &note) {
    const auto data=QJsonDocument(metadata(groups,filter,source,note)).toJson(QJsonDocument::Compact).toBase64();
    return "# VSR_PRESET "+QString::fromLatin1(data)+"\n"+VpyScriptBuilder::buildGroups(source.isEmpty()?QString("__VSR_INPUT__"):source,filter,groups).script;
}
QList<FilterTimeGroup> PresetStore::timeGroups(const QJsonObject &data) {
    QList<FilterTimeGroup> groups;
    for(const auto &entry:data.value("timeGroups").toArray()){const auto item=entry.toObject();groups.append({item.value("name").toString(),qint64(item.value("startSeconds").toDouble()),graph(item),item.value("script").toString()});}
    if(groups.isEmpty())groups.append({QStringLiteral("全片"),0,graph(data)});
    return groups;
}
FilterGraph PresetStore::graph(const QJsonObject &data)
{
    FilterGraph graph;
    for (const auto &entry : data.value("nodes").toArray()) {
        const auto object = entry.toObject();
        const int row = graph.add(object.value("id").toString());
        if (row < 0) continue;
        graph.setEnabled(row, object.value("enabled").toBool(true));
        const auto params = object.value("parameters").toObject().toVariantMap();
        for (auto it = params.begin(); it != params.end(); ++it) graph.setParameter(row, it.key(), it.value());
    }
    return graph;
}
ScriptBuildResult PresetStore::load(const QString &path, const QString &source)
{
    ScriptBuildResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { result.errors << file.errorString(); return result; }
    result.script = QString::fromUtf8(file.readAll());
    result.script = VpyScriptBuilder::upgradeSharpen(result.script,graph(metadata(result.script)));
    result.script = VpyScriptBuilder::networkSource(result.script,source);
    QString prefix = "_vsr_directory = " + VpyScriptBuilder::pythonString(QCoreApplication::applicationDirPath()) + '\n';
    if (!source.isEmpty()) prefix += "_vsr_source = " + VpyScriptBuilder::pythonString(source) + '\n';
    result.script.prepend(prefix);
    return result;
}
bool PresetStore::write(const QString &path, const QString &script, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(script.toUtf8()) < 0 || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}
}
