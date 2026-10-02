#include "ui/ExportWindow.h"
#include "ui/PreparedFiles.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QSet>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
namespace {
QString pathKey(const QString &path)
{
    const QFileInfo info(path);
    return QDir::cleanPath(info.exists() ? info.canonicalFilePath() : info.absoluteFilePath()).toCaseFolded();
}
}

ExportWindow::ExportWindow() : QWidget(nullptr, Qt::Window)
{
    setWindowTitle(QStringLiteral("导出当前处理结果"));
    resize(1050, 620);
    auto *layout = new QVBoxLayout(this);
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QStringLiteral("exportTabs"));
    tabs_->setStyleSheet(QStringLiteral(
        "QTabWidget::pane{border:1px solid #c8c8c8;}"
        "QTabWidget::tab-bar{left:13px;}"
        "QTabBar::tab{background:#e9e9e9;color:#4a4a4a;border:1px solid #c8c8c8;padding:9px 20px;margin-right:4px;}"
        "QTabBar::tab:selected{background:#d9ebf7;color:#005a9e;border-bottom:3px solid #0067c0;font-weight:600;}"
        "QTabBar::tab:hover:!selected{background:#f0f6fb;border-color:#7baed6;}"));
    layout->addWidget(tabs_);
    message_ = new QLabel(this);
    message_->setWordWrap(true);
    layout->addWidget(message_);
    auto *settings = new QWidget(tabs_);
    auto *form = new QVBoxLayout(settings);
    form->setContentsMargins(12, 12, 12, 12);
    form->addWidget(new QLabel(QStringLiteral("3FUI 参数（单文件与批处理共用，VS 设置取自主界面）"), settings));
    command_ = new QPlainTextEdit(settings);
    command_->setObjectName(QStringLiteral("exportCommand"));
    command_->setPlaceholderText(QStringLiteral("粘贴包含 -i <输入文件> 与 <输出文件> 的完整 FFmpeg 命令。"));
    form->addWidget(command_, 1);
    sourceLabel_ = new QLabel(QStringLiteral("当前文件：未导入"), settings);
    sourceLabel_->setWordWrap(true);
    form->addWidget(sourceLabel_);
    auto *outputRow = new QHBoxLayout;
    singleOutput_ = new QLineEdit(settings);
    singleOutput_->setObjectName(QStringLiteral("singleOutput"));
    singleOutput_->setPlaceholderText(QStringLiteral("单文件输出路径（MKV）"));
    auto *browse = new QPushButton(QStringLiteral("选择输出…"), settings);
    outputRow->addWidget(singleOutput_, 1);
    outputRow->addWidget(browse);
    form->addLayout(outputRow);
    auto *add = new QPushButton(QStringLiteral("导出当前文件"), settings);
    add->setObjectName(QStringLiteral("exportSingle"));
    form->addWidget(add);
    tabs_->addTab(settings, QStringLiteral("导出设置"));

    auto *prepare = new QWidget(tabs_);
    auto *prepareLayout = new QVBoxLayout(prepare);
    prepareLayout->setContentsMargins(12, 12, 12, 12);
    prepareLayout->addWidget(new QLabel(QStringLiteral("将视频拖入下方列表；全部复用当前 VS 设置与“导出设置”中的 3FUI 参数。"), prepare));
    prepared_ = new PreparedFiles(prepare);
    prepared_->setObjectName(QStringLiteral("preparedFiles"));
    prepareLayout->addWidget(prepared_, 1);
    auto *fileActions = new QHBoxLayout;
    auto *chooseFiles = new QPushButton(QStringLiteral("添加文件…"), prepare);
    auto *removeFiles = new QPushButton(QStringLiteral("移除选中文件"), prepare);
    fileActions->addWidget(chooseFiles);
    fileActions->addWidget(removeFiles);
    fileActions->addStretch();
    prepareLayout->addLayout(fileActions);
    auto *directoryRow = new QHBoxLayout;
    directoryRow->addWidget(new QLabel(QStringLiteral("输出目录"), prepare));
    destination_ = new QComboBox(prepare);
    destination_->setObjectName(QStringLiteral("outputDestination"));
    destination_->addItems({QStringLiteral("原始目录"), QStringLiteral("指定目录")});
    directory_ = new QLineEdit(prepare);
    directory_->setObjectName(QStringLiteral("outputDirectory"));
    directory_->setEnabled(false);
    auto *chooseDirectory = new QPushButton(QStringLiteral("浏览…"), prepare);
    directoryRow->addWidget(destination_);
    directoryRow->addWidget(directory_, 1);
    directoryRow->addWidget(chooseDirectory);
    prepareLayout->addLayout(directoryRow);
    auto *namingRow = new QHBoxLayout;
    namingRow->addWidget(new QLabel(QStringLiteral("命名规则"), prepare));
    naming_ = new QComboBox(prepare);
    naming_->setObjectName(QStringLiteral("outputNaming"));
    naming_->addItems({QStringLiteral("原文件名 + 时间戳（默认）"), QStringLiteral("原文件名，不加时间戳（高风险：可能覆盖已有输出）")});
    namingRow->addWidget(naming_, 1);
    prepareLayout->addLayout(namingRow);
    auto *enqueueAll = new QPushButton(QStringLiteral("全部加入编码队列"), prepare);
    enqueueAll->setObjectName(QStringLiteral("enqueueAll"));
    prepareLayout->addWidget(enqueueAll);
    tabs_->addTab(prepare, QStringLiteral("准备文件"));

    auto *queue = new QWidget(tabs_);
    auto *queueLayout = new QVBoxLayout(queue);
    queueLayout->setContentsMargins(12, 12, 12, 12);
    table_ = new QTreeWidget(queue);
    table_->setObjectName(QStringLiteral("encodingQueue"));
    table_->setColumnCount(5);
    table_->setHeaderLabels({QStringLiteral("输出文件"), QStringLiteral("状态"),
        QStringLiteral("进度"), QStringLiteral("处理帧数"), QStringLiteral("预计剩余时间")});
    table_->setRootIsDecorated(false);
    table_->setUniformRowHeights(true);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setDragDropMode(QAbstractItemView::InternalMove);
    table_->setDefaultDropAction(Qt::MoveAction);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->header()->setSectionsMovable(true);
    table_->header()->setStretchLastSection(false);
    table_->header()->setSectionResizeMode(QHeaderView::Interactive);
    table_->header()->setStyleSheet(QStringLiteral(
        "QHeaderView::section{background:#eeeeee;color:#1b1b1b;border:0;border-right:2px solid #999999;border-bottom:1px solid #aaaaaa;padding:6px 8px;}"));
    table_->setColumnWidth(0, 420);
    table_->setColumnWidth(1, 230);
    queueLayout->addWidget(table_, 1);
    auto *actions = new QHBoxLayout;
    pause_ = new QPushButton(QStringLiteral("挂起 / 暂停队列"), queue);
    pause_->setObjectName(QStringLiteral("pauseQueue"));
    auto *stop = new QPushButton(QStringLiteral("停止选中任务"), queue);
    stop->setObjectName(QStringLiteral("stopSelected"));
    auto *remove = new QPushButton(QStringLiteral("删除未处理任务"), queue);
    remove->setObjectName(QStringLiteral("removeWaiting"));
    auto *restart = new QPushButton(QStringLiteral("从头重新处理"), queue);
    restart->setObjectName(QStringLiteral("restartStopped"));
    for (auto *button : {pause_, stop, remove, restart}) actions->addWidget(button);
    queueLayout->addLayout(actions);
    log_ = new QPlainTextEdit(queue);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);
    log_->setMaximumHeight(100);
    queueLayout->addWidget(log_);
    tabs_->addTab(queue, QStringLiteral("编码队列"));

    connect(browse, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"), singleOutput_->text(), QStringLiteral("Matroska 视频 (*.mkv)"));
        if (!path.isEmpty()) singleOutput_->setText(path);
    });
    connect(add, &QPushButton::clicked, this, [this] { enqueue(false); });
    connect(enqueueAll, &QPushButton::clicked, this, [this] { enqueue(true); });
    connect(chooseFiles, &QPushButton::clicked, this, [this] {
        prepared_->addFiles(QFileDialog::getOpenFileNames(this, QStringLiteral("准备视频文件"), {}, QStringLiteral("视频 (*.mkv *.mp4 *.mov *.avi *.ts *.m2ts *.webm);;所有文件 (*.*)")));
    });
    connect(removeFiles, &QPushButton::clicked, this, [this] {
        for (auto *item : prepared_->selectedItems()) delete item;
    });
    connect(destination_, &QComboBox::currentIndexChanged, this, [this](int index) { directory_->setEnabled(index == 1); });
    connect(chooseDirectory, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("输出目录"), directory_->text());
        if (!path.isEmpty()) { destination_->setCurrentIndex(1); directory_->setText(path); }
    });
    connect(pause_, &QPushButton::clicked, this, &ExportWindow::togglePause);
    connect(remove, &QPushButton::clicked, this, &ExportWindow::removeWaiting);
    connect(restart, &QPushButton::clicked, this, &ExportWindow::restartStopped);
    connect(stop, &QPushButton::clicked, this, [this] {
        for (auto *item : table_->selectedItems()) stopJob(jobFor(item));
    });
    connect(&pipeline_, &ExportPipeline::logMessage, log_, &QPlainTextEdit::appendPlainText);
    connect(&pipeline_, &ExportPipeline::durationKnown, this, [this](double duration) {
        if (active_) active_->durationSeconds = duration;
    });
    connect(&pipeline_, &ExportPipeline::progress, this, [this](qint64 frames, qint64 microseconds) {
        if (!active_) return;
        auto *item = active_->item;
        item->setText(3, QString::number(frames));
        const double seconds = microseconds / 1000000.0;
        const double duration = active_->durationSeconds;
        if (duration > 0 && seconds > 0) {
            item->setText(2, QStringLiteral("%1%").arg(std::clamp(static_cast<int>(seconds * 100 / duration), 0, 99)));
            const qint64 running = elapsed_.elapsed() - pausedMilliseconds_ - (pipeline_.isPaused() ? pauseTime_.elapsed() : 0);
            const qint64 remaining = static_cast<qint64>(std::max(0.0, duration / seconds - 1) * running / 1000);
            item->setText(4, QStringLiteral("约 %1 分 %2 秒").arg(remaining / 60).arg(remaining % 60));
        }
    });
    connect(&pipeline_, &ExportPipeline::finished, this, [this](bool success, const QString &message) {
        if (!active_) return;
        active_->state = active_->state == State::Stopping ? State::Stopped : (success ? State::Completed : State::Failed);
        active_->item->setText(1, message);
        if (success) active_->item->setText(2, QStringLiteral("100%"));
        active_->item->setText(4, QStringLiteral("—"));
        active_.reset();
        emit statusMessage(message);
        QTimer::singleShot(0, this, &ExportWindow::startNext);
    });
}

