#pragma once
#include <QObject>
class QWidget;
namespace vsr {
// Only the original single-file launcher may be replaced, never its cache.
class SinglePlayerUpdater final : public QObject {
public:
    explicit SinglePlayerUpdater(QWidget *parent);
    void check(bool silent=true);
private:
    QWidget *window_;
};
}
