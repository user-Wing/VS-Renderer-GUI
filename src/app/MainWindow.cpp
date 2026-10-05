#include "app/MainWindow.h"

#include "backend/FrameTimeline.h"
#include "backend/StartupWarmup.h"
#include "ui/ExportWindow.h"
#include "ui/AnalysisPage.h"
#include "bluray/BlurayWidget.h"
#include <QProcess>
#include "graph/PresetStore.h"
#include <QStackedWidget>
#include <QToolButton>
#include <QStyle>
#include <QProgressBar>
#include <QSettings>
#include "update/PortableUpdater.h"
#include <QCloseEvent>
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "graph/FilterCatalog.h"
#include "graph/VpyScriptBuilder.h"
#include "ui/CollapsibleSection.h"
#include "ui/CompareView.h"
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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyleOptionToolButton>

#include <algorithm>
#include <utility>

namespace vsr {
namespace {

class NavigationButton final : public QToolButton {
public:
    using QToolButton::QToolButton;
protected:
    void paintEvent(QPaintEvent *event) override {
        if (toolButtonStyle() == Qt::ToolButtonIconOnly) { QToolButton::paintEvent(event); return; }
        QStyleOptionToolButton option; initStyleOption(&option);
        option.text.clear(); option.icon = QIcon();
        QPainter painter(this);
        style()->drawComplexControl(QStyle::CC_ToolButton, &option, &painter, this);
        icon().paint(&painter, QRect(10, (height() - 16) / 2, 16, 16));
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(QRect(38, 0, width() - 48, height()), Qt::AlignLeft | Qt::AlignVCenter, text());
    }
};

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
    setAcceptDrops(true);
    resize(1800, 900);
    setMinimumSize(1120, 700);
    buildToolbar();