void ExportWindow::setScriptBuilder(std::function<ScriptBuildResult(const QString &)> builder) { scriptBuilder_ = std::move(builder); }

void ExportWindow::setCurrentSource(const QString &source)
{
    if (currentSource_ == source) return;
    currentSource_ = source;
    sourceLabel_->setText(QStringLiteral("当前文件：%1").arg(source));
    singleOutput_->setText(source.isEmpty() ? QString() : outputPath(source, {}, true));
}

void ExportWindow::setComposition(const QString &script, const QStringList &sources, double audioOffset, bool muted, const QString &description, const QSize &outputSize)
{
    compositionSources_ = sources;
    compositionAudioOffset_ = audioOffset;
    compositionMuted_ = muted;
    compositionSize_ = outputSize;
    setScriptBuilder([script](const QString &) { ScriptBuildResult result; result.script = script; return result; });
    tabs_->setTabEnabled(1, false);
    sourceLabel_->setText(description);
    findChild<QPushButton *>(QStringLiteral("exportSingle"))->setText(QStringLiteral("导出对比画布"));
}

QString ExportWindow::outputPath(const QString &source, const QString &directory, bool timestamp)
{
    const QFileInfo info(source);
    const QDir target(directory.isEmpty() ? info.absolutePath() : directory);
    const QString suffix = timestamp ? QStringLiteral("_") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")) : QString();
    const QString stem = info.completeBaseName() + suffix;
    QString path = target.filePath(stem + QStringLiteral(".mkv"));
    for (int serial = 2; timestamp && QFileInfo::exists(path); ++serial)
        path = target.filePath(stem + QStringLiteral("_%1.mkv").arg(serial));
    return path;
}

