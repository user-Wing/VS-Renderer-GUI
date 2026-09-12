#include "app/MainWindow.h"

#include "backend/FrameTimeline.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "graph/FilterCatalog.h"
#include "graph/VpyScriptBuilder.h"
#include "ui/CollapsibleSection.h"
#include "ui/ParameterEditor.h"
#include "ui/PreviewPane.h"

#include <QAction>
#include <QAbstractItemModel>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace vsr {
namespace {

QString formatTime(std::int64_t value100ns)
{
    const auto totalMs = std::max<std::int64_t>(0, value100ns / 10000);
    const auto hours = totalMs / 3600000;
    const auto minutes = (totalMs / 60000) % 60;
    const auto seconds = (totalMs / 1000) % 60;
    const auto millis = totalMs % 1000;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0')).arg(millis, 3, 10, QLatin1Char('0'));
}

QPushButton *compactButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setFixedHeight(30);
    return button;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("VS Renderer — VapourSynth 实时滤镜工作台"));
    resize(1440, 900);
    setMinimumSize(1120, 700);
    buildToolbar();

    auto *root = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *content = new QSplitter(Qt::Horizontal, root);
    content->setHandleWidth(1);
    content->addWidget(buildSidebar());
    content->addWidget(buildWorkspace());
    content->setSizes({328, 1112});
    content->setStretchFactor(0, 0);
    content->setStretchFactor(1, 1);
    rootLayout->addWidget(content, 1);
    rootLayout->addWidget(buildTransport());
    setCentralWidget(root);

    statusBar()->setFixedHeight(24);
    taskStatus_ = new QLabel(this);
    frameStatus_ = new QLabel(QStringLiteral("帧 --  |  VS --  |  3FP --"), this);
    statusBar()->addWidget(taskStatus_, 1);
    statusBar()->addPermanentWidget(frameStatus_);

    sourcePlayer_ = std::make_unique<ThreeFpPlayer>(api_, sourcePane_->surface(), this);
    processedPlayer_ = std::make_unique<ThreeFpPlayer>(api_, processedPane_->surface(), this);
    frameServer_ = std::make_unique<VapourSynthFrameServer>(this);
    processedPlayer_->setMuted(true);

    const QString fp = api_.available()
        ? QStringLiteral("3FP API %1").arg(api_.apiVersion())
        : QStringLiteral("3FP 未加载");
    const QString vs = frameServer_->available() ? QStringLiteral("VapourSynth 帧接口已就绪") : QStringLiteral("VapourSynth 未加载");
    runtimeStatus_->setText(fp + QStringLiteral(" · ") + vs);
    runtimeStatus_->setToolTip(QStringLiteral("3FP: %1\nVSScript: %2")
        .arg(api_.available() ? api_.libraryPath() : api_.errorString(),
             frameServer_->available() ? frameServer_->libraryPath() : frameServer_->errorString()));
    const bool runtimesReady = api_.available() && frameServer_->available();
    setStatus(runtimesReady ? QStringLiteral("就绪；打开源并点击“渲染预览”。")
                            : QStringLiteral("%1 %2").arg(api_.errorString(), frameServer_->errorString()), !runtimesReady);

    connectPlayback();
    populateCatalog();

    connect(frameServer_.get(), &VapourSynthFrameServer::scriptLoaded, this,
            [this](const VapourSynthClipInfo &processed, const VapourSynthClipInfo &source) {
        vsScriptReady_ = true;
        sourceTotalFrames_ = source.totalFrames;
        sourceFpsNumerator_ = source.fpsNumerator;
        sourceFpsDenominator_ = source.fpsDenominator;
        vsTotalFrames_ = processed.totalFrames;
        vsFpsNumerator_ = processed.fpsNumerator;
        vsFpsDenominator_ = processed.fpsDenominator;
        requestedVsFrame_ = -1;
        lastVsFrame_ = -1;
        processedPane_->setBadge(QStringLiteral("VS · %1 · 首帧渲染中").arg(processed.formatName));
        processedPane_->setSurfaceActive(true);
        processedPlayer_->redraw();
        setStatus(QStringLiteral("VPY 已载入：源 %1 帧，处理后 %2×%3 / %4 帧 / %5；正在定位当前帧。")
            .arg(source.totalFrames).arg(processed.width).arg(processed.height)
            .arg(processed.totalFrames).arg(processed.formatName));
        const auto snap = sourcePlayer_->snapshot();
        requestProcessedFrame(static_cast<int>(frameAtPosition100ns(
            snap.position100ns, vsTotalFrames_, vsFpsNumerator_, vsFpsDenominator_)));
    });
    connect(frameServer_.get(), &VapourSynthFrameServer::frameReady, this,
            [this](const VapourSynthFrame &frame) {
        const int desired = requestedVsFrame_;
        const auto source = sourcePlayer_->snapshot();
        const int sourceTarget = static_cast<int>(frameAtPosition100ns(
            source.position100ns, vsTotalFrames_, vsFpsNumerator_, vsFpsDenominator_));
        if (timelineSeekPending_ || source.frameIndex < 0 || desired < 0 ||
            frame.frameIndex != desired || sourceTarget != desired) {
            if (!timelineSeekPending_ && source.frameIndex >= 0 && sourceTarget >= 0)
                requestProcessedFrame(sourceTarget);
            return;
        }
        if (processedPlayer_->submitFrame(frame)) {
            lastVsFrame_ = static_cast<int>(frame.frameIndex);
            processedPane_->setSurfaceActive(true);
            processedPane_->setBadge(QStringLiteral("VS · 实时 · 帧 %1").arg(frame.frameIndex));
        } else {
            processedPane_->setPlaceholderText(processedPlayer_->lastError());
            processedPane_->setSurfaceActive(false);
        }
    });
    connect(frameServer_.get(), &VapourSynthFrameServer::errorOccurred, this,
            [this](const QString &message) {
        vsScriptReady_ = false;
        processedPane_->setPlaceholderText(message);
        processedPane_->setSurfaceActive(false);
        processedPane_->setBadge(QStringLiteral("VS · 错误"));
        setStatus(message, true);
    });

    qApp->installEventFilter(this);

    stateTimer_ = new QTimer(this);
    stateTimer_->setInterval(33);
    connect(stateTimer_, &QTimer::timeout, this, &MainWindow::updatePlaybackState);
    stateTimer_->start();
}

