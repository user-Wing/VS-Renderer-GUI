#include "ui/ExportWindow.h"
#include "backend/DetachedExport.h"
#include <QLockFile>
#include <QSignalBlocker>
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
#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
void ExportWindow::rememberCommand(){
    const auto command=command_->toPlainText().trimmed();if(command.isEmpty())return;
    auto commands=QSettings().value("export/commandHistory").toStringList();commands.removeAll(command);commands.prepend(command);
    while(commands.size()>50)commands.removeLast();QSettings().setValue("export/commandHistory",commands);
    history_->clear();history_->addItems(commands);
}
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
        "QTabWidget::pane{border:1px solid #50535a;}"
        "QTabWidget::tab-bar{left:13px;}"
        "QTabBar::tab{background:#27292e;color:#c8cbd1;border:1px solid #50535a;padding:4px 12px;margin-right:2px;}"
        "QTabBar::tab:selected{background:#343840;color:#e8eaed;border-bottom:1px solid #8ab4f8;}"
        "QTabBar::tab:hover:!selected{background:#30343b;border-color:#7086a4;}"));
    layout->addWidget(tabs_);
    message_ = new QLabel(this);
    message_->setWordWrap(true);
    layout->addWidget(message_);
    auto *settings = new QWidget(tabs_);
    auto *form = new QVBoxLayout(settings);
    form->setContentsMargins(12, 12, 12, 12);
    routes_=new QComboBox(settings);routes_->setObjectName("exportRouteChoice");routes_->addItem(QStringLiteral("路 1"));form->addWidget(new QLabel(QStringLiteral("选择路（本次入队任务保存对应 VPY）"),settings));form->addWidget(routes_);
    form->addWidget(new QLabel(QStringLiteral("3FUI 参数(单文件与批处理共用，VS 设置取自主界面)"), settings));
    command_ = new QPlainTextEdit(settings);
    command_->setObjectName(QStringLiteral("exportCommand"));
    command_->setPlaceholderText(QStringLiteral("粘贴包含 -i <输入文件> 与 <输出文件> 的完整 FFmpeg 命令。"));
    form->addWidget(command_, 1);
    auto *historyRow=new QHBoxLayout;
    history_=new QComboBox(settings);history_->setObjectName("exportCommandHistory");history_->setMinimumContentsLength(20);history_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    history_->addItems(QSettings().value("export/commandHistory").toStringList());historyRow->addWidget(history_,1);
    auto *recall=new QPushButton(QStringLiteral("调用历史命令"),settings);historyRow->addWidget(recall);
    connect(recall,&QPushButton::clicked,this,[this]{if(history_->currentIndex()>=0)command_->setPlainText(history_->currentText());});
    auto *saveCommand=new QPushButton(QStringLiteral("保存命令"),settings);historyRow->addWidget(saveCommand);
    connect(saveCommand,&QPushButton::clicked,this,&ExportWindow::rememberCommand);form->addLayout(historyRow);
    auto *server=new QLocalServer(this);server->setSocketOptions(QLocalServer::UserAccessOption);
    server->listen("VSRenderer.ExportCommand");
    connect(server,&QLocalServer::newConnection,this,[this,server]{
        while(auto *socket=server->nextPendingConnection()){
            auto *timeout=new QTimer(socket);timeout->setSingleShot(true);timeout->start(3000);connect(timeout,&QTimer::timeout,socket,&QLocalSocket::abort);
            connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
            connect(socket,&QLocalSocket::readyRead,this,[this,socket]{
                auto buffer=socket->property("commandBuffer").toByteArray()+socket->readAll();
                if(buffer.size()>65536){socket->abort();return;}
                if(!buffer.contains('\n')){socket->setProperty("commandBuffer",buffer);return;}
                const auto command=QJsonDocument::fromJson(buffer.left(buffer.indexOf('\n'))).object().value("command").toString();
                if(!isVisible() || command.trimmed().isEmpty()){socket->write("ERROR: open the export window first\n");socket->disconnectFromServer();return;}
                command_->setPlainText(command);command_->setToolTip(QStringLiteral("此命令来自本机外部程序；导出前请核对可执行文件、参数和输出路径。"));rememberCommand();tabs_->setCurrentIndex(0);raise();activateWindow();
                socket->write("OK\n");socket->disconnectFromServer();
            });
        }
    });
    sourceLabel_ = new QLabel(QStringLiteral("当前文件：未导入"), settings);
    sourceLabel_->setWordWrap(true);
    form->addWidget(sourceLabel_);
    auto *outputRow = new QHBoxLayout;
    singleOutput_ = new QLineEdit(settings);
    singleOutput_->setObjectName(QStringLiteral("singleOutput"));
    singleOutput_->setPlaceholderText(QStringLiteral("单文件输出路径(MKV)"));
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
    naming_->addItems({QStringLiteral("原文件名 + 时间戳(默认)"), QStringLiteral("原文件名，不加时间戳(高风险：可能覆盖已有输出)")});
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
    connect(table_->model(),&QAbstractItemModel::rowsMoved,this,[this]{QJsonArray ids;for(int row=0;row<table_->topLevelItemCount();++row){const auto j=jobFor(table_->topLevelItem(row));if(j)ids.append(j->id);}exportjobs::write(QDir(exportjobs::root()).filePath("order.json"),{{"ids",ids}});});
    auto *poll=new QTimer(this);poll->setInterval(750);connect(poll,&QTimer::timeout,this,&ExportWindow::syncWorker);poll->start();
    syncWorker();
    if(isBusy())exportjobs::ensureWorker();

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
        QFileInfo outputInfo(output);QString stem=outputInfo.completeBaseName();stem.remove(QRegularExpression("_路[0-9]+(?:_[0-9]+)?$"));stem+=QString("_路%1").arg(selectedRoute()+1);
        output=QDir(outputInfo.absolutePath()).filePath(stem+".mkv");
        for(int serial=2;QFileInfo::exists(output)||reservedOutputs.contains(pathKey(output));++serial)output=QDir(outputInfo.absolutePath()).filePath(stem+QString("_%1.mkv").arg(serial));
        if (batch && naming_->currentIndex() == 0) {
            const QString base = output.left(output.size() - 4);
            int serial = 2;
            while (reservedOutputs.contains(pathKey(output)))
                output = base + QStringLiteral("_%1.mkv").arg(serial++);
        }
        if (inputPaths.contains(pathKey(output)))
            error = QStringLiteral("输出不能覆盖本批次的任何输入文件，请添加时间戳或选择其他目录。");
        if (error.isEmpty() && addJob(command_->toPlainText(), output, source, script.script, 0, &error)) {
            rememberCommand();
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
        const auto tokens = QProcess::splitCommand(normalized);
        const QRegularExpression geometry(QStringLiteral("\\b(scale\\w*|crop\\w*|pad\\w*|zscale|transpose\\w*|rotate|v360|perspective|zoompan)\\s*(?:=|,|;|$)"));
        for (int i = 0; i < tokens.size(); ++i) {
            const auto &option = tokens[i];
            if (option == QStringLiteral("-s") || option.startsWith(QStringLiteral("-s:")) ||
                ((option == QStringLiteral("-vf") || option == QStringLiteral("-filter") || option.startsWith(QStringLiteral("-filter:"))) &&
                 i + 1 < tokens.size() && geometry.match(QString(tokens[i + 1]).replace(QStringLiteral("\\_"),QStringLiteral("_"))).hasMatch()))
                return fail(QStringLiteral("对比画布输出必须保持最大源视频分辨率，请移除 3FUI 参数中的尺寸、缩放、裁切或旋转设置。"));
        }
    }
    const QString sourceKey = pathKey(source);
    const QString outputKey = pathKey(output);
    QStringList inputKeys{sourceKey};
    for (const auto &input : compositionSources_) inputKeys.append(pathKey(input));
    if (inputKeys.contains(outputKey)) return fail(QStringLiteral("输出不能覆盖任何输入视频。"));
    for (const auto &existing : jobs_) {
        if (existing->outputKey == outputKey || existing->inputKeys.contains(outputKey) || inputKeys.contains(existing->outputKey))
            return fail(QStringLiteral("输出路径与队列中的输入或输出冲突。"));
    }
    if(!QFileInfo::exists(exportjobs::workerProgram()))return fail(QStringLiteral("缺少 vs-export-worker.exe，请更新完整程序包。"));
    QDir().mkpath(exportjobs::root());QLockFile submitLock(QDir(exportjobs::root()).filePath("submit.lock"));if(!submitLock.tryLock(2000))return fail(QStringLiteral("队列正在提交，请重试。"));
    for(const auto &id:exportjobs::jobs(exportjobs::root())){const auto m=exportjobs::read(QDir(exportjobs::root()).filePath(id+"/job.json"));if(m.isEmpty())continue;
        if(pathKey(m["output"].toString())==outputKey)return fail(QStringLiteral("输出已被持久队列占用，请使用新的文件名。"));
        for(const auto &input:m["inputs"].toArray())if(pathKey(input.toString())==outputKey)return fail(QStringLiteral("输出与队列输入冲突。"));
        if(inputKeys.contains(pathKey(m["output"].toString())))return fail(QStringLiteral("输入与队列输出冲突。"));}
    if(QFileInfo::exists(output))return fail(QStringLiteral("输出文件已存在，请更换名称；已停止任务可明确选择从头重做。"));
    auto job = std::make_shared<Job>();
    job->id="job-"+QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    job->directory=QDir(exportjobs::root()).filePath(job->id);
    if(!QDir().mkpath(job->directory))return fail(QStringLiteral("无法创建持久任务目录。"));
    QSaveFile file(QDir(job->directory).filePath("export.vpy"));
    const QByteArray code = script.toUtf8() +
        "\nimport sys as __vsr_sys\nimport vapoursynth as __vsr_vs\n"
        "__vsr_output = __vsr_vs.get_output(0)\n"
        "__vsr_clip = getattr(__vsr_output, 'clip', __vsr_output)\n"
        "if __vsr_clip.fps_num: print('__VSR_DURATION__=' + str(__vsr_clip.num_frames * __vsr_clip.fps_den / __vsr_clip.fps_num), file=__vsr_sys.stderr, flush=True)\n";
    if (!file.open(QIODevice::WriteOnly) || file.write(code) != code.size())
        return fail(QStringLiteral("无法保存导出脚本。"));
    if(!file.commit())return fail(QStringLiteral("冻结脚本提交失败。"));
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
    job->plan.ffmpegArguments.removeAll("-y");if(!job->plan.ffmpegArguments.contains("-n"))job->plan.ffmpegArguments.prepend("-n");
    if(!exportjobs::write(QDir(job->directory).filePath("job.json"),{{"version",1},{"source",source},{"output",output},{"inputs",QJsonArray::fromStringList(inputKeys)},{"plan",exportjobs::planJson(job->plan)},{"duration",durationSeconds},{"route",selectedRoute()+1}}))return fail(QStringLiteral("任务清单保存失败。"));
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