    auto *root = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *content = new QSplitter(Qt::Horizontal, root);
    content->setHandleWidth(1);
    content->addWidget(buildSidebar());
    content->addWidget(buildParameterPanel());
    content->addWidget(buildWorkspace());
    content->setSizes({280, 320, 1156});
    content->setStretchFactor(0, 0);
    content->setStretchFactor(1, 0);
    content->setStretchFactor(2, 1);
    rootLayout->addWidget(content, 1);
    auto *shell = new QWidget(this);
    auto *shellLayout = new QHBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);
    navigation_ = new QWidget(shell);
    navigation_->setObjectName(QStringLiteral("pageNavigation"));
    navigation_->setFixedWidth(44);
    auto *navigationLayout = new QVBoxLayout(navigation_);
    navigationLayout->setContentsMargins(2, 4, 2, 4);
    for (int i = 0; i < 2; ++i) {
        auto *button = new NavigationButton(navigation_);
        button->setText(i == 0 ? QStringLiteral("VS 实时渲染") : QStringLiteral("图像分析比对"));
        button->setToolTip(button->text());
        button->setIcon(style()->standardIcon(i == 0 ? QStyle::SP_ComputerIcon : QStyle::SP_FileDialogContentsView));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setCheckable(true);
        button->setChecked(i == 0);
        button->setMinimumHeight(36);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        navigationButtons_.append(button);
        navigationLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, i] { selectPage(i); });
    }
    navigationLayout->addStretch();
    auto *settings = new NavigationButton(navigation_);
    settings->setObjectName(QStringLiteral("applicationSettings"));
    settings->setText(QStringLiteral("设置"));
    settings->setToolTip(settings->text());
    settings->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    settings->setToolButtonStyle(Qt::ToolButtonIconOnly);
    settings->setCheckable(true);
    settings->setMinimumHeight(36);
    settings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    navigationButtons_.append(settings);
    navigationLayout->addWidget(settings);
    connect(settings, &QToolButton::clicked, this, &MainWindow::showSettings);
    pages_ = new QStackedWidget(shell);
    pages_->addWidget(root);
    pages_->addWidget(new QWidget(pages_)); // Reserve the lazily initialized analysis page.
    pages_->addWidget(buildSettingsPage());
    auto *bluray = new BlurayWidget(pages_);
    pages_->addWidget(bluray);
    auto *bdButton = new NavigationButton(navigation_);
    bdButton->setText(QStringLiteral("BD 一键 Remux"));
    bdButton->setToolTip(bdButton->text());
    bdButton->setIcon(style()->standardIcon(QStyle::SP_DriveCDIcon));
    bdButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    bdButton->setCheckable(true);
    bdButton->setMinimumHeight(36);
    navigationButtons_.append(bdButton);
    navigationLayout->insertWidget(2, bdButton);
    connect(bdButton, &QToolButton::clicked, this, [this] { selectPage(3); });
    connect(bluray, &BlurayWidget::playRequested, this, [this](const QVector<BlurayTitle> &titles, int index) {
        QString error;
        const auto playlist = BlurayCatalog::prepare(titles.at(index), &error);
        if (playlist.isEmpty()) { setStatus(error, true); return; }
        if (!QProcess::startDetached(QDir(QCoreApplication::applicationDirPath()).filePath("vs-player.exe"), {playlist}))
            setStatus(QStringLiteral("无法启动 VS Player"), true);
    });
    shellLayout->addWidget(navigation_);
    shellLayout->addWidget(pages_, 1);
    setCentralWidget(shell);

    statusBar()->setFixedHeight(24);
    taskStatus_ = new QLabel(this);
    frameStatus_ = new QLabel(QStringLiteral("帧 --  |  VS --  |  3FP --"), this);
    statusBar()->addWidget(taskStatus_, 1);
    statusBar()->addPermanentWidget(frameStatus_);

    sourcePlayer_ = std::make_unique<ThreeFpPlayer>(api_, sourcePane_->surface(), this);
    processedPlayer_ = std::make_unique<ThreeFpPlayer>(api_, processedPane_->surface(), this);
    frameServer_ = std::make_unique<VapourSynthFrameServer>(this);

    processedPlayer_->setMuted(true);

    runtimeStatus_->setText(QStringLiteral("预热中"));
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
        if (startupWarmup_ && startupWarmup_->ownsScript()) return;
        vsScriptReady_ = true;
        sourceTotalFrames_ = source.totalFrames;
        sourceFpsNumerator_ = source.fpsNumerator;
        sourceFpsDenominator_ = source.fpsDenominator;
        vsTotalFrames_ = processed.totalFrames;
        vsFpsNumerator_ = processed.fpsNumerator;
        vsFpsDenominator_ = processed.fpsDenominator;
        requestedVsFrame_ = -1;
        lastVsFrame_ = -1;
        vsFramePending_ = false;
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
        if (startupWarmup_ && startupWarmup_->ownsScript()) return;
        vsFramePending_ = false;
        if (timelineSeekPending_ || requestedVsFrame_ < 0 || frame.frameIndex != requestedVsFrame_)
            return;
        bool recoveredDevice = false;
        bool submitted = processedPlayer_->submitFrame(frame);
        if (!submitted && processedPlayer_->lastError().contains(QStringLiteral("DeviceFailure"))) {
            processedPane_->setSurfaceActive(true);
            if (processedPlayer_->resetVideoOutput()) {
                processedPlayer_->setMuted(true);
                submitted = processedPlayer_->submitFrame(frame);
                recoveredDevice = submitted;
            }
        }
        if (submitted) {
            lastVsFrame_ = static_cast<int>(frame.frameIndex);
            processedPane_->setSurfaceActive(true);
            processedPane_->setBadge(QStringLiteral("VS · 实时 · 帧 %1").arg(frame.frameIndex));
            if (recoveredDevice)
                setStatus(QStringLiteral("右侧渲染设备已重建，处理链继续实时预览。"));
            const auto source = sourcePlayer_->snapshot();
            const int latest = static_cast<int>(frameAtPosition100ns(
                source.position100ns, vsTotalFrames_, vsFpsNumerator_, vsFpsDenominator_));
            if (!timelineSeekPending_ && latest >= 0 && latest != lastVsFrame_)
                requestProcessedFrame(latest);
        } else {
            processedPane_->setPlaceholderText(processedPlayer_->lastError());
            processedPane_->setSurfaceActive(false);
        }
    });
    connect(frameServer_.get(), &VapourSynthFrameServer::errorOccurred, this,
            [this](const QString &message) {
        if (startupWarmup_ && startupWarmup_->ownsScript()) return;
        vsScriptReady_ = false;
        vsFramePending_ = false;
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
    startupWarmup_ = std::make_unique<StartupWarmup>(sourcePlayer_.get(), processedPlayer_.get(), frameServer_.get(), this);
    auto *warmupProgress = new QProgressBar(this);
    warmupProgress->setRange(0, 100);
    warmupProgress->setFixedWidth(180);
    statusBar()->addPermanentWidget(warmupProgress);
    connect(startupWarmup_.get(), &StartupWarmup::progress, this, [this, warmupProgress](int value, const QString &stage) {
        warmupProgress->setValue(value);
        warmupProgress->setToolTip(stage);
        runtimeStatus_->setText(QStringLiteral("预热中"));
        runtimeStatus_->setToolTip(stage);
        if (value == 100) warmupProgress->hide();
    });
    connect(startupWarmup_.get(), &StartupWarmup::finished, this, [this](bool success, const QString &message) {
        runtimeStatus_->setText(success ? QStringLiteral("已预热") : QStringLiteral("预热中"));
        runtimeStatus_->setToolTip(message);
        primedProcessedOutput_ = success;
        sourcePane_->setSurfaceActive(false);
        processedPane_->setSurfaceActive(false);
        sourcePane_->setBadge(QStringLiteral("Fit"));
        processedPane_->setBadge(QStringLiteral("VS · 待渲染"));
        setStatus(message, !success);
        if (!deferredSource_.isEmpty()) loadSource(std::exchange(deferredSource_, {}));
    });
    QTimer::singleShot(0, this, [this] {
        sourcePane_->setSurfaceActive(true);
        processedPane_->setSurfaceActive(true);
        sourcePane_->setBadge(QStringLiteral("启动预热中"));
        processedPane_->setBadge(QStringLiteral("启动预热中"));
        setStatus(QStringLiteral("正在预热解码、VS 源滤镜和双路渲染…"));
        startupWarmup_->start();
    });
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

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty() &&
        event->mimeData()->urls().first().isLocalFile())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls() || event->mimeData()->urls().isEmpty())
        return;
    if (pages_->currentIndex() == 1) {
        QStringList paths;
        for (const auto &url : event->mimeData()->urls()) if (url.isLocalFile()) paths.append(url.toLocalFile());
        analysisPage_->openFiles(paths);
        event->acceptProposedAction();
        return;
    }
    const QString path = event->mimeData()->urls().first().toLocalFile();
    if (!path.isEmpty() && QFileInfo(path).isFile() && loadSource(path))
        event->acceptProposedAction();
}

