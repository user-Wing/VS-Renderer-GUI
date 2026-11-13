#include "agent/AgentSession.h"
#include "agent/AgentToolLibrary.h"

#include <QJsonArray>
#include <QTest>

using namespace vsr;

namespace {
QJsonObject parameters()
{
    return {{"type", "object"}, {"properties", QJsonObject{
        {"value", QJsonObject{{"type", "integer"}}},
        {"options", QJsonObject{{"type", "object"}}}}},
        {"required", QJsonArray{"value"}}, {"additionalProperties", false}};
}

QJsonObject step(const QString &tool, const QJsonObject &arguments = {})
{
    return {{"tool", tool}, {"arguments", arguments}};
}

QJsonObject workflow(const QString &name, const QJsonArray &steps)
{
    return {{"name", name}, {"description", "Test workflow"}, {"parameters", parameters()}, {"steps", steps}};
}

QJsonObject document(const QJsonArray &tools)
{
    return {{"schemaVersion", 1}, {"tools", tools}};
}

void addTool(AgentSession &session, const QString &name, AgentSession::Handler handler)
{
    session.registerTool(name, "Test built-in", {{"type", "object"}}, std::move(handler));
}
}

class TestAgentToolLibrary final : public QObject {
    Q_OBJECT
private slots:
    void forwardsTypedArguments()
    {
        AgentSession session;
        QJsonObject seen;
        addTool(session, "capture", [&](const QJsonObject &args) { seen = args; return QJsonObject{{"ok", true}}; });
        AgentToolLibrary library(&session);
        const QJsonObject nested{{"number", "$value"}, {"array", QJsonArray{"$value", "$options"}},
            {"literal", "prefix $value"}, {"object", "$options"}};
        QString error;
        QVERIFY2(library.importTools(document({workflow("custom_capture", {step("capture", nested)})}), &error), qPrintable(error));
        const QJsonObject options{{"enabled", true}, {"weights", QJsonArray{1, 2, 3}}};
        const auto result = session.executeTool("custom_capture", {{"value", 7}, {"options", options}});
        QVERIFY(result.value("ok").toBool());
        QCOMPARE(seen.value("number"), QJsonValue(7));
        QCOMPARE(seen.value("array").toArray(), QJsonArray({7, options}));
        QCOMPARE(seen.value("literal").toString(), QString("prefix $value"));
        QCOMPARE(seen.value("object").toObject(), options);
        QCOMPARE(result.value("steps").toArray().size(), 1);
    }

    void stopsAfterFailedStep()
    {
        AgentSession session;
        int calls = 0;
        addTool(session, "capture", [&](const QJsonObject &) { ++calls; return QJsonObject{{"ok", true}}; });
        addTool(session, "failure", [](const QJsonObject &) { return QJsonObject{{"error", "Expected failure"}}; });
        AgentToolLibrary library(&session);
        QVERIFY(library.importTools(document({workflow("custom_failure", {step("capture"), step("failure"), step("capture")})})));
        const auto result = session.executeTool("custom_failure", {{"value", 1}});
        QVERIFY(result.contains("error"));
        QCOMPARE(result.value("failedStep").toInt(), 2);
        QCOMPARE(result.value("steps").toArray().size(), 2);
        QCOMPARE(calls, 1);
    }

