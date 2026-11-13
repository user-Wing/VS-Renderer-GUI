#include "app/MainWindow.h"
#include "agent/AgentSession.h"
#include "agent/AgentProvider.h"
#include "agent/AgentInstructions.h"
#include "agent/AgentMcp.h"
#include "agent/AgentTaskQueue.h"
#include "agent/AgentModelCapabilities.h"
#include "agent/AgentInput.h"
#include "ui/AgentPage.h"
#include "ui/PageTheme.h"
#include "ui/ExportWindow.h"
#include "app/Style.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSet>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <memory>

using namespace vsr;

class TestAgentIntegration final : public QObject {
    Q_OBJECT
    QVariant savedAccessMode_, savedWorkingDirectory_;
    static QJsonObject call(MainWindow &window, const QString &tool, const QJsonObject &args = {}) {
        return window.agentSession_->executeTool(tool, args);
    }
    static QString fixture(const QString &directory) {
        const auto output = QDir(directory).filePath("source.mkv");
        QProcess ff;
        ff.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
                 {"-v", "error", "-f", "lavfi", "-i", "testsrc2=size=128x72:rate=24", "-t", "1", "-c:v", "ffv1", output});
        return ff.waitForFinished(20000) && ff.exitCode() == 0 ? output : QString();
    }
    static QJsonObject callAsync(MainWindow &window, const QString &tool, const QJsonObject &arguments, int timeout = 15000) {
        struct Reply { bool done = false; QJsonObject result; };
        const auto reply = std::make_shared<Reply>();
        window.agentSession_->executeToolAsync(tool, arguments, [reply](QJsonObject result) { reply->result = result; reply->done = true; });
        QElapsedTimer timer; timer.start();
        while (!reply->done && timer.elapsed() < timeout) QTest::qWait(5);
        return reply->done ? reply->result : QJsonObject{{"ok", false}, {"error", "Timed out waiting for the local tool."}};
    }
    static bool chooseAccess(MainWindow &window, AgentAccessMode mode) {
        auto *button = window.agentPage_->findChild<QPushButton *>("agentAccessMode");
        if (!button) return false;
        QTest::mouseClick(button, Qt::LeftButton); QTest::qWait(10);
        auto *popup = window.agentPage_->findChild<QWidget *>("agentAccessPopup");
        auto *choices = popup ? popup->findChild<QListWidget *>("agentChoiceList") : nullptr;
        if (!choices) return false;
        const auto id = mode == AgentAccessMode::FullEnvironment ? "full" : "software";
        for (int i = 0; i < choices->count(); ++i) if (choices->item(i)->data(Qt::UserRole).toString() == id) {
            QTest::mouseClick(choices->viewport(), Qt::LeftButton, Qt::NoModifier, choices->visualItemRect(choices->item(i)).center());
            QTest::qWait(10); return window.agentSession_->accessMode() == mode;
        }
        return false;
    }