void MainWindow::selectPage(int index)
{
    if (index == 1 && !analysisPage_) {
        analysisPage_ = std::make_unique<AnalysisPage>(api_);
        auto *placeholder = pages_->widget(1);
        pages_->removeWidget(placeholder);
        delete placeholder;
        pages_->insertWidget(1, analysisPage_.get());
    }
    if (index != pages_->currentIndex()) {
        if (index != 0 && sourcePlayer_ && sourcePlayer_->snapshot().state == ThreeFpState::Playing)
            sourcePlayer_->pause();
        if (index != 1 && analysisPage_) analysisPage_->pause();
        pages_->setCurrentIndex(index);
    }
    for (int i = 0; i < navigationButtons_.size(); ++i) navigationButtons_[i]->setChecked(i == index);
    for (auto *action : vsActions_) action->setVisible(index == 0);
    frameStatus_->setVisible(index == 0);
    setStatus(index == 0 ? QStringLiteral("VS 实时渲染") : index == 1 ? QStringLiteral("图像分析比对 · 双路直接解码") : index == 3 ? QStringLiteral("BD 一键 Remux") : QStringLiteral("设置"));
}

void MainWindow::showSettings()
{
    selectPage(2);
}

QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("rendererSettingsPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(new QLabel(QStringLiteral("设置"), page));
    auto *label = new QLabel(QStringLiteral("VapourSynth 源滤镜"), page);
    layout->addWidget(label);
    auto *source = new QComboBox(page);
    source->setObjectName(QStringLiteral("rendererSettingsSource"));
    source->addItems({QStringLiteral("L-SMASH Works"), QStringLiteral("FFMS2")});
    source->setCurrentIndex(sourceFilter_->currentIndex());
    connect(sourceFilter_, &QComboBox::currentIndexChanged, source, &QComboBox::setCurrentIndex);
    layout->addWidget(source);
    auto *hint = new QLabel(QStringLiteral("用于 VS 预览与导出。更改后重新生成并验证脚本生效。"), page);
    hint->setWordWrap(true); layout->addWidget(hint);
    auto *apply = new QPushButton(QStringLiteral("应用"), page);
    apply->setObjectName(QStringLiteral("rendererApplySettings"));
    layout->addWidget(apply);
    connect(apply, &QPushButton::clicked, this, [this, source] {
        sourceFilter_->setCurrentIndex(source->currentIndex());
        QSettings().setValue(QStringLiteral("sourceFilter"), source->currentIndex());
        setStatus(QStringLiteral("设置已保存，重新生成并验证脚本后生效。"));
    });
    layout->addWidget(new QLabel(QStringLiteral("当前版本：") + VSR_VERSION, page));
    auto *updates = new QPushButton(QStringLiteral("检查更新"), page);
    layout->addWidget(updates);
    connect(updates, &QPushButton::clicked, page, [page] { PortableUpdater updater(page); updater.check(); updater.exec(); });
    return page;
}

