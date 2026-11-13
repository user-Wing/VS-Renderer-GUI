#include "agent/AgentInstructions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace vsr;

namespace {
bool write(const QString &path, const QByteArray &bytes)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray entry(const QByteArray &name = "resize-guide")
{
    return "---\nname: " + name + "\ndescription: Configure resize filters\n---\n# Resize guide\nSECRET_BODY_MARKER\nRead references/resize.md first.\n";
}
}

class TestAgentInstructions final : public QObject {
    Q_OBJECT
private slots:
    void globalPromptPersists()
    {
        QTemporaryDir dir;
        AgentInstructions instructions(dir.path());
        QVERIFY(QFileInfo::exists(instructions.globalPromptPath()));
        QVERIFY(instructions.systemPrompt().contains("get_state"));
        QSignalSpy changed(&instructions, &AgentInstructions::changed);
        QString error;
        QVERIFY2(instructions.saveGlobalPrompt(QStringLiteral("# 全局指令\n按用户指定尺寸处理。"), &error), qPrintable(error));
        QCOMPARE(changed.size(), 1);
        QCOMPARE(instructions.loadGlobalPrompt(), QStringLiteral("# 全局指令\n按用户指定尺寸处理。"));
        AgentInstructions restored(dir.path());
        QCOMPARE(restored.systemPrompt(), instructions.systemPrompt());
        QVERIFY(write(instructions.globalPromptPath(), "Edited outside UI"));
        QVERIFY(restored.refresh());
        QCOMPARE(restored.systemPrompt(), QString("Edited outside UI"));
    }

    void importsBundleAndDisclosesOnDemand()
    {
        QTemporaryDir dir, source;
        QVERIFY(write(source.filePath("SKILL.md"), entry()));
        QVERIFY(write(source.filePath("references/resize.md"), "Resize reference marker"));
        QVERIFY(write(source.filePath("scripts/example.py"), "print('reference script')\n"));
        const QByteArray asset("binary\0asset", 12);
        QVERIFY(write(source.filePath("assets/example.bin"), asset));
        AgentInstructions instructions(dir.path());
        QString error;
        QVERIFY2(instructions.importSkill(source.path(), &error), qPrintable(error));
        QCOMPARE(instructions.skills().size(), 1);
        const auto skill = instructions.skills().first().toObject();
        QCOMPARE(skill.value("name").toString(), QString("resize-guide"));
        QVERIFY(skill.value("enabled").toBool());
        QVERIFY(instructions.systemPrompt().contains("Configure resize filters"));
        QVERIFY(!instructions.systemPrompt().contains("SECRET_BODY_MARKER"));
        QVERIFY(!instructions.systemPrompt().contains(source.path()));
        QVERIFY(instructions.readSkill("resize-guide").value("content").toString().contains("SECRET_BODY_MARKER"));
        QCOMPARE(instructions.readSkill("resize-guide", "references/resize.md").value("content").toString(), QString("Resize reference marker"));
        QVERIFY(instructions.readSkill("resize-guide", "scripts/example.py").value("ok").toBool());
        QVERIFY(instructions.readSkill("resize-guide", "assets/example.bin").contains("error"));
        QFile copied(QDir(skill.value("path").toString()).filePath("assets/example.bin"));
        QVERIFY(copied.open(QIODevice::ReadOnly)); QCOMPARE(copied.readAll(), asset);
        QVERIFY(QFileInfo::exists(source.filePath("SKILL.md")));
    }

    void enablesRemovesAndReloads()
    {
        QTemporaryDir dir, source;
        const auto path = source.filePath("guide.md");
        QVERIFY(write(path, entry()));
        AgentInstructions instructions(dir.path());
        QVERIFY(instructions.importSkill(path));
        QVERIFY(instructions.setSkillEnabled("resize-guide", false));
        QVERIFY(!instructions.systemPrompt().contains("Configure resize filters"));
        QVERIFY(instructions.readSkill("resize-guide").contains("error"));
        AgentInstructions restored(dir.path());
        QVERIFY(!restored.skills().first().toObject().value("enabled").toBool());
        QVERIFY(restored.setSkillEnabled("resize-guide", true));
        QVERIFY(restored.readSkill("resize-guide").value("ok").toBool());
        QVERIFY(restored.removeSkill("resize-guide"));
        QVERIFY(restored.skills().isEmpty());
        QVERIFY(QFileInfo::exists(path));
        QVERIFY(!restored.removeSkill("missing"));
    }

    void refusesBadImportsAndTraversal()
    {
        QTemporaryDir dir, source;
        AgentInstructions instructions(dir.path());
        const auto path = source.filePath("SKILL.md");
        QVERIFY(write(path, entry("../outside")));
        QString error;
        QVERIFY(!instructions.importSkill(source.path(), &error)); QVERIFY(!error.isEmpty());
        QVERIFY(instructions.skills().isEmpty());
        QVERIFY(write(path, entry()));
        QVERIFY(instructions.importSkill(source.path()));
        QVERIFY(!instructions.importSkill(source.path(), &error));
        QCOMPARE(instructions.skills().size(), 1);
        for (const auto &reference : QStringList{"../AGENTS.md", "references/../../AGENTS.md", "C:/private.txt", "/private.txt", "SKILL.md:secret", "missing.md"})
            QVERIFY2(instructions.readSkill("resize-guide", reference).contains("error"), qPrintable(reference));
        QVERIFY(!instructions.setSkillEnabled("missing", false, &error));
        QCOMPARE(instructions.skills().size(), 1);
    }

    void parsesMultilineMetadata()
    {
        QTemporaryDir dir, source;
        const auto path = source.filePath("SKILL.md");
        QVERIFY(write(path, "---\nname: 'multiline-skill'\ndescription: >-\n  Use for resize work,\n  including UHD output.\n---\nBODY\n"));
        AgentInstructions instructions(dir.path());
        QVERIFY(instructions.importSkill(source.path()));
        QCOMPARE(instructions.skills().first().toObject().value("description").toString(), QString("Use for resize work, including UHD output."));
        QVERIFY(!instructions.systemPrompt().contains("BODY"));
    }

    void refusesMalformedAndOversizedText()
    {
        QTemporaryDir dir, source;
        AgentInstructions instructions(dir.path());
        const auto path = source.filePath("bad.md");
        QVERIFY(write(path, "---\nname: broken\n"));
        QVERIFY(!instructions.importSkill(path));
        QVERIFY(write(path, QByteArray("\xff\xfe\xff", 3)));
        QVERIFY(!instructions.importSkill(path));
        QVERIFY(write(path, QByteArray(512 * 1024 + 1, 'x')));
        QVERIFY(!instructions.importSkill(path));
        QVERIFY(!instructions.saveGlobalPrompt(QString(512 * 1024 + 1, QChar('x'))));
        QVERIFY(instructions.skills().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestAgentInstructions)
#include "TestAgentInstructions.moc"