MainWindow::~MainWindow() = default;

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (isActiveWindow() && event->type() == QEvent::ShortcutOverride) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Space && key->modifiers() == Qt::NoModifier) {
            event->accept();
            return true;
        }
    }
    if (isActiveWindow() && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Space && key->modifiers() == Qt::NoModifier) {
            if (!key->isAutoRepeat())
                togglePlayback();
            event->accept();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::buildToolbar()
{
    auto *bar = addToolBar(QStringLiteral("命令栏"));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setFixedHeight(44);
    bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto *openSourceAction = bar->addAction(QStringLiteral("打开源"));
    auto *openProjectAction = bar->addAction(QStringLiteral("打开项目"));
    auto *saveProjectAction = bar->addAction(QStringLiteral("保存项目"));
    bar->addSeparator();
    auto *viewScriptAction = bar->addAction(QStringLiteral("查看 VPY"));
    auto *validateAction = bar->addAction(QStringLiteral("生成并验证"));
    auto *previewAction = bar->addAction(QStringLiteral("渲染预览"));
    previewAction->setToolTip(QStringLiteral("通过 VSScript 载入脚本，并把当前帧直接提交给 3FP。"));
    auto *spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);
    auto *settingsAction = bar->addAction(QStringLiteral("实验设置"));

    connect(openSourceAction, &QAction::triggered, this, &MainWindow::openSource);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(viewScriptAction, &QAction::triggered, this, &MainWindow::showScript);
    connect(validateAction, &QAction::triggered, this, &MainWindow::validateScript);
    connect(previewAction, &QAction::triggered, this, &MainWindow::validateScript);
    connect(settingsAction, &QAction::triggered, this, [this] {
        vrrPresent_->setFocus(Qt::ShortcutFocusReason);
        setStatus(QStringLiteral("实验设置位于比较栏：VRR low-latency present 与 VRR Pacing。"));
    });
}