void ExportWindow::enqueue(bool batch)
{
    if (!scriptBuilder_) return;
    const QString directory = destination_->currentIndex() == 0 ? QString() : directory_->text().trimmed();
    if (batch && destination_->currentIndex() == 1 && !QFileInfo(directory).isDir()) {
        message_->setText(QStringLiteral("请选择有效的输出目录。"));
        return;
    }
    const QStringList sources = batch ? prepared_->files() : QStringList{currentSource_};
    QSet<QString> inputPaths;
    QSet<QString> reservedOutputs;
    for (const auto &source : sources) inputPaths.insert(pathKey(source));
    for (const auto &job : jobs_) reservedOutputs.insert(job->outputKey);
    QStringList errors;
    int added = 0;
    for (const auto &source : sources) {
        const auto script = scriptBuilder_(source);
        QString error = script.errors.join('\n');
        QString output = batch ? outputPath(source, directory, naming_->currentIndex() == 0) : singleOutput_->text();
        if (batch && naming_->currentIndex() == 0) {
            const QString base = output.left(output.size() - 4);
            int serial = 2;
            while (reservedOutputs.contains(pathKey(output)))
                output = base + QStringLiteral("_%1.mkv").arg(serial++);
        }
        if (inputPaths.contains(pathKey(output)))
            error = QStringLiteral("输出不能覆盖本批次的任何输入文件，请添加时间戳或选择其他目录。");
        if (error.isEmpty() && addJob(command_->toPlainText(), output, source, script.script, 0, &error)) {
            ++added;
            reservedOutputs.insert(pathKey(output));
            if (batch) {
                const auto matches = prepared_->findItems(source, Qt::MatchExactly);
                for (auto *item : matches) delete item;
            }
        } else errors.append(QFileInfo(source).fileName() + QStringLiteral(": ") + error);
    }
    message_->setText(QStringLiteral("已加入 %1 个任务%2").arg(added).arg(errors.isEmpty() ? QString() : QStringLiteral("；未加入 %1 个，详情见队列日志。").arg(errors.size())));
    for (const QString &error : errors) log_->appendPlainText(error);
    if (added > 0) tabs_->setCurrentIndex(2);
}