void MainWindow::buildToolbar()
{
    auto *bar = addToolBar(QStringLiteral("命令栏"));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setFixedHeight(44);
    bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto *menu = bar->addAction(QStringLiteral("☰"));
    menu->setObjectName(QStringLiteral("navigationToggle"));
    menu->setToolTip(QStringLiteral("展开 / 收起页面导航"));
    connect(menu, &QAction::triggered, this, [this] {
        const bool expand = navigation_->width() == 44;
        navigation_->setFixedWidth(expand ? 180 : 44);
        for (auto *button : navigationButtons_)
            button->setToolButtonStyle(expand ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    });
    auto *openSourceAction = bar->addAction(QStringLiteral("打开源"));
    auto *openProjectAction = bar->addAction(QStringLiteral("打开项目"));
    auto *saveProjectAction = bar->addAction(QStringLiteral("保存项目"));
    auto *presetsAction = bar->addAction(QStringLiteral("预设管理"));
    presetsAction->setObjectName("presetManagement");
    auto *loadVpyAction = bar->addAction(QStringLiteral("加载 VPY"));
    connect(presetsAction, &QAction::triggered, this, &MainWindow::showPresets);
    connect(loadVpyAction, &QAction::triggered, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("加载 VPY"), PresetStore::directory(), "VapourSynth (*.vpy)");
        if (!path.isEmpty()) loadPreset(path);
    });
    bar->addSeparator();
    auto *viewScriptAction = bar->addAction(QStringLiteral("查看 VPY"));
    auto *validateAction = bar->addAction(QStringLiteral("生成并验证"));
    auto *previewAction = bar->addAction(QStringLiteral("渲染预览"));
    previewAction->setToolTip(QStringLiteral("通过 VSScript 载入脚本，并把当前帧直接提交给 3FP。"));
    auto *exportAction = bar->addAction(QStringLiteral("导出当前处理结果"));
    exportAction->setToolTip(QStringLiteral("用 vspipe 输出当前 VS 处理链，再套用粘贴的 3FUI FFmpeg 参数。"));
    auto *spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    vsActions_ = bar->actions();
    vsActions_.removeFirst();

    connect(openSourceAction, &QAction::triggered, this, &MainWindow::openSource);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(viewScriptAction, &QAction::triggered, this, &MainWindow::showScript);
    connect(validateAction, &QAction::triggered, this, &MainWindow::validateScript);
    connect(previewAction, &QAction::triggered, this, &MainWindow::validateScript);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportCurrentResult);
}

QWidget *MainWindow::buildSidebar()
{
    auto *sidebar = new QWidget(this);
    sidebar->setMinimumWidth(250);
    sidebar->setMaximumWidth(360);
    sidebar->setObjectName(QStringLiteral("processingSidebar"));
    sidebar->setStyleSheet(QStringLiteral("#processingSidebar{background:#ffffff;border-right:1px solid #d1d1d1;}"));
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    auto *input = new QWidget(sidebar);
    auto *inputLayout = new QVBoxLayout(input);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(4);
    sourcePath_ = new QLineEdit(input);
    sourcePath_->setObjectName(QStringLiteral("rendererSourcePath"));
    sourcePath_->setMinimumWidth(0);
    sourcePath_->setReadOnly(true);
    sourcePath_->setAcceptDrops(false);
    sourcePath_->setPlaceholderText(QStringLiteral("尚未选择视频"));
    auto *inputRow = new QHBoxLayout;
    sourceFilter_ = new QComboBox(this);
    sourceFilter_->hide();
    sourceFilter_->setObjectName(QStringLiteral("sourceFilter"));
    sourceFilter_->addItem(QStringLiteral("L-SMASH Works"), static_cast<int>(SourceFilter::Lsmas));
    sourceFilter_->addItem(QStringLiteral("FFMS2"), static_cast<int>(SourceFilter::Ffms2));
    auto *browse = compactButton(QStringLiteral("浏览…"), input);
    browse->setObjectName(QStringLiteral("rendererBrowse"));
    sourceFilter_->setCurrentIndex(QSettings().value(QStringLiteral("sourceFilter"),0).toInt());
    inputRow->addWidget(sourcePath_, 1);
    inputRow->addWidget(browse);
    inputLayout->addLayout(inputRow);
    connect(browse, &QPushButton::clicked, this, &MainWindow::openSource);
    connect(sourceFilter_, &QComboBox::currentIndexChanged, this, [this] { setStatus(QStringLiteral("源滤镜已更改，脚本待验证。")); });

    auto *filters = new QWidget(sidebar);
    auto *filtersLayout = new QVBoxLayout(filters);
    filtersLayout->setContentsMargins(8, 8, 8, 8);
    filtersLayout->setSpacing(4);
    catalogSearch_ = new QLineEdit(filters);
    catalogSearch_->setPlaceholderText(QStringLiteral("搜索滤镜"));
    catalogList_ = new QListWidget(filters);
    catalogList_->setObjectName(QStringLiteral("rendererCatalog"));
    catalogList_->setMinimumHeight(190);
    pipelineList_ = new QListWidget(filters);
    pipelineList_->setMinimumHeight(68);
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
    filtersLayout->addWidget(catalogList_, 3);
    filtersLayout->addWidget(add);
    filtersLayout->addWidget(new QLabel(QStringLiteral("处理链(自上而下执行)"), filters));
    filtersLayout->addWidget(pipelineList_, 1);
    filtersLayout->addLayout(nodeButtons);

    connect(catalogSearch_, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < catalogList_->count(); ++i)
            catalogList_->item(i)->setHidden(!catalogList_->item(i)->data(Qt::UserRole + 1).toString().contains(text, Qt::CaseInsensitive));
    });
    connect(add, &QPushButton::clicked, this, [this] {
        const auto *item = catalogList_->currentItem();
        if (!item)
            return;
        activePreset_.clear();
        const int row = graph_.add(item->data(Qt::UserRole).toString());
        refreshPipeline(row);
        setStatus(QStringLiteral("已添加滤镜，VPY 待验证。"));
    });
    connect(catalogList_, &QListWidget::itemDoubleClicked, add, &QPushButton::click);
    connect(pipelineList_, &QListWidget::currentRowChanged, this, &MainWindow::selectPipelineRow);
    connect(pipelineList_, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        activePreset_.clear();
        graph_.setEnabled(pipelineList_->row(item), item->checkState() == Qt::Checked);
        setStatus(QStringLiteral("节点状态已更改，VPY 待验证。"));
    });
    connect(up, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        activePreset_.clear();
        if (row > 0 && graph_.move(row, row - 1)) refreshPipeline(row - 1);
    });
    connect(down, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        activePreset_.clear();
        if (row >= 0 && row + 1 < graph_.nodes().size() && graph_.move(row, row + 1)) refreshPipeline(row + 1);
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        const int row = pipelineList_->currentRow();
        activePreset_.clear();
        if (graph_.remove(row)) refreshPipeline(std::min(row, static_cast<int>(graph_.nodes().size()) - 1));
    });

    auto *inputSection = new CollapsibleSection(QStringLiteral("输入与环境"), input, sidebar);
    auto *filterSection = new CollapsibleSection(QStringLiteral("滤镜库与处理链"), filters, sidebar);
    layout->addWidget(inputSection);
    layout->addWidget(filterSection, 1);

    const auto keepOneOpen = [inputSection, filterSection](bool) {
        if (!inputSection->isExpanded() && !filterSection->isExpanded())
            filterSection->setExpanded(true);
    };
    connect(inputSection, &CollapsibleSection::expandedChanged, sidebar, keepOneOpen);
    connect(filterSection, &CollapsibleSection::expandedChanged, sidebar, keepOneOpen);
    return sidebar;
}

