#include "agent/AgentLocalFiles.h"
#include "agent/AgentSession.h"
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using namespace vsr;
namespace {
bool put(const QString &path,const QByteArray &bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);return file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size();
}
QByteArray get(const QString &path) { QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{}; }
QJsonObject run(AgentSession &session,const QString &name,const QJsonObject &args)
{
    struct State { bool done=false;QJsonObject result{{"ok",false},{"error","Tool timed out in test."}}; };auto state=std::make_shared<State>();QEventLoop loop;const QPointer<QEventLoop> guard(&loop);
    session.executeToolAsync(name,args,[state,guard](QJsonObject result){state->done=true;state->result=std::move(result);if(guard)guard->quit();});
    if(!state->done){QTimer::singleShot(15000,&loop,&QEventLoop::quit);loop.exec();}return state->result;
}
void prepare(AgentSession &session,const QString &directory)
{
    session.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(session.setWorkingDirectory(directory));
}
}

class TestAgentLocalFiles final : public QObject {
    Q_OBJECT
private slots:
    void paginatedTextAndLargeUtf8Stream() {
        QTemporaryDir dir;QVERIFY(dir.isValid());AgentSession session;prepare(session,dir.path());AgentLocalFiles files(dir.path());files.registerTools(&session);
        QVERIFY(put(dir.filePath("text.txt"),QByteArray("\xEF\xBB\xBF")+QStringLiteral("第一行\r\n第二行\r\n第三行\r\n").toUtf8()));
        auto page=run(session,"read_file",{{"path","text.txt"},{"offset",2},{"limit",1}});QVERIFY2(page.value("ok").toBool(),qPrintable(page.value("error").toString()));QCOMPARE(page.value("lines").toArray().first().toObject().value("text").toString(),QStringLiteral("第二行"));QCOMPARE(page.value("next_offset").toInt(),3);QVERIFY(page.value("has_more").toBool());
        page=run(session,"read_file",{{"path","text.txt"},{"offset",3},{"limit",1}});QVERIFY(!page.value("has_more").toBool());QCOMPARE(page.value("total_lines").toInt(),3);
        const QByteArray wide("\xFF\xFEH\0i\0\n\0",8);QVERIFY(put(dir.filePath("wide.txt"),wide));QVERIFY(run(session,"read_file",{{"path","wide.txt"}}).value("content").toString().contains("Hi"));
        QVERIFY(put(dir.filePath("invalid.bin"),QByteArray("a\0b",3)));QVERIFY(!run(session,"read_file",{{"path","invalid.bin"}}).value("ok").toBool());QVERIFY(put(dir.filePath("invalid.txt"),QByteArray("\xE4",1)));QVERIFY(!run(session,"read_file",{{"path","invalid.txt"}}).value("ok").toBool());
        QVERIFY(put(dir.filePath("large.txt"),QByteArray("FIRST\n")+QByteArray(33*1024*1024,'a')+"\nTAIL\n"));page=run(session,"read_file",{{"path","large.txt"},{"offset",2},{"limit",1}});QVERIFY(page.value("ok").toBool());QVERIFY(page.value("lines").toArray().first().toObject().value("truncated").toBool());QCOMPARE(page.value("next_offset").toInt(),3);page=run(session,"read_file",{{"path","large.txt"},{"offset",3},{"limit",1}});QVERIFY(page.value("content").toString().contains("TAIL"));
    }

