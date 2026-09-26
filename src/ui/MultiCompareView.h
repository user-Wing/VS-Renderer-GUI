#pragma once
#include <QWidget>
#include <QList>
#include <array>
class QComboBox;
class QToolButton;
namespace vsr {
class PreviewPane;
class MultiCompareView final : public QWidget {
    Q_OBJECT
public:
    enum Layout { Pair, Wipe, ThreeRow, ThreeColumn, ThreeLeft, ThreeRight, Four, Grid, ThreeNormal, FourWipe };
    explicit MultiCompareView(QWidget *parent = nullptr);
    PreviewPane *pane(int video) const;
    void removeSource(int video);
    void setSources(const QStringList &names);
    void setSlots(const QList<int> &assignments, Layout layout);
    QList<int> assignments() const { return slots_; }
    void setDivision(double x, double y);
    QRect cellRect(int slot) const;
    bool isWipe() const;
signals:
    void sourceRemoved(int video);
    void sourceSelected(int slot, int video);
protected:
    void resizeEvent(QResizeEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
private:
    void arrange();
    std::array<QWidget *, 9> clips_{};
    std::array<PreviewPane *, 9> panes_{};
    std::array<QComboBox *, 9> selectors_{};
    std::array<QToolButton *, 9> remove_{};
    std::array<QWidget *, 5> handles_{};
    QList<int> slots_;
    QList<QRect> cells_;
    Layout layout_ = Pair;
    double x_ = .5, y_ = .5, x2_ = .67, y2_ = .67;
    int dragging_ = -1;
};
}
