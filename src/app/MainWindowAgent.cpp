#include "app/MainWindow.h"
#include "app/RenderRoute.h"
#include "app/AgentVideoFrames.h"
#include "agent/AgentSession.h"
#include "ui/AgentPage.h"
#include "ui/AnalysisPage.h"
#include "ui/CompareView.h"
#include "ui/ExportWindow.h"
#include "ui/PageTheme.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "graph/FilterCatalog.h"
#include "graph/PresetStore.h"
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProcess>
#include <QSettings>
#include <QStackedWidget>
#include <cmath>

namespace vsr {
namespace {
QJsonObject failure(const QString &message) { return {{"ok", false}, {"error", message}}; }
QJsonObject success() { return {{"ok", true}}; }
QJsonObject field(const char *type, const QString &description = {}) {
    QJsonObject result{{"type", type}};
    if (!description.isEmpty()) result.insert("description", description);
    return result;
}
QJsonObject schema(const QJsonObject &properties, const QJsonArray &required = {}) {
    return {{"type", "object"}, {"properties", properties}, {"required", required}, {"additionalProperties", false}};
}
int index(const QJsonObject &args, const char *name = "index") { return args.value(name).toInt(-1); }
bool writablePath(const QString &path, QString *error) {
    if (path.isEmpty() || !QFileInfo(path).isAbsolute()) { *error = QStringLiteral("请提供绝对文件路径。"); return false; }
    if (QFileInfo::exists(path)) { *error = QStringLiteral("目标已存在，请选择新文件名，避免覆盖现有文件。"); return false; }
    return true;
}
}

void MainWindow::initializeAgent()
{
    agentSession_ = new AgentSession(this);
    const auto add = [this](const QString &name, const QString &description, const QJsonObject &parameters, AgentSession::Handler handler) {
        agentSession_->registerTool(name, description, parameters, std::move(handler));
    };
    add("get_state", QStringLiteral("读取当前源、滤镜图、处理路、时间组、预览和编码状态。索引均从0开始。"), schema({}), [this](const QJsonObject &) {
        auto result = PresetStore::metadata(currentTimeGroups(), static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), sourcePath_->text(), {});
        result.insert("ok", true); result.insert("route", routeIndex_); result.insert("routeCount", int(routes_.size()));
        result.insert("timeGroup", timeGroupIndex_); result.insert("page", pages_->currentIndex()); result.insert("preset", activePreset_);
        result.insert("encoding", exportWindow_ && exportWindow_->isBusy());
        result.insert("exports", exportWindow_ ? exportWindow_->jobsStatus() : QJsonArray());
        const bool current = routes_[routeIndex_].previewScript == currentScript().script;
        result.insert("previewReady", current && (routeIndex_ == 0 ? vsScriptReady_ : additionalRoutes_[routeIndex_-1]->hasFrame()));
        result.insert("previewError", frameServer_ ? frameServer_->errorString() : QString());
        if (sourcePlayer_) {
            const auto snap = sourcePlayer_->snapshot();
            result.insert("sourceWidth", int(snap.videoWidth)); result.insert("sourceHeight", int(snap.videoHeight));
            result.insert("positionSeconds", snap.position100ns / 10000000.); result.insert("durationSeconds", snap.duration100ns / 10000000.);
            result.insert("playing", snap.state == ThreeFpState::Playing);
        }
        return result;
    });
    add("list_filters", QStringLiteral("列出可添加的滤镜。传id可读取该滤镜参数类型、允许范围、选项和默认值。修改前先查询。"), schema({{"id", field("string")}}), [](const QJsonObject &args) {
        QJsonArray filters;
        for (const auto &definition : FilterCatalog::all()) {
            if (!args.value("id").toString().isEmpty() && definition.id != args.value("id").toString()) continue;
            QJsonObject item{{"id", definition.id}, {"name", definition.name}, {"category", definition.category}, {"description", definition.description}};
            if (!args.value("id").toString().isEmpty()) {
                QJsonArray parameters;
                for (const auto &p : definition.parameters) parameters.append(QJsonObject{{"id", p.id}, {"label", p.label}, {"type", int(p.type)}, {"default", QJsonValue::fromVariant(p.defaultValue)}, {"minimum", p.minimum}, {"maximum", p.maximum}, {"choices", QJsonArray::fromStringList(p.choices)}});
                item.insert("parameters", parameters);
                item.insert("parameterTypes", QJsonObject{{"0", "integer"}, {"1", "number"}, {"2", "boolean"}, {"3", "choice string"}, {"4", "file path"}});
            }
            filters.append(item);
        }
        return QJsonObject{{"ok", true}, {"filters", filters}};
    });
    add("open_source", QStringLiteral("导入本地媒体到渲染页；不自动渲染。"), schema({{"path", field("string")}}, {"path"}), [this](const QJsonObject &args) {
        const auto path = args.value("path").toString();
        if (QFileInfo(path).suffix().compare("vpy", Qt::CaseInsensitive) == 0) return failure(QStringLiteral("VPY预设请使用import_preset。"));
        if (!QFileInfo(path).isFile()) return failure(QStringLiteral("源文件不存在。"));
        return loadSource(path) ? success() : failure(QStringLiteral("源文件打开失败。"));
    });
    const auto changed = [this](int selected) { activePreset_.clear(); refreshPipeline(selected); };
    add("add_filter", QStringLiteral("向当前时间组添加滤镜，可同时指定parameters。参数无效时不改变处理链。"), schema({{"id", field("string")}, {"parameters", field("object")}}, {"id"}), [this, changed](const QJsonObject &args) {
        auto candidate = graph_; const int row = candidate.add(args.value("id").toString());
        if (row < 0) return failure(QStringLiteral("未知滤镜，请先list_filters。"));
        const auto parameters = args.value("parameters").toObject();
        for (auto it = parameters.begin(); it != parameters.end(); ++it)
            if (!candidate.setParameter(row, it.key(), it.value().toVariant())) return failure(QStringLiteral("参数无效：%1").arg(it.key()));
        graph_ = candidate; changed(row); return QJsonObject{{"ok", true}, {"index", row}};
    });
    add("set_filter_parameter", QStringLiteral("修改当前时间组滤镜参数，校验类型、范围及选项。"), schema({{"index", field("integer")}, {"parameter", field("string")}, {"value", QJsonObject{}}}, {"index", "parameter", "value"}), [this, changed](const QJsonObject &args) {
        if (!graph_.setParameter(index(args), args.value("parameter").toString(), args.value("value").toVariant())) return failure(QStringLiteral("滤镜索引或参数无效。"));
        changed(index(args)); return success();
    });
    add("remove_filter", QStringLiteral("删除当前时间组指定滤镜。"), schema({{"index", field("integer")}}, {"index"}), [this, changed](const QJsonObject &args) {
        if (!graph_.remove(index(args))) return failure(QStringLiteral("滤镜索引无效。"));
        changed(-1); return success();
    });
    add("move_filter", QStringLiteral("调整滤镜执行顺序。"), schema({{"from", field("integer")}, {"to", field("integer")}}, {"from", "to"}), [this, changed](const QJsonObject &args) {
        if (!graph_.move(index(args, "from"), index(args, "to"))) return failure(QStringLiteral("滤镜索引无效。"));
        changed(index(args, "to")); return success();
    });
    add("enable_filter", QStringLiteral("启用或停用滤镜。"), schema({{"index", field("integer")}, {"enabled", field("boolean")}}, {"index", "enabled"}), [this, changed](const QJsonObject &args) {
        if (!graph_.setEnabled(index(args), args.value("enabled").toBool())) return failure(QStringLiteral("滤镜索引无效。"));
        changed(index(args)); return success();
    });
    add("manage_route", QStringLiteral("新增、选择、删除或显示/隐藏当前处理路，最多三路。select需要index。"), schema({{"action", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"add", "select", "remove", "show", "hide"}}}}, {"index", field("integer")}}, {"action"}), [this](const QJsonObject &args) {
        const auto action = args.value("action").toString();
        if (action == "add") { if (!addRoute()) return failure(QStringLiteral("最多三路。")); }
        else if (action == "select") { if (index(args) < 0 || index(args) >= routes_.size()) return failure(QStringLiteral("路索引无效。")); selectRoute(index(args)); }
        else if (action == "remove") { if (routes_.size() == 1) return failure(QStringLiteral("至少保留一路。")); removeRoute(); }
        else if (action == "show" || action == "hide") {
            const bool visible = action == "show"; routes_[routeIndex_].visible = visible; compareView_->setRouteVisible(routeIndex_, visible);
            if (routeIndex_ > 0) additionalRoutes_[routeIndex_-1]->setVisible(visible);
            else if (!visible) { frameServer_->unloadScript(); processedPlayer_->stop(); vsScriptReady_ = false; }
            updateRouteChoices();
        }
        else return failure(QStringLiteral("action无效。"));
        return QJsonObject{{"ok", true}, {"route", routeIndex_}, {"routeCount", int(routes_.size())}};
    });
    add("manage_time_group", QStringLiteral("新增、选择、删除或重命名时间组。add需要唯一正整数startSeconds，可选name；select/remove/rename需要index。"), schema({{"action", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"add", "select", "remove", "rename"}}}}, {"index", field("integer")}, {"startSeconds", field("integer")}, {"name", field("string")}}, {"action"}), [this](const QJsonObject &args) {
        auto groups = currentTimeGroups(); const auto action = args.value("action").toString(); int selected = index(args);
        if (action == "add") {
            const int second = args.value("startSeconds").toInt();
            if (second < 1 || second > 8640000) return failure(QStringLiteral("开始时间必须为1–8640000的整数秒。"));
            selected = 0; while (selected < groups.size() && groups[selected].startSeconds < second) ++selected;
            if (selected < groups.size() && groups[selected].startSeconds == second) return failure(QStringLiteral("该开始时间已存在。"));
            groups.insert(selected, {args.value("name").toString(QStringLiteral("时间组")), second, graph_, {}});
        } else {
            if (selected < 0 || selected >= groups.size()) return failure(QStringLiteral("时间组索引无效。"));
            if (action == "remove") { if (groups.size() == 1) return failure(QStringLiteral("至少保留一组。")); groups.removeAt(selected); groups.first().startSeconds = 0; selected = qMin(selected, int(groups.size())-1); }
            else if (action == "rename") { if (args.value("name").toString().trimmed().isEmpty()) return failure(QStringLiteral("名称不能为空。")); groups[selected].name = args.value("name").toString(); }
            else if (action != "select") return failure(QStringLiteral("action无效。"));
        }
        if (action != "select") activePreset_.clear();
        timeGroups_ = groups; refreshTimeGroups(selected); return success();
    });
    add("import_project", QStringLiteral("载入.vsr.json项目，替换当前工作区。"), schema({{"path", field("string")}}, {"path"}), [this](const QJsonObject &args) {
        QString error; return openProjectFrom(args.value("path").toString(), &error) ? success() : failure(error);
    });
    add("export_project", QStringLiteral("保存完整多路项目到新.vsr.json文件。"), schema({{"path", field("string")}}, {"path"}), [this](const QJsonObject &args) {
        QString error; const auto path = args.value("path").toString();
        if (!writablePath(path, &error)) return failure(error);
        return saveProjectTo(path, &error) ? success() : failure(error);
    });
    add("import_preset", QStringLiteral("导入VPY预设，可仅替换当前时间组。"), schema({{"path", field("string")}, {"currentGroup", field("boolean")}}, {"path"}), [this](const QJsonObject &args) {
        const auto path = args.value("path").toString(); const auto result = PresetStore::load(path);
        if (!result.errors.isEmpty()) return failure(result.errors.join('\n'));
        const auto metadata = PresetStore::metadata(result.script); const bool current = args.value("currentGroup").toBool();
        if (current) {
            const auto groups = PresetStore::timeGroups(metadata); QFile file(path); FilterGraph graph; SourceFilter filter;
            const bool verified = file.open(QIODevice::ReadOnly) && PresetStore::verifiedGraph(QString::fromUtf8(file.readAll()), &graph, &filter);
            if (metadata.isEmpty() || groups.size() != 1 || groups.first().startSeconds != 0 || (!verified && groups.first().script.isEmpty())) return failure(QStringLiteral("该预设不能仅导入当前时间组，请导入完整预设。"));
        }
        loadPreset(path, current); return success();
    });
    add("export_preset", QStringLiteral("导出当前处理路为可分享的VPY新文件，保留时间组和自定义正文。"), schema({{"path", field("string")}, {"note", field("string")}}, {"path"}), [this](const QJsonObject &args) {
        QString error; const auto path = args.value("path").toString(); if (!writablePath(path, &error)) return failure(error);
        QString script;
        if (!activePreset_.isEmpty()) { QFile file(activePreset_); if (!file.open(QIODevice::ReadOnly)) return failure(file.errorString()); script = QString::fromUtf8(file.readAll()); }
        else script = PresetStore::create(currentTimeGroups(), static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), sourcePath_->text(), args.value("note").toString());
        return PresetStore::write(path, script, &error) ? success() : failure(error);
    });
    add("get_script", QStringLiteral("生成当前VPY并返回依赖和错误，不执行Python。"), schema({}), [this](const QJsonObject &) {
        const auto result = currentScript(); return QJsonObject{{"ok", result.errors.isEmpty()}, {"script", result.script}, {"errors", QJsonArray::fromStringList(result.errors)}, {"requiredNamespaces", QJsonArray::fromStringList(result.requiredNamespaces)}};
    });
    add("render_preview", QStringLiteral("请求实际渲染当前路。异步加载，随后get_state的previewReady才表示成功。"), schema({}), [this](const QJsonObject &) {
        if (!routes_[routeIndex_].visible) return failure(QStringLiteral("当前路已隐藏，请先显示该路。"));
        if (sourcePath_->text().isEmpty()) return failure(QStringLiteral("请先导入源媒体。"));
        const auto script = currentScript();
        if (!script.errors.isEmpty()) return failure(script.errors.join('\n'));
        validateScript(); return QJsonObject{{"ok", true}, {"status", "requested"}};
    });
    add("enqueue_export", QStringLiteral("按当前滤镜链实际入队编码，异步执行。get_state.exports的state=completed才代表完成，failed代表失败。"), schema({{"command", field("string", QStringLiteral("FFmpeg命令模板，使用占位符<输入文件>和<输出文件>，例如 ffmpeg -i <输入文件> -c:v libx264 -crf 18 -an <输出文件>"))}, {"output", field("string", QStringLiteral("尚不存在的新输出文件绝对路径"))}}, {"command", "output"}), [this](const QJsonObject &args) {
        const auto script = currentScript(); if (!script.errors.isEmpty()) return failure(script.errors.join('\n'));
        QString error; const auto output = args.value("output").toString(); if (!writablePath(output, &error)) return failure(error);
        if (sourcePath_->text().isEmpty()) return failure(QStringLiteral("请先导入媒体。"));
        exportCurrentResult(); const double duration = sourcePlayer_->snapshot().duration100ns / 10000000.;
        return exportWindow_->addJob(args.value("command").toString(), output, sourcePath_->text(), script.script, duration, &error) ? QJsonObject{{"ok", true}, {"status", "queued"}, {"output", output}} : failure(error);
    });
    add("stop_exports", QStringLiteral("停止当前编码队列，保留已有输出文件。"), schema({}), [this](const QJsonObject &) { if (exportWindow_) exportWindow_->stopAll(); return success(); });
    add("playback", QStringLiteral("控制渲染源播放：play/pause/seek，seek需要seconds。"), schema({{"action", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"play", "pause", "seek"}}}}, {"seconds", field("number")}}, {"action"}), [this](const QJsonObject &args) {
        if (sourcePath_->text().isEmpty()) return failure(QStringLiteral("没有打开的源媒体。"));
        const auto action = args.value("action").toString(); bool ok = false;
        if (action == "play") ok = sourcePlayer_->play(); else if (action == "pause") ok = sourcePlayer_->pause();
        else if (action == "seek") { const double seconds = args.value("seconds").toDouble(-1); if (!std::isfinite(seconds) || seconds < 0 || seconds > sourcePlayer_->snapshot().duration100ns / 10000000.) return failure(QStringLiteral("时间超出媒体范围。")); ok = sourcePlayer_->seek(qRound64(seconds * 10000000.)); }
        return ok ? success() : failure(QStringLiteral("播放操作失败。"));
    });
    add("open_comparison", QStringLiteral("把多个本地媒体导入图像分析比对页，tracks为2/3/4。"), schema({{"paths", QJsonObject{{"type", "array"}, {"items", field("string")}, {"minItems", 1}, {"maxItems", 9}}}, {"tracks", QJsonObject{{"type", "integer"}, {"enum", QJsonArray{2,3,4}}}}}, {"paths"}), [this](const QJsonObject &args) {
        QStringList paths; for (const auto &p : args.value("paths").toArray()) { if (!QFileInfo(p.toString()).isFile()) return failure(QStringLiteral("比对文件不存在。")); paths.append(p.toString()); }
        if (paths.isEmpty() || paths.size() > 9) return failure(QStringLiteral("请提供1–9个文件。"));
        selectPage(1); analysisPage_->setMode(args.value("tracks").toInt(2)); analysisPage_->openFiles(paths); return QJsonObject{{"ok", true}, {"status", "requested"}};
    });
    add("show_page", QStringLiteral("切换页面：0渲染/1比对/2设置/3BDRemux/4Agent。"), schema({{"index", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"maximum", 4}}}}, {"index"}), [this](const QJsonObject &args) {
        if (index(args) < 0 || index(args) > 4) return failure(QStringLiteral("页面索引无效。"));
        selectPage(index(args)); return success();
    });
    add("list_files", QStringLiteral("列出指定本地文件夹，不递归。最多200项。"), schema({{"path", field("string")}}, {"path"}), [](const QJsonObject &args) {
        QDir dir(args.value("path").toString()); if (!dir.exists() || args.value("path").toString().isEmpty()) return failure(QStringLiteral("文件夹不存在。"));
        QJsonArray files; const auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);
        for (const auto &entry : entries) { if (files.size() >= 200) break; files.append(QJsonObject{{"path", entry.absoluteFilePath()}, {"directory", entry.isDir()}, {"bytes", double(entry.size())}}); }
        return QJsonObject{{"ok", true}, {"entries", files}, {"truncated", entries.size() > 200}};
    });
    new AgentVideoFrames(agentSession_, [this](bool processed, QString *error) {
        if (sourcePath_->text().isEmpty()) { *error = QStringLiteral("请先导入源媒体。"); return QString(); }
        const auto result = processed ? currentScript() : VpyScriptBuilder::build(sourcePath_->text(), static_cast<SourceFilter>(sourceFilter_->currentData().toInt()), FilterGraph{});
        *error = result.errors.join('\n'); return result.script;
    });
    initializeAgentFeatures();
}
}