    void rejectsInvalidImportsAtomically()
    {
        AgentSession session;
        addTool(session, "capture", [](const QJsonObject &) { return QJsonObject{{"ok", true}}; });
        AgentToolLibrary library(&session);
        const auto valid = workflow("custom_valid", {step("capture")});
        auto badSchema = workflow("custom_bad", {step("capture")});
        badSchema.insert("parameters", QJsonObject{{"type", "object"}, {"required", QJsonArray{"missing"}}});
        auto badBounds = workflow("custom_bad", {step("capture")});
        badBounds.insert("parameters", QJsonObject{{"type", "object"}, {"properties", QJsonObject{
            {"value", QJsonObject{{"type", "integer"}, {"minimum", "invalid"}}}}}});
        auto unsupportedSchema = workflow("custom_bad", {step("capture")});
        unsupportedSchema.insert("parameters", QJsonObject{{"type", "object"}, {"oneOf", QJsonArray{parameters()}}});
        const QList<QJsonObject> invalid{
            workflow("capture", {step("capture")}),
            workflow("custom_valid", {step("capture")}),
            workflow("custom_bad", {step("missing")}),
            workflow("custom_bad", {step("custom_bad")}),
            workflow("custom_bad", {}),
            workflow("custom_bad", {step("capture", {{"unknown", "$missing"}})}),
            workflow("custom_bad", {QJsonObject{{"tool", "capture"}}}),
            badSchema, badBounds, unsupportedSchema};
        for (const auto &bad : invalid) {
            QString error;
            QVERIFY(!library.importTools(document({valid, bad}), &error));
            QVERIFY(!error.isEmpty());
            QVERIFY(library.exportTools().value("tools").toArray().isEmpty());
            QCOMPARE(session.allTools().size(), 1);
        }
        QVERIFY(library.importTools(document({valid})));
        QVERIFY(!library.importTools(document({workflow("custom_nested", {step("custom_valid")})})));
        QCOMPARE(library.exportTools().value("tools").toArray().size(), 1);
    }

    void validatesArgumentsBeforeSideEffects()
    {
        AgentSession session;
        int calls = 0;
        addTool(session, "capture", [&](const QJsonObject &) { ++calls; return QJsonObject{{"ok", true}}; });
        AgentToolLibrary library(&session);
        QVERIFY(library.importTools(document({workflow("custom_capture", {step("capture"), step("capture", {{"options", "$options"}})})})));
        for (const auto &args : QList<QJsonObject>{{}, {{"value", "wrong type"}}, {{"value", 1}},
                {{"value", 1}, {"options", QJsonObject{}}, {"unknown", true}}}) {
            const auto result = session.executeTool("custom_capture", args);
            QVERIFY(result.contains("error"));
            QCOMPARE(calls, 0);
        }
        QVERIFY(session.executeTool("custom_capture", {{"value", 1}, {"options", QJsonObject{}}}).value("ok").toBool());
        QCOMPARE(calls, 2);
    }

    void exportsAndImportsMultipleTools()
    {
        AgentSession session;
        addTool(session, "capture", [](const QJsonObject &args) { return QJsonObject{{"value", args.value("value")}}; });
        QJsonObject saved;
        {
            AgentToolLibrary library(&session);
            QVERIFY(library.importTools(document({workflow("custom_first", {step("capture", {{"value", "$value"}})})})));
            QVERIFY(library.importTools(document({workflow("custom_second", {step("capture")})})));
            saved = library.exportTools();
            QCOMPARE(saved.value("tools").toArray().size(), 2);
            QCOMPARE(session.allTools().size(), 3);
        }
        QCOMPARE(session.allTools().size(), 1);
        AgentToolLibrary restored(&session);
        QVERIFY(restored.importTools(saved));
        QCOMPARE(restored.exportTools(), saved);
        const auto result = session.executeTool("custom_first", {{"value", 23}});
        QCOMPARE(result.value("steps").toArray().first().toObject().value("result").toObject().value("value"), QJsonValue(23));
    }

    void rejectsLimitsAndDisabledTools()
    {
        AgentSession session;
        int calls = 0;
        addTool(session, "capture", [&](const QJsonObject &) { ++calls; return QJsonObject{{"ok", true}}; });
        AgentToolLibrary library(&session);
        QJsonArray steps, tools;
        for (int i = 0; i < 33; ++i) steps.append(step("capture"));
        QVERIFY(!library.importTools(document({workflow("too_many_steps", steps)})));
        for (int i = 0; i < 65; ++i) tools.append(workflow("tool_" + QString::number(i), {step("capture")}));
        QVERIFY(!library.importTools(document(tools)));
        QVERIFY(library.importTools(document({workflow("custom_capture", {step("capture")})})));
        session.setToolEnabled("capture", false);
        QVERIFY(session.executeTool("custom_capture", {{"value", 1}}).contains("error"));
        QCOMPARE(calls, 0);
    }
};

QTEST_GUILESS_MAIN(TestAgentToolLibrary)
#include "TestAgentToolLibrary.moc"