void ExportWindow::startNext(){if(!exportjobs::ensureWorker())message_->setText(QStringLiteral("编码worker启动失败；任务已保存在磁盘，可稍后重连。"));syncWorker();}
void ExportWindow::syncWorker(){
    const bool wasBusy=isBusy();active_.reset();queuePaused_=exportjobs::read(QDir(exportjobs::root()).filePath("queue.json"))["paused"].toBool();
    for(const auto &id:exportjobs::jobs(exportjobs::root())){
        const auto dir=QDir(exportjobs::root()).filePath(id);const auto m=exportjobs::read(QDir(dir).filePath("job.json"));if(m.isEmpty())continue;
        auto st=exportjobs::read(QDir(dir).filePath("status.json"));const auto state=st["state"].toString("waiting");
        std::shared_ptr<Job> job;for(const auto &j:jobs_)if(j->id==id){job=j;break;}
        if(st["hidden"].toBool()){if(job){jobs_.remove(job->item->data(0,Qt::UserRole).toULongLong());delete job->item;}continue;}
        if(!job){job=std::make_shared<Job>();job->id=id;job->directory=dir;job->source=m["source"].toString();job->output=m["output"].toString();job->sourceKey=pathKey(job->source);job->outputKey=pathKey(job->output);job->inputKeys=exportjobs::strings(m["inputs"]);job->item=new QTreeWidgetItem(table_,{QFileInfo(job->output).fileName()});job->item->setData(0,Qt::UserRole,QVariant::fromValue(++nextId_));jobs_[nextId_]=job;}
        job->state=state=="running"?State::Running:state=="paused"?State::Paused:state=="stopping"?State::Stopping:state=="completed"?State::Completed:state=="failed"?State::Failed:state=="stopped"?State::Stopped:State::Waiting;
        job->started=job->state!=State::Waiting;job->durationSeconds=st["duration"].toDouble(m["duration"].toDouble());
        if(state=="running"||state=="paused"||state=="stopping")active_=job;
        const QMap<QString,QString> names{{"waiting","等待中"},{"running","独立进程编码中"},{"paused","已暂停"},{"stopping","正在停止并封装"},{"completed","已完成"},{"failed","失败/中断（从头重做）"},{"stopped","已停止"}};
        job->item->setText(1,names.value(state,state));job->item->setToolTip(1,st["message"].toString());job->item->setToolTip(0,job->source+"\n"+job->output+"\n日志："+QDir(dir).filePath("worker.log"));
        const double seconds=st["microseconds"].toDouble()/1e6,duration=job->durationSeconds;
        job->item->setText(2,state=="completed"?"100%":duration>0?QString("%1%").arg(qBound(0,int(seconds*100/duration),99)):QStringLiteral("—"));job->item->setText(3,QString::number(qint64(st["frames"].toDouble())));const auto started=QDateTime::fromString(st["started"].toString(),Qt::ISODateWithMs);const qint64 elapsed=started.isValid()?started.secsTo(QDateTime::currentDateTimeUtc()):0;job->item->setText(4,state=="running"&&seconds>0&&duration>seconds?QStringLiteral("约 %1 分（含暂停时间）").arg(qMax<qint64>(0,qint64(elapsed*(duration/seconds-1)/60))):QStringLiteral("—"));
    }
    if(auto selected=jobFor(table_->currentItem());selected){QFile file(QDir(selected->directory).filePath("worker.log"));if(file.open(QIODevice::ReadOnly)){if(file.size()>65536)file.seek(file.size()-65536);const auto text=QString::fromUtf8(file.readAll());if(log_->toPlainText()!=text)log_->setPlainText(text);}}
    pause_->setText(queuePaused_?QStringLiteral("继续编码队列"):QStringLiteral("挂起 / 暂停队列"));
    if(wasBusy&&!isBusy())emit idle();
}
QJsonArray ExportWindow::jobsStatus() const
{
    const QStringList states{"waiting", "running", "paused", "stopping", "stopped", "completed", "failed"};
    QJsonArray result;
    for (const auto &job : jobs_)
        result.append(QJsonObject{{"output", job->output}, {"state", states.value(int(job->state))},
                                  {"bytes", double(QFileInfo(job->output).size())}});
    return result;
}

