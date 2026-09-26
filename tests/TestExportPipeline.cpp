#include "backend/ExportPipeline.h"
#include "app/Style.h"
#include <QApplication>
#include <QFont>
#include "ui/ExportWindow.h"
#include <QTreeWidget>
#include "ui/PreparedFiles.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTabWidget>
#include <QPushButton>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace vsr;

class TestExportPipeline final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QFont font(QStringLiteral("Segoe UI Variable"));
        font.setPixelSize(13);
        qApp->setFont(font);
        qApp->setStyleSheet(applicationStyleSheet());
    }

    void batchOutputNames()
    {
        QTemporaryDir directory;
        const QString source = directory.filePath(QStringLiteral("sample.name.mp4"));
        const QString plain = ExportWindow::outputPath(source, {}, false);
        QCOMPARE(plain, directory.filePath(QStringLiteral("sample.name.mkv")));
        const QString stamped = ExportWindow::outputPath(source, {}, true);
        QVERIFY(stamped.startsWith(directory.filePath(QStringLiteral("sample.name_"))));
        QVERIFY(stamped.endsWith(QStringLiteral(".mkv")));
        QVERIFY(stamped != plain);
        const QString target = directory.filePath(QStringLiteral("output"));
        QVERIFY(QDir().mkpath(target));
        QCOMPARE(ExportWindow::outputPath(source, target, false), QDir(target).filePath(QStringLiteral("sample.name.mkv")));
    }

    void rewritesThreeFuiMapsAndPlaceholders()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("input.mkv"));
        const QString script = directory.filePath(QStringLiteral("preview.vpy"));
        const QString ffmpeg = directory.filePath(QStringLiteral("runtime/ffmpeg/ffmpeg.exe"));
        const QString vspipe = directory.filePath(QStringLiteral("runtime/python/Lib/site-packages/vapoursynth/vspipe.exe"));
        QDir().mkpath(QFileInfo(ffmpeg).absolutePath());
        QDir().mkpath(QFileInfo(vspipe).absolutePath());
        for (const QString &path : {source, script, ffmpeg, vspipe}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        const QString output = directory.filePath(QStringLiteral("output.mkv"));
        const QString command = QStringLiteral(
            "ffmpeg.exe -hide_banner -y -hwaccel cuda -i <输入文件> "
            "-map 0:v:0? -map 0:a:0? -c:v:0 av1\\_nvenc -c:a:0 libopus <输出文件>");

        const auto plan = ExportPipeline::buildPlan(command, source, output, script, directory.path());
        QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));
        QCOMPARE(QDir::cleanPath(plan.ffmpegProgram), QDir::cleanPath(ffmpeg));
        QVERIFY(plan.ffmpegArguments.contains(QStringLiteral("1:v:0?")));
        QVERIFY(plan.ffmpegArguments.contains(QStringLiteral("0:a:0?")));
        QVERIFY(plan.ffmpegArguments.contains(QStringLiteral("av1_nvenc")));
        QVERIFY(plan.ffmpegArguments.contains(QDir::toNativeSeparators(source)));
        QVERIFY(plan.ffmpegArguments.contains(QDir::toNativeSeparators(output)));
        const int pipeInput = plan.ffmpegArguments.indexOf(QStringLiteral("yuv4mpegpipe"));
        QVERIFY(pipeInput > 0);
        QCOMPARE(plan.ffmpegArguments.at(pipeInput + 1), QStringLiteral("-i"));
        QCOMPARE(plan.ffmpegArguments.at(pipeInput + 2), QStringLiteral("-"));
    }

    void addsVideoMapAndCudaUploadWhenNeeded()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("input.mkv"));
        const QString script = directory.filePath(QStringLiteral("preview.vpy"));
        const QString ffmpeg = directory.filePath(QStringLiteral("runtime/ffmpeg/ffmpeg.exe"));
        const QString vspipe = directory.filePath(QStringLiteral("runtime/python/Lib/site-packages/vapoursynth/vspipe.exe"));
        QDir().mkpath(QFileInfo(ffmpeg).absolutePath());
        QDir().mkpath(QFileInfo(vspipe).absolutePath());
        for (const QString &path : {source, script, ffmpeg, vspipe}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        const auto plan = ExportPipeline::buildPlan(
            QStringLiteral("ffmpeg.exe -y -i <输入文件> -map 0:a:0? -filter:v scale_cuda=format=p010le <输出文件>"),
            source, directory.filePath(QStringLiteral("out.mkv")), script, directory.path());
        QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));
        QVERIFY(plan.ffmpegArguments.contains(QStringLiteral("1:v:0?")));
        QVERIFY(plan.ffmpegArguments.contains(QStringLiteral("hwupload_cuda,scale_cuda=format=p010le")));
    }

    void rejectsUnsupportedComplexGraph()
    {
        const auto plan = ExportPipeline::buildPlan(
            QStringLiteral("ffmpeg.exe -i <输入文件> -filter_complex [0:v]null[v] <输出文件>"),
            QStringLiteral("input.mkv"), QStringLiteral("output.mkv"), QStringLiteral("preview.vpy"), {});
        QVERIFY(!plan.error.isEmpty());
    }

    void exportsThroughRealProcessPipe_data()
    {
        QTest::addColumn<bool>("stopEarly");
        QTest::addColumn<QString>("extension");
        QTest::newRow("complete-mkv") << false << QStringLiteral("mkv");
        QTest::newRow("stop-mkv") << true << QStringLiteral("mkv");
    }

    void exportsThroughRealProcessPipe()
    {
        QFETCH(bool, stopEarly);
        QFETCH(QString, extension);
        QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg.exe"));
        if (ffmpeg.isEmpty())
            ffmpeg = QStringLiteral("C:\\PortableSoft\\FFmpegFreeUI ReadyToRun x64\\ffmpeg.exe");
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString vspipe = QDir(appDir).filePath(
            QStringLiteral("runtime/python/Lib/site-packages/vapoursynth/vspipe.exe"));
        if (!QFileInfo::exists(ffmpeg) || !QFileInfo::exists(vspipe))
            QSKIP("Real FFmpeg/VapourSynth runtime is not available.");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("source.mkv"));
        const QString script = directory.filePath(QStringLiteral("preview.vpy"));
        const QString output = directory.filePath(QStringLiteral("output.") + extension);
        QProcess generator;
        generator.start(ffmpeg, {
            QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"), QStringLiteral("error"),
            QStringLiteral("-y"), QStringLiteral("-f"), QStringLiteral("lavfi"),
            QStringLiteral("-i"), QStringLiteral("testsrc2=size=64x36:rate=6:duration=10"),
            QStringLiteral("-f"), QStringLiteral("lavfi"), QStringLiteral("-i"),
            QStringLiteral("anullsrc=r=48000:cl=stereo"), QStringLiteral("-shortest"),
            QStringLiteral("-c:v"), QStringLiteral("ffv1"), QStringLiteral("-c:a"),
            QStringLiteral("pcm_s16le"), source});
        QVERIFY(generator.waitForFinished(30000));
        QCOMPARE(generator.exitCode(), 0);

        QFile scriptFile(script);
        QVERIFY(scriptFile.open(QIODevice::WriteOnly));
        scriptFile.write(stopEarly ?
            "import time\nimport vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=64, height=36, format=vs.YUV420P8, length=600, fpsnum=6)\n"
            "def slow(n, f):\n    time.sleep(0.1)\n    return f\n"
            "clip = vs.core.std.ModifyFrame(clip, clips=clip, selector=slow)\nclip.set_output()\n"
            :
            "import vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=64, height=36, format=vs.YUV420P8, "
            "length=3, fpsnum=6, fpsden=1)\n"
            "clip.set_output()\n");
        scriptFile.close();

        const QString command = QStringLiteral(
            "\"%1\" -hide_banner -loglevel error -y -i <输入文件> "
            "-map 0:v:0? -map 0:a:0? -c:v mpeg4 -c:a aac <输出文件>").arg(ffmpeg);
        ExportPlan plan = ExportPipeline::buildPlan(command, source, output, script, appDir);
        QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));

        ExportPipeline pipeline;
        plan.vspipeArguments[1] = QStringLiteral("1");
        QSignalSpy progress(&pipeline, &ExportPipeline::progress);
        QSignalSpy finished(&pipeline, &ExportPipeline::finished);
        QVERIFY(pipeline.start(plan));
        if (stopEarly) {
            QTRY_VERIFY_WITH_TIMEOUT(!progress.isEmpty() && progress.last().at(0).toLongLong() >= 3, 15000);
            QVERIFY(pipeline.setPaused(true));
            QVERIFY(pipeline.isPaused());
            QTest::qWait(150);
            const auto pausedSize = QFileInfo(output).size();
            const auto pausedCount = progress.count();
            QTest::qWait(600);
            QCOMPARE(QFileInfo(output).size(), pausedSize);
            QCOMPARE(progress.count(), pausedCount);
            QVERIFY(pipeline.setPaused(false));
            QTRY_VERIFY_WITH_TIMEOUT(progress.count() > pausedCount, 5000);
            QVERIFY(pipeline.setPaused(true));
            pipeline.cancel();
            QVERIFY(!pipeline.isPaused());
            QVERIFY(pipeline.isRunning());
        }
        QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty(), 30000);
        const QList<QVariant> result = finished.takeFirst();
        QCOMPARE(result.at(0).toBool(), !stopEarly);
        if (stopEarly) QVERIFY2(result.at(1).toString().contains(QStringLiteral("已输出的文件已保留")), qPrintable(result.at(1).toString()));
        QCOMPARE(finished.size(), 0);
        QVERIFY(!pipeline.isRunning());
        QProcess decode;
        decode.start(ffmpeg, {QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-i"), output,
            QStringLiteral("-map"), QStringLiteral("0:v:0"), QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")});
        QVERIFY(decode.waitForFinished(10000));
        QVERIFY2(decode.exitCode() == 0, decode.readAllStandardError().constData());
        QVERIFY(QFileInfo(output).size() > 0);
        if (!stopEarly) {
            QVERIFY(scriptFile.open(QIODevice::ReadOnly));
            const QString snapshot = QString::fromUtf8(scriptFile.readAll());
            const QString source2 = directory.filePath(QStringLiteral("source2.mkv"));
            const QString source3 = directory.filePath(QStringLiteral("source3.mkv"));
            QVERIFY(QFile::copy(source, source2));
            QVERIFY(QFile::copy(source, source3));
            ExportWindow window;
            QVERIFY(window.parentWidget() == nullptr);
            QVERIFY(!(window.windowFlags() & Qt::WindowStaysOnTopHint));
            QVERIFY(!window.isModal());
            window.setCurrentSource(source);
            window.setScriptBuilder([snapshot](const QString &) { ScriptBuildResult result; result.script = snapshot; return result; });
            QSignalSpy completed(&window, &ExportWindow::statusMessage);
            QSignalSpy idle(&window, &ExportWindow::idle);
            window.show();
            window.findChild<QPushButton *>(QStringLiteral("pauseQueue"))->click();
            QVERIFY(window.addJob(command, directory.filePath(QStringLiteral("queue1.mkv")), source, snapshot, 0.5));
            QVERIFY(window.addJob(command, directory.filePath(QStringLiteral("queue2.mkv")), source2, snapshot, 0.5));
            QVERIFY(window.addJob(command, directory.filePath(QStringLiteral("cancelled.mkv")), source3, snapshot, 0.5));
            QString duplicateError;
            QVERIFY(!window.addJob(command, directory.filePath(QStringLiteral("duplicate.mkv")), source, snapshot, 0.5, &duplicateError));
            QVERIFY(!duplicateError.isEmpty());
            auto *table = window.findChild<QTreeWidget *>();
            QVERIFY(table);
            QCOMPARE(table->topLevelItemCount(), 3);
            QCOMPARE(window.findChild<QTabWidget *>()->count(), 3);
            QCOMPARE(table->dragDropMode(), QAbstractItemView::InternalMove);
            auto *second = table->takeTopLevelItem(1);
            table->insertTopLevelItem(0, second);
            table->topLevelItem(2)->setSelected(true);
            window.findChild<QPushButton *>(QStringLiteral("removeWaiting"))->click();
            QCOMPARE(table->topLevelItemCount(), 2);
            window.close();
            QVERIFY(window.isBusy());
            window.show();
            window.findChild<QPushButton *>(QStringLiteral("pauseQueue"))->click();
            QCOMPARE(table->topLevelItem(0)->text(1), QStringLiteral("编码中"));
            QVERIFY(table->topLevelItem(0)->text(0).endsWith(QStringLiteral("queue2.mkv")));
            QTRY_VERIFY_WITH_TIMEOUT(completed.count() == 2, 15000);
            QVERIFY(!window.isBusy());
            QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("cancelled.mkv"))));
            for (int row = 0; row < 2; ++row) {
                QCOMPARE(table->topLevelItem(row)->text(1), QStringLiteral("导出完成。"));
                QVERIFY(table->topLevelItem(row)->text(3).toLongLong() > 0);
            }

            ExportWindow batch;
            batch.show();
            batch.findChild<QPushButton *>(QStringLiteral("pauseQueue"))->click();
            batch.setCurrentSource(source);
            QStringList builtSources;
            batch.setScriptBuilder([snapshot, &builtSources](const QString &path) {
                builtSources.append(path);
                ScriptBuildResult result; result.script = snapshot; return result;
            });
            batch.findChild<QPlainTextEdit *>(QStringLiteral("exportCommand"))->setPlainText(command);
            auto *prepared = batch.findChild<PreparedFiles *>(QStringLiteral("preparedFiles"));
            QVERIFY(prepared);
            QMimeData mime;
            mime.setUrls({QUrl::fromLocalFile(source), QUrl::fromLocalFile(source2), QUrl::fromLocalFile(source)});
            batch.findChild<QTabWidget *>()->setCurrentIndex(1);
            QDragEnterEvent enter(QPoint(20,20), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(prepared->viewport(), &enter);
            QVERIFY(enter.isAccepted());
            QDropEvent drop(QPointF(20,20), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(prepared->viewport(), &drop);
            QCOMPARE(prepared->count(), 2);
            const QStringList expectedSources = prepared->files();
            batch.findChild<QComboBox *>(QStringLiteral("outputNaming"))->setCurrentIndex(1);
            batch.findChild<QPushButton *>(QStringLiteral("enqueueAll"))->click();
            QCOMPARE(batch.findChild<QTreeWidget *>()->topLevelItemCount(), 0);
            QCOMPARE(prepared->count(), 2);
            builtSources.clear();
            batch.findChild<QComboBox *>(QStringLiteral("outputNaming"))->setCurrentIndex(0);
            QVERIFY(batch.grab().save(QDir(appDir).filePath(QStringLiteral("prepare-page.png"))));
            batch.findChild<QPushButton *>(QStringLiteral("enqueueAll"))->click();
            QCOMPARE(builtSources, expectedSources);
            QCOMPARE(prepared->count(), 0);
            QCOMPARE(batch.findChild<QTabWidget *>()->currentIndex(), 2);
            auto *batchTable = batch.findChild<QTreeWidget *>();
            QCOMPARE(batchTable->topLevelItemCount(), 2);
            QVERIFY(batch.grab().save(QDir(appDir).filePath(QStringLiteral("queue-page.png"))));
            QVERIFY(batchTable->topLevelItem(0)->text(0) != source);
            batch.findChild<QTabWidget *>()->setCurrentIndex(0);
            batch.findChild<QPushButton *>(QStringLiteral("exportSingle"))->click();
            batch.findChild<QPushButton *>(QStringLiteral("exportSingle"))->click();
            QCOMPARE(batchTable->topLevelItemCount(), 2);
            batch.stopAll();

            ExportWindow restart;
            restart.show();
            QString slow = QStringLiteral(
                "import time\nimport vapoursynth as vs\nvs.core.num_threads = 1\n"
                "clip = vs.core.std.BlankClip(width=64,height=36,format=vs.YUV420P8,length=120,fpsnum=6)\n"
                "def slow(n,f):\n    time.sleep(0.05)\n    return f\n"
                "clip=vs.core.std.ModifyFrame(clip,clips=clip,selector=slow)\nclip.set_output()\n");
            QSignalSpy restartedDone(&restart, &ExportWindow::statusMessage);
            const QString restartedOutput = directory.filePath(QStringLiteral("restart.mkv"));
            const QString restartCommand = QStringLiteral("\"%1\" -hide_banner -loglevel error -y -i <输入文件> -map 0:v:0 -c:v ffv1 <输出文件>").arg(ffmpeg);
            QVERIFY(restart.addJob(restartCommand, restartedOutput, source, slow, 20));
            auto *restartTable = restart.findChild<QTreeWidget *>();
            auto *item = restartTable->topLevelItem(0);
            QTRY_VERIFY_WITH_TIMEOUT(item->text(3).toLongLong() > 0, 15000);
            item->setSelected(true);
            restart.findChild<QPushButton *>(QStringLiteral("pauseQueue"))->click();
            QCOMPARE(item->text(1), QStringLiteral("已暂停"));
            restart.findChild<QPushButton *>(QStringLiteral("stopSelected"))->click();
            QTRY_COMPARE_WITH_TIMEOUT(restartedDone.count(), 1, 15000);
            QVERIFY(QFileInfo(restartedOutput).size() > 0);
            restart.findChild<QPushButton *>(QStringLiteral("restartStopped"))->click();
            QCOMPARE(item->text(3), QStringLiteral("0"));
            QCOMPARE(restartTable->topLevelItemCount(), 1);
            restart.findChild<QPushButton *>(QStringLiteral("pauseQueue"))->click();
            QTRY_COMPARE_WITH_TIMEOUT(restartedDone.count(), 2, 20000);
            QCOMPARE(item->text(1), QStringLiteral("导出完成。"));
            QCOMPARE(item->text(3).toLongLong(), 120);
            decode.start(ffmpeg, {QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-i"), restartedOutput,
                QStringLiteral("-map"), QStringLiteral("0:v:0"), QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")});
            QVERIFY(decode.waitForFinished(10000));
            QCOMPARE(decode.exitCode(), 0);
        }
    }
};

QTEST_MAIN(TestExportPipeline)
#include "TestExportPipeline.moc"
