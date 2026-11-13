#include "agent/AgentInstructions.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QTemporaryDir>

namespace vsr {
namespace {
constexpr qint64 textLimit = 512 * 1024;
const auto defaultPrompt = QStringLiteral(
    "# Renderer Agent\n\n"
    "先用 get_state 核实工作区，添加或调整滤镜前用 list_filters 查询真实参数。\n"
    "任务匹配技能说明时，先调用 read_skill 读取 SKILL.md，再按需读取其中的相对路径资料。\n"
    "使用软件实际提供的工具执行操作；按工具结果汇报已完成、失败与仍在等待的工作。\n"
    "预览请求和编码入队属于异步操作，完成情况以实际状态、导出窗口和输出文件为准。\n");

bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

bool writeText(const QString &path, const QByteArray &bytes, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return fail(error, QStringLiteral("无法保存文件：%1").arg(file.errorString()));
    return true;
}

bool readText(const QString &path, QString *text, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(error, QStringLiteral("无法读取文件：%1").arg(QFileInfo(path).fileName()));
    if (file.size() > textLimit) return fail(error, QStringLiteral("文本文件超过 512 KiB，请拆分为多个 reference。"));
    const auto bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    *text = decoder(bytes);
    if (decoder.hasError() || text->contains(QChar(0))) return fail(error, QStringLiteral("技能资料必须是有效 UTF-8 文本。"));
    return true;
}

bool linked(const QFileInfo &info) { return info.isSymLink() || info.isJunction(); }

bool inside(const QString &path, const QString &root)
{
    const auto full = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    const auto base = QDir::cleanPath(QFileInfo(root).absoluteFilePath());
    return full.startsWith(base + '/', Qt::CaseInsensitive);
}

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 && ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\''))))
        value = value.mid(1, value.size() - 2);
    return value;
}

bool metadata(const QString &path, const QString &fallback, QJsonObject *result, QString *error)
{
    QString text;
    if (!readText(path, &text, error)) return false;
    const auto lines = text.split('\n');
    QString name = fallback, description;
    if (!lines.isEmpty() && lines.first().trimmed() == "---") {
        bool closed = false;
        for (int i = 1; i < lines.size(); ++i) {
            const auto line = lines[i].trimmed();
            if (line == "---") { closed = true; break; }
            if (line.startsWith("name:")) name = unquote(line.mid(5));
            if (line.startsWith("description:")) {
                description = unquote(line.mid(12));
                if (description == ">" || description == "|" || description == ">-" || description == "|-") {
                    description.clear();
                    while (i + 1 < lines.size() && (lines[i + 1].startsWith(' ') || lines[i + 1].startsWith('\t')))
                        description += " " + lines[++i].trimmed();
                }
            }
        }
        if (!closed) return fail(error, QStringLiteral("SKILL.md 的 frontmatter 缺少结束分隔符。"));
    }
    static const QRegularExpression validName(QStringLiteral("^[\\p{L}\\p{N}][\\p{L}\\p{N}._-]{0,79}$"));
    static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\..*)?$"), QRegularExpression::CaseInsensitiveOption);
    if (!validName.match(name).hasMatch() || name.endsWith('.') || reserved.match(name).hasMatch())
        return fail(error, QStringLiteral("技能名称须为 1–80 个字母、数字、点、下划线或连字符，且可用于文件夹名称。"));
    if (description.trimmed().isEmpty()) {
        for (const auto &line : lines) {
            const auto candidate = line.trimmed();
            if (!candidate.isEmpty() && candidate != "---" && !candidate.startsWith('#') && !candidate.startsWith("name:")) { description = candidate; break; }
        }
    }
    *result = {{"name", name}, {"description", description.simplified().left(1000)}};
    return true;
}
}