bool ExportWindow::addJob(const QString &command, const QString &output, const QString &source,
                          const QString &script, double durationSeconds, QString *error)
{
    const auto fail = [&](const QString &message) { if (error) *error = message; return false; };
    if (!compositionSources_.isEmpty()) {
        auto normalized = command;
        normalized.replace(QStringLiteral("\\_"), QStringLiteral("_"));
        const auto tokens = QProcess::splitCommand(normalized);
        const QRegularExpression geometry(QStringLiteral("\\b(scale\\w*|crop\\w*|pad\\w*|zscale|transpose\\w*|rotate|v360|perspective|zoompan)\\s*(?:=|,|;|$)"));
        for (int i = 0; i < tokens.size(); ++i) {
            const auto &option = tokens[i];
            if (option == QStringLiteral("-s") || option.startsWith(QStringLiteral("-s:")) ||
                ((option == QStringLiteral("-vf") || option == QStringLiteral("-filter") || option.startsWith(QStringLiteral("-filter:"))) &&
                 i + 1 < tokens.size() && geometry.match(tokens[i + 1]).hasMatch()))
                return fail(QStringLiteral("对比画布输出必须保持最大源视频分辨率，请移除 3FUI 参数中的尺寸、缩放、裁切或旋转设置。"));
        }
    }
    const QString sourceKey = pathKey(source);
    const QString outputKey = pathKey(output);
    QStringList inputKeys{sourceKey};
    for (const auto &input : compositionSources_) inputKeys.append(pathKey(input));
    if (inputKeys.contains(outputKey)) return fail(QStringLiteral("输出不能覆盖任何输入视频。"));
    for (const auto &existing : jobs_) {
        if (existing->sourceKey == sourceKey)
            return fail(QStringLiteral("此文件已在队列中，停止后请使用“从头重新处理”，不要重复添加。"));
        if (existing->outputKey == outputKey || existing->inputKeys.contains(outputKey) || inputKeys.contains(existing->outputKey))
            return fail(QStringLiteral("输出路径与队列中的输入或输出冲突。"));
    }
    auto job = std::make_shared<Job>();
    job->directory = std::make_shared<QTemporaryDir>();
    QFile file(job->directory->filePath(QStringLiteral("export.vpy")));
    const QByteArray code = script.toUtf8() +
        "\nimport sys as __vsr_sys\nimport vapoursynth as __vsr_vs\n"
        "__vsr_output = __vsr_vs.get_output(0)\n"
        "__vsr_clip = getattr(__vsr_output, 'clip', __vsr_output)\n"
        "if __vsr_clip.fps_num: print('__VSR_DURATION__=' + str(__vsr_clip.num_frames * __vsr_clip.fps_den / __vsr_clip.fps_num), file=__vsr_sys.stderr, flush=True)\n";
    if (!job->directory->isValid() || !file.open(QIODevice::WriteOnly) || file.write(code) != code.size())
        return fail(QStringLiteral("无法保存导出脚本。"));
    file.close();
    job->plan = ExportPipeline::buildPlan(command, source, output, file.fileName(), QCoreApplication::applicationDirPath());
    if (!job->plan.error.isEmpty()) return fail(job->plan.error);
    for (const auto &input : compositionSources_)
        if (pathKey(input) == outputKey) return fail(QStringLiteral("输出不能覆盖任何对比源视频。"));
    if (!compositionSources_.isEmpty()) {
        auto &args = job->plan.ffmpegArguments;
        const int destination = args.indexOf(QDir::toNativeSeparators(output));
        args.insert(destination,QStringLiteral("-s:v"));
        args.insert(destination+1,QStringLiteral("%1x%2").arg(compositionSize_.width()).arg(compositionSize_.height()));
        const int input = args.indexOf(QStringLiteral("-i"));
        if (input >= 0 && compositionAudioOffset_ != 0) {
            args.insert(input, QStringLiteral("-itsoffset"));
            args.insert(input + 1, QString::number(-compositionAudioOffset_, 'f', 7));
        }
        if (compositionMuted_) {
            const int destination = args.indexOf(QDir::toNativeSeparators(output));
            args.insert(destination, QStringLiteral("-an"));
        }
    }
    job->source = source;
    job->sourceKey = sourceKey;
    job->inputKeys = inputKeys;
    job->outputKey = outputKey;
    job->output = output;
    job->durationSeconds = durationSeconds;
    job->item = new QTreeWidgetItem(table_, {QFileInfo(output).fileName(), QStringLiteral("等待中"), QStringLiteral("0%"), QStringLiteral("0"), QStringLiteral("估算中")});
    job->item->setFlags(job->item->flags() & ~Qt::ItemIsDropEnabled);
    job->item->setToolTip(0, QStringLiteral("输入：%1\n输出：%2").arg(source, output));
    job->item->setData(0, Qt::UserRole, QVariant::fromValue(++nextId_));
    jobs_.insert(nextId_, job);
    QTimer::singleShot(0, this, &ExportWindow::startNext);
    return true;
}

