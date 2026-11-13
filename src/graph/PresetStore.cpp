#include "graph/PresetStore.h"
#include "graph/FilterCatalog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <algorithm>

namespace vsr {
QString PresetStore::compatibilityError(const QJsonObject &data){
    if(data.value("schemaVersion").toInt(1)>3 || data.value("schema").toInt(1)>3)return QStringLiteral("此项目/预设版本较新，请通过组件更新使用最新版 VS Renderer。为保护原项目，已停止加载。");
    for(const auto &value:data.value("nodes").toArray()){const auto id=value.toObject().value("id").toString();if(!FilterCatalog::find(id))return QStringLiteral("无法识别滤镜 %1，请更新到最新版 VS Renderer；原项目未更改。").arg(id);}
    for(const auto *key:{"timeGroups","routes"})for(const auto &value:data.value(key).toArray()){const auto e=compatibilityError(value.toObject());if(!e.isEmpty())return e;}
    if(data.value("routes").toArray().size()>3)return QStringLiteral("此项目包含更多处理路，请更新到支持该项目的版本。");
    return {};
}

QString PresetStore::directory() { return QDir(QCoreApplication::applicationDirPath()).filePath("vpy"); }
QString PresetStore::description(const QString &path)
{
    const QDir root(directory());
    const auto relative = root.relativeFilePath(path);
    if (relative.startsWith("builtin/")) {
        QFile descriptions(root.filePath("preset-descriptions.json"));
        if (descriptions.open(QIODevice::ReadOnly))
            return QJsonDocument::fromJson(descriptions.readAll()).object().value(relative).toString();
        return {};
    }
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? metadata(QString::fromUtf8(file.readAll())).value("note").toString() : QString();
}
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
    QJsonArray entries;for(const auto &group:groups){auto entry=metadata(group.graph,filter,{},{});entry.insert("name",group.name);entry.insert("startSeconds",group.startSeconds);if(group.startFrame>=0)entry.insert("startFrame",group.startFrame);if(!group.script.isEmpty())entry.insert("script",group.script);entries<<entry;}
    data.insert("schema",std::any_of(groups.begin(),groups.end(),[](const auto &g){return g.startFrame>=0;})?3:2);data.insert("timeGroups",entries);return data;
}
QString PresetStore::create(const QList<FilterTimeGroup> &groups,SourceFilter filter,const QString &source,const QString &note) {
    const auto data=QJsonDocument(metadata(groups,filter,source,note)).toJson(QJsonDocument::Compact).toBase64();
    return "# VSR_PRESET "+QString::fromLatin1(data)+"\n"+VpyScriptBuilder::buildGroups(source.isEmpty()?QString("__VSR_INPUT__"):source,filter,groups).script;
}
QList<FilterTimeGroup> PresetStore::timeGroups(const QJsonObject &data) {
    QList<FilterTimeGroup> groups;
    for(const auto &entry:data.value("timeGroups").toArray()){const auto item=entry.toObject();groups.append({item.value("name").toString(),qint64(item.value("startSeconds").toDouble()),graph(item),item.value("script").toString(),item.value("startFrame").toInteger(-1)});}
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
bool PresetStore::verifiedGraph(const QString &script,FilterGraph *output,SourceFilter *filter) {
    const auto data=metadata(script);const int schema=data.value("schema").toInt();
    if((schema!=1 && schema!=2 && schema!=3) || (schema>=2 && data.value("timeGroups").toArray().size()!=1))return false;
    const auto groups=timeGroups(data);if(groups.size()!=1 || groups.first().startSeconds!=0 || groups.first().startFrame>0 || !groups.first().script.isEmpty())return false;
    const auto graph=groups.first().graph;
    const auto nodes=data.value("schema").toInt()>=2?data.value("timeGroups").toArray().first().toObject().value("nodes").toArray():data.value("nodes").toArray();
    if(nodes.size()!=graph.nodes().size())return false;
    const auto sourceFilter=static_cast<SourceFilter>(data.value("sourceFilter").toInt());
    if(sourceFilter!=SourceFilter::Ffms2 && sourceFilter!=SourceFilter::Lsmas)return false;
    auto expected=data.value("schema").toInt()>=2?create(groups,sourceFilter,data.value("sourcePath").toString(),data.value("note").toString()):create(graph,sourceFilter,data.value("sourcePath").toString(),data.value("note").toString());
    const auto body=[](QString text){text.replace("\r\n","\n");if(!text.startsWith("# VSR_PRESET "))return QString();return text.mid(text.indexOf('\n')+1).trimmed();};
    const auto actual=body(script),current=body(expected);
    // Only strip known generated telemetry from OUR expected template. Never
    // discard arbitrary Python lines from the external script being verified.
    QStringList legacyLines;for(const auto &line:current.split('\n'))if(line!="_vsr_trace = []"&&!line.startsWith("_vsr_trace.append(")&&!line.startsWith("clip = core.std.SetFrameProp(clip, prop='_VsrTrace'"))legacyLines<<line;
    if(actual.isEmpty() || (actual!=current && actual!=legacyLines.join('\n').trimmed()))return false;
    *output=graph;*filter=sourceFilter;return true;
}
ScriptBuildResult PresetStore::load(const QString &path, const QString &source)
{
    ScriptBuildResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { result.errors << file.errorString(); return result; }
    result.script = QString::fromUtf8(file.readAll());
    const auto compatibility=compatibilityError(metadata(result.script));if(!compatibility.isEmpty()){result.errors<<compatibility;return result;}
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