QWidget *MainWindow::buildSidebar()
{
    auto *sidebar = new QWidget(this);
    sidebar->setMinimumWidth(296);
    sidebar->setMaximumWidth(460);
    sidebar->setStyleSheet(QStringLiteral("background:#ffffff;border-right:1px solid #d1d1d1;"));
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    auto *input = new QWidget(sidebar);
    auto *inputLayout = new QVBoxLayout(input);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(4);
    sourcePath_ = new QLineEdit(input);
    sourcePath_->setReadOnly(true);
    sourcePath_->setPlaceholderText(QStringLiteral("尚未选择视频"));
    auto *inputRow = new QHBoxLayout;
    sourceFilter_ = new QComboBox(input);
    sourceFilter_->addItem(QStringLiteral("L-SMASH Works"), static_cast<int>(SourceFilter::Lsmas));
    sourceFilter_->addItem(QStringLiteral("FFMS2"), static_cast<int>(SourceFilter::Ffms2));
    auto *browse = compactButton(QStringLiteral("浏览…"), input);
    inputRow->addWidget(sourceFilter_, 1);
    inputRow->addWidget(browse);
    runtimeStatus_ = new QLabel(input);
    runtimeStatus_->setWordWrap(true);
    runtimeStatus_->setStyleSheet(QStringLiteral("color:#5c5c5c;border:0;"));
    inputLayout->addWidget(sourcePath_);
    inputLayout->addLayout(inputRow);
    inputLayout->addWidget(runtimeStatus_);
    connect(browse, &QPushButton::clicked, this, &MainWindow::openSource);
    connect(sourceFilter_, &QComboBox::currentIndexChanged, this, [this] { setStatus(QStringLiteral("源滤镜已更改，脚本待验证。")); });

    auto *filters = new QWidget(sidebar);
    auto *filtersLayout = new QVBoxLayout(filters);
    filtersLayout->setContentsMargins(8, 8, 8, 8);
    filtersLayout->setSpacing(4);
    catalogSearch_ = new QLineEdit(filters);
    catalogSearch_->setPlaceholderText(QStringLiteral("搜索滤镜"));
    catalogList_ = new QListWidget(filters);
    catalogList_->setMaximumHeight(150);
    pipelineList_ = new QListWidget(filters);
    pipelineList_->setSelectionMode(QAbstractItemView::SingleSelection);
    auto *add = compactButton(QStringLiteral("添加到处理链"), filters);
    auto *nodeButtons = new QHBoxLayout;
    auto *up = compactButton(QStringLiteral("上移"), filters);
    auto *down = compactButton(QStringLiteral("下移"), filters);
    auto *remove = compactButton(QStringLiteral("删除"), filters);
    nodeButtons->addWidget(up);
    nodeButtons->addWidget(down);
    nodeButtons->addWidget(remove);
    filtersLayout->addWidget(catalogSearch_);
    filtersLayout->addWidget(catalogList_);
    filtersLayout->addWidget(add);
    filtersLayout->addWidget(new QLabel(QStringLiteral("处理链（自上而下执行）"), filters));
    filtersLayout->addWidget(pipelineList_, 1);
    filtersLayout->addLayout(nodeButtons);

    connect(catalogSearch_, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < catalogList_->count(); ++i)
            catalogList_->item(i)->setHidden(!catalogList_->item(i)->text().contains(text, Qt::CaseInsensitive));
    });
    connect(add, &QPushButton::clicked, this, [this] {
        const auto *item = catalogList_->currentItem();
        if (!item)
            return;
        const int row = graph_.add(item->data(Qt::UserRole).toString());
        refreshPipeline(row);
        setStatus(QStringLiteral("已添加滤镜，VPY 待验证。"));
    });
    connect(catalogList_, &QListWidget::itemDoubleClicked, add, &QPushButton::click);
    connect(pipelineList_, &QListWidget::currentRowChanged, this, &MainWindow::selectPipelineRow);
    connect(pipelineList_, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        graph_.setEnabled(pipelineList_->row(item), item->checkState() == Qt::Checked);
        setStatus(QStringLiteral("节点状态已更改，VPY 待验证。"));
    });
    connect(up, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        if (row > 0 && graph_.move(row, row - 1)) refreshPipeline(row - 1);
    });
    connect(down, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        if (row >= 0 && row + 1 < graph_.nodes().size() && graph_.move(row, row + 1)) refreshPipeline(row + 1);
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        if (graph_.remove(row)) refreshPipeline(std::min(row, static_cast<int>(graph_.nodes().size()) - 1));
    });

    auto *parameters = new QWidget(sidebar);
    auto *parameterLayout = new QVBoxLayout(parameters);
    parameterLayout->setContentsMargins(0, 0, 0, 8);
    parameterLayout->setSpacing(4);
    parameterEditor_ = new ParameterEditor(parameters);
    auto *script = compactButton(QStringLiteral("查看生成的 VPY"), parameters);
    parameterLayout->addWidget(parameterEditor_);
    parameterLayout->addWidget(script, 0, Qt::AlignRight);
    connect(script, &QPushButton::clicked, this, &MainWindow::showScript);
    connect(parameterEditor_, &ParameterEditor::parameterChanged, this,
            [this](const QString &id, const QVariant &value) {
        if (graph_.setParameter(pipelineList_->currentRow(), id, value))
            setStatus(QStringLiteral("参数已更新，VPY 待验证。"));
    });

    auto *inputSection = new CollapsibleSection(QStringLiteral("输入与环境"), input, sidebar);
    auto *filterSection = new CollapsibleSection(QStringLiteral("滤镜库与处理链"), filters, sidebar);
    auto *parameterSection = new CollapsibleSection(QStringLiteral("参数与脚本"), parameters, sidebar);
    layout->addWidget(inputSection);
    layout->addWidget(filterSection, 1);
    layout->addWidget(parameterSection);

    const auto keepOneOpen = [inputSection, filterSection, parameterSection](bool) {
        if (!inputSection->isExpanded() && !filterSection->isExpanded() && !parameterSection->isExpanded())
            filterSection->setExpanded(true);
    };
    connect(inputSection, &CollapsibleSection::expandedChanged, sidebar, keepOneOpen);
    connect(filterSection, &CollapsibleSection::expandedChanged, sidebar, keepOneOpen);
    connect(parameterSection, &CollapsibleSection::expandedChanged, sidebar, keepOneOpen);
    return sidebar;
}