QWidget *MainWindow::buildParameterPanel()
{
    auto *parameters = new QWidget(this);
    parameters->setObjectName(QStringLiteral("rendererParameterPanel"));
    parameters->setMinimumWidth(260);
    parameters->setStyleSheet(QStringLiteral("#rendererParameterPanel{background:#ffffff;}"));
    auto *parameterLayout = new QVBoxLayout(parameters);
    parameterLayout->setContentsMargins(0, 0, 0, 0);
    parameterLayout->setSpacing(0);
    auto *header = new QLabel(QStringLiteral("参数与脚本"), parameters);
    header->setFixedHeight(32);
    header->setStyleSheet(QStringLiteral("background:#eaeaea;font-weight:600;padding:0 8px;"));
    parameterLayout->addWidget(header);
    parameterEditor_ = new ParameterEditor(parameters);
    auto *parameterScroll = new QScrollArea(parameters);
    parameterScroll->setObjectName(QStringLiteral("rendererParameters"));
    parameterScroll->setWidgetResizable(true);
    parameterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    parameterScroll->setStyleSheet(QStringLiteral("QScrollArea#rendererParameters{background:#ffffff;border:0;}"));
    parameterScroll->viewport()->setStyleSheet(QStringLiteral("background:#ffffff;"));
    parameterScroll->setWidget(parameterEditor_);
    parameterLayout->addWidget(parameterScroll, 1);
    connect(parameterEditor_, &ParameterEditor::parameterChanged, this,
            [this](const QString &id, const QVariant &value) {
        activePreset_.clear();
        if (id == "mode" && value.toString() != QStringLiteral("自定义 GLSL")) graph_.setParameter(pipelineList_->currentRow(), "shader", "");
        if (id == "shader" && !value.toString().isEmpty()) graph_.setParameter(pipelineList_->currentRow(), "mode", QStringLiteral("自定义 GLSL"));
        if (graph_.setParameter(pipelineList_->currentRow(), id, value))
            setStatus(QStringLiteral("参数已更新，VPY 待验证。"));
    });

    return parameters;
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
    runtimeStatus_ = new QLabel(QStringLiteral("预热中"), compareBar);
    runtimeStatus_->setObjectName(QStringLiteral("rendererWarmupState"));
    runtimeStatus_->setStyleSheet(QStringLiteral("color:#5c5c5c;border:0;"));
    compareLayout->addWidget(runtimeStatus_);
    compareLayout->addStretch();
    syncView_ = new QCheckBox(QStringLiteral("同步视图"), compareBar);
    syncView_->setChecked(true);
    vrrPresent_ = new QCheckBox(QStringLiteral("VRR 低延迟(实验)"), compareBar);
    vrrPacing_ = new QCheckBox(QStringLiteral("VRR Pacing(实验)"), compareBar);
    vrrPresent_->setProperty("vrrToggle", true); vrrPacing_->setProperty("vrrToggle", true);
    vrrPresent_->setToolTip(QStringLiteral("调用 FFF3FP_SetPresentConfig；不支持时保持 VSync。"));
    vrrPacing_->setToolTip(QStringLiteral("调用 FFF3FP_SetPacingConfig；建议与 VRR 低延迟配合。"));
    compareLayout->addWidget(syncView_);
    compareLayout->addWidget(vrrPresent_);
    compareLayout->addWidget(vrrPacing_);

    compareView_ = new CompareView(workspace);
    sourcePane_ = compareView_->sourcePane();
    processedPane_ = compareView_->processedPane();
    sourcePane_->setActive(true);

    layout->addWidget(compareBar);
    layout->addWidget(compareView_, 1);
    layout->addWidget(buildTransport());
    return workspace;
}

