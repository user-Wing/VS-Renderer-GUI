#pragma once

#include "backend/ThreeFpApi.h"
#include "graph/FilterGraph.h"
#include "graph/VpyScriptBuilder.h"
#include "player/PlayerResources.h"

#include <QMainWindow>
#include <QJsonObject>
#include <QList>

#include <memory>
#include <vector>

class QAction;
class QCheckBox;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;
class QTimer;
class QStackedWidget;
class QToolButton;
class QSplitter;
class QSpinBox;

class TestAnalysis;
class TestAgentIntegration;

namespace vsr {

class AnalysisPage;
class RenderRoute;
class ParameterEditor;
class CompareView;
class PreviewPane;
class ExportWindow;
class ThreeFpPlayer;
class VapourSynthFrameServer;
class PlayerLanguage;
class AgentSession;
class AgentPage;
class AgentInstructions;
class AgentMcpManager;
class AgentTaskQueue;
class AgentModelCapabilities;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    friend class ::TestAnalysis;
    friend class ::TestAgentIntegration;
    void initializeAgent();
    void initializeAgentFeatures();
    void initializeAgentInteractions();
    void submitAgentInput(const QString &text);
    void refreshAgentCompletions();
    QWidget *buildSidebar();
    QWidget *buildParameterPanel();
    QWidget *buildSettingsPage();
    QWidget *buildWorkspace();
    QWidget *buildTransport();
    void applyPreviewScaling();
    QWidget *buildTimeGroups();
    void refreshTimeGroups(int selected);
    QList<FilterTimeGroup> currentTimeGroups() const;
    void replaceTimeGroups(const QList<FilterTimeGroup> &groups);
    void showCommunity();
    void buildToolbar();
    void selectPage(int index);
    void showSettings();
    std::unique_ptr<PlayerLanguage> language_;
    void connectPlayback();
    void populateCatalog();
    void refreshPipeline(int selectRow = -1);
    void selectPipelineRow(int row);
    void updateParameterInputSize();
    void openSource();
    bool addRoute();
    void createAdditionalRoute();
    void selectRoute(int index);
    void removeRoute();
    void updateRouteChoices();
    void renderAdditionalRoute(int index);
    void scanWorkingFolder(const QString &folder = {});
    struct RouteState {QList<FilterTimeGroup> groups{{QStringLiteral("全片"),0,{}}};QString preset;bool visible=true;QString previewScript;};
    QList<RouteState> routes_{{}};
    std::vector<std::unique_ptr<RenderRoute>> additionalRoutes_;
    int routeIndex_=0;
    QComboBox *routeChoice_=nullptr,*sourceChoice_=nullptr;
    QStackedWidget *sourceInputStack_=nullptr;
    QPushButton *addRouteButton_=nullptr,*removeRouteButton_=nullptr;

    bool loadSource(const QString &path);
    void showScript();
    void validateScript();
    void exportCurrentResult();
    void togglePlayback();
    void seekTimeline(int sliderValue);
    void requestProcessedFrame(int frameIndex);
    QString writePreviewScript(QString *error = nullptr, const QString &script = {}) const;
    QString hlslPreviewPackage_, hlslPreviewFallback_;
    QVariantMap hlslPreviewParameters_;
    bool hlslPreviewRetryGlSl_ = false;
    QJsonObject projectSnapshot() const;
    bool applyProjectSnapshot(const QJsonObject &root);
    bool confirmProjectReplacement();
    void initializeProjectProtection();
    void checkpointProject();
    void stepProjectHistory(int direction);
    QString projectPath_, recoveryPath_;
    QList<QJsonObject> projectHistory_;
    int projectHistoryIndex_=0;
    QJsonObject savedProject_;
    bool projectApplying_=false, projectProtectionReady_=false;
    quint64 folderScanGeneration_=0;
    void saveProject();
    void openProject();
    bool saveProjectTo(const QString &path, QString *error = nullptr);
    bool openProjectFrom(const QString &path, QString *error = nullptr);
    void showPresets();
    void loadPreset(const QString &path, bool currentGroup = false);
    ScriptBuildResult currentScript(const QString &source = {}) const;
    QString activePreset_;
    void updatePlaybackState();
    void updatePixel(ThreeFpPlayer *player, PreviewPane *pane, int x, int y);
    void setStatus(const QString &text, bool error = false);