QWidget *MainWindow::buildWorkspace()
{
    auto *workspace = new QWidget(this);
    auto *layout = new QVBoxLayout(workspace);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *compareBar = new QWidget(workspace);
    compareBar->setFixedHeight(36);
    compareBar->setStyleSheet(QStringLiteral("background:#f9f9f9;border-bottom:1px solid #d1d1d1;"));
    auto *compareLayout = new QHBoxLayout(compareBar);
    compareLayout->setContentsMargins(8, 2, 8, 2);
    compareLayout->setSpacing(12);
    compareLayout->addWidget(new QLabel(QStringLiteral("双路对比 · 左侧播放时钟 / 右侧 VS 同帧"), compareBar));
    compareLayout->addStretch();
    syncView_ = new QCheckBox(QStringLiteral("同步视图"), compareBar);
    syncView_->setChecked(true);
    vrrPresent_ = new QCheckBox(QStringLiteral("VRR 低延迟（实验）"), compareBar);
    vrrPacing_ = new QCheckBox(QStringLiteral("VRR Pacing（实验）"), compareBar);
    vrrPresent_->setToolTip(QStringLiteral("调用 FFF3FP_SetPresentConfig；不支持时保持 VSync。"));
    vrrPacing_->setToolTip(QStringLiteral("调用 FFF3FP_SetPacingConfig；建议与 VRR 低延迟配合。"));
    compareLayout->addWidget(syncView_);
    compareLayout->addWidget(vrrPresent_);
    compareLayout->addWidget(vrrPacing_);

    auto *previews = new QSplitter(Qt::Horizontal, workspace);
    previews->setHandleWidth(1);
    sourcePane_ = new PreviewPane(QStringLiteral("源视频"), QStringLiteral("Fit"));
    processedPane_ = new PreviewPane(QStringLiteral("处理后"), QStringLiteral("VS · 待渲染"));
    previews->addWidget(sourcePane_);
    previews->addWidget(processedPane_);
    previews->setSizes({556, 556});
    sourcePane_->setActive(true);

    layout->addWidget(compareBar);
    layout->addWidget(previews, 1);
    return workspace;
}