private slots:
    void init() {
        savedAccessMode_ = QSettings().value("agent/accessMode"); savedWorkingDirectory_ = QSettings().value("agent/workingDirectory");
        // Software-only cases must not inherit a previous live test's Full preference.
        QSettings().setValue("agent/accessMode", "software");
    }
    void cleanup() {
        if (savedAccessMode_.isValid()) QSettings().setValue("agent/accessMode", savedAccessMode_); else QSettings().remove("agent/accessMode");
        if (savedWorkingDirectory_.isValid()) QSettings().setValue("agent/workingDirectory", savedWorkingDirectory_); else QSettings().remove("agent/workingDirectory");
    }
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName("VSRenderer-Agent-Tests");
        QCoreApplication::setApplicationName("AgentIntegration");
        QFont font(QStringLiteral("Comic Sans MS")); font.setPixelSize(13);
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han,{"Microsoft YaHei UI"}); qApp->setFont(font);
        qApp->setStyleSheet(applicationStyleSheet(true));
    }
    void toolsOperateActualWorkspace() {
        QTemporaryDir dir; MainWindow window; window.show();
        window.agentSession_->setAccessMode(AgentAccessMode::SoftwareOnly);
        QVERIFY(window.agentPage_); QCOMPARE(window.pages_->count(), 5);
        const auto source = fixture(dir.path()); QVERIFY(!source.isEmpty());
        QVERIFY(call(window, "open_source", {{"path", source}}).value("ok").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(call(window, "get_state").value("sourceWidth").toInt(), 128, 10000);
        auto result = call(window, "add_filter", {{"id", "resize"}, {"parameters", QJsonObject{{"width", 96}, {"height", 54}}}});
        QVERIFY(result.value("ok").toBool()); QCOMPARE(window.graph_.nodes().size(), 1);
        QVERIFY(!call(window, "set_filter_parameter", {{"index", 0}, {"parameter", "width"}, {"value", -1}}).value("ok").toBool());
        QCOMPARE(window.graph_.at(0)->parameters.value("width").toInt(), 96);
        QVERIFY(!call(window, "add_filter", {{"id", "resize"}, {"parameters", QJsonObject{{"bad", 1}}}}).value("ok").toBool());
        QCOMPARE(window.graph_.nodes().size(), 1);
        const auto project = dir.filePath("project.vsr.json"), preset = dir.filePath("preset.vpy");
        QVERIFY(call(window, "export_project", {{"path", project}}).value("ok").toBool());
        QVERIFY(call(window, "export_preset", {{"path", preset}}).value("ok").toBool());
        QVERIFY(!call(window, "export_project", {{"path", project}}).value("ok").toBool());
        QVERIFY(call(window, "manage_route", {{"action", "add"}}).value("ok").toBool()); QCOMPARE(window.routeIndex_, 1);
        QVERIFY(call(window, "import_preset", {{"path", preset}}).value("ok").toBool()); QCOMPARE(window.graph_.nodes().size(), 1);
        QVERIFY(call(window, "manage_time_group", {{"action", "add"}, {"startSeconds", 1}, {"name", "test"}}).value("ok").toBool()); QCOMPARE(window.currentTimeGroups().size(), 2);
        QVERIFY(call(window, "import_project", {{"path", project}}).value("ok").toBool()); QCOMPARE(window.routes_.size(), 1); QCOMPARE(window.graph_.at(0)->parameters.value("width").toInt(), 96);
        result = call(window, "get_script"); QVERIFY(result.value("ok").toBool()); QVERIFY(result.value("script").toString().contains("96"));
        QVERIFY(call(window, "render_preview").value("ok").toBool());
        QTRY_VERIFY_WITH_TIMEOUT(window.vsScriptReady_, 25000);
        QVERIFY(call(window, "get_state").value("previewReady").toBool());
        QVERIFY(call(window, "set_filter_parameter", {{"index", 0}, {"parameter", "width"}, {"value", 98}}).value("ok").toBool());
        QVERIFY(!call(window, "get_state").value("previewReady").toBool());
        QVERIFY(call(window, "set_filter_parameter", {{"index", 0}, {"parameter", "width"}, {"value", 96}}).value("ok").toBool());
        const auto encoded = dir.filePath("encoded.mkv");
        result = call(window, "enqueue_export", {{"command", "ffmpeg -i <输入文件> -c:v ffv1 -an <输出文件>"}, {"output", encoded}});
        QVERIFY2(result.value("ok").toBool(), qPrintable(QJsonDocument(result).toJson()));
        QTRY_VERIFY_WITH_TIMEOUT(!window.exportWindow_->isBusy(), 30000); QVERIFY(QFileInfo(encoded).size() > 0);
        QCOMPARE(call(window, "get_state").value("exports").toArray().first().toObject().value("state").toString(), QString("completed"));
        QProcess probe; probe.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffprobe.exe"), {"-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height", "-of", "csv=p=0", encoded});
        QVERIFY(probe.waitForFinished(10000)); QCOMPARE(probe.readAllStandardOutput().trimmed(), QByteArray("96,54"));
    }
    void agentPageBackgroundAndUi() {
        QTemporaryDir dir; MainWindow window; window.show(); window.selectPage(4);
        window.agentSession_->setAccessMode(AgentAccessMode::SoftwareOnly);
        QVERIFY(QTest::qWaitForWindowExposed(&window)); QTest::qWait(100);
        QCOMPARE(window.pages_->currentWidget(), static_cast<QWidget *>(window.agentPage_));
        auto *page = window.agentPage_; const auto before = page->grab().toImage();
        auto *input = page->findChild<QPlainTextEdit *>("agentInput"); QVERIFY(input);
        window.activateWindow(); input->setFocus(); QTest::keyClicks(input, "hello world"); QCOMPARE(input->toPlainText(), QString("hello world"));
        const auto path = dir.filePath("theme.ini"); QSettings skin(path, QSettings::IniFormat);
        skin.setValue("VSR-Agent/color", "#472562"); skin.sync(); applyPageTheme(page, path, "VSR-Agent"); QTest::qWait(100);
        const auto after = page->grab().toImage(); QVERIFY(before != after);
        QVERIFY(page->findChild<QTextBrowser *>()); QVERIFY(page->findChild<QPlainTextEdit *>());
        auto *composer=page->findChild<QWidget *>("agentComposer"), *sidebar=page->findChild<QWidget *>("agentSidebar");
        QVERIFY(composer && sidebar); QVERIFY(composer->width()>=page->width()-sidebar->width()-100);
        const auto artifacts = qEnvironmentVariable("VSR_AGENT_ARTIFACTS");
        if (!artifacts.isEmpty()) { QDir().mkpath(artifacts); QVERIFY(window.grab().save(QDir(artifacts).filePath("agent-background.png"))); }
    }
    void liveDeepSeekTools() {
        const auto settings = qEnvironmentVariable("VSR_AGENT_3FUI_SETTINGS");
        if (settings.isEmpty()) QSKIP("Set VSR_AGENT_3FUI_SETTINGS to opt into a paid live endpoint test.");
        QString error; const auto providers = AgentProviderStore::importFfui(settings, &error);
        QVERIFY2(!providers.isEmpty(), qPrintable(error));
        QTemporaryDir dir;
        const auto artifacts = qEnvironmentVariable("VSR_AGENT_ARTIFACTS");
        const auto outputDir = artifacts.isEmpty() ? dir.path() : artifacts; QDir().mkpath(outputDir);
        const auto source = fixture(outputDir); QVERIFY(!source.isEmpty());
        const auto project = QDir(outputDir).filePath("deepseek-project.vsr.json"), preset = QDir(outputDir).filePath("deepseek-preset.vpy");
        QVERIFY(AgentProviderStore::save(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json"), providers, &error));
        QSettings().setValue("agent/providerId", providers.first().id);
        MainWindow window; window.show(); window.selectPage(4);
        auto *session = window.agentSession_; session->clear(); session->setProvider(providers.first());
        session->setAccessMode(AgentAccessMode::SoftwareOnly);
        const QStringList enabled{"get_state", "list_filters", "open_source", "add_filter", "export_project", "export_preset", "get_script"};
        for (const auto &value : session->tools()) { const auto name = value.toObject().value("function").toObject().value("name").toString(); session->setToolEnabled(name, enabled.contains(name)); }
        QSignalSpy finished(session, &AgentSession::finished), errors(session, &AgentSession::errorOccurred), calls(session, &AgentSession::toolExecuted);
        auto *input = window.agentPage_->findChild<QPlainTextEdit *>("agentInput"); QVERIFY(input);
        input->setPlainText(QStringLiteral("这是一次VS Renderer集成测试，请实际调用工具完成，不要只给建议。先检查工作区，再打开源文件 %1，查询resize滤镜参数并添加一个resize，width=96，height=54。将项目导出到 %2，将VPY预设导出到 %3。最后读取get_state确认只有一个resize，尺寸参数正确，并用中文简短报告。请勿操作其他文件。").arg(source, project, preset));
        QTest::mouseClick(window.agentPage_->findChild<QPushButton *>("agentSend"), Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(finished.size() > 0, 240000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        QVERIFY(calls.size() >= 6); QCOMPARE(window.graph_.nodes().size(), 1);
        QCOMPARE(window.graph_.at(0)->definitionId, QString("resize")); QCOMPARE(window.graph_.at(0)->parameters.value("width").toInt(), 96); QCOMPARE(window.graph_.at(0)->parameters.value("height").toInt(), 54);
        QVERIFY(QFileInfo::exists(project)); QVERIFY(QFileInfo::exists(preset));
        auto *transcript=window.agentPage_->findChild<QTextBrowser *>("agentTranscript");QVERIFY(transcript);
        QVERIFY(transcript->toHtml().contains("href=\"process:"));QVERIFY(!transcript->toHtml().contains("href=\"detail:"));
        const auto exported = QJsonDocument(session->exportConversation()).toJson(); QVERIFY(!exported.contains(providers.first().apiKey.toUtf8()));
        if (!artifacts.isEmpty()) {
            QFile conversation(QDir(artifacts).filePath("deepseek-conversation.json")); QVERIFY(conversation.open(QIODevice::WriteOnly)); conversation.write(exported); conversation.close();
            QFile visible(QDir(artifacts).filePath("agent-deepseek-visible.md"));QVERIFY(visible.open(QIODevice::WriteOnly));visible.write(transcript->toPlainText().toUtf8());visible.close();
            window.selectPage(4); QTest::qWait(300); QVERIFY(window.grab().save(QDir(artifacts).filePath("agent-deepseek.png")));
        }
        auto second = providers.first(); second.id = "switched-source"; second.name = "Second source"; session->setProvider(second);
        const int old = finished.size(); session->send(QStringLiteral("已切换来源。请调用get_state读取刚才的处理链，并简短确认resize的宽高。"));
        QTRY_VERIFY_WITH_TIMEOUT(finished.size() > old, 120000); QVERIFY(errors.isEmpty()); QCOMPARE(window.graph_.nodes().size(), 1);
        QVERIFY(session->messages().size() > 4);
    }
    void mcpRestoresDisabledPreferencesAfterDiscovery() {
        const auto saved = QSettings().value("agent/disabledTools");
        const auto restore = qScopeGuard([&] { if (saved.isValid()) QSettings().setValue("agent/disabledTools", saved); else QSettings().remove("agent/disabledTools"); });
        QString tool;
        const auto connectFixture = [](MainWindow &window) {
            window.agentSession_->setAccessMode(AgentAccessMode::SoftwareOnly);
            AgentMcpConfig config; config.id = "persistent-preferences"; config.name = "Persistence test";
            config.command = window.agentMcp_->runtime()->nodePath(); config.arguments = {QFINDTESTDATA("../tools/agent-runtime/fixture-server.mjs")};
            window.agentMcp_->setServers({config}); window.agentMcp_->connectServer(config.id);
        };
        {
            MainWindow window; connectFixture(window);
            QTRY_COMPARE_WITH_TIMEOUT(window.agentMcp_->state("persistent-preferences"), QString("connected"), 15000);
            for (const auto &item : window.agentSession_->allTools()) { const auto name = item.toObject().value("function").toObject().value("name").toString(); if (name.startsWith("mcp_") && name.endsWith("_add")) tool = name; }
            QVERIFY(!tool.isEmpty());
        }
        QSettings().setValue("agent/disabledTools", QStringList{tool});
        MainWindow restored; connectFixture(restored);
        QTRY_COMPARE_WITH_TIMEOUT(restored.agentMcp_->state("persistent-preferences"), QString("connected"), 15000);
        QVERIFY(!restored.agentSession_->toolEnabled(tool));
    }
    void liveDeepSeekExtensions() {
        const auto settings = qEnvironmentVariable("VSR_AGENT_3FUI_SETTINGS");
        if (settings.isEmpty()) QSKIP("Set VSR_AGENT_3FUI_SETTINGS for the paid extension integration test.");
        QString error; const auto providers = AgentProviderStore::importFfui(settings, &error); QVERIFY2(!providers.isEmpty(), qPrintable(error));
        MainWindow window; window.show(); window.selectPage(4);
        auto *session = window.agentSession_; session->clear(); session->setProvider(providers.first());
        session->setAccessMode(AgentAccessMode::FullEnvironment);
        window.agentQueue_->clearFinished();
        QTemporaryDir directory; const auto skillPath = directory.filePath("SKILL.md"); const auto skillName = "vsr-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QFile skill(skillPath); QVERIFY(skill.open(QIODevice::WriteOnly));
        skill.write(("---\nname: " + skillName + "\ndescription: Renderer只读工作区集成验收\n---\n先读取get_state，只报告实际状态；本技能仅查询，不修改媒体。\n").toUtf8()); skill.close();
        QVERIFY2(window.agentInstructions_->importSkill(skillPath, &error), qPrintable(error));
        const auto cleanup = qScopeGuard([&] { window.agentInstructions_->removeSkill(skillName); });
        AgentMcpConfig config; config.id = "integration-add"; config.name = "Integration MCP"; config.command = window.agentMcp_->runtime()->nodePath();
        config.arguments = {QFINDTESTDATA("../tools/agent-runtime/fixture-server.mjs")};
        QVERIFY(!config.arguments.first().isEmpty()); QVERIFY(window.agentMcp_->setServers({config}, &error)); window.agentMcp_->connectServer(config.id);
        QTRY_COMPARE_WITH_TIMEOUT(window.agentMcp_->state(config.id), QString("connected"), 15000);
        QString addTool; for (const auto &tool : session->allTools()) { const auto name = tool.toObject().value("function").toObject().value("name").toString(); if (name.startsWith("mcp_") && name.endsWith("_add")) addTool = name; }
        QVERIFY(!addTool.isEmpty());
        const QStringList allowed{"get_state", "list_skills", "read_skill", "get_token_usage", "enqueue_agent_task", "list_agent_tasks", "read_agent_task", "wait_agent_tasks", addTool};
        for (const auto &tool : session->allTools()) { const auto name = tool.toObject().value("function").toObject().value("name").toString(); session->setToolEnabled(name, allowed.contains(name)); }
        QSignalSpy finished(session, &AgentSession::finished), errors(session, &AgentSession::errorOccurred), calls(session, &AgentSession::toolExecuted);
        session->send(QStringLiteral("请实际执行这次扩展集成验收：1. 用list_skills找到技能%1并用read_skill读取正文；2. 调用%2，a=19,b=23，核实结果42；3. 用enqueue_agent_task创建一个名为integration-child的只读子任务，prompt为“调用get_state查询当前源是否为空并简短报告”，allowWrites=false；4. 用wait_agent_tasks等待它完成，读取结果和get_token_usage，最后简短报告。不要修改软件或文件，不要创建多个重复子任务。").arg(skillName, addTool));
        QTRY_VERIFY_WITH_TIMEOUT(finished.size() >= 1, 240000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        QStringList called; bool externalSum = false;
        for (const auto &call : calls) { called.append(call[0].toString()); if (call[0].toString() == addTool) externalSum |= call[2].toJsonObject().value("structuredContent").toObject().value("sum").toInt() == 42; }
        QVERIFY(called.contains("read_skill")); QVERIFY(called.contains("enqueue_agent_task")); QVERIFY(externalSum);
        bool completed = false; for (const auto &task : window.agentQueue_->tasks()) completed |= task.toObject().value("name").toString() == "integration-child" && task.toObject().value("state").toString() == "completed";
        QVERIFY(completed); QVERIFY(session->usage().value("promptTokens").toDouble() > 0); QVERIFY(session->usage().value("hasActualUsage").toBool());
        session->send(QStringLiteral("请再次调用get_state确认当前工作区，然后简短结束本轮。"));
        QTRY_VERIFY_WITH_TIMEOUT(finished.size() >= 2, 120000); QVERIFY(errors.isEmpty()); const auto history = session->messages();
        session->compactContext(); QTRY_VERIFY_WITH_TIMEOUT(finished.size() >= 3, 120000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.last().first().toString()));
        QCOMPARE(session->messages(), history); QVERIFY(session->usage().value("compactions").toInt() >= 1);
        const auto exported = QJsonDocument(session->exportConversation()).toJson(); QVERIFY(!exported.contains(providers.first().apiKey.toUtf8()));
        const auto artifacts = qEnvironmentVariable("VSR_AGENT_ARTIFACTS"); if (!artifacts.isEmpty()) {
            QFile file(QDir(artifacts).filePath("deepseek-extensions.json")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(exported); file.close();
            QFile tasks(QDir(artifacts).filePath("deepseek-tasks.json")); QVERIFY(tasks.open(QIODevice::WriteOnly)); tasks.write(QJsonDocument(window.agentQueue_->exportTasks()).toJson());
            window.grab().save(QDir(artifacts).filePath("agent-extensions.png"));
        }
    }
    void routerErrorsRetainInputWithoutStartingModel() {
        QTemporaryDir directory;QString error;
        const auto providerPath=QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json");QFile previous(providerPath);const bool existed=previous.exists();QByteArray bytes;if(previous.open(QIODevice::ReadOnly))bytes=previous.readAll();previous.close();const auto selected=QSettings().value("agent/providerId");
        const auto restore=qScopeGuard([&]{if(existed){QFile file(providerPath);if(file.open(QIODevice::WriteOnly))file.write(bytes);}else QFile::remove(providerPath);if(selected.isValid())QSettings().setValue("agent/providerId",selected);else QSettings().remove("agent/providerId");});
        AgentProvider provider{"router-test","Router test","http://127.0.0.1:1/v1",{},"router-model",{}};QVERIFY(AgentProviderStore::save(providerPath,{provider},&error));QSettings().setValue("agent/providerId",provider.id);
        MainWindow window;window.show();window.selectPage(4);window.agentSession_->clear();window.agentSession_->setAccessMode(AgentAccessMode::SoftwareOnly);auto *input=window.agentPage_->findChild<QPlainTextEdit*>("agentInput");auto *send=window.agentPage_->findChild<QPushButton*>("agentSend");QVERIFY(input && send);
        const auto skillName="disabled-router-"+QUuid::createUuid().toString(QUuid::WithoutBraces);const auto path=directory.filePath("SKILL.md");QFile skill(path);QVERIFY(skill.open(QIODevice::WriteOnly));skill.write(("---\nname: "+skillName+"\ndescription: Router disabled skill\n---\nDo not execute.\n").toUtf8());skill.close();QVERIFY(window.agentInstructions_->importSkill(path,&error));QVERIFY(window.agentInstructions_->setSkillEnabled(skillName,false,&error));const auto removeSkill=qScopeGuard([&]{window.agentInstructions_->removeSkill(skillName);});
        QSignalSpy busy(window.agentSession_,&AgentSession::busyChanged);
        const QStringList rejected{"/unknown-router-command",QStringLiteral("读取 ")+AgentInput::mention("file",directory.filePath("missing.md")),QStringLiteral("使用 ")+AgentInput::mention("skill",skillName)};
        for(const auto &text:rejected){input->setPlainText(text);QTest::mouseClick(send,Qt::LeftButton);QCOMPARE(input->toPlainText(),text);QVERIFY(window.agentSession_->messages().isEmpty());QVERIFY(window.agentSession_->pendingMessages().isEmpty());QVERIFY(!window.agentSession_->isBusy());}
        QCOMPARE(busy.size(),0);
    }
    void localEnvironmentModeIsolation() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto source = directory.filePath("source.txt"), blocked = directory.filePath("must-not-exist.txt"), output = directory.filePath("processed.txt");
        QFile fixtureFile(source); QVERIFY(fixtureFile.open(QIODevice::WriteOnly)); fixtureFile.write("original\n"); fixtureFile.close();
        QSettings().setValue("agent/workingDirectory", directory.path());
        MainWindow window; window.show(); window.selectPage(4); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *session = window.agentSession_; session->clear(); session->setAccessMode(AgentAccessMode::SoftwareOnly);
        for (const auto &name : {"read_file", "write_file", "edit_file", "exec_command", "write_stdin", "list_processes"}) session->setToolEnabled(name, true);
        const auto node = QFileInfo::exists(QDir(QCoreApplication::applicationDirPath()).filePath("runtime/agent/node.exe"))
            ? QDir(QCoreApplication::applicationDirPath()).filePath("runtime/agent/node.exe") : QStandardPaths::findExecutable("node");
        QVERIFY2(!node.isEmpty(), "Node is required for the local environment integration test.");
        const QList<QPair<QString, QJsonObject>> denied{
            {"read_file", {{"path", source}}},
            {"write_file", {{"path", blocked}, {"content", "must not be written"}}},
            {"exec_command", {{"executable", node}, {"arguments", QJsonArray{"-e", "require('fs').writeFileSync(process.argv[1],'must not run')", blocked}}}}
        };
        for (const auto &request : denied) {
            const auto sync = session->executeTool(request.first, request.second); QVERIFY(!sync.value("ok").toBool()); QVERIFY(sync.value("error").toString().contains(QStringLiteral("环境完全访问")));
            bool immediate = false; QJsonObject async;
            session->executeToolAsync(request.first, request.second, [&](QJsonObject result) { immediate = true; async = result; });
            QVERIFY(immediate); QVERIFY(!async.value("ok").toBool()); QVERIFY(async.value("error").toString().contains(QStringLiteral("环境完全访问")));
        }
        QVERIFY(!QFileInfo::exists(blocked)); QVERIFY(fixtureFile.open(QIODevice::ReadOnly)); QCOMPARE(fixtureFile.readAll(), QByteArray("original\n")); fixtureFile.close();
        QVERIFY(chooseAccess(window, AgentAccessMode::FullEnvironment)); QCOMPARE(QSettings().value("agent/accessMode").toString(), QString("full"));
        QCOMPARE(session->workingDirectory(), QDir::cleanPath(directory.path()));
        QVERIFY(callAsync(window, "write_file", {{"path", "processed.txt"}, {"content", "before\n"}}).value("ok").toBool());
        QVERIFY(callAsync(window, "edit_file", {{"path", "processed.txt"}, {"old_string", "before"}, {"new_string", "after"}}).value("ok").toBool());
        const auto read = callAsync(window, "read_file", {{"path", "processed.txt"}}); QVERIFY(read.value("ok").toBool()); QVERIFY(read.value("content").toString().contains("after"));
        const auto executed = callAsync(window, "exec_command", {{"executable", node}, {"arguments", QJsonArray{"-e", "process.stdout.write(require('fs').readFileSync(process.argv[1],'utf8'))", output}}, {"yield_time_ms", 5000}});
        QVERIFY2(executed.value("ok").toBool(), qPrintable(QJsonDocument(executed).toJson())); QCOMPARE(executed.value("stdout").toString(), QString("after\n")); QCOMPARE(executed.value("exit_code").toInt(-1), 0);
        const auto background = callAsync(window, "exec_command", {{"executable", node}, {"arguments", QJsonArray{"-e", "setInterval(()=>{},1000)"}}, {"background", true}});
        QVERIFY(background.value("ok").toBool()); QVERIFY(background.value("running").toBool());
        QVERIFY(chooseAccess(window, AgentAccessMode::SoftwareOnly)); QCOMPARE(QSettings().value("agent/accessMode").toString(), QString("software"));
        const auto inaccessible = callAsync(window, "write_stdin", {{"session_id", background.value("session_id")}, {"yield_time_ms", 0}});
        QVERIFY(!inaccessible.value("ok").toBool()); QVERIFY(inaccessible.value("error").toString().contains(QStringLiteral("环境完全访问")));
        {
            MainWindow restored; QCOMPARE(restored.agentSession_->accessMode(), AgentAccessMode::SoftwareOnly);
            QCOMPARE(restored.agentSession_->workingDirectory(), QDir::cleanPath(directory.path()));
            QCOMPARE(restored.agentPage_->findChild<QPushButton *>("agentAccessMode")->text(), QStringLiteral("软件内操作"));
        }
        QVERIFY(chooseAccess(window, AgentAccessMode::FullEnvironment));
        const auto stopped = callAsync(window, "write_stdin", {{"session_id", background.value("session_id")}, {"yield_time_ms", 1000}});
        QVERIFY(!stopped.value("running").toBool()); QVERIFY(stopped.value("cancelled").toBool());
        QVERIFY(chooseAccess(window, AgentAccessMode::SoftwareOnly)); QVERIFY(!QFileInfo::exists(blocked));
    }
    void liveDeepSeekLocalEnvironment() {
        const auto settings = qEnvironmentVariable("VSR_AGENT_3FUI_SETTINGS");
        if (settings.isEmpty()) QSKIP("Set VSR_AGENT_3FUI_SETTINGS for the paid local environment acceptance test.");
        QString error; auto providers = AgentProviderStore::importFfui(settings, &error); QVERIFY2(!providers.isEmpty(), qPrintable(error)); auto provider = providers.first();
        AgentModelCapabilities capabilities; QSignalSpy resolved(&capabilities, &AgentModelCapabilities::resolved);
        capabilities.resolve(provider.baseUrl, provider.apiKey, provider.model, true); QTRY_VERIFY_WITH_TIMEOUT(!resolved.isEmpty(), 30000);
        const auto capability = resolved.last()[2].toJsonObject(); QCOMPARE(capability.value("model").toString(), provider.model);
        QVERIFY(capability.value("models").toArray().contains(provider.model)); QVERIFY(capability.value("status").toString() != "error");
        provider.modelCapability = capability; provider.reasoningEffort = capability.value("supportsOff").toBool() ? QStringLiteral("off") : capability.value("levels").toArray().contains("low") ? QStringLiteral("low") : QStringLiteral("auto"); provider.maxOutputTokens = 4096;
        const auto providerPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json");
        QFile previous(providerPath); const bool existed = previous.exists(); QByteArray previousBytes; if (previous.open(QIODevice::ReadOnly)) previousBytes = previous.readAll(); previous.close(); const auto selected = QSettings().value("agent/providerId");
        const auto restore = qScopeGuard([&] { if (existed) { QFile file(providerPath); if (file.open(QIODevice::WriteOnly)) file.write(previousBytes); } else QFile::remove(providerPath); if (selected.isValid()) QSettings().setValue("agent/providerId", selected); else QSettings().remove("agent/providerId"); });
        QVERIFY2(AgentProviderStore::save(providerPath, {provider}, &error), qPrintable(error)); QSettings().setValue("agent/providerId", provider.id);
        QTemporaryDir work, outside; QVERIFY(work.isValid()); QVERIFY(outside.isValid());
        const auto marker = "OUTSIDE_" + QUuid::createUuid().toString(QUuid::WithoutBraces), execMarker = "NODE_VERIFIED_" + QUuid::createUuid().toString(QUuid::WithoutBraces), childName = "local-readonly-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto source = work.filePath("seed.txt"), external = outside.filePath("external-marker.txt"), resultPath = work.filePath("result.txt");
        QFile seed(source); QVERIFY(seed.open(QIODevice::WriteOnly)); seed.write("alpha=2\nbeta=3\nstatus=raw\n"); seed.close();
        QFile externalFile(external); QVERIFY(externalFile.open(QIODevice::WriteOnly)); externalFile.write((marker + "\n").toUtf8()); externalFile.close();
        QVERIFY(!QDir(work.path()).relativeFilePath(external).startsWith("./")); QVERIFY(QDir(work.path()).relativeFilePath(external).startsWith(".."));
        QSettings().setValue("agent/workingDirectory", work.path()); QSettings().setValue("agent/accessMode", "software");
        MainWindow window; window.show(); window.selectPage(4); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *session = window.agentSession_; session->clear(); window.agentQueue_->cancelAll(); window.agentQueue_->clearFinished();
        window.agentPage_->setModelCapabilities(capability); session->setProvider(provider); QVERIFY(chooseAccess(window, AgentAccessMode::FullEnvironment));
        QCOMPARE(session->workingDirectory(), QDir::cleanPath(work.path())); QCOMPARE(QSettings().value("agent/accessMode").toString(), QString("full"));
        const QStringList allowed{"get_state", "get_environment", "get_token_usage", "read_file", "glob", "grep", "write_file", "edit_file", "apply_patch", "exec_command", "write_stdin", "spawn_agent", "enqueue_agent_task", "send_agent_message", "list_agent_tasks", "read_agent_task", "wait_agent_tasks"};
        for (const auto &value : session->allTools()) { const auto name = value.toObject().value("function").toObject().value("name").toString(); session->setToolEnabled(name, allowed.contains(name)); }
        QSignalSpy finished(session, &AgentSession::finished), errors(session, &AgentSession::errorOccurred), calls(session, &AgentSession::toolExecuted), queueErrors(window.agentQueue_, &AgentTaskQueue::errorOccurred);
        QJsonArray childCalls; bool childReadOnly = false;
        connect(window.agentQueue_, &AgentTaskQueue::sessionCreated, &window, [&](AgentSession *child, bool allowWrites) {
            const QPointer<AgentSession> guarded(child);
            connect(child, &AgentSession::busyChanged, &window, [&, guarded, allowWrites](bool busy) { if (busy && guarded) childReadOnly |= !allowWrites && guarded->toolAvailable("read_file") && !guarded->toolAvailable("exec_command") && !guarded->toolAvailable("write_file"); });
            connect(child, &AgentSession::toolExecuted, &window, [&](const QString &name, const QJsonObject &arguments, const QJsonObject &result) { childCalls.append(QJsonObject{{"name", name}, {"arguments", arguments}, {"result", result}}); });
        });
        const auto stop = qScopeGuard([&] { session->cancel(); window.agentQueue_->cancelAll(); });
        const auto artifacts = qEnvironmentVariable("VSR_AGENT_ARTIFACTS");
        const auto capture = qScopeGuard([&] {
            if (artifacts.isEmpty()) return;
            QDir().mkpath(artifacts); QJsonArray trace; for (const auto &event : calls) trace.append(QJsonObject{{"name", event[0].toString()}, {"arguments", event[1].toJsonObject()}, {"result", event[2].toJsonObject()}});
            auto evidence = QJsonDocument(QJsonObject{{"model", provider.model}, {"reasoning", provider.reasoningEffort}, {"capability", capability}, {"parent", session->exportConversation()}, {"tasks", window.agentQueue_->exportTasks()}, {"calls", trace}, {"childCalls", childCalls}, {"childReadOnly", childReadOnly}}).toJson();
            if (!provider.apiKey.isEmpty()) evidence.replace(provider.apiKey.toUtf8(), "[REDACTED]");
            QFile output(QDir(artifacts).filePath("deepseek-local-environment.json")); if (output.open(QIODevice::WriteOnly)) output.write(evidence);
            window.grab().save(QDir(artifacts).filePath("agent-local-environment.png"));
        });
        auto *input = window.agentPage_->findChild<QPlainTextEdit *>("agentInput"); auto *send = window.agentPage_->findChild<QPushButton *>("agentSend"); QVERIFY(input && send);
        input->setPlainText(QStringLiteral("/agent 这是一次本地环境真实验收，必须实际调用工具。当前工作目录为%1。只允许在这个新临时目录内修改文件；另允许仅读取外部临时文件%2中的唯一校验词，不读取凭证、用户私有文件或扫描其他目录。步骤：1. get_environment检查目录，用glob查找当前目录*.txt，用grep查找status=raw并read_file读取seed.txt，再read_file读取上述外部文件；外部文件不在工作目录内，这是授权的只读跨目录访问。2. 依据seed.txt的alpha与beta计算和，用write_file创建result.txt，精确内容为sum=5换行status=raw换行；然后必须调用edit_file或apply_patch将它的status=raw改成status=verified。3. 用exec_command实际运行Node（node -e或node可执行文件），读取result.txt并检查sum=5和status=verified，检查失败须非零退出，成功打印%3；不要仅echo常量冒充校验。4. 创建恰好一个名为%4、allowWrites=false的只读子智能体，要求它实际用read_file读取%5并核实sum=5和status=verified，不运行shell、不写文件；默认继承当前来源，无需用户配置。等待它完成并用read_agent_task读取实际结果。最终答复汇总父命令与子任务的真实检查结果，并原样保留外部唯一校验词及Node成功校验词。不要重复创建任务，不修改其他文件或软件状态。").arg(work.path(), external, execMarker, childName, resultPath));
        QTest::mouseClick(send, Qt::LeftButton); QVERIFY(session->isBusy());
        QTRY_VERIFY_WITH_TIMEOUT(([&] { if (session->isBusy() || finished.isEmpty()) return false; for (const auto &value : window.agentQueue_->tasks()) if (value.toObject().value("name").toString() == childName && value.toObject().value("state").toString() == "completed") return true; return false; })(), 360000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString())); QVERIFY2(queueErrors.isEmpty(), queueErrors.isEmpty() ? "" : qPrintable(queueErrors.first().first().toString()));
        QFile resultFile(resultPath); QVERIFY(resultFile.open(QIODevice::ReadOnly)); QCOMPARE(resultFile.readAll(), QByteArray("sum=5\nstatus=verified\n")); resultFile.close();
        QVERIFY(externalFile.open(QIODevice::ReadOnly)); QCOMPARE(externalFile.readAll(), (marker + "\n").toUtf8()); externalFile.close();
        QSet<QString> actualCalls, children; bool outsideRead = false, nodeValidated = false;
        for (const auto &event : calls) {
            const auto name = event[0].toString(); const auto arguments = event[1].toJsonObject(), result = event[2].toJsonObject(); actualCalls.insert(name);
            QVERIFY2(result.value("ok").toBool(true) && result.value("error").toString().isEmpty(), qPrintable(name + ": " + result.value("error").toString()));
            if (name == "read_file") outsideRead |= result.value("content").toString().contains(marker);
            if (name == "exec_command" || name == "write_stdin") nodeValidated |= result.value("exit_code").isDouble() && result.value("exit_code").toInt(-1) == 0 && result.value("stdout").toString().contains(execMarker);
            if (name == "spawn_agent" || name == "enqueue_agent_task") children.insert(result.value("taskId").toString());
        }
        for (const auto &required : {"get_environment", "read_file", "glob", "grep", "write_file", "exec_command", "read_agent_task"}) QVERIFY2(actualCalls.contains(required), required);
        QVERIFY(actualCalls.contains("edit_file") || actualCalls.contains("apply_patch")); QVERIFY(outsideRead); QVERIFY(nodeValidated); QCOMPARE(children.size(), 1); QVERIFY(!children.contains(QString())); QVERIFY(childReadOnly);
        const auto task = window.agentQueue_->task(*children.begin()); QCOMPARE(task.value("state").toString(), QString("completed")); QVERIFY(!task.value("allowWrites").toBool()); QCOMPARE(task.value("workingDirectory").toString(), QDir::cleanPath(work.path()));
        bool childReadResult = false; for (const auto &event : childCalls) {
            const auto entry = event.toObject(); QVERIFY(entry.value("name") != "exec_command"); QVERIFY(entry.value("name") != "write_file");
            if (entry.value("name") == "read_file") childReadResult |= entry.value("result").toObject().value("content").toString().contains("status=verified");
        }
        QVERIFY(childReadResult); QVERIFY(task.value("messages").toArray().size() >= 4); QVERIFY(task.value("result").toString().contains("verified"));
        QString answer; for (const auto &value : session->messages()) { const auto message = value.toObject(); if (message.value("role") == "assistant" && !message.contains("tool_calls")) answer = message.value("content").toString(); }
        QVERIFY(answer.contains(marker)); QVERIFY(answer.contains(execMarker));
        QVERIFY(!provider.apiKey.isEmpty()); QVERIFY(!QJsonDocument(session->exportConversation()).toJson().contains(provider.apiKey.toUtf8())); QVERIFY(!QJsonDocument(window.agentQueue_->exportTasks()).toJson().contains(provider.apiKey.toUtf8()));
    }
    void liveDeepSeekInteractiveAgents() {
        const auto settings=qEnvironmentVariable("VSR_AGENT_3FUI_SETTINGS");if(settings.isEmpty())QSKIP("Set VSR_AGENT_3FUI_SETTINGS for the paid interactive Agent acceptance test.");
        QString error;auto providers=AgentProviderStore::importFfui(settings,&error);QVERIFY2(!providers.isEmpty(),qPrintable(error));auto provider=providers.first();
        AgentModelCapabilities capabilities;QSignalSpy resolved(&capabilities,&AgentModelCapabilities::resolved);capabilities.resolve(provider.baseUrl,provider.apiKey,provider.model,true);QTRY_VERIFY_WITH_TIMEOUT(!resolved.isEmpty(),30000);
        const auto capability=resolved.last()[2].toJsonObject();QCOMPARE(capability.value("model").toString(),provider.model);QVERIFY2(capability.value("models").toArray().contains(provider.model),"The live model metadata request did not return the configured model.");QVERIFY(capability.value("status").toString()!="error");
        provider.modelCapability=capability;provider.reasoningEffort=capability.value("supportsOff").toBool()?QStringLiteral("off"):capability.value("levels").toArray().contains("low")?QStringLiteral("low"):QStringLiteral("auto");provider.maxOutputTokens=4096;providers={provider};
        const auto providerPath=QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("agent/providers.json");QFile previous(providerPath);const bool existed=previous.exists();QByteArray previousBytes;if(previous.open(QIODevice::ReadOnly))previousBytes=previous.readAll();previous.close();const auto selected=QSettings().value("agent/providerId");
        const auto restore=qScopeGuard([&]{if(existed){QFile file(providerPath);if(file.open(QIODevice::WriteOnly))file.write(previousBytes);}else QFile::remove(providerPath);if(selected.isValid())QSettings().setValue("agent/providerId",selected);else QSettings().remove("agent/providerId");});
        QVERIFY2(AgentProviderStore::save(providerPath,providers,&error),qPrintable(error));QSettings().setValue("agent/providerId",provider.id);
        QTemporaryDir directory;const auto fileMarker="FILE_TOKEN_"+QUuid::createUuid().toString(QUuid::WithoutBraces),followupMarker="FOLLOWUP_TOKEN_"+QUuid::createUuid().toString(QUuid::WithoutBraces),taskName="interactive-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto document=directory.filePath("fresh-acceptance-data.md");QFile file(document);QVERIFY(file.open(QIODevice::WriteOnly));file.write((QStringLiteral("# 新建验收资料\n\n唯一校验词：")+fileMarker+QStringLiteral("\n\n资料仅供读取与核对，不修改软件或文件。\n")).toUtf8());file.close();
        MainWindow window;window.show();window.selectPage(4);auto *session=window.agentSession_;session->clear();window.agentQueue_->cancelAll();window.agentQueue_->clearFinished();window.agentPage_->setModelCapabilities(capability);session->setProvider(provider);
        session->setAccessMode(AgentAccessMode::SoftwareOnly);
        const QStringList allowed{"get_state","get_token_usage","spawn_agent","enqueue_agent_task","send_agent_message","list_agent_tasks","read_agent_task","wait_agent_tasks"};
        for(const auto &value:session->allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();session->setToolEnabled(name,allowed.contains(name));}
        QSignalSpy finished(session,&AgentSession::finished),errors(session,&AgentSession::errorOccurred),calls(session,&AgentSession::toolExecuted),cancelled(session,&AgentSession::toolExecutionCancelled),queueErrors(window.agentQueue_,&AgentTaskQueue::errorOccurred);bool observedPending=false;
        connect(session,&AgentSession::pendingMessagesChanged,&window,[&](const QJsonArray &pending){for(const auto &entry:pending)observedPending|=entry.toObject().value("content").toString().contains(followupMarker);});
        auto *input=window.agentPage_->findChild<QPlainTextEdit*>("agentInput");auto *send=window.agentPage_->findChild<QPushButton*>("agentSend");QVERIFY(input && send);
        const auto first=QStringLiteral("/agent 读取这份新临时资料 %1，实际创建恰好一个只读子智能体，名称%2。你必须把资料中的唯一校验词完整传给它（子智能体不继承父对话），让它调用get_state并原样报告校验词和当前来源是否为空。等第一轮完成，再用send_agent_message给同一个taskId追加“第二轮请再次确认校验词和只读状态”，等待并读取第二轮实际会话，最后汇总。不要重复创建任务，不修改任何文件或软件状态，最终答复保留校验词原文。").arg(AgentInput::mention("file",document),taskName);
        input->setPlainText(first);QTest::mouseClick(send,Qt::LeftButton);QVERIFY(session->isBusy());QCOMPARE(session->messages().size(),1);QVERIFY(session->messages().first().toObject().value("content").toString().contains(fileMarker));QVERIFY(!session->messages().first().toObject().value("displayContent").toString().contains(fileMarker));
        input->setPlainText("/new");QTest::mouseClick(send,Qt::LeftButton);QCOMPARE(input->toPlainText(),QString("/new"));QVERIFY(session->isBusy());QCOMPARE(session->messages().size(),1);
        const auto supplement=QStringLiteral("运行中补充：继续既定子智能体两轮协作，不停止当前模型或工具；最终父答复还必须原样包含这个追加校验词：%1。请避免无内容确认来回。").arg(followupMarker);
        input->setPlainText(supplement);QTest::mouseClick(send,Qt::LeftButton);QVERIFY(session->isBusy());QVERIFY(observedPending);QVERIFY(cancelled.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(([&]{if(session->isBusy() || finished.isEmpty())return false;for(const auto &entry:window.agentQueue_->tasks()){const auto task=entry.toObject();if(task.value("state")=="completed" && task.value("rounds").toInt()>=2)return true;}return false;})(),360000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));QVERIFY2(queueErrors.isEmpty(),queueErrors.isEmpty()?"":qPrintable(queueErrors.first().first().toString()));QVERIFY(cancelled.isEmpty());
        QSet<QString> created;bool communicated=false;for(const auto &event:calls){const auto name=event[0].toString();const auto result=event[2].toJsonObject();if((name=="spawn_agent" || name=="enqueue_agent_task") && result.value("ok").toBool())created.insert(result.value("taskId").toString());}
        QCOMPARE(created.size(),1);const auto id=*created.begin();for(const auto &event:calls)if(event[0].toString()=="send_agent_message" && event[1].toJsonObject().value("id")==id && event[2].toJsonObject().value("ok").toBool())communicated=true;QVERIFY(communicated);
        const auto task=window.agentQueue_->task(id);QCOMPARE(task.value("state").toString(),QString("completed"));QVERIFY(!task.value("allowWrites").toBool());QVERIFY(task.value("rounds").toInt()>=2);QVERIFY(task.value("messages").toArray().size()>=4);QVERIFY(!task.contains("error"));
        bool parentReceivedSupplement=false,parentReceivedChild=false;QString lastAnswer;
        for(const auto &entry:session->messages()){const auto message=entry.toObject();if(message.value("role")=="user"){parentReceivedSupplement|=message.value("content").toString().contains(followupMarker);parentReceivedChild|=message.value("source").toString().contains(QStringLiteral("子智能体完成汇报"));}if(message.value("role")=="assistant" && !message.contains("tool_calls"))lastAnswer=message.value("content").toString();}
        QVERIFY(parentReceivedSupplement);QVERIFY(parentReceivedChild);QVERIFY(lastAnswer.contains(fileMarker));QVERIFY(lastAnswer.contains(followupMarker));
        QJsonArray trace;for(const auto &event:calls)trace.append(QJsonObject{{"name",event[0].toString()},{"arguments",event[1].toJsonObject()},{"result",event[2].toJsonObject()}});
        const auto evidence=QJsonDocument(QJsonObject{{"model",provider.model},{"reasoning",provider.reasoningEffort},{"capability",capability},{"observedPending",observedPending},{"parent",session->exportConversation()},{"tasks",window.agentQueue_->exportTasks()},{"calls",trace}}).toJson();QVERIFY(!evidence.contains(provider.apiKey.toUtf8()));
        const auto artifacts=qEnvironmentVariable("VSR_AGENT_ARTIFACTS");if(!artifacts.isEmpty()){QDir().mkpath(artifacts);QFile output(QDir(artifacts).filePath("deepseek-interactive-agents.json"));QVERIFY(output.open(QIODevice::WriteOnly));QCOMPARE(output.write(evidence),qint64(evidence.size()));output.close();QTest::qWait(150);QVERIFY(window.grab().save(QDir(artifacts).filePath("agent-interactive-agents.png")));}
    }
};
QTEST_MAIN(TestAgentIntegration)
#include "TestAgentIntegration.moc"
