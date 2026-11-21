#pragma once

#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>
#include <functional>
#include <memory>

namespace vsr {

class CommunityWebView final : public QWidget {
public:
    using SessionChanged = std::function<void(const QByteArray &cookie)>;
    explicit CommunityWebView(SessionChanged sessionChanged, QWidget *parent);
    using DocumentLink = std::function<void(const QUrl &)>;
    explicit CommunityWebView(DocumentLink link, QWidget *parent);
    ~CommunityWebView() override;
    void open(const QUrl &url, const QByteArray &cookie);
    void setSession(const QByteArray &cookie);
    void clearSession();
    void syncSession();
    void goBack();
    QUrl currentUrl() const;
    void showDocument(const QJsonObject &document);
    void onOutline(std::function<void(const QJsonArray &)> changed);
    void scrollToHeading(int index);
    void openEditor(std::function<void(bool)> ready, std::function<void(const QJsonObject &)> message);
    void editorCommand(const QJsonObject &command);
    void documentZoom(int step);
    void documentTheme(bool dark);
    void exportDocumentPdf(const QString &path, std::function<void(bool)> finished);
    static QUrl home();
    static bool isSite(const QUrl &url);
    static bool isEmbeddedSite(const QUrl &url);

protected:
    bool event(QEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
