#pragma once

#include "backend/ThreeFpApi.h"
#include "graph/FilterGraph.h"

#include <QMainWindow>

#include <memory>

class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;
class QTimer;

namespace vsr {

class ParameterEditor;
class PreviewPane;
class ThreeFpPlayer;
class VapourSynthFrameServer;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *buildSidebar();
    QWidget *buildWorkspace();
    QWidget *buildTransport();
    void buildToolbar();
    void connectPlayback();
    void populateCatalog();
    void refreshPipeline(int selectRow = -1);
    void selectPipelineRow(int row);
    void openSource();
    bool loadSource(const QString &path);
    void showScript();
    void validateScript();
    void togglePlayback();
    void seekTimeline(int sliderValue);
    void requestProcessedFrame(int frameIndex);
    QString writePreviewScript(QString *error = nullptr) const;
    void saveProject();
    void openProject();
    void updatePlaybackState();
    void updatePixel(ThreeFpPlayer *player, PreviewPane *pane, int x, int y);
    void setStatus(const QString &text, bool error = false);

    ThreeFpApi api_;
    std::unique_ptr<ThreeFpPlayer> sourcePlayer_;
    std::unique_ptr<ThreeFpPlayer> processedPlayer_;
    std::unique_ptr<VapourSynthFrameServer> frameServer_;
    FilterGraph graph_;

    QLineEdit *sourcePath_ = nullptr;
    QComboBox *sourceFilter_ = nullptr;
    QLabel *runtimeStatus_ = nullptr;
    QLineEdit *catalogSearch_ = nullptr;
    QListWidget *catalogList_ = nullptr;
    QListWidget *pipelineList_ = nullptr;
    ParameterEditor *parameterEditor_ = nullptr;
    PreviewPane *sourcePane_ = nullptr;
    PreviewPane *processedPane_ = nullptr;
    QSlider *timeline_ = nullptr;
    QLabel *positionLabel_ = nullptr;
    QLabel *durationLabel_ = nullptr;
    QLabel *frameStatus_ = nullptr;
    QLabel *taskStatus_ = nullptr;
    QCheckBox *syncView_ = nullptr;
    QCheckBox *vrrPresent_ = nullptr;
    QCheckBox *vrrPacing_ = nullptr;
    QPushButton *playButton_ = nullptr;
    QTimer *stateTimer_ = nullptr;
    bool playing_ = false;
    bool timelinePressed_ = false;
    bool timelineSeekPending_ = false;
    bool vsScriptReady_ = false;
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