    ThreeFpApi api_;
    std::unique_ptr<ThreeFpPlayer> sourcePlayer_;
    std::unique_ptr<ThreeFpPlayer> processedPlayer_;
    std::unique_ptr<VapourSynthFrameServer> frameServer_;
    std::unique_ptr<ExportWindow> exportWindow_;
    bool pendingPreview_ = false;
    std::unique_ptr<AnalysisPage> analysisPage_;
    AgentSession *agentSession_ = nullptr;
    AgentPage *agentPage_ = nullptr;
    AgentInstructions *agentInstructions_ = nullptr;
    AgentMcpManager *agentMcp_ = nullptr;
    AgentTaskQueue *agentQueue_ = nullptr;
    AgentModelCapabilities *agentModels_ = nullptr;
    QStackedWidget *pages_ = nullptr;
    QWidget *navigation_ = nullptr;
    QList<QAction *> vsActions_;
    QList<QToolButton *> navigationButtons_;
    FilterGraph graph_;
    QList<FilterTimeGroup> timeGroups_{{QStringLiteral("全片"),0,{}}};
    FilterGraph copiedFilters_;
    QString copiedGroupScript_;
    bool hasCopiedFilters_ = false;
    int timeGroupIndex_ = 0;
    QListWidget *timeGroupList_ = nullptr;
    QSpinBox *groupStart_ = nullptr, *groupFrame_ = nullptr;
    qint64 groupFrameBoundary(const FilterTimeGroup &group) const;
    QLineEdit *groupName_ = nullptr;
    QSplitter *content_ = nullptr;
    QWidget *featureOne_ = nullptr, *featureTwo_ = nullptr, *pipelinePanel_ = nullptr, *fileInput_ = nullptr;
    QList<int> featureWidths_{290,310};
    PlayerResources resources_;
    QLabel *sourceMonitor_ = nullptr, *processedMonitor_ = nullptr;

    QLineEdit *sourcePath_ = nullptr;
    QComboBox *sourceFilter_ = nullptr;
    QLabel *runtimeStatus_ = nullptr;
    QLineEdit *catalogSearch_ = nullptr;
    QComboBox *catalogCategory_ = nullptr;
    QListWidget *catalogList_ = nullptr;
    QListWidget *pipelineList_ = nullptr;
    ParameterEditor *parameterEditor_ = nullptr;
    CompareView *compareView_ = nullptr;
    PreviewPane *sourcePane_ = nullptr;
    PreviewPane *processedPane_ = nullptr;
    QSlider *timeline_ = nullptr;
    QLabel *positionLabel_ = nullptr;
    QLabel *durationLabel_ = nullptr;
    QLabel *frameStatus_ = nullptr;
    QLabel *taskStatus_ = nullptr;
    QTimer *taskStatusTimer_ = nullptr;
    QCheckBox *syncView_ = nullptr;
    QCheckBox *vrrPresent_ = nullptr;
    QCheckBox *vrrPacing_ = nullptr;
    QPushButton *playButton_ = nullptr;
    QTimer *stateTimer_ = nullptr;
    bool playing_ = false;
    bool timelinePressed_ = false;
    bool timelineSeekPending_ = false;
    bool vsScriptReady_ = false;
    bool vsFramePending_ = false;
    bool sourcePrimePending_ = false;
    bool sourcePrimeStarted_ = false;
    int pendingTimelineValue_ = -1;
    std::uint64_t timelineSeekGeneration_ = 0;
    int sourceTotalFrames_ = 0;
    std::int64_t sourceFpsNumerator_ = 0;
    std::int64_t sourceFpsDenominator_ = 0;
    int vsTotalFrames_ = 0;
    std::int64_t vsFpsNumerator_ = 0;
    std::int64_t vsFpsDenominator_ = 0;
    int requestedVsFrame_ = -1;
    int lastVsFrame_ = -1;
};

}