std::shared_ptr<ExportWindow::Job> ExportWindow::jobFor(QTreeWidgetItem *item) const
{
    return item ? jobs_.value(item->data(0, Qt::UserRole).toULongLong()) : nullptr;
}

void ExportWindow::startNext()
{
    if (active_) return;
    if (queuePaused_) { if (!isBusy()) emit idle(); return; }
    for (int row = 0; row < table_->topLevelItemCount(); ++row) {
        auto job = jobFor(table_->topLevelItem(row));
        if (job->state != State::Waiting) continue;
        active_ = job;
        job->state = State::Running;
        job->started = true;
        job->item->setText(1, QStringLiteral("编码中"));
        elapsed_.start();
        pausedMilliseconds_ = 0;
        pipeline_.start(job->plan);
        return;
    }
    emit idle();
}

bool ExportWindow::isBusy() const
{
    return active_ || std::any_of(jobs_.cbegin(), jobs_.cend(), [](const auto &job) { return job->state == State::Waiting; });
}

void ExportWindow::togglePause()
{
    const bool pause = !queuePaused_;
    if (active_ && active_->state != State::Stopping) {
        if (!pipeline_.setPaused(pause)) { message_->setText(QStringLiteral("暂停/恢复失败，请重试。")); return; }
        active_->state = pause ? State::Paused : State::Running;
        active_->item->setText(1, pause ? QStringLiteral("已暂停") : QStringLiteral("编码中"));
        if (pause) pauseTime_.start(); else pausedMilliseconds_ += pauseTime_.elapsed();
    }
    queuePaused_ = pause;
    pause_->setText(pause ? QStringLiteral("继续编码队列") : QStringLiteral("挂起 / 暂停队列"));
    if (!pause) startNext();
}

