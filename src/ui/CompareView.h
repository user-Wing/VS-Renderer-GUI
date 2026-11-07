#pragma once
#include <QWidget>
namespace vsr {
class PreviewPane;
class MultiCompareView;
enum class CompareMode { SideBySide, Slider };
class CompareView final : public QWidget {
    Q_OBJECT
public:
    explicit CompareView(QWidget *parent=nullptr,const QString &leftTitle=QStringLiteral("源视频"),const QString &rightTitle=QStringLiteral("处理后"));
    PreviewPane *addProcessedPane();
    void removeProcessedPane(int route);
    void setRouteVisible(int route,bool visible);
    PreviewPane *sourcePane() const;
    PreviewPane *processedPane() const;
    CompareMode mode() const { return mode_; }
    double splitRatio() const;
    void setMode(CompareMode mode);
    void setSplitRatio(double ratio);
signals:
    void visiblePaneCountChanged(int count);
private:
    void updateLayout();
    MultiCompareView *view_;
    QList<bool> visible_{true,true};
    CompareMode mode_=CompareMode::SideBySide;
};
}
