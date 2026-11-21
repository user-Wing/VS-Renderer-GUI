#pragma once

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

namespace vsr {

inline constexpr qint64 cubeFileLimit = 32 * 1024 * 1024;

inline QDir cubeLibrary(const QString &programDirectory = QCoreApplication::applicationDirPath())
{
    return QDir(QDir(programDirectory).filePath("luts"));
}

inline QDir playerCubeLibrary()
{
#ifdef VSR_LITE_PLAYER
    const auto launcher = qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH");
    if (QDir::isAbsolutePath(launcher)) return cubeLibrary(QFileInfo(launcher).absolutePath());
#endif
    return cubeLibrary();
}

inline QString cubeHeader(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.read(8192));
}

inline bool cubeUsesDisplayP3(const QString &path)
{
    const QString header = cubeHeader(path);
    using Options = QRegularExpression::PatternOption;
    static const QRegularExpression p3(
        QStringLiteral(R"((?<![a-z0-9])(?:display[\s_-]*p3|smpte[\s_-]*432|p3)(?![a-z0-9]))"),
        Options::CaseInsensitiveOption);
    static const QRegularExpression srgb(QStringLiteral(R"((?<![a-z0-9])sRGB(?![a-z0-9]))"), Options::CaseInsensitiveOption);
    if (p3.match(header).hasMatch()) return true;
    if (srgb.match(header).hasMatch()) return false;
    return p3.match(QFileInfo(path).completeBaseName()).hasMatch();
}

inline QString cubeLabel(const QString &path)
{
    static const QRegularExpression title(QStringLiteral("\\bTITLE\\s+\"([^\"\\r\\n]+)\""));
    const auto name = title.match(cubeHeader(path)).captured(1).trimmed();
    return name.isEmpty() ? QFileInfo(path).completeBaseName() : name.left(100);
}

inline QString importCubeFile(const QString &path, const QDir &library, QString *error = nullptr)
{
    const auto fail = [error](const QString &message) {
        if (error) *error = message;
        return QString();
    };
    const QFileInfo info(path);
    QFile input(path);
    if (info.suffix().compare("cube", Qt::CaseInsensitive) != 0 || !info.isFile() ||
        !input.open(QIODevice::ReadOnly) || input.size() <= 0 || input.size() > cubeFileLimit)
        return fail(QCoreApplication::translate("CubeLut", "无法读取 .cube 文件，或文件超过 32 MiB。"));
    if (info.dir().canonicalPath() == library.canonicalPath()) return info.absoluteFilePath();
    if (!QDir().mkpath(library.absolutePath()))
        return fail(QCoreApplication::translate("CubeLut", "无法创建程序目录下的 LUT 库。"));
    const auto bytes = input.read(cubeFileLimit + 1);
    if (bytes.isEmpty() || bytes.size() > cubeFileLimit || input.error() != QFile::NoError)
        return fail(input.errorString());
    auto saved = library.absoluteFilePath(info.fileName());
    const auto matches = [&bytes](const QString &candidate) {
        QFile existing(candidate);
        return existing.open(QIODevice::ReadOnly) && existing.size() == bytes.size() && existing.readAll() == bytes;
    };
    const auto hash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    const auto legacyName = hash + ".cube";
    const auto legacy = library.absoluteFilePath(legacyName);
    if (info.fileName() == legacyName) {
        for (const auto &name : library.entryList({"*.cube"}, QDir::Files | QDir::NoSymLinks)) {
            const auto candidate = library.absoluteFilePath(name);
            if (name != legacyName && matches(candidate)) return candidate;
        }
    } else if (!QFileInfo::exists(saved) && matches(legacy) && cubeUsesDisplayP3(path) == cubeUsesDisplayP3(legacy)) {
        return legacy;
    }
    if (QFileInfo::exists(saved)) {
        if (matches(saved)) return saved;
        saved = library.absoluteFilePath(info.completeBaseName().left(110) + "-" + hash + ".cube");
        if (QFileInfo::exists(saved)) return saved;
    }
    QSaveFile output(saved);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit())
        return fail(QCoreApplication::translate("CubeLut", "无法保存 LUT 副本。"));
    return saved;
}

}