QWidget *MainWindow::buildTransport()
{
    auto *transport = new QWidget(this);
    transport->setObjectName(QStringLiteral("rendererTransport"));
    transport->setFixedHeight(76);
    transport->setStyleSheet(QStringLiteral("background:#f9f9f9;border-top:1px solid #d1d1d1;"));
    auto *layout = new QVBoxLayout(transport);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(2);

    auto *timelineRow = new QHBoxLayout;
    positionLabel_ = new QLabel(QStringLiteral("00:00:00.000"), transport);
    positionLabel_->setObjectName(QStringLiteral("rendererPosition"));
    durationLabel_ = new QLabel(QStringLiteral("00:00:00.000"), transport);
    positionLabel_->setFixedWidth(92);
    durationLabel_->setFixedWidth(92);
    durationLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeline_ = new QSlider(Qt::Horizontal, transport);
    timeline_->setObjectName(QStringLiteral("rendererTimeline"));
    timeline_->setRange(0, 100000);
    timelineRow->addWidget(positionLabel_);
    timelineRow->addStretch();
    timelineRow->addWidget(durationLabel_);

    auto *controlRow = new QGridLayout;
    controlRow->setContentsMargins(0, 0, 0, 0);
    controlRow->setColumnStretch(0, 1);
    controlRow->setColumnStretch(2, 1);
    auto *mode = new QComboBox(transport);
    mode->setObjectName(QStringLiteral("rendererCompareMode"));
    mode->addItems({QStringLiteral("并排"), QStringLiteral("A/B 滑块")});
    auto *previous = compactButton(QStringLiteral("◀ 帧"), transport);
    playButton_ = compactButton(QStringLiteral("播放"), transport);
    auto *next = compactButton(QStringLiteral("帧 ▶"), transport);
    auto *scaler = new QComboBox(transport);
    scaler->addItem(QStringLiteral("放大：Nearest"), static_cast<int>(ThreeFpScalingAlgorithm::Nearest));
    scaler->addItem(QStringLiteral("放大：Bilinear"), static_cast<int>(ThreeFpScalingAlgorithm::Bilinear));
    scaler->addItem(QStringLiteral("放大：Bicubic"), static_cast<int>(ThreeFpScalingAlgorithm::Bicubic));
    scaler->addItem(QStringLiteral("放大：Lanczos 3"), static_cast<int>(ThreeFpScalingAlgorithm::Lanczos3));
    scaler->addItem(QStringLiteral("放大：Jinc 2"), static_cast<int>(ThreeFpScalingAlgorithm::Jinc2));
    scaler->addItem(QStringLiteral("放大：Spline36"), static_cast<int>(ThreeFpScalingAlgorithm::Spline36));
    scaler->addItem(QStringLiteral("放大：Super-XBR(单阶段)"), static_cast<int>(ThreeFpScalingAlgorithm::SuperXbrSinglePass));
    scaler->addItem(QStringLiteral("放大：Lanczos 4"), static_cast<int>(ThreeFpScalingAlgorithm::Lanczos4));
    scaler->setToolTip(QStringLiteral("仅超过源像素密度后使用所选算法；缩小固定使用 Lanczos 3。"));
    auto *frameControls = new QWidget(transport);
    frameControls->setObjectName(QStringLiteral("rendererFrameControls"));
    auto *frames = new QHBoxLayout(frameControls);
    frames->setContentsMargins(0, 0, 0, 0);
    frames->addWidget(previous); frames->addWidget(playButton_); frames->addWidget(next);
    controlRow->addWidget(mode, 0, 0, Qt::AlignLeft);
    controlRow->addWidget(frameControls, 0, 1);
    controlRow->addWidget(scaler, 0, 2, Qt::AlignRight);

    layout->addWidget(timeline_);
    layout->addLayout(timelineRow);
    layout->addLayout(controlRow);

    connect(mode, &QComboBox::currentIndexChanged, this, [this](int index) {
        compareView_->setMode(index == 1 ? CompareMode::Slider : CompareMode::SideBySide);
        QTimer::singleShot(40, this, [this] {
            sourcePlayer_->redraw();
            processedPlayer_->redraw();
        });
        setStatus(index == 1 ? QStringLiteral("A/B 滑块：左侧源视频，右侧处理后；拖动蓝色中线调整位置。")
                             : QStringLiteral("已切换为并排比较。"));
    });

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
    int textWidth = 0;
    for (const auto &definition : FilterCatalog::all()) {
        auto *item = new QListWidgetItem(definition.name, catalogList_);
        textWidth = std::max(textWidth, catalogList_->fontMetrics().horizontalAdvance(definition.name));
        item->setData(Qt::UserRole, definition.id);
        item->setData(Qt::UserRole + 1, definition.name + ' ' + definition.category);
        item->setToolTip(QStringLiteral("%1 · %2\n%3\n依赖 namespace: %4").arg(definition.name, definition.category, definition.description, definition.pluginNamespace));
    }
    auto *sidebar = findChild<QWidget *>(QStringLiteral("processingSidebar"));
    sidebar->setMinimumWidth(std::max(250, textWidth + 44));
    sidebar->setMaximumWidth(std::max(360, sidebar->minimumWidth()));
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
    updateParameterInputSize();
}