void ExportWindow::stopJob(const std::shared_ptr<Job> &job)
{
    if (!job) return;
    if (job == active_ && job->state != State::Stopping) {
        if (pipeline_.isPaused()) {
            if (!pipeline_.setPaused(false)) return;
            pausedMilliseconds_ += pauseTime_.elapsed();
        }
        job->state = State::Stopping;
        job->item->setText(1, QStringLiteral("正在停止并完成文件封装…"));
        pipeline_.cancel();
    } else if (job->state == State::Waiting) {
        job->state = State::Stopped;
        job->item->setText(1, QStringLiteral("已停止（未开始）"));
    }
}

void ExportWindow::removeWaiting()
{
    for (auto *item : table_->selectedItems()) {
        auto job = jobFor(item);
        if (job->started || job == active_) continue;
        jobs_.remove(item->data(0, Qt::UserRole).toULongLong());
        delete item;
    }
    if (!isBusy()) emit idle();
}

void ExportWindow::restartStopped()
{
    for (auto *item : table_->selectedItems()) {
        auto job = jobFor(item);
        if (job->state != State::Stopped && job->state != State::Failed) continue;
        job->plan.ffmpegArguments.removeAll(QStringLiteral("-n"));
        if (!job->plan.ffmpegArguments.contains(QStringLiteral("-y"))) job->plan.ffmpegArguments.prepend(QStringLiteral("-y"));
        job->state = State::Waiting;
        item->setText(1, QStringLiteral("等待中（从头处理）"));
        item->setText(2, QStringLiteral("0%"));
        item->setText(3, QStringLiteral("0"));
        item->setText(4, QStringLiteral("估算中"));
    }
    startNext();
}

void ExportWindow::stopAll()
{
    for (const auto &job : jobs_) stopJob(job);
    if (!active_) QTimer::singleShot(0, this, &ExportWindow::startNext);
}

}
