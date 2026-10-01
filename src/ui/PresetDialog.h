#pragma once
#include <QDialog>
#include <functional>
namespace vsr {
class PresetDialog final : public QDialog {
public:
    PresetDialog(std::function<QString(const QString &)> saveCurrent, std::function<void(const QString &)> load,
                 std::function<void()> reset, QWidget *parent = nullptr);
};
}
