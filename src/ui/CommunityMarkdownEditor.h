#pragma once

#include <QWidget>
#include <QUrl>
#include <functional>

class QPlainTextEdit;
class QTimer;
class QLabel;
class QStackedWidget;

namespace vsr {

class CommunityWebView;

class CommunityMarkdownEditor final : public QWidget {
public:
    using Upload = std::function<void(const QString &, const QByteArray &,
                                      std::function<void(const QUrl &, const QString &)>)>;
    explicit CommunityMarkdownEditor(Upload upload, QWidget *parent);
    QString markdown() const;
    void setMarkdown(const QString &value);
    void onChange(std::function<void()> changed);
    void setStatus(const QString &message);
    void setLocked(bool locked);
    bool uploading() const { return uploading_; }

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QString selection() const;
    void insertMarkup(const QString &prefix, const QString &text, const QString &suffix = {});
    void insertColor();
    void insertBlackout();
    void uploadImage(const QString &name, const QByteArray &bytes);
    void preview();
    QPlainTextEdit *source_;
    CommunityWebView *preview_ = nullptr;
    CommunityWebView *rich_ = nullptr;
    QStackedWidget *views_ = nullptr;
    QWidget *basic_ = nullptr;
    QString richText_;
    std::function<void()> changed_;
    int revision_ = 0;
    bool richReady_ = false;
    QLabel *status_;
    QTimer *refresh_;
    Upload upload_;
    bool uploading_ = false;
    bool locked_ = false;
};

}