void MainWindow::updateParameterInputSize()
{
    if(!sourcePlayer_ || !parameterEditor_)return;
    const auto source=sourcePlayer_->snapshot();QSize size(source.videoWidth,source.videoHeight);
    if(size.isEmpty())return;
    for(int i=0;i<pipelineList_->currentRow();++i){
        if(size.isEmpty())return;
        const auto *node=graph_.at(i);if(!node || !node->enabled)continue;const auto &p=node->parameters;
        if(node->definitionId=="crop")size-=QSize(p.value("left").toInt()+p.value("right").toInt(),p.value("top").toInt()+p.value("bottom").toInt());
        else if(node->definitionId=="transpose")size.transpose();
        else if(node->definitionId=="resize" || node->definitionId=="descale")size=QSize(p.value("width").toInt(),p.value("height").toInt());
        else if(node->definitionId=="anime4k"){
            if(p.value("output_mode").toString()!="指定分辨率"){const auto scale=p.value("scale").toString().section(QChar(0x00d7),0,0).toDouble();size=QSize(qMax(2,int(size.width()*scale)),qMax(2,int(size.height()*scale)));}
            else if(!p.value("keep_aspect",true).toBool())size=QSize(p.value("width",1920).toInt(),p.value("height",1080).toInt());
            else if(p.value("dimension_axis","width")=="height")size=QSize(qMax(2,qRound(double(p.value("height",1080).toInt())*size.width()/size.height())),p.value("height",1080).toInt());
            else size=QSize(p.value("width",1920).toInt(),qMax(2,qRound(double(p.value("width",1920).toInt())*size.height()/size.width())));
        }
    }
    parameterEditor_->setInputSize(size);
}

void MainWindow::openSource()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开源视频"), {},
        QStringLiteral("视频与图像 (*.mkv *.mp4 *.m2ts *.ts *.mov *.avi *.webm *.vpy *.png *.jpg *.tif);;所有文件 (*.*)"));
    if (!path.isEmpty())
        loadSource(path);
}

bool MainWindow::loadSource(const QString &path)
{
    if (QFileInfo(path).suffix().compare("vpy", Qt::CaseInsensitive) == 0) { loadPreset(path); return true; }
    if (!QFileInfo(path).isFile()) {
        setStatus(QStringLiteral("源文件不存在：%1").arg(path), true);
        return false;
    }
    if (startupWarmup_ && startupWarmup_->isRunning()) {
        deferredSource_ = path;
        setStatus(QStringLiteral("已接收视频，启动预热结束后自动加载。"));
        return true;
    }
    sourcePath_->setText(QDir::toNativeSeparators(path));
    sourcePath_->setToolTip(path);
    if (exportWindow_) exportWindow_->setCurrentSource(path);
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
    vsFramePending_ = false;
    sourcePrimePending_ = left;
    sourcePrimeStarted_ = false;
    setStatus(left ? QStringLiteral("源已打开；正在预取首帧，点击“渲染预览”可生成右侧 VS 输出。")
                   : QStringLiteral("3FP 打开源失败。"), !left);
    if (left && !activePreset_.isEmpty()) QTimer::singleShot(0, this, &MainWindow::validateScript);
    return left;
}