QWidget *MainWindow::buildTransport()
{
    auto *transport = new QWidget(this);
    transport->setFixedHeight(76);
    transport->setStyleSheet(QStringLiteral("background:#f9f9f9;border-top:1px solid #d1d1d1;"));
    auto *layout = new QVBoxLayout(transport);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(2);

    auto *timelineRow = new QHBoxLayout;
    positionLabel_ = new QLabel(QStringLiteral("00:00:00.000"), transport);
    durationLabel_ = new QLabel(QStringLiteral("00:00:00.000"), transport);
    positionLabel_->setFixedWidth(92);
    durationLabel_->setFixedWidth(92);
    durationLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeline_ = new QSlider(Qt::Horizontal, transport);
    timeline_->setRange(0, 100000);
    timelineRow->addWidget(positionLabel_);
    timelineRow->addWidget(timeline_, 1);
    timelineRow->addWidget(durationLabel_);

    auto *controlRow = new QHBoxLayout;
    auto *mode = new QComboBox(transport);
    mode->addItems({QStringLiteral("并排"), QStringLiteral("A/B 滑块（后续）")});
    mode->model()->setData(mode->model()->index(1, 0), 0, Qt::UserRole - 1);
    auto *previous = compactButton(QStringLiteral("◀ 帧"), transport);
    playButton_ = compactButton(QStringLiteral("播放"), transport);
    auto *next = compactButton(QStringLiteral("帧 ▶"), transport);
    auto *scaler = new QComboBox(transport);
    scaler->addItem(QStringLiteral("放大：Nearest"), static_cast<int>(ThreeFpScalingAlgorithm::Nearest));
    scaler->addItem(QStringLiteral("放大：Bilinear"), static_cast<int>(ThreeFpScalingAlgorithm::Bilinear));
    scaler->addItem(QStringLiteral("放大：Bicubic"), static_cast<int>(ThreeFpScalingAlgorithm::Bicubic));
    scaler->addItem(QStringLiteral("放大：Lanczos 3"), static_cast<int>(ThreeFpScalingAlgorithm::Lanczos3));
    scaler->addItem(QStringLiteral("放大：Jinc 2"), static_cast<int>(ThreeFpScalingAlgorithm::Jinc2));
    scaler->setToolTip(QStringLiteral("仅超过源像素密度后使用所选算法；缩小固定使用 Lanczos 3。"));
    controlRow->addWidget(mode);
    controlRow->addStretch();
    controlRow->addWidget(previous);
    controlRow->addWidget(playButton_);
    controlRow->addWidget(next);
    controlRow->addStretch();
    controlRow->addWidget(scaler);

    layout->addLayout(timelineRow);
    layout->addLayout(controlRow);

    connect(timeline_, &QSlider::sliderPressed, this, [this] { timelinePressed_ = true; });
    connect(timeline_, &QSlider::sliderMoved, this, [this](int value) {
        const auto snap = sourcePlayer_ ? sourcePlayer_->snapshot() : ThreeFpSnapshot{};
        if (snap.duration100ns > 0)
            positionLabel_->setText(formatTime(snap.duration100ns * value / timeline_->maximum()));
        seekTimeline(value);
    });
    connect(timeline_, &QSlider::sliderReleased, this, [this] {
        timelinePressed_ = false;
        seekTimeline(timeline_->value());
    });
    connect(previous, &QPushButton::clicked, this, [this] {
        if (sourcePlayer_) sourcePlayer_->stepFrame(-1);
        playing_ = false;
    });
    connect(next, &QPushButton::clicked, this, [this] {
        if (sourcePlayer_) sourcePlayer_->stepFrame(1);
        playing_ = false;
    });
    connect(scaler, &QComboBox::currentIndexChanged, this, [this, scaler] {
        const auto upscale = static_cast<ThreeFpScalingAlgorithm>(scaler->currentData().toInt());
        const bool left = sourcePlayer_->setScalingAlgorithms(upscale, ThreeFpScalingAlgorithm::Lanczos3);
        const bool right = processedPlayer_->setScalingAlgorithms(upscale, ThreeFpScalingAlgorithm::Lanczos3);
        setStatus(left && right
            ? QStringLiteral("放大算法已切换为 %1；缩小保持 Lanczos 3。")
                .arg(scaler->currentText().section(QStringLiteral("："), 1))
            : QStringLiteral("3FP 缩放算法切换失败。"), !(left && right));
    });
    return transport;
}

void MainWindow::connectPlayback()
{
    connect(playButton_, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(vrrPresent_, &QCheckBox::toggled, this, [this](bool enabled) {
        const bool left = sourcePlayer_->setVrrPresent(enabled);
        const bool right = processedPlayer_->setVrrPresent(enabled);
        setStatus(enabled && !(left && right) ? QStringLiteral("显示链不支持 tearing，已保持 VSync。")
                                             : QStringLiteral("VRR low-latency present 已更新。"));
    });
    connect(vrrPacing_, &QCheckBox::toggled, this, [this](bool enabled) {
        const bool left = sourcePlayer_->setVrrPacing(enabled);
        const bool right = processedPlayer_->setVrrPacing(enabled);
        setStatus(enabled && !(left && right) ? QStringLiteral("当前 3FP 不支持 VRR Pacing。")
                                             : QStringLiteral("VRR Pacing 已更新。"));
    });

    const auto wireView = [this](PreviewPane *from, PreviewPane *other, ThreeFpPlayer *first, ThreeFpPlayer *second) {
        connect(from, &PreviewPane::viewChanged, this, [this, other, first, second](float zoom, float x, float y) {
            first->setView(zoom, x, y);
            if (syncView_->isChecked()) {
                other->adoptView(zoom, x, y);
                second->setView(zoom, x, y);
            }
        });
    };
    wireView(sourcePane_, processedPane_, sourcePlayer_.get(), processedPlayer_.get());
    wireView(processedPane_, sourcePane_, processedPlayer_.get(), sourcePlayer_.get());
    connect(sourcePane_, &PreviewPane::redrawRequested, sourcePlayer_.get(), &ThreeFpPlayer::redraw);
    connect(processedPane_, &PreviewPane::redrawRequested, processedPlayer_.get(), &ThreeFpPlayer::redraw);
    connect(sourcePane_, &PreviewPane::pixelHovered, this, [this](int x, int y) { updatePixel(sourcePlayer_.get(), sourcePane_, x, y); });
    connect(processedPane_, &PreviewPane::pixelHovered, this, [this](int x, int y) { updatePixel(processedPlayer_.get(), processedPane_, x, y); });
    connect(sourcePane_, &PreviewPane::activated, this, [this] { sourcePane_->setActive(true); processedPane_->setActive(false); });
    connect(processedPane_, &PreviewPane::activated, this, [this] { sourcePane_->setActive(false); processedPane_->setActive(true); });
    connect(sourcePlayer_.get(), &ThreeFpPlayer::errorOccurred, this, [this](const QString &e) { setStatus(e, true); });
    connect(processedPlayer_.get(), &ThreeFpPlayer::errorOccurred, this, [this](const QString &e) { setStatus(e, true); });
}

void MainWindow::populateCatalog()
{
    for (const auto &definition : FilterCatalog::all()) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  ·  %2").arg(definition.name, definition.category), catalogList_);
        item->setData(Qt::UserRole, definition.id);
        item->setToolTip(QStringLiteral("%1\n依赖 namespace: %2").arg(definition.description, definition.pluginNamespace));
    }
    if (catalogList_->count() > 0)
        catalogList_->setCurrentRow(0);
}

