#pragma once

#include <QWidget>

namespace vsr {

class PreviewPane;

enum class CompareMode {
    SideBySide,
    Slider
};

class CompareView final : public QWidget {
    Q_OBJECT
public:
    explicit CompareView(QWidget *parent = nullptr,
                         const QString &leftTitle = QStringLiteral("源视频"),
                         const QString &rightTitle = QStringLiteral("处理后"));

    PreviewPane *sourcePane() const;
    PreviewPane *processedPane() const;
    CompareMode mode() const;
    double splitRatio() const;
    void setMode(CompareMode mode);
    void setSplitRatio(double ratio);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLayout();

    PreviewPane *sourcePane_ = nullptr;
    PreviewPane *processedPane_ = nullptr;
    QWidget *sourceClip_ = nullptr;
    QWidget *processedClip_ = nullptr;
    QWidget *sliderHandle_ = nullptr;
    CompareMode mode_ = CompareMode::SideBySide;
    double splitRatio_ = 0.5;
    bool dragging_ = false;
};

}
