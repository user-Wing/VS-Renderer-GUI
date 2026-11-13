#include "agent/AgentInput.h"
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace vsr;
class TestAgentInput final : public QObject {
    Q_OBJECT
private slots:
    void commandsAndLiteralSlash() {
        const auto command = AgentInput::parse(" /agent inspect current source ");
        QCOMPARE(command.command, QString("agent")); QCOMPARE(command.arguments, QString("inspect current source"));
        const auto literal = AgentInput::parse("//example"); QVERIFY(literal.command.isEmpty()); QCOMPARE(literal.modelText, QString("/example"));
        QVERIFY(!AgentInput::commands().isEmpty());
        QCOMPARE(AgentInput::parse(QStringLiteral("/中文技能 任务")).command, QStringLiteral("中文技能"));
        QCOMPARE(AgentInput::parse("/resize.v2 task").command, QString("resize.v2"));
    }
    void referencesPreserveDisplayAndDeduplicate() {
        int calls = 0; QString filePath;
        const QString input = "Inspect @workspace and @workspace with "+AgentInput::mention("file", "C:/folder with spaces/test.md");
        const auto expanded = AgentInput::expand(input, [&](const QString &kind, const QString &value) {
            ++calls; if (kind == "file") filePath = value;
            return QJsonObject{{"ok", true}, {"content", "private reference"}};
        });
        QVERIFY2(expanded.error.isEmpty(), qPrintable(expanded.error)); QCOMPARE(calls, 2); QCOMPARE(expanded.references.size(), 2); QCOMPARE(filePath, QString("C:/folder with spaces/test.md"));
        QCOMPARE(expanded.displayText, input); QVERIFY(expanded.modelText.contains("private reference")); QVERIFY(!expanded.displayText.contains("private reference"));
    }
    void codeAndEscapesDoNotUpload() {
        int calls = 0;
        const auto expanded = AgentInput::expand("`@file:C:/secret.txt` \\@workspace\n```md\n@file:C:/secret.txt\n```\n~~~\n@workspace", [&](const QString &, const QString &) { ++calls; return QJsonObject{}; });
        QVERIFY(expanded.error.isEmpty()); QCOMPARE(calls, 0); QVERIFY(expanded.references.isEmpty());
        const auto nested = AgentInput::expand("````md\n```\n@file:C:/secret.txt\n```\n````\n@workspace", [&](const QString &kind, const QString &) { ++calls; return QJsonObject{{"kind",kind}}; });
        QCOMPARE(calls, 1); QCOMPARE(nested.references.first().toObject().value("kind").toString(), QString("workspace"));
    }
    void missingAndFailedReferences() {
        const auto resolver = [](const QString &, const QString &) { return QJsonObject{{"ok", false}, {"error", "missing"}}; };
        QVERIFY(!AgentInput::expand("@file:", resolver).error.isEmpty());
        QVERIFY(!AgentInput::expand("@file:C:/missing.txt", resolver).error.isEmpty());
        QString many; for (int i=0; i<17; ++i) many += "@agent:"+QString::number(i)+" ";
        QVERIFY(!AgentInput::expand(many, [](const QString &, const QString &) { return QJsonObject{{"ok", true}}; }).error.isEmpty());
    }
    void filesRejectBinaryAndOversize() {
        QTemporaryDir directory; const auto path = directory.filePath("document.md");
        const auto write = [&](const QByteArray &bytes) { QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), qint64(bytes.size())); };
        write(QStringLiteral("# 中文\nBody").toUtf8()); const auto valid = AgentInput::readFile(path); QVERIFY(valid.value("ok").toBool()); QVERIFY(valid.value("content").toString().contains(QStringLiteral("中文")));
        write(QByteArray("\xFF\xFEH\0i\0", 6)); QCOMPARE(AgentInput::readFile(path).value("content").toString(), QString("Hi"));
        write(QByteArray("a\0b", 3)); QVERIFY(!AgentInput::readFile(path).value("ok").toBool());
        write(QByteArray(128*1024+1, 'a')); QVERIFY(!AgentInput::readFile(path).value("ok").toBool());
        QVERIFY(!AgentInput::readFile("relative.md").value("ok").toBool());
    }
};
QTEST_GUILESS_MAIN(TestAgentInput)
#include "TestAgentInput.moc"