void MainWindow::refreshPipeline(int selectRow)
{
    QSignalBlocker blocker(pipelineList_);
    pipelineList_->clear();
    for (const auto &node : graph_.nodes()) {
        const auto *definition = FilterCatalog::find(node.definitionId);
        auto *item = new QListWidgetItem(definition ? definition->name : node.definitionId, pipelineList_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(node.enabled ? Qt::Checked : Qt::Unchecked);
    }
    pipelineList_->setCurrentRow(selectRow);
    selectPipelineRow(selectRow);
}

void MainWindow::selectPipelineRow(int row)
{
    const auto *node = graph_.at(row);
    parameterEditor_->setNode(node ? FilterCatalog::find(node->definitionId) : nullptr, node);
}

void MainWindow::openSource()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开源视频"), {},
        QStringLiteral("视频与图像 (*.mkv *.mp4 *.m2ts *.ts *.mov *.avi *.webm *.png *.jpg *.tif);;所有文件 (*.*)"));
    if (!path.isEmpty())
        loadSource(path);
}

bool MainWindow::loadSource(const QString &path)
{
    sourcePath_->setText(QDir::toNativeSeparators(path));
    sourcePath_->setToolTip(path);
    if (!api_.available()) {
        setStatus(api_.errorString(), true);
        return false;
    }
    const bool left = sourcePlayer_->openFile(path);
    sourcePane_->setSurfaceActive(left);
    processedPane_->setSurfaceActive(false);
    processedPane_->setPlaceholderText(left
        ? QStringLiteral("源已打开\n点击“渲染预览”生成右侧画面")
        : QStringLiteral("源视频打开失败"));
    processedPane_->setBadge(QStringLiteral("VS · 待渲染"));
    timelineSeekPending_ = false;
    pendingTimelineValue_ = -1;
    vsScriptReady_ = false;
    sourceTotalFrames_ = 0;
    sourceFpsNumerator_ = 0;
    sourceFpsDenominator_ = 0;
    vsTotalFrames_ = 0;
    vsFpsNumerator_ = 0;
    vsFpsDenominator_ = 0;
    requestedVsFrame_ = -1;
    lastVsFrame_ = -1;
    setStatus(left ? QStringLiteral("源已打开；点击“渲染预览”生成右侧 VS 输出。")
                   : QStringLiteral("3FP 打开源失败。"), !left);
    return left;
}