AgentInstructions::AgentInstructions(const QString &rootDirectory, QObject *parent) : QObject(parent)
{
    root_ = QDir::cleanPath(rootDirectory.isEmpty() ? QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent") : QFileInfo(rootDirectory).absoluteFilePath());
    if (QDir().mkpath(QDir(root_).filePath("skills")) && !QFileInfo::exists(globalPromptPath())) {
        QFile file(globalPromptPath());
        if (file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) file.write(defaultPrompt.toUtf8());
    }
    refresh();
}

QString AgentInstructions::globalPromptPath() const { return QDir(root_).filePath("AGENTS.md"); }

QString AgentInstructions::loadGlobalPrompt(QString *error) const
{
    if (error) error->clear();
    QString text;
    readText(globalPromptPath(), &text, error);
    return text;
}

bool AgentInstructions::saveGlobalPrompt(const QString &text, QString *error)
{
    if (error) error->clear();
    if (text.toUtf8().size() > textLimit || text.contains(QChar(0))) return fail(error, QStringLiteral("全局提示词须为不超过 512 KiB 的文本。"));
    if (!writeText(globalPromptPath(), text.toUtf8(), error)) return false;
    globalPrompt_ = text;
    emit changed();
    return true;
}

bool AgentInstructions::refresh(QString *error)
{
    if (error) error->clear();
    QString text;
    if (!readText(globalPromptPath(), &text, error)) return false;
    QJsonObject enabled;
    QFile settings(QDir(root_).filePath("skills.json"));
    if (settings.exists()) {
        if (!settings.open(QIODevice::ReadOnly) || settings.size() > textLimit) return fail(error, QStringLiteral("无法读取技能启用设置。"));
        QJsonParseError parse;
        const auto doc = QJsonDocument::fromJson(settings.readAll(), &parse);
        if (parse.error != QJsonParseError::NoError || !doc.isObject() || doc.object().value("schemaVersion") != QJsonValue(1) || !doc.object().value("enabled").isObject())
            return fail(error, QStringLiteral("技能启用设置格式无效。"));
        enabled = doc.object().value("enabled").toObject();
        for (auto it = enabled.begin(); it != enabled.end(); ++it) if (!it.value().isBool()) return fail(error, QStringLiteral("技能启用设置必须为布尔值。"));
    }
    const auto skillsRoot = QDir(root_).filePath("skills");
    if (linked(QFileInfo(skillsRoot))) return fail(error, QStringLiteral("技能根目录不能是符号链接或目录联接。"));
    QJsonArray skills;
    QSet<QString> names;
    for (const auto &entry : QDir(skillsRoot).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (linked(entry)) return fail(error, QStringLiteral("技能文件夹不能是符号链接或目录联接。"));
        const auto path = QDir(entry.absoluteFilePath()).filePath("SKILL.md");
        if (!QFileInfo::exists(path)) continue;
        if (linked(QFileInfo(path))) return fail(error, QStringLiteral("SKILL.md 不能是符号链接。"));
        QJsonObject item;
        if (!metadata(path, entry.fileName(), &item, error)) return false;
        const auto name = item.value("name").toString();
        if (names.contains(name.toCaseFolded())) return fail(error, QStringLiteral("技能名称重复：%1").arg(name));
        names.insert(name.toCaseFolded());
        item.insert("path", entry.absoluteFilePath()); item.insert("enabled", enabled.value(name).toBool(true)); skills.append(item);
    }
    globalPrompt_ = text; enabled_ = enabled; skills_ = skills;
    emit changed();
    return true;
}

QString AgentInstructions::systemPrompt() const
{
    QString text = globalPrompt_;
    QStringList entries;
    for (const auto &value : skills_) {
        const auto skill = value.toObject();
        if (skill.value("enabled").toBool()) entries.append("- " + skill.value("name").toString() + ": " + skill.value("description").toString());
    }
    if (!entries.isEmpty()) text += QStringLiteral("\n\n## 可用技能\n任务符合说明时，用 read_skill(name, path=SKILL.md) 读取入口；技能正文与资料按需读取。\n") + entries.join('\n');
    return text;
}

bool AgentInstructions::saveEnabled(const QJsonObject &enabled, QString *error)
{
    if (!writeText(QDir(root_).filePath("skills.json"), QJsonDocument(QJsonObject{{"schemaVersion", 1}, {"enabled", enabled}}).toJson(), error)) return false;
    enabled_ = enabled;
    return true;
}

bool AgentInstructions::importSkill(const QString &input, QString *error)
{
    if (error) error->clear();
    if (!refresh(error)) return false;
    const QFileInfo source(input);
    if (!source.exists() || linked(source)) return fail(error, QStringLiteral("请选择普通技能文件或包含 SKILL.md 的文件夹。"));
    const auto entryPath = source.isDir() ? QDir(source.absoluteFilePath()).filePath("SKILL.md") : source.absoluteFilePath();
    if (linked(QFileInfo(entryPath))) return fail(error, QStringLiteral("技能入口不能是符号链接。"));
    QJsonObject item;
    if (!metadata(entryPath, source.isDir() ? source.fileName() : source.completeBaseName(), &item, error)) return false;
    const auto name = item.value("name").toString();
    for (const auto &value : skills_) if (value.toObject().value("name").toString().compare(name, Qt::CaseInsensitive) == 0) return fail(error, QStringLiteral("技能已存在，请先删除旧版本：%1").arg(name));
    const auto target = QDir(root_).filePath("skills/" + name);
    if (QFileInfo::exists(target)) return fail(error, QStringLiteral("同名技能目录已存在。"));
    QList<QFileInfo> entries;
    qint64 bytes = 0;
    if (source.isDir()) {
        QDirIterator iterator(source.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next(); const auto entry = iterator.fileInfo();
            if (linked(entry) || (!entry.isFile() && !entry.isDir())) return fail(error, QStringLiteral("技能包不能包含符号链接、目录联接或特殊文件。"));
            if (entry.isFile()) bytes += entry.size();
            entries.append(entry);
            if (entries.size() > 1024 || bytes > 32 * 1024 * 1024) return fail(error, QStringLiteral("技能包超过 1024 项或 32 MiB。"));
        }
    } else if (!source.isFile()) return fail(error, QStringLiteral("技能入口必须是普通文件。"));
    QTemporaryDir staging(QDir(root_).filePath(".skill-import-XXXXXX"));
    if (!staging.isValid()) return fail(error, QStringLiteral("无法创建技能导入目录。"));
    const auto bundle = staging.filePath("bundle");
    if (!QDir().mkpath(bundle)) return fail(error, QStringLiteral("无法创建技能目录。"));
    if (source.isDir()) {
        const QDir sourceDir(source.absoluteFilePath());
        for (const auto &entry : entries) {
            const auto destination = QDir(bundle).filePath(sourceDir.relativeFilePath(entry.absoluteFilePath()));
            if ((entry.isDir() && !QDir().mkpath(destination)) || (entry.isFile() && (!QDir().mkpath(QFileInfo(destination).absolutePath()) || !QFile::copy(entry.absoluteFilePath(), destination))))
                return fail(error, QStringLiteral("无法复制完整技能包。"));
        }
    } else if (!QFile::copy(entryPath, QDir(bundle).filePath("SKILL.md"))) return fail(error, QStringLiteral("无法复制技能入口。"));
    if (!inside(target, QDir(root_).filePath("skills")) || !QDir().rename(bundle, target)) return fail(error, QStringLiteral("无法安装技能目录。"));
    return refresh(error);
}

bool AgentInstructions::removeSkill(const QString &name, QString *error)
{
    if (error) error->clear();
    if (!refresh(error)) return false;
    for (const auto &value : skills_) {
        const auto item = value.toObject();
        if (item.value("name").toString() != name) continue;
        const auto path = item.value("path").toString();
        const auto skillsRoot = QDir(root_).filePath("skills");
        if (!inside(path, skillsRoot) || !inside(QFileInfo(path).canonicalFilePath(), QFileInfo(skillsRoot).canonicalFilePath()) || linked(QFileInfo(path)))
            return fail(error, QStringLiteral("技能目录超出管理范围。"));
        QDirIterator entries(path, QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (entries.hasNext()) { entries.next(); if (linked(entries.fileInfo())) return fail(error, QStringLiteral("请先移除技能目录中的符号链接或目录联接。")); }
        if (!QDir(path).removeRecursively())
            return fail(error, QStringLiteral("无法删除技能目录。"));
        auto enabled = enabled_; enabled.remove(name);
        if (!saveEnabled(enabled, error)) return false;
        return refresh(error);
    }
    return fail(error, QStringLiteral("找不到技能：%1").arg(name));
}

bool AgentInstructions::setSkillEnabled(const QString &name, bool enabled, QString *error)
{
    if (error) error->clear();
    for (const auto &value : skills_) {
        if (value.toObject().value("name").toString() != name) continue;
        auto states = enabled_; states.insert(name, enabled);
        return saveEnabled(states, error) && refresh(error);
    }
    return fail(error, QStringLiteral("找不到技能：%1").arg(name));
}

QJsonObject AgentInstructions::readSkill(const QString &name, const QString &relativePath) const
{
    const auto failure = [](const QString &error) { return QJsonObject{{"ok", false}, {"error", error}}; };
    QJsonObject skill;
    for (const auto &value : skills_) if (value.toObject().value("name").toString() == name) { skill = value.toObject(); break; }
    if (skill.isEmpty() || !skill.value("enabled").toBool()) return failure(QStringLiteral("技能不存在或已禁用。"));
    const auto relative = QString(relativePath).replace('\\', '/');
    if (relative.isEmpty() || QFileInfo(relative).isAbsolute() || relative.contains(':') || relative.split('/').contains("..")) return failure(QStringLiteral("资料路径须位于当前技能包内。"));
    const auto base = skill.value("path").toString();
    const auto skillsRoot = QDir(root_).filePath("skills");
    if (linked(QFileInfo(skillsRoot)) || !inside(base, skillsRoot) ||
        !inside(QFileInfo(base).canonicalFilePath(), QFileInfo(skillsRoot).canonicalFilePath())) return failure(QStringLiteral("技能目录超出管理范围。"));
    auto path = base;
    for (const auto &part : relative.split('/', Qt::SkipEmptyParts)) {
        path = QDir(path).filePath(part);
        if (linked(QFileInfo(path))) return failure(QStringLiteral("不能读取符号链接或目录联接。"));
    }
    if (linked(QFileInfo(base)) || !inside(path, base) || !inside(QFileInfo(path).canonicalFilePath(), QFileInfo(base).canonicalFilePath())) return failure(QStringLiteral("资料路径须位于当前技能包内。"));
    QString text, error;
    if (!readText(path, &text, &error)) return failure(error);
    return {{"ok", true}, {"name", name}, {"relativePath", relative}, {"content", text}};
}

}
