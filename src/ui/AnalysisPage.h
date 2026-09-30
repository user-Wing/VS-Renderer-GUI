#pragma once

#include "backend/ThreeFpApi.h"
#include "graph/VpyScriptBuilder.h"
#include <QElapsedTimer>
#include <QWidget>
#include <array>
#include <memory>
#include <optional>

class QLabel;
class QSlider;
class QPushButton;
class QCheckBox;
class QComboBox;

namespace vsr {
class ExportWindow;
class MultiCompareView;
class PreviewPane;
class ThreeFpPlayer;

class AnalysisPage final : public QWidget {
    Q_OBJECT
public:
    explicit AnalysisPage(ThreeFpApi &api, QWidget *parent = nullptr);
    ~AnalysisPage() override;
    bool openVideo(int lane, const QString &path);
    void removeVideo(int video);
    void openFiles(const QStringList &paths);
    void togglePlayback();
    void pause();
    void seekGlobal(std::int64_t position);
    void alignVideo(int lane, int direction, bool seconds);
    void stepGlobal(int direction);
    ThreeFpSnapshot snapshot(int lane) const;
    std::int64_t offset(int lane) const;
    bool busy() const;
    int videoCount() const { return videoCount_; }
    QList<int> visibleVideos() const;
    void selectSource(int slot, int video);
    void setMode(int tracks);
    void refreshLayout();
    ScriptBuildResult compositionScript() const;
    bool exportBusy() const;
    void stopExport();
signals:
    void exportIdle();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void exportComparison();
    void poll();
    void resizeEvent(QResizeEvent *event) override;
    void adjustTrackWidths();
    void configurePlayer(int video);
    int masterVideo() const;
    bool activeVideo(int video) const;
    bool loaded(int lane) const;
    void seekLane(int lane, std::int64_t target);
    void beginStep(int lane, int direction, bool global);
    std::int64_t clampGlobal(std::int64_t position) const;
    std::array<std::unique_ptr<ThreeFpPlayer>, 9> players_;
    std::array<PreviewPane *, 9> panes_{};
    std::array<QSlider *, 4> sliders_{};
    std::array<QLabel *, 4> times_{};
    std::array<QComboBox *, 4> sources_{};
    std::array<QWidget *, 4> rows_{};
    QList<int> slots_;
    QWidget *controls_ = nullptr;
    QComboBox *mode_ = nullptr;
    QComboBox *layoutChoice_ = nullptr;
    QComboBox *audio_ = nullptr;
    QComboBox *scaler_ = nullptr;
    QCheckBox *vrr_ = nullptr;
    QCheckBox *pacing_ = nullptr;
    ThreeFpApi &api_;
    int videoCount_ = 0;
    int trackCount_ = 2;
    int chromaAlgorithm_ = 1;
    std::array<QString, 9> paths_;
    std::array<std::int64_t, 9> offsets_{};
    std::array<bool, 9> opening_{};
    std::array<bool, 9> pending_{};
    std::array<std::uint64_t, 9> generations_{};
    std::array<std::uint64_t, 9> presents_{};
    MultiCompareView *view_ = nullptr;
    QPushButton *play_ = nullptr;
    QLabel *status_ = nullptr;
    std::unique_ptr<ExportWindow> exportWindow_;
    QCheckBox *sync_ = nullptr;
    std::int64_t global_ = 0;
    std::optional<std::int64_t> queuedSeek_;
    int aligning_ = -1;
    bool globalStep_ = false;
    bool playing_ = false;
    bool resume_ = false;
    QElapsedTimer operationTime_;
    QElapsedTimer driftTime_;
};
}