void MainWindow::showScript()
{
    const auto filter = static_cast<SourceFilter>(sourceFilter_->currentData().toInt());
    const auto result = VpyScriptBuilder::build(sourcePath_->text(), filter, graph_);
    if (!result.errors.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法生成 VPY"), result.errors.join('\n'));
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("生成的 VapourSynth 脚本"));
    dialog.resize(860, 640);
    auto *layout = new QVBoxLayout(&dialog);
    auto *editor = new QPlainTextEdit(result.script, &dialog);
    editor->setReadOnly(true);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    auto *close = compactButton(QStringLiteral("关闭"), &dialog);
    layout->addWidget(new QLabel(QStringLiteral("依赖 namespace：%1").arg(result.requiredNamespaces.join(", ")), &dialog));
    layout->addWidget(editor, 1);
    layout->addWidget(close, 0, Qt::AlignRight);
    connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::validateScript()
{
    QString error;
    const QString path = writePreviewScript(&error);
    if (path.isEmpty()) {
        setStatus(error, true);
        return;
    }
    if (!frameServer_->available()) {
        setStatus(frameServer_->errorString(), true);
        return;
    }
    const auto filter = static_cast<SourceFilter>(sourceFilter_->currentData().toInt());
    const auto result = VpyScriptBuilder::build(sourcePath_->text(), filter, graph_);
    vsScriptReady_ = false;
    sourceTotalFrames_ = 0;
    sourceFpsNumerator_ = 0;
    sourceFpsDenominator_ = 0;
    vsTotalFrames_ = 0;
    vsFpsNumerator_ = 0;
    vsFpsDenominator_ = 0;
    requestedVsFrame_ = -1;
    lastVsFrame_ = -1;
    processedPane_->setSurfaceActive(false);
    processedPane_->setPlaceholderText(QStringLiteral("正在加载 VPY 处理链…"));
    processedPane_->setBadge(QStringLiteral("VS · 加载中"));
    setStatus(QStringLiteral("正在通过 VSScript 载入处理链…"));
    frameServer_->loadScript(result.script, path);
}

void MainWindow::togglePlayback()
{
    if (!sourcePlayer_)
        return;
    const bool wasPlaying = sourcePlayer_->snapshot().state == ThreeFpState::Playing;
    playing_ = wasPlaying ? !sourcePlayer_->pause() : sourcePlayer_->play();
    playButton_->setText(playing_ ? QStringLiteral("暂停") : QStringLiteral("播放"));
}

void MainWindow::seekTimeline(int sliderValue)
{
    pendingTimelineValue_ = sliderValue;
    if (!sourcePlayer_ || timelineSeekPending_ || timeline_->maximum() <= 0)
        return;

    const auto snap = sourcePlayer_->snapshot();
    if (snap.duration100ns <= 0)
        return;

    const int value = std::exchange(pendingTimelineValue_, -1);
    const auto position = snap.duration100ns * value / timeline_->maximum();
    requestedVsFrame_ = -1;
    processedPane_->setBadge(QStringLiteral("VS · 同步中"));
    timelineSeekGeneration_ = snap.timelineGeneration;
    timelineSeekPending_ = sourcePlayer_->seek(position);
}

void MainWindow::requestProcessedFrame(int frameIndex)
{
    if (!vsScriptReady_ || frameIndex < 0)
        return;
    requestedVsFrame_ = vsTotalFrames_ > 0 ? std::min(frameIndex, vsTotalFrames_ - 1) : frameIndex;
    processedPane_->setBadge(QStringLiteral("VS · 同步中 · 目标帧 %1").arg(requestedVsFrame_));
    frameServer_->requestFrame(requestedVsFrame_);
}

QString MainWindow::writePreviewScript(QString *error) const
{
    const auto filter = static_cast<SourceFilter>(sourceFilter_->currentData().toInt());
    const auto result = VpyScriptBuilder::build(sourcePath_->text(), filter, graph_);
    if (!result.errors.isEmpty()) {
        if (error) *error = result.errors.join('\n');
        return {};
    }
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!QDir().mkpath(directory)) {
        if (error) *error = QStringLiteral("无法创建脚本缓存目录：%1").arg(directory);
        return {};
    }
    const QString path = QDir(directory).filePath(QStringLiteral("preview.vpy"));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(result.script.toUtf8()) < 0 || !file.commit()) {
        if (error) *error = QStringLiteral("无法写入预览脚本：%1").arg(file.errorString());
        return {};
    }
    return path;
}

void MainWindow::saveProject()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存 VS Renderer 项目"), {},
                                                      QStringLiteral("VS Renderer Project (*.vsr.json)"));
    if (path.isEmpty())
        return;
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("sourcePath"), sourcePath_->text());
    root.insert(QStringLiteral("sourceFilter"), sourceFilter_->currentData().toInt());
    QJsonArray nodes;
    for (const auto &node : graph_.nodes()) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), node.definitionId);
        object.insert(QStringLiteral("enabled"), node.enabled);
        object.insert(QStringLiteral("parameters"), QJsonObject::fromVariantMap(node.parameters));
        nodes.append(object);
    }
    root.insert(QStringLiteral("nodes"), nodes);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        setStatus(QStringLiteral("项目保存失败：%1").arg(file.errorString()), true);
        return;
    }
    setStatus(QStringLiteral("项目已保存：%1").arg(QFileInfo(path).fileName()));
}