void MainWindow::showScript()
{
    const auto result = currentScript();
    if (!result.errors.isEmpty()) {
        QMessageBox warning(QMessageBox::Warning, QStringLiteral("无法生成 VPY"), result.errors.join('\n'), QMessageBox::Ok, this);
        warning.setObjectName(QStringLiteral("rendererVpyWarning"));
        warning.setStyleSheet(QStringLiteral("QLabel#qt_msgbox_label{min-width:480px;}"));
        warning.exec();
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
    if (startupWarmup_ && startupWarmup_->isRunning()) {
        setStatus(QStringLiteral("启动链路正在预热，完成后可渲染预览。"));
        return;
    }
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
    const auto result = currentScript();
    vsScriptReady_ = false;
    sourceTotalFrames_ = 0;
    sourceFpsNumerator_ = 0;
    sourceFpsDenominator_ = 0;
    vsTotalFrames_ = 0;
    vsFpsNumerator_ = 0;
    vsFpsDenominator_ = 0;
    requestedVsFrame_ = -1;
    lastVsFrame_ = -1;
    vsFramePending_ = false;
    processedPane_->setSurfaceActive(true);
    processedPane_->setBadge(QStringLiteral("VS · 加载中"));
    setStatus(QStringLiteral("正在通过 VSScript 载入处理链…"));
    if (!std::exchange(primedProcessedOutput_, false) && !processedPlayer_->resetVideoOutput()) {
        processedPane_->setSurfaceActive(false);
        processedPane_->setPlaceholderText(processedPlayer_->lastError());
        setStatus(processedPlayer_->lastError(), true);
        return;
    }
    processedPlayer_->setMuted(true);
    if (startupWarmup_) startupWarmup_->releaseScript();
    frameServer_->loadScript(result.script, path);
}

void MainWindow::exportCurrentResult()
{
    if (!exportWindow_) {
        exportWindow_ = std::make_unique<ExportWindow>();
        connect(exportWindow_.get(), &ExportWindow::statusMessage, this, [this](const QString &message) {
            setStatus(message);
        });
        exportWindow_->setScriptBuilder([this](const QString &source) {
            return currentScript(source);
        });
    }
    exportWindow_->setCurrentSource(sourcePath_->text());
    exportWindow_->showNormal();
    exportWindow_->raise();
    exportWindow_->activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (exportWindow_ && exportWindow_->isBusy()) {
        event->ignore();
        connect(exportWindow_.get(), &ExportWindow::idle, this, &QWidget::close, Qt::UniqueConnection);
        exportWindow_->stopAll();
        setStatus(QStringLiteral("正在停止编码并保留输出文件，完成后退出。"));
        return;
    }
    if (analysisPage_ && analysisPage_->exportBusy()) {
        event->ignore();
        connect(analysisPage_.get(), &AnalysisPage::exportIdle, this, &QWidget::close, Qt::UniqueConnection);
        analysisPage_->stopExport();
        setStatus(QStringLiteral("正在停止对比编码并保留输出文件，完成后退出。"));
        return;
    }
    if (exportWindow_) exportWindow_->close();
    QMainWindow::closeEvent(event);
}

void MainWindow::togglePlayback()
{
    if (pages_->currentIndex() == 1) { analysisPage_->togglePlayback(); return; }
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
    vsFramePending_ = false;
    processedPane_->setBadge(QStringLiteral("VS · 同步中"));
    timelineSeekGeneration_ = snap.timelineGeneration;
    timelineSeekPending_ = sourcePlayer_->seek(position);
}

void MainWindow::requestProcessedFrame(int frameIndex)
{
    if (!vsScriptReady_ || vsFramePending_ || frameIndex < 0)
        return;
    requestedVsFrame_ = vsTotalFrames_ > 0 ? std::min(frameIndex, vsTotalFrames_ - 1) : frameIndex;
    vsFramePending_ = true;
    frameServer_->requestFrame(requestedVsFrame_, lastVsFrame_ < 0 ? 6 : 0);
}

QString MainWindow::writePreviewScript(QString *error) const
{
    const auto result = currentScript();
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
    activePreset_.clear();
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
    if (!sourcePlayer_ || sourcePath_->text().isEmpty() || (startupWarmup_ && startupWarmup_->isRunning()))
        return;
    const auto snap = sourcePlayer_->snapshot();
    if (sourcePrimePending_ && snap.state != ThreeFpState::Opening) {
        updateParameterInputSize();
        if (snap.decodedVideoFrames > 0) {
            sourcePrimePending_ = false;
        } else if (!sourcePrimeStarted_ &&
                   (snap.state == ThreeFpState::Ready || snap.state == ThreeFpState::Paused)) {
            sourcePrimeStarted_ = sourcePlayer_->seekFrame(0);
        }
    }
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
    const bool higherFps = vsFpsNumerator_ > 0 && sourceFpsNumerator_ > 0 &&
        vsFpsNumerator_ * sourceFpsDenominator_ > sourceFpsNumerator_ * vsFpsDenominator_;
    stateTimer_->setInterval(playing_ && higherFps
        ? std::clamp<int>(int(500 * vsFpsDenominator_ / vsFpsNumerator_), 4, 33) : 33);
    playButton_->setText(playing_ ? QStringLiteral("暂停") : QStringLiteral("播放"));
    const auto sourceFrame = frameAtPosition100ns(
        snap.position100ns, sourceTotalFrames_, sourceFpsNumerator_, sourceFpsDenominator_);
    const auto vsTarget = frameAtPosition100ns(
        snap.position100ns, vsTotalFrames_, vsFpsNumerator_, vsFpsDenominator_);
    if (!timelineSeekPending_ && !vsFramePending_ && snap.frameIndex >= 0 &&
        vsTarget >= 0 && vsTarget != lastVsFrame_)
        requestProcessedFrame(static_cast<int>(vsTarget));
    frameStatus_->setText(QStringLiteral("源帧 %1  |  VS 帧 %2  |  %3×%4  |  %5-bit  |  3FP API %6")
        .arg(sourceFrame >= 0 ? QString::number(sourceFrame) : QStringLiteral("--(待 VPY 定位)"))
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
