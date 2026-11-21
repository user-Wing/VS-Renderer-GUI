#pragma once

#include <QWidget>
#include <memory>
#include <functional>

class QNetworkAccessManager;

namespace vsr {

class CommunityPage final : public QWidget {
public:
    using ImportPreset = std::function<void(const QString &, bool)>;
    explicit CommunityPage(ImportPreset importPreset, QWidget *parent,
                           std::unique_ptr<QNetworkAccessManager> transport = {});
    ~CommunityPage() override;
    void openAccount();
    void setRememberLogin(bool enabled);
    void forgetSession();
    void resetPresetSource();

protected:
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