void MainWindow::openProject()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开 VS Renderer 项目"), {},
                                                      QStringLiteral("VS Renderer Project (*.vsr.json)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus(QStringLiteral("项目读取失败：%1").arg(file.errorString()), true);
        return;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setStatus(QStringLiteral("项目 JSON 无效：%1").arg(parseError.errorString()), true);
        return;
    }
    const auto root = document.object();
    FilterGraph loaded;
    for (const auto value : root.value(QStringLiteral("nodes")).toArray()) {
        const auto object = value.toObject();
        const int row = loaded.add(object.value(QStringLiteral("id")).toString());
        if (row < 0)
            continue;
        loaded.setEnabled(row, object.value(QStringLiteral("enabled")).toBool(true));
        const auto parameters = object.value(QStringLiteral("parameters")).toObject().toVariantMap();
        for (auto it = parameters.cbegin(); it != parameters.cend(); ++it)
            loaded.setParameter(row, it.key(), it.value());
    }
    graph_ = std::move(loaded);
    sourceFilter_->setCurrentIndex(sourceFilter_->findData(root.value(QStringLiteral("sourceFilter")).toInt()));
    refreshPipeline(graph_.nodes().isEmpty() ? -1 : 0);
    const QString source = root.value(QStringLiteral("sourcePath")).toString();
    if (!source.isEmpty() && QFileInfo::exists(source))
        loadSource(source);
    else
        sourcePath_->setText(source);
    setStatus(QStringLiteral("项目已加载：%1").arg(QFileInfo(path).fileName()));
}

void MainWindow::updatePlaybackState()
{
    if (!sourcePlayer_)
        return;
    const auto snap = sourcePlayer_->snapshot();
    if (snap.duration100ns > 0) {
        positionLabel_->setText(formatTime(snap.position100ns));
        durationLabel_->setText(formatTime(snap.duration100ns));
        if (!timelinePressed_ && !timelineSeekPending_ && pendingTimelineValue_ < 0) {
            const QSignalBlocker blocker(timeline_);
            timeline_->setValue(static_cast<int>(snap.position100ns * timeline_->maximum() / snap.duration100ns));
        }
    }
    if (timelineSeekPending_ && snap.timelineGeneration != timelineSeekGeneration_) {
        timelineSeekPending_ = false;
        if (pendingTimelineValue_ >= 0)
            seekTimeline(pendingTimelineValue_);
    }
    playing_ = snap.state == ThreeFpState::Playing;
    playButton_->setText(playing_ ? QStringLiteral("暂停") : QStringLiteral("播放"));
    const auto sourceFrame = frameAtPosition100ns(
        snap.position100ns, sourceTotalFrames_, sourceFpsNumerator_, sourceFpsDenominator_);
    const auto vsTarget = frameAtPosition100ns(
        snap.position100ns, vsTotalFrames_, vsFpsNumerator_, vsFpsDenominator_);
    if (!timelineSeekPending_ && snap.frameIndex >= 0 && vsTarget >= 0 && vsTarget != lastVsFrame_)
        requestProcessedFrame(static_cast<int>(vsTarget));
    frameStatus_->setText(QStringLiteral("源帧 %1  |  VS 帧 %2  |  %3×%4  |  %5-bit  |  3FP API %6")
        .arg(sourceFrame >= 0 ? QString::number(sourceFrame) : QStringLiteral("--（待 VPY 定位）"))
        .arg(lastVsFrame_ >= 0 ? QString::number(lastVsFrame_) : QStringLiteral("--"))
        .arg(snap.videoWidth).arg(snap.videoHeight).arg(snap.videoOutputBitDepth).arg(api_.apiVersion()));
}

void MainWindow::updatePixel(ThreeFpPlayer *player, PreviewPane *pane, int x, int y)
{
    ThreeFpPixelProbe sample{};
    if (!player->samplePixel(x, y, sample)) {
        pane->setPixelText({});
        return;
    }
    const auto maxCode = (1u << std::min(sample.outputBitDepth, 16u)) - 1u;
    pane->setPixelText(QStringLiteral("(%1, %2)  RGB %3 %4 %5  |  %6 %7 %8")
        .arg(x).arg(y).arg(qRound(sample.red * maxCode)).arg(qRound(sample.green * maxCode)).arg(qRound(sample.blue * maxCode))
        .arg(sample.red, 0, 'f', 4).arg(sample.green, 0, 'f', 4).arg(sample.blue, 0, 'f', 4));
}

void MainWindow::setStatus(const QString &text, bool error)
{
    if (!taskStatus_)
        return;
    taskStatus_->setText(text);
    taskStatus_->setStyleSheet(error ? QStringLiteral("color:#b10e1e;") : QString());
}

}