    void directoryGlobAndRegexSearch() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);
        QVERIFY(put(dir.filePath("root.txt"),"needle-one\nplain\n"));QVERIFY(put(dir.filePath("nested/child.txt"),"needle-two\nneedle(three)\n"));QVERIFY(put(dir.filePath(".hidden.txt"),"hidden\n"));QVERIFY(put(dir.filePath(".git/objects/ignored.txt"),"needle-ignored\n"));QVERIFY(put(dir.filePath("binary.bin"),QByteArray("x\0y",3)));
        auto result=run(session,"glob",{{"pattern","**/*.txt"}});QVERIFY(result.value("ok").toBool());QCOMPARE(result.value("entries").toArray().size(),3);
        result=run(session,"list_directory",{{"limit",1}});QVERIFY(result.value("truncated").toBool());QCOMPARE(result.value("entries").toArray().size(),1);QCOMPARE(result.value("next_offset").toInt(),1);
        result=run(session,"grep",{{"pattern","needle-.*"},{"include","**/*.txt"}});QCOMPARE(result.value("entries").toArray().size(),2);QVERIFY(result.value("entries").toArray().first().toObject().value("line").toInt()>=1);
        result=run(session,"search_files",{{"pattern","needle(three)"},{"literal",true}});QCOMPARE(result.value("entries").toArray().size(),1);QCOMPARE(result.value("skipped_files").toInt(),1);
        QVERIFY(!run(session,"grep",{{"pattern","[invalid"}}).value("ok").toBool());QVERIFY(!run(session,"list_directory",{{"path","root.txt"}}).value("ok").toBool());
    }

    void exactEditsAndEncodingPreservation() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);const auto path=dir.filePath("edit.txt");const QByteArray original=QByteArray("\xEF\xBB\xBF")+"one\r\none\r\ntail\r\n";QVERIFY(put(path,original));
        QVERIFY(!run(session,"edit_file",{{"path","edit.txt"},{"old_string","one"},{"new_string","two"}}).value("ok").toBool());QCOMPARE(get(path),original);
        QVERIFY(!run(session,"edit_file",{{"path","edit.txt"},{"old_string","one"},{"new_string","two"},{"replace_all",true},{"expected_matches",3}}).value("ok").toBool());QCOMPARE(get(path),original);
        auto result=run(session,"edit_file",{{"path","edit.txt"},{"old_string","one"},{"new_string","two"},{"replace_all",true},{"expected_matches",2}});QVERIFY(result.value("ok").toBool());QCOMPARE(result.value("replacements").toInt(),2);QCOMPARE(get(path),QByteArray("\xEF\xBB\xBF")+"two\r\ntwo\r\ntail\r\n");
        result=run(session,"edit_file",{{"path","edit.txt"},{"old_string","two\ntail"},{"new_string","final\nend"}});QVERIFY(result.value("ok").toBool());QCOMPARE(get(path),QByteArray("\xEF\xBB\xBF")+"two\r\nfinal\r\nend\r\n");
        QVERIFY(!run(session,"edit_file",{{"path","edit.txt"},{"old_string",""},{"new_string","unexpected"}}).value("ok").toBool());
    }

    void writeCopyMoveMetadataAndOutsideCwd() {
        QTemporaryDir dir;QVERIFY(QDir().mkdir(dir.filePath("work")));AgentSession session;prepare(session,dir.filePath("work"));AgentLocalFiles files;files.registerTools(&session);
        const auto outside=dir.filePath("outside.txt");QVERIFY(run(session,"write_file",{{"path",outside},{"content","outside permitted"}}).value("ok").toBool());QCOMPARE(get(outside),QByteArray("outside permitted"));
        auto result=run(session,"write_file",{{"path","nested/source.txt"},{"content","original"}});QVERIFY(result.value("ok").toBool());QVERIFY(!run(session,"write_file",{{"path","nested/source.txt"},{"content","clobbered"}}).value("ok").toBool());
        QVERIFY(run(session,"copy_file",{{"source","nested/source.txt"},{"destination","copy.txt"}}).value("ok").toBool());QVERIFY(!run(session,"copy_file",{{"source","nested/source.txt"},{"destination","copy.txt"}}).value("ok").toBool());
        QVERIFY(run(session,"move_file",{{"source","copy.txt"},{"destination","moved.txt"}}).value("ok").toBool());QVERIFY(!QFileInfo::exists(dir.filePath("work/copy.txt")));result=run(session,"file_info",{{"path","moved.txt"}});QVERIFY(result.value("is_file").toBool());QCOMPARE(result.value("size").toInt(),8);
        QVERIFY(run(session,"delete_file",{{"path","moved.txt"}}).value("ok").toBool());QVERIFY(!run(session,"file_info",{{"path","moved.txt"}}).value("exists").toBool());QVERIFY(run(session,"create_directory",{{"path","empty/child"}}).value("ok").toBool());QVERIFY(!run(session,"delete_file",{{"path","empty"}}).value("ok").toBool());
        QVERIFY(run(session,"write_file",{{"cwd",dir.path()},{"path","via-cwd.txt"},{"content","override"}}).value("ok").toBool());QVERIFY(QFileInfo::exists(dir.filePath("via-cwd.txt")));QCOMPARE(session.workingDirectory(),QDir::cleanPath(dir.filePath("work")));QVERIFY(files.setWorkingDirectory(dir.path()));QCOMPARE(files.workingDirectory(),session.workingDirectory());
    }

    void patchAddUpdateDeleteMoveAndCrLf() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);QVERIFY(put(dir.filePath("edit.txt"),QByteArray("\xEF\xBB\xBF")+"head\r\nold\r\ntail\r\n"));QVERIFY(put(dir.filePath("delete.bin"),QByteArray("\0binary",7)));QVERIFY(put(dir.filePath("move.txt"),"before\n"));
        const QString patch="*** Begin Patch\n*** Add File: created/note.txt\n+hello\n+world\n*** Update File: edit.txt\n@@\n head\n-old\n+new\n tail\n*** End of File\n*** Delete File: delete.bin\n*** Update File: move.txt\n*** Move to: moved.txt\n@@\n-before\n+after\n*** End Patch";
        const auto result=run(session,"apply_patch",{{"patch",patch}});QVERIFY2(result.value("ok").toBool(),qPrintable(result.value("error").toString()));QCOMPARE(result.value("operations").toArray().size(),4);QCOMPARE(get(dir.filePath("created/note.txt")),QByteArray("hello\nworld\n"));QCOMPARE(get(dir.filePath("edit.txt")),QByteArray("\xEF\xBB\xBF")+"head\r\nnew\r\ntail\r\n");QVERIFY(!QFileInfo::exists(dir.filePath("delete.bin")));QVERIFY(!QFileInfo::exists(dir.filePath("move.txt")));QCOMPARE(get(dir.filePath("moved.txt")),QByteArray("after\n"));
    }

    void rejectedPatchLeavesEveryFileUntouched() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);QVERIFY(put(dir.filePath("original.txt"),"same\nsame\n"));QVERIFY(put(dir.filePath("blocker"),"regular file"));
        const QString first="*** Begin Patch\n*** Add File: must-not-exist/new.txt\n+created\n";
        const QStringList failures{
            "*** Update File: missing.txt\n@@\n-no\n+yes\n",
            "*** Update File: original.txt\n@@\n-no-match\n+replacement\n",
            "*** Update File: original.txt\n@@\n-same\n+ambiguous\n",
            "*** Add File: original.txt\n+overwrite\n",
            "*** Update File: original.txt\n*** Move to: blocker\n@@\n same\n same\n",
            "*** Add File: must-not-exist/new.txt\n+duplicate\n",
            "*** Update File: original.txt\n@@\n?invalid\n"
        };
        for(const auto &tail:failures){const auto result=run(session,"apply_patch",{{"patch",first+tail+"*** End Patch"}});QVERIFY2(!result.value("ok").toBool(),qPrintable(tail));QCOMPARE(get(dir.filePath("original.txt")),QByteArray("same\nsame\n"));QCOMPARE(get(dir.filePath("blocker")),QByteArray("regular file"));QVERIFY(!QFileInfo::exists(dir.filePath("must-not-exist")));}
        QVERIFY(put(dir.filePath("zz-blocker"),"later staging failure"));const auto stagedFailure=run(session,"apply_patch",{{"patch",first+"*** Add File: zz-blocker/child.txt\n+bad-parent\n*** End Patch"}});QVERIFY(!stagedFailure.value("ok").toBool());QVERIFY(!QFileInfo::exists(dir.filePath("must-not-exist")));QCOMPARE(get(dir.filePath("zz-blocker")),QByteArray("later staging failure"));
    }

    void aliasesCannotMoveAFileOntoItselfOrPatchItTwice() {
        QTemporaryDir dir;const auto real=dir.filePath("real"),alias=dir.filePath("alias");QVERIFY(QDir().mkdir(real));const QByteArray original("must survive alias operations\n");QVERIFY(put(QDir(real).filePath("a.txt"),original));
#ifdef Q_OS_WIN
        QProcess junction;junction.start("cmd.exe",{"/d","/c","mklink","/J",QDir::toNativeSeparators(alias),QDir::toNativeSeparators(real)});QVERIFY(junction.waitForFinished(5000));QVERIFY2(junction.exitCode()==0,junction.readAllStandardError().constData());
#else
        QVERIFY(QFile::link(real,alias));
#endif
        AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);
        QCOMPARE(get(QDir(alias).filePath("a.txt")),original);
        const auto move=run(session,"move_file",{{"source","real/a.txt"},{"destination","alias/a.txt"},{"overwrite",true}});QCOMPARE(get(QDir(real).filePath("a.txt")),original);QVERIFY(!move.value("ok").toBool());
        QVERIFY(!run(session,"copy_file",{{"source","real/a.txt"},{"destination","alias/a.txt"},{"overwrite",true}}).value("ok").toBool());
        const auto patch=run(session,"apply_patch",{{"patch","*** Begin Patch\n*** Update File: real/a.txt\n@@\n-must survive alias operations\n+first change\n*** Update File: alias/a.txt\n@@\n-must survive alias operations\n+second change\n*** End Patch"}});QVERIFY(!patch.value("ok").toBool());QCOMPARE(get(QDir(real).filePath("a.txt")),original);
        const auto create=run(session,"apply_patch",{{"patch","*** Begin Patch\n*** Add File: real/new.txt\n+first\n*** Add File: alias/new.txt\n+second\n*** End Patch"}});QVERIFY(!create.value("ok").toBool());QVERIFY(!QFileInfo::exists(QDir(real).filePath("new.txt")));QVERIFY(QDir().rmdir(alias));
        const auto parentConflict=run(session,"apply_patch",{{"patch","*** Begin Patch\n*** Add File: parent-file\n+first\n*** Add File: parent-file/child.txt\n+second\n*** End Patch"}});QVERIFY(!parentConflict.value("ok").toBool());QVERIFY(!QFileInfo::exists(dir.filePath("parent-file")));
    }

    void permissionsCannotBeElevatedByArgumentsOrDelegation() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);QVERIFY(put(dir.filePath("existing.txt"),"known"));session.setAccessMode(AgentAccessMode::SoftwareOnly);
        QVERIFY(!run(session,"read_file",{{"path","existing.txt"}}).value("ok").toBool());QVERIFY(!run(session,"write_file",{{"path","not-written.txt"},{"content","bad"},{"access_mode","full"}}).value("ok").toBool());QVERIFY(!QFileInfo::exists(dir.filePath("not-written.txt")));
        session.setAccessMode(AgentAccessMode::FullEnvironment);QVERIFY(!run(session,"write_file",{{"path","not-written.txt"},{"content","bad"},{"permissions","full"}}).value("ok").toBool());session.setToolEnabled("read_file",false);QVERIFY(!run(session,"read_file",{{"path","existing.txt"}}).value("ok").toBool());
        AgentSession child;prepare(child,dir.path());session.copyToolsTo(&child);bool inherited=false;for(const auto &value:child.allTools())inherited|=value.toObject().value("function").toObject().value("name").toString()=="read_file";QVERIFY(!inherited);
    }

    void asynchronousSearchCancellationAndModuleLifetime() {
        QTemporaryDir dir;AgentSession session;prepare(session,dir.path());AgentLocalFiles files;files.registerTools(&session);QVERIFY(put(dir.filePath("many.txt"),QByteArray(31*1024*1024,'x')));
        bool completed=false;QJsonObject result;session.executeToolAsync("grep",{{"path",dir.path()},{"pattern","not-present"}},[&](QJsonObject value){result=value;completed=true;});QVERIFY(!completed);emit session.toolExecutionCancelled();QTRY_VERIFY_WITH_TIMEOUT(completed,10000);QVERIFY(result.value("cancelled").toBool());
        QVERIFY(run(session,"write_file",{{"path","completed.txt"},{"content","durable"}}).value("ok").toBool());emit session.toolExecutionCancelled();QCOMPARE(get(dir.filePath("completed.txt")),QByteArray("durable"));
        AgentSession other;prepare(other,dir.path());auto *temporary=new AgentLocalFiles;temporary->registerTools(&other);other.executeToolAsync("grep",{{"path",dir.path()},{"pattern","x"}},[](QJsonObject){});delete temporary;QTest::qWait(30);QVERIFY(!run(other,"read_file",{{"path","completed.txt"}}).value("ok").toBool());
        AgentSession callbackSession;prepare(callbackSession,dir.path());auto *callbackModule=new AgentLocalFiles;callbackModule->registerTools(&callbackSession);bool callbackDone=false;callbackSession.executeToolAsync("read_file",{{"path","completed.txt"}},[&](QJsonObject value){delete callbackModule;callbackDone=value.value("ok").toBool();});QTRY_VERIFY_WITH_TIMEOUT(callbackDone,10000);
    }
};
QTEST_GUILESS_MAIN(TestAgentLocalFiles)
#include "TestAgentLocalFiles.moc"