bool ExportWindow::isBusy() const {for(const auto &j:jobs_)if(j->state==State::Waiting||j->state==State::Running||j->state==State::Paused||j->state==State::Stopping)return true;return false;}
void ExportWindow::togglePause(){
    auto target=active_;if(!target)for(const auto &j:jobs_)if(j->state==State::Waiting){target=j;break;}
    if(target)exportjobs::command(target->id,queuePaused_?"resume":"pause");else {queuePaused_=!queuePaused_;exportjobs::write(QDir(exportjobs::root()).filePath("queue.json"),{{"paused",queuePaused_}});}
    startNext();
}
void ExportWindow::stopJob(const std::shared_ptr<Job> &job){if(job)exportjobs::command(job->id,"stop");}
void ExportWindow::removeWaiting(){for(auto *item:table_->selectedItems()){const auto j=jobFor(item);if(!j)continue;if(j->state!=State::Running&&j->state!=State::Paused&&j->state!=State::Stopping)exportjobs::command(j->id,"remove");}startNext();}
void ExportWindow::restartStopped(){for(auto *item:table_->selectedItems()){const auto j=jobFor(item);if(j&&(j->state==State::Stopped||j->state==State::Failed))exportjobs::command(j->id,"restart");}startNext();}
void ExportWindow::stopAll(){for(const auto &j:jobs_)stopJob(j);startNext();}

}

namespace vsr {
void ExportWindow::setRoutes(int count,int selected){const QSignalBlocker blocker(routes_);routes_->clear();for(int i=0;i<count;++i)routes_->addItem(QString("路 %1").arg(i+1));routes_->setCurrentIndex(qBound(0,selected,qMax(0,count-1)));}
int ExportWindow::selectedRoute() const{return routes_->currentIndex();}
}
