#include "runtime/PortableLayout.h"
#include "ui/CommunityPage.h"
#include "ui/CommunityMarkdownEditor.h"
#include "ui/CommunityWebView.h"
#include "ui/PresetCommunityDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHttpMultiPart>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QPointer>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSet>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QStandardPaths>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUrlQuery>
#include <algorithm>

#include <windows.h>
#include <wincrypt.h>

namespace vsr {
namespace {

class CommunityTopicDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        // Keep the row inside the viewport even when its title is wider than the pane.
        size.setWidth(0);
        return size;
    }
};

constexpr int apiLimit = 16 * 1024 * 1024;
constexpr auto rememberedSession = "community/sessionDpapi";
constexpr auto rememberLogin = "community/rememberLogin";

QUrl siteUrl(const QString &path)
{
    return CommunityWebView::home().resolved(QUrl(path));
}

QByteArray protectSession(const QByteArray &value, bool encrypt)
{
    if (value.isEmpty()) return {};
    DATA_BLOB input{DWORD(value.size()), reinterpret_cast<BYTE *>(const_cast<char *>(value.constData()))}, output{};
    const BOOL ok = encrypt
        ? CryptProtectData(&input, L"VS Renderer Forum", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)
        : CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
    if (!ok) return {};
    const QByteArray result(reinterpret_cast<const char *>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return result;
}

QByteArray checkedSession(const QByteArray &value)
{
    static const QRegularExpression token(QStringLiteral("^[A-Za-z0-9_-]{20,160}$"));
    return token.match(QString::fromLatin1(value)).hasMatch() ? value : QByteArray();
}

QString localDate(const QJsonObject &item, const char *key)
{
    return QDateTime::fromString(item.value(key).toString(), Qt::ISODate).toLocalTime().toString("yyyy-MM-dd HH:mm");
}

QNetworkRequest requestFor(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setTransferTimeout(20000);
    return request;
}


}

struct CommunityPage::Impl {
    CommunityPage *page;
    ImportPreset importPreset;
    std::unique_ptr<QNetworkAccessManager> network;
    QStackedWidget *screens;
    QWidget *forum;
    QWidget *left;
    QWidget *webPanel;
    CommunityWebView *web;
    QPointer<QDialog> loginDialog;
    QPointer<CommunityWebView> loginWeb;
    PresetCommunityDialog *presets = nullptr;
    QTabBar *tabs;
    QTreeWidget *boards;
    QListWidget *topics;
    QLineEdit *search;
    QLabel *status;
    QLabel *listStatus;
    QPushButton *account;
    QPushButton *back;
    QPushButton *exportButton;
    QPushButton *manageButton;
    QPushButton *replyButton;
    QStackedWidget *detail;
    QWidget *reading;
    QStackedWidget *postPreview;
    QLabel *postMessage;
    CommunityWebView *body;
    QWidget *composer = nullptr;
    CommunityMarkdownEditor *editor = nullptr;
    QComboBox *composeBoard;
    QLineEdit *composeTitle;
    QLabel *composeLabel;
    QPushButton *submit;
    QTimer *draftTimer;
    QJsonArray taxonomy;
    QJsonObject currentPost;
    QJsonArray comments;
    QHash<qint64, QString> replies;
    QJsonObject accountInfo;
    QString board;
    QByteArray cookie;
    QByteArray adminCookie;
    QString adminCsrf;
    QString csrf;
    QString user;
    qint64 postId = 0;
    unsigned listRequest = 0;
    unsigned postRequest = 0;
    unsigned accountRequest = 0;
    bool loadingPost = false;
    bool loggedIn = false;
    bool initialized = false;
    bool sending = false;
    bool composingReply = false;

    enum Tab { Home, Docs, Resources, Download, Tools, Forum, About, Presets };
    QWidget *tutorials = nullptr;
    QTreeWidget *tutorialTree = nullptr;
    QTreeWidget *tutorialOutline = nullptr;
    CommunityWebView *tutorialBody = nullptr;
    QLabel *tutorialStatus = nullptr;
    unsigned tutorialRequest = 0;
    bool tutorialLoading = false;
    QString requestedTutorial;

    explicit Impl(CommunityPage *owner, ImportPreset import, std::unique_ptr<QNetworkAccessManager> transport)
        : page(owner), importPreset(std::move(import)),
          network(transport ? std::move(transport) : std::make_unique<QNetworkAccessManager>())
    {
        if (vsr::rendererSettings().value(rememberLogin, false).toBool())
            cookie = checkedSession(protectSession(vsr::rendererSettings().value(rememberedSession).toByteArray(), false));

        auto *layout = new QVBoxLayout(page);
        auto *top = new QHBoxLayout;
        layout->addLayout(top);
        tabs = new QTabBar(page);
        tabs->setObjectName("communityTabs");
        tabs->setExpanding(false);
        tabs->setDrawBase(false);
        for (const auto &text : {QStringLiteral("首页"), QStringLiteral("公开教程"), QStringLiteral("资源分享"),
                 QStringLiteral("下载"), QStringLiteral("工具"), QStringLiteral("论坛"), QStringLiteral("关于"),
                 QStringLiteral("预设下载")}) tabs->addTab(text);
        tabs->setCurrentIndex(Forum);
        top->addWidget(tabs, 1);
        account = button(top, QStringLiteral("登录"), [this] { accountMenu(); });
        account->setObjectName("communityAccount");
        status = label(page);
        status->setObjectName("communityStatus");
        status->hide();
        layout->addWidget(status);
        screens = new QStackedWidget(page);
        layout->addWidget(screens, 1);
        buildForum();
        webPanel = new QWidget(screens);
        auto *webLayout = new QVBoxLayout(webPanel);
        webLayout->setContentsMargins(0, 0, 0, 0);
        auto *actions = new QHBoxLayout;
        webLayout->addLayout(actions);
        button(actions, QStringLiteral("← 返回论坛"), [this] {
            tabs->setCurrentIndex(Forum);
            screens->setCurrentWidget(forum);
            checkSession();
        });
        button(actions, QStringLiteral("网页后退"), [this] { web->goBack(); });
        actions->addStretch();
        button(actions, QStringLiteral("浏览器打开"), [this] { QDesktopServices::openUrl(web->currentUrl()); });
        web = new CommunityWebView([this](const QByteArray &session) { acceptWebSession(session); }, webPanel);
        webLayout->addWidget(web, 1);
        screens->addWidget(webPanel);
        QObject::connect(tabs, &QTabBar::currentChanged, page, [this](int index) { selectTab(index); });
        QObject::connect(tabs, &QTabBar::tabBarClicked, page, [this](int index) {
            if (tabs->currentIndex() != index) return;
            if ((index == Forum && screens->currentWidget() != forum) ||
                (index == Docs && screens->currentWidget() != tutorials)) selectTab(index);
        });
    }

    static QLabel *label(QWidget *parent)
    {
        auto *result = new QLabel(parent);
        result->setTextFormat(Qt::PlainText);
        result->setWordWrap(true);
        return result;
    }

    QPushButton *button(QHBoxLayout *row, const QString &text, std::function<void()> action)
    {
        auto *result = new QPushButton(text, page);
        row->addWidget(result);
        QObject::connect(result, &QPushButton::clicked, page, std::move(action));
        return result;
    }

    void buildForum()
    {
        forum = new QWidget(screens);
        screens->addWidget(forum);
        auto *layout = new QVBoxLayout(forum);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *bar = new QHBoxLayout;
        layout->addLayout(bar);
        auto *toggle = button(bar, QStringLiteral("☰"), [this] { left->setVisible(left->isHidden()); });
        toggle->setObjectName("communitySidebarToggle");
        toggle->setToolTip(QStringLiteral("展开 / 收起板块与帖子列表"));
        search = new QLineEdit(forum);
        search->setObjectName("communitySearch");
        search->setPlaceholderText(QStringLiteral("搜索标题、正文或作者"));
        search->setClearButtonEnabled(true);
        search->setMaxLength(200);
        bar->addWidget(search, 1);
        button(bar, QStringLiteral("搜索"), [this] { loadTopics(); });
        button(bar, QStringLiteral("刷新"), [this] { loadBoards(); loadTopics(); checkSession(); });
        auto *create = button(bar, QStringLiteral("发布新话题"), [this] { compose(false); });
        create->setObjectName("communityNewPost");
        QObject::connect(search, &QLineEdit::returnPressed, page, [this] { loadTopics(); });

        auto *split = new QSplitter(forum);
        split->setChildrenCollapsible(false);
        split->setOpaqueResize(false);
        layout->addWidget(split, 1);
        left = new QWidget(split);
        auto *leftLayout = new QVBoxLayout(left);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        auto *leftSplit = new QSplitter(Qt::Vertical, left);
        leftSplit->setChildrenCollapsible(false);
        leftLayout->addWidget(leftSplit);
        boards = new QTreeWidget(leftSplit);
        boards->setObjectName("communityBoards");
        boards->setHeaderLabel(QStringLiteral("板块"));
        boards->setUniformRowHeights(true);
        boards->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto listFont = boards->font();
        listFont.setFamily(QStringLiteral("Microsoft YaHei UI"));
        listFont.setPointSizeF(qMax(12.0, listFont.pointSizeF() + 1.5));
        boards->setFont(listFont);
        auto *feed = new QWidget(leftSplit);
        auto *feedLayout = new QVBoxLayout(feed);
        feedLayout->setContentsMargins(0, 0, 0, 0);
        listStatus = label(feed);
        feedLayout->addWidget(listStatus);
        topics = new QListWidget(feed);
        topics->setObjectName("communityTopics");
        topics->setWordWrap(false);
        topics->setUniformItemSizes(true);
        topics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        topics->setTextElideMode(Qt::ElideRight);
        topics->setItemDelegate(new CommunityTopicDelegate(topics));
        topics->setFont(listFont);
        feedLayout->addWidget(topics, 1);
        leftSplit->setSizes({220, 360});
        QObject::connect(boards, &QTreeWidget::currentItemChanged, page, [this](QTreeWidgetItem *item) {
            if (!item) return;
            board = item->data(0, Qt::UserRole).toString();
            loadTopics();
        });
        const auto openTopic = [this](QListWidgetItem *item) { loadPost(item->data(Qt::UserRole).toLongLong()); };
        QObject::connect(topics, &QListWidget::itemClicked, page, openTopic);
        QObject::connect(topics, &QListWidget::itemActivated, page, openTopic);

        detail = new QStackedWidget(split);
        reading = new QWidget(detail);
        detail->addWidget(reading);
        auto *readingLayout = new QVBoxLayout(reading);
        readingLayout->setContentsMargins(0, 0, 0, 0);
        auto *readerToolbar = new QWidget(reading);
        auto font = readerToolbar->font();
        font.setFamily(QStringLiteral("Microsoft YaHei UI"));
        readerToolbar->setFont(font);
        auto *readerBar = new QHBoxLayout(readerToolbar);
        readerBar->setContentsMargins(0, 0, 0, 0);
        readingLayout->addWidget(readerToolbar);
        back = button(readerBar, QStringLiteral("← 返回列表"), [this] {
            closePost();
            topics->setFocus();
        });
        readerBar->addStretch();
        for (const auto &[name, step] : {std::pair{QStringLiteral("字号−"), -1},
                {QStringLiteral("默认"), 0}, {QStringLiteral("字号＋"), 1}}) {
            auto *zoom = button(readerBar, name, [this, step] { body->documentZoom(step); });
            zoom->setToolTip(QStringLiteral("Ctrl + 滚轮调整正文字号"));
        }
        manageButton = button(readerBar, QStringLiteral("管理"), [this] { managePost(); });
        manageButton->setObjectName("communityManagePost");
        replyButton = button(readerBar, QStringLiteral("回复"), [this] { compose(true); });
        replyButton->setObjectName("communityReply");
        exportButton = button(readerBar, QStringLiteral("导出"), [this] { exportThread(); });
        postPreview = new QStackedWidget(reading);
        postMessage = label(postPreview);
        postMessage->setAlignment(Qt::AlignCenter);
        postPreview->addWidget(postMessage);
        body = new CommunityWebView(CommunityWebView::DocumentLink([this](const QUrl &link) {
            followDocumentLink(link);
        }), postPreview);
        body->setObjectName("communityPost");
        postPreview->addWidget(body);
        readingLayout->addWidget(postPreview, 1);
        split->setSizes({320, 740});
        split->setStretchFactor(1, 1);
        closePost();
    }

    using Response = std::function<void(const QJsonObject &, const QString &)>;
    using LoginResponse = std::function<void(const QJsonObject &, const QByteArray &, const QString &)>;
    void api(const QUrl &url, Response done, const QJsonObject &payload = {}, const QByteArray &method = "GET",
             QHttpMultiPart *upload = nullptr, LoginResponse login = {})
    {
        auto request = requestFor(url);
        request.setRawHeader("Accept", "application/json");
        const bool admin = url.path().startsWith("/api/admin/");
        if (admin && !adminCookie.isEmpty()) request.setRawHeader("Cookie", "blog_session=" + adminCookie);
        if (!admin && !cookie.isEmpty()) request.setRawHeader("Cookie", "forum_session=" + cookie);
        if (method != "GET") {
            request.setRawHeader(admin ? "X-Blog-CSRF" : "X-Forum-CSRF", (admin ? adminCsrf : csrf).toUtf8());
            request.setRawHeader("Origin", "https://www1.arxchem.top");
        }
        if (!upload && (method == "POST" || method == "PUT"))
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        auto *reply = upload ? network->post(request, upload)
            : network->sendCustomRequest(request, method, method == "POST" || method == "PUT" ?
                QJsonDocument(payload).toJson(QJsonDocument::Compact) : QByteArray());
        if (upload) upload->setParent(reply);
        QObject::connect(reply, &QNetworkReply::downloadProgress, page, [reply](qint64 received) {
            if (received > apiLimit) reply->abort();
        });
        QObject::connect(reply, &QNetworkReply::finished, page,
            [this, reply, url, admin, done = std::move(done), login = std::move(login)] {
            reply->deleteLater();
            const auto raw = reply->bytesAvailable() <= apiLimit ? reply->readAll() : QByteArray();
            const auto data = QJsonDocument::fromJson(raw);
            const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString error;
            if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300)
                error = data.object().value("error").toString(status >= 300 && status < 400 ?
                    QStringLiteral("论坛返回了重定向（HTTP %1）。").arg(status) : reply->errorString());
            else if (!data.isObject()) error = QStringLiteral("论坛返回的内容无法读取。");
            if (admin && status == 401) {
                adminCookie.clear();
                adminCsrf.clear();
            }
            if (error.isEmpty() && url.path() == "/api/admin/login") {
                const auto cookies = reply->header(QNetworkRequest::SetCookieHeader).value<QList<QNetworkCookie>>();
                for (const auto &entry : cookies)
                    if (entry.name() == "blog_session") adminCookie = checkedSession(entry.value());
                adminCsrf = data.object().value("csrf").toString();
                if (adminCookie.isEmpty() || adminCsrf.isEmpty()) error = QStringLiteral("后台管理员会话无效。");
            }
            if (login) {
                QByteArray session;
                if (error.isEmpty()) {
                    const auto cookies = reply->header(QNetworkRequest::SetCookieHeader).value<QList<QNetworkCookie>>();
                    for (const auto &entry : cookies)
                        if (entry.name() == "forum_session") session = checkedSession(entry.value());
                    if (session.isEmpty() || data.object().value("csrf").toString().isEmpty() ||
                        data.object().value("user").toObject().value("id").toInteger() <= 0)
                        error = QStringLiteral("源站未返回有效的论坛会话。");
                }
                login(data.object(), session, error);
            } else {
                done(data.object(), error);
            }
        });
    }

    void notice(const QString &message)
    {
        status->setText(message);
        status->setVisible(!message.isEmpty());
    }

    void loadBoards()
    {
        api(siteUrl("/api/forum/boards"), [this](const QJsonObject &data, const QString &error) {
            if (!error.isEmpty()) { notice(error); return; }
            taxonomy = data.value("boards").toArray();
            if (editor && detail->currentWidget() == composer && !composingReply && !sending)
                fillComposeBoards(composeBoard->currentData().toString());
            const QSignalBlocker guard(boards);
            boards->clear();
            auto *selected = new QTreeWidgetItem(boards, {QStringLiteral("全部话题")});
            const auto append = [&](const QJsonObject &value, QTreeWidgetItem *parent) {
                auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(boards);
                item->setText(0, QStringLiteral("%1  ·  %2").arg(value.value("name").toString()).arg(value.value("post_count").toInt()));
                const auto slug = value.value("slug").toString();
                item->setData(0, Qt::UserRole, slug);
                item->setToolTip(0, value.value("description").toString());
                if (slug == board) selected = item;
                return item;
            };
            for (const auto &entry : data.value("domains").toArray()) {
                const auto domain = entry.toObject();
                auto *parent = append(domain, nullptr);
                for (const auto &child : domain.value("subs").toArray()) append(child.toObject(), parent);
            }
            if (data.value("domains").toArray().isEmpty())
                for (const auto &entry : data.value("boards").toArray()) append(entry.toObject(), nullptr);
            boards->setCurrentItem(selected);
            if (selected->parent()) selected->parent()->setExpanded(true);
        });
    }

    void loadTopics()
    {
        auto url = siteUrl("/api/forum/posts");
        QUrlQuery query;
        if (!board.isEmpty()) query.addQueryItem("board", board);
        if (!search->text().trimmed().isEmpty()) query.addQueryItem("q", search->text().trimmed());
        url.setQuery(query);
        const auto generation = ++listRequest;
        listStatus->setText(QStringLiteral("正在读取帖子…"));
        api(url, [this, generation](const QJsonObject &data, const QString &error) {
            if (generation != listRequest) return;
            if (!error.isEmpty()) { listStatus->setText(error); return; }
            topics->clear();
            const auto posts = data.value("posts").toArray();
            listStatus->setText(posts.isEmpty() ? QStringLiteral("暂无话题") : QStringLiteral("%1 个话题").arg(posts.size()));
            for (const auto &entry : posts) {
                const auto post = entry.toObject();
                auto *item = new QListWidgetItem(topics);
                const auto title = post.value("title").toString().simplified();
                const auto pinned = post.value("pinned").toInt() || post.value("pinned").toBool();
                item->setText(QStringLiteral("%1%2\n%3 · %4 · %5 回复")
                    .arg(pinned ? QStringLiteral("置顶  ") : QString(), title, post.value("board_name").toString(),
                         post.value("username").toString()).arg(post.value("comment_count").toInt()));
                item->setToolTip(item->text() + QStringLiteral("\n") + localDate(post, "updated_at"));
                item->setData(Qt::UserRole, post.value("id").toInteger());
                if (postId && postId == post.value("id").toInteger()) topics->setCurrentItem(item);
            }
        });
    }

    QUrl postUrl() const { return siteUrl(QString("/forum/post/%1").arg(postId)); }

    void followDocumentLink(const QUrl &link)
    {
        if (CommunityWebView::isSite(link)) {
            if (link.path() == "/docs" || link.path().startsWith("/docs/")) {
                requestedTutorial = link.path().section('/', 2, 2);
                if (tabs->currentIndex() != Docs) tabs->setCurrentIndex(Docs);
                else openTutorials();
                return;
            }
            static const QRegularExpression forumPost(QStringLiteral("^/forum/post/(\\d+)/?$"));
            const auto match = forumPost.match(link.path());
            if (match.hasMatch()) { loadPost(match.captured(1).toLongLong()); return; }
            if (link.path() == "/forum/new") { compose(false); return; }
            if (link.path() == "/forum") { closePost(); return; }
            openWeb(link);
        } else {
            QDesktopServices::openUrl(link);
        }
    }

    void openTutorials()
    {
        if (!tutorials) {
            auto *split = new QSplitter(Qt::Horizontal, screens);
            tutorials = split;
            split->setChildrenCollapsible(false);
            tutorialTree = new QTreeWidget(split);
            tutorialTree->setObjectName("communityTutorialTree");
            tutorialTree->setHeaderLabel(QStringLiteral("公开教程"));
            auto *reader = new QWidget(split);
            auto *layout = new QVBoxLayout(reader);
            layout->setContentsMargins(10, 0, 10, 0);
            tutorialStatus = label(reader);
            layout->addWidget(tutorialStatus);
            tutorialBody = new CommunityWebView(CommunityWebView::DocumentLink([this](const QUrl &link) {
                followDocumentLink(link);
            }), reader);
            tutorialBody->setObjectName("communityTutorialArticle");
            layout->addWidget(tutorialBody, 1);
            tutorialOutline = new QTreeWidget(split);
            tutorialOutline->setObjectName("communityTutorialOutline");
            tutorialOutline->setHeaderLabel(QStringLiteral("文章结构"));
            split->setSizes({245, 800, 225});
            split->setStretchFactor(1, 1);
            screens->addWidget(split);
            tutorialBody->onOutline([this](const QJsonArray &headings) {
                tutorialOutline->clear();
                QList<QTreeWidgetItem *> parents;
                int index = 0;
                for (const auto &entry : headings) {
                    const auto heading = entry.toObject();
                    const int level = heading.value("level").toInt();
                    while (!parents.isEmpty() && parents.constLast()->data(0, Qt::UserRole + 1).toInt() >= level)
                        parents.removeLast();
                    auto *item = parents.isEmpty() ? new QTreeWidgetItem(tutorialOutline) : new QTreeWidgetItem(parents.constLast());
                    item->setText(0, heading.value("title").toString());
                    item->setData(0, Qt::UserRole, index++);
                    item->setData(0, Qt::UserRole + 1, level);
                    parents.append(item);
                }
                tutorialOutline->expandAll();
            });
            QObject::connect(tutorialOutline, &QTreeWidget::currentItemChanged, page, [this](QTreeWidgetItem *item) {
                if (item) tutorialBody->scrollToHeading(item->data(0, Qt::UserRole).toInt());
            });
            QObject::connect(tutorialTree, &QTreeWidget::currentItemChanged, page, [this](QTreeWidgetItem *item) {
                if (!item) return;
                const auto slug = item->data(0, Qt::UserRole + 1).toString();
                if (slug.isEmpty()) return;
                const auto id = item->data(0, Qt::UserRole).toString();
                vsr::rendererSettings().setValue("community/lastTutorial", id);
                const auto generation = ++tutorialRequest;
                tutorialOutline->clear();
                tutorialBody->hide();
                tutorialStatus->setText(QStringLiteral("正在载入教程…"));
                tutorialStatus->show();
                api(siteUrl("/api/articles/" + QString::fromLatin1(QUrl::toPercentEncoding(slug))),
                    [this, id, generation](const QJsonObject &data, const QString &error) {
                        if (generation != tutorialRequest) return;
                        const auto article = data.value("article").toObject();
                        if (!error.isEmpty() || article.isEmpty()) {
                            tutorialStatus->setText(error.isEmpty() ? QStringLiteral("教程不存在。") : error);
                            return;
                        }
                        tutorialStatus->hide();
                        tutorialBody->show();
                        tutorialBody->showDocument({
                            {"blocks", QJsonArray{QJsonObject{{"title", article.value("title")}, {"markdown", article.value("content")}}}},
                            {"base", siteUrl("/docs/" + id).toString()}, {"key", id},
                            {"dark", vsr::rendererSettings().value("appearance/dark", false).toBool()}
                        });
                    });
            });
        }
        screens->setCurrentWidget(tutorials);
        const auto openRequested = [this] {
            const auto id = requestedTutorial.isEmpty() ? vsr::rendererSettings().value("community/lastTutorial").toString() : requestedTutorial;
            requestedTutorial.clear();
            QTreeWidgetItem *first = nullptr;
            for (QTreeWidgetItemIterator it(tutorialTree); *it; ++it) {
                auto *item = *it;
                if (item->data(0, Qt::UserRole + 1).toString().isEmpty()) continue;
                if (!first) first = item;
                if (item->data(0, Qt::UserRole).toString() == id) { first = item; break; }
            }
            if (first) {
                tutorialTree->setCurrentItem(first);
                tutorialTree->scrollToItem(first);
            } else tutorialStatus->setText(QStringLiteral("暂无公开教程。"));
        };
        if (tutorialTree->topLevelItemCount()) { openRequested(); return; }
        if (tutorialLoading) return;
        tutorialLoading = true;
        tutorialStatus->setText(QStringLiteral("正在载入教程目录…"));
        api(siteUrl("/api/site-config"), [this, openRequested](const QJsonObject &data, const QString &error) {
            tutorialLoading = false;
            if (!error.isEmpty()) { tutorialStatus->setText(error); return; }
            const auto nodes = data.value("sections").toObject().value("essay").toObject().value("docs").toArray();
            QMultiHash<QString, int> children;
            for (int i = 0; i < nodes.size(); ++i) {
                const auto n = nodes.at(i).toObject();
                if (!n.value("deleted").toBool()) children.insert(n.value("parent").toString(), i);
            }
            QSet<QString> seen;
            const auto insert = [&](auto &&recurse, QTreeWidgetItem *parent, const QString &id, int depth) -> void {
                if (depth > 64) return;
                auto indexes = children.values(id);
                std::sort(indexes.begin(), indexes.end());
                for (const int index : indexes) {
                    const auto node = nodes.at(index).toObject();
                    const auto key = node.value("id").toString();
                    if (key.isEmpty() || seen.contains(key)) continue;
                    seen.insert(key);
                    auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tutorialTree);
                    item->setText(0, node.value("title").toString());
                    item->setData(0, Qt::UserRole, key);
                    if (node.value("kind") == "folder") recurse(recurse, item, key, depth + 1);
                    else item->setData(0, Qt::UserRole + 1, node.value("slug").toString());
                }
            };
            insert(insert, nullptr, {}, 0);
            tutorialTree->expandToDepth(0);
            openRequested();
        });
    }

    void showPostMessage(const QString &message)
    {
        postMessage->setText(message);
        postPreview->setCurrentWidget(postMessage);
    }

    void loadPost(qint64 id, bool refresh = false)
    {
        if (id <= 0 || !leaveComposer()) return;
        if (postId == id && !refresh && (loadingPost || !currentPost.isEmpty())) return;
        postId = id;
        loadingPost = true;
        currentPost = {};
        comments = {};
        const auto generation = ++postRequest;
        updateControls();
        showPostMessage(QStringLiteral("正在加载帖子…"));
        api(siteUrl(QString("/api/forum/posts/%1").arg(id)), [this, generation](const QJsonObject &data, const QString &error) {
            if (generation != postRequest) return;
            loadingPost = false;
            if (!error.isEmpty()) { showPostMessage(error); return; }
            currentPost = data.value("post").toObject();
            comments = data.value("comments").toArray();
            renderPost();
            updateControls();
        });
    }

    void renderPost()
    {
        const auto title = currentPost.value("title").toString();
        const auto meta = QStringLiteral("%1  ·  %2  ·  %3  ·  %4 次浏览  ·  %5 条回复")
            .arg(currentPost.value("board_name").toString(), currentPost.value("username").toString(),
                 localDate(currentPost, "created_at")).arg(currentPost.value("views").toInt()).arg(comments.size());
        QJsonArray blocks{QJsonObject{{"title", title}, {"meta", meta}, {"markdown", currentPost.value("content")}}};
        for (const auto &entry : comments) {
            const auto comment = entry.toObject();
            blocks.append(QJsonObject{{"title", comment.value("username")}, {"meta", localDate(comment, "created_at")},
                                      {"markdown", comment.value("content")}});
        }
        postPreview->setCurrentWidget(body);
        body->showDocument({{"blocks", blocks}, {"base", postUrl().toString()}, {"key", QString::number(postId)},
                            {"dark", vsr::rendererSettings().value("appearance/dark", false).toBool()}});
    }

    void closePost()
    {
        if (!leaveComposer()) return;
        ++postRequest;
        postId = 0;
        loadingPost = false;
        currentPost = {};
        comments = {};
        updateControls();
        showPostMessage(QStringLiteral("从左侧选择帖子，或发布新话题。"));
    }

    bool moderator() const
    {
        const auto role = accountInfo.value("role").toString();
        return loggedIn && (role == "admin" || role == "forum" || role == "editor_forum");
    }

    bool canDelete(const QJsonObject &item) const
    {
        return moderator() || (loggedIn && item.value("user_id").toInteger() == accountInfo.value("id").toInteger()
            && accountInfo.value("id").toInteger() > 0);
    }

    void updateControls()
    {
        const bool ready = !currentPost.isEmpty();
        const bool hasSelection = postId != 0;
        back->setVisible(hasSelection);
        exportButton->setVisible(hasSelection);
        exportButton->setEnabled(ready);
        replyButton->setVisible(hasSelection);
        replyButton->setEnabled(ready);
        bool canManage = ready && canDelete(currentPost);
        for (const auto &entry : comments) canManage = canManage || canDelete(entry.toObject());
        manageButton->setVisible(canManage);
        if (editor && detail->currentWidget() == composer)
            submit->setText(loggedIn ? (composingReply ? QStringLiteral("发送回复") : QStringLiteral("发布话题"))
                                     : QStringLiteral("登录后发布"));
    }

    void managePost()
    {
        const auto id = postId;
        if (currentPost.isEmpty()) return;
        QMenu menu(page);
        const auto path = QString("/api/forum/posts/%1").arg(id);
        if (moderator()) {
            const auto value = currentPost.value("pinned");
            const bool pinned = value.toBool() || value.toInt();
            menu.addAction(pinned ? QStringLiteral("取消置顶") : QStringLiteral("置顶"), page, [this, id, path, pinned] {
                api(siteUrl(path + "/moderation"), [this, id](const QJsonObject &, const QString &error) {
                    notice(error);
                    if (error.isEmpty()) { if (postId == id) loadPost(id, true); loadTopics(); }
                }, {{"action", pinned ? "unpin" : "pin"}}, "PUT");
            });
        }
        if (canDelete(currentPost)) menu.addAction(QStringLiteral("移入回收站…"), page, [this, id, path] {
            if (QMessageBox::question(page, QStringLiteral("移入回收站"), QStringLiteral("将这篇帖子移入论坛回收站？"))
                != QMessageBox::Yes) return;
            api(siteUrl(path), [this, id](const QJsonObject &, const QString &error) {
                notice(error);
                if (error.isEmpty()) { if (postId == id) closePost(); loadTopics(); }
            }, {}, "DELETE");
        });
        QMenu *removeReply = nullptr;
        for (const auto &entry : comments) {
            const auto comment = entry.toObject();
            if (!canDelete(comment)) continue;
            if (!removeReply) removeReply = menu.addMenu(QStringLiteral("删除回复"));
            auto title = QStringLiteral("%1 · %2").arg(comment.value("username").toString(),
                comment.value("content").toString().simplified().left(52));
            removeReply->addAction(title.replace("&", "&&"), page, [this, id, comment] {
                if (QMessageBox::question(page, QStringLiteral("删除回复"), QStringLiteral("永久删除选中的回复？"))
                    != QMessageBox::Yes) return;
                api(siteUrl(QString("/api/forum/comments/%1").arg(comment.value("id").toInteger())),
                    [this, id](const QJsonObject &, const QString &error) {
                        notice(error);
                        if (error.isEmpty()) { if (postId == id) loadPost(id, true); loadTopics(); }
                    }, {}, "DELETE");
            });
        }
        menu.exec(manageButton->mapToGlobal(QPoint(0, manageButton->height())));
    }

    QString threadMarkdown() const
    {
        auto text = QString("# %1\n\n%2\n\n").arg(currentPost.value("title").toString(),
                                                  currentPost.value("content").toString());
        for (const auto &entry : comments) {
            const auto comment = entry.toObject();
            text += QString("## %1 · %2\n\n%3\n\n").arg(comment.value("username").toString(),
                localDate(comment, "created_at"), comment.value("content").toString());
        }
        return text;
    }

    void exportThread()
    {
        if (currentPost.isEmpty()) return;
        QMenu menu(page);
        menu.addAction(QStringLiteral("复制整帖 Markdown"), page, [this] { QApplication::clipboard()->setText(threadMarkdown()); });
        menu.addAction(QStringLiteral("导出 Markdown…"), page, [this] {
            const auto path = QFileDialog::getSaveFileName(page, QStringLiteral("导出帖子"), QString("forum-%1.md").arg(postId),
                                                          QStringLiteral("Markdown (*.md)"));
            if (path.isEmpty()) return;
            QSaveFile file(path);
            const auto data = threadMarkdown().toUtf8();
            if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) notice(file.errorString());
        });
        menu.addAction(QStringLiteral("导出 PDF…"), page, [this] {
            const auto path = QFileDialog::getSaveFileName(page, QStringLiteral("导出帖子"), QString("forum-%1.pdf").arg(postId),
                                                          QStringLiteral("PDF (*.pdf)"));
            if (!path.isEmpty()) body->exportDocumentPdf(path, [this](bool ok) {
                notice(ok ? QStringLiteral("PDF 已保存。") : QStringLiteral("无法导出 PDF，请等待帖子渲染完成。"));
            });
        });
        menu.exec(exportButton->mapToGlobal(QPoint(0, exportButton->height())));
    }

    QString draftPath() const
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath("community-post-draft.json");
    }

    void saveDraft()
    {
        if (!editor || detail->currentWidget() != composer) return;
        if (composingReply) {
            replies.insert(postId, editor->markdown());
            return;
        }
        const auto path = draftPath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile file(path);
        const auto data = QJsonDocument(QJsonObject{{"title", composeTitle->text()},
            {"board", composeBoard->currentData().toString()}, {"content", editor->markdown()}}).toJson(QJsonDocument::Compact);
        if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size()) file.commit();
    }

    void buildComposer()
    {
        composer = new QWidget(detail);
        auto *layout = new QVBoxLayout(composer);
        layout->setContentsMargins(12, 2, 0, 0);
        auto *row = new QHBoxLayout;
        layout->addLayout(row);
        button(row, QStringLiteral("← 返回阅读"), [this] { leaveComposer(); });
        composeLabel = label(composer);
        row->addWidget(composeLabel, 1);
        submit = button(row, QStringLiteral("发布话题"), [this] { submitComposer(); });
        submit->setObjectName("communitySubmitPost");
        auto *fields = new QHBoxLayout;
        layout->addLayout(fields);
        composeBoard = new QComboBox(composer);
        composeBoard->setObjectName("communityComposeBoard");
        fields->addWidget(composeBoard);
        composeTitle = new QLineEdit(composer);
        composeTitle->setObjectName("communityComposeTitle");
        composeTitle->setMaxLength(120);
        composeTitle->setPlaceholderText(QStringLiteral("帖子标题"));
        fields->addWidget(composeTitle, 1);
        editor = new CommunityMarkdownEditor([this](const QString &name, const QByteArray &data, auto done) {
            uploadImage(name, data, std::move(done));
        }, composer);
        layout->addWidget(editor, 1);
        draftTimer = new QTimer(composer);
        draftTimer->setSingleShot(true);
        draftTimer->setInterval(450);
        QObject::connect(draftTimer, &QTimer::timeout, page, [this] { saveDraft(); });
        editor->onChange([this] { draftTimer->start(); });
        QObject::connect(composeTitle, &QLineEdit::textChanged, page, [this] { draftTimer->start(); });
        QObject::connect(composeBoard, &QComboBox::currentIndexChanged, page, [this] { draftTimer->start(); });
        detail->addWidget(composer);
    }

    void fillComposeBoards(const QString &preferred)
    {
        const QSignalBlocker guard(composeBoard);
        composeBoard->clear();
        int match = -1;
        for (const auto &entry : taxonomy) {
            const auto sub = entry.toObject();
            const auto slug = sub.value("slug").toString();
            composeBoard->addItem(sub.value("name").toString() + QStringLiteral(" · ") + slug, slug);
            if (slug == preferred || (match < 0 && slug.startsWith(preferred + '.'))) match = composeBoard->count() - 1;
        }
        if (match >= 0) composeBoard->setCurrentIndex(match);
    }

    void compose(bool replying)
    {
        if (replying && currentPost.isEmpty()) return;
        if (!leaveComposer()) return;
        if (!editor) buildComposer();
        composingReply = replying;
        composeBoard->setVisible(!replying);
        composeTitle->setVisible(!replying);
        composeLabel->setText(replying ? QStringLiteral("回复 %1").arg(currentPost.value("title").toString())
                                       : QStringLiteral("发布新话题"));
        if (replying) {
            editor->setMarkdown(replies.value(postId));
        } else {
            QFile source(draftPath());
            const auto draft = source.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(source.read(8 * 1024 * 1024)).object() : QJsonObject();
            fillComposeBoards(draft.value("board").toString(board));
            composeTitle->setText(draft.value("title").toString());
            editor->setMarkdown(draft.value("content").toString());
        }
        editor->setStatus({});
        detail->setCurrentWidget(composer);
        updateControls();
    }

    bool leaveComposer()
    {
        if (!editor || detail->currentWidget() != composer) return true;
        if (sending || editor->uploading()) {
            editor->setStatus(QStringLiteral("请等待发送或上传完成。"));
            return false;
        }
        draftTimer->stop();
        saveDraft();
        detail->setCurrentWidget(reading);
        return true;
    }

    void submitComposer()
    {
        if (sending || editor->uploading()) return;
        if (!loggedIn) { saveDraft(); showAccount(); return; }
        const auto content = editor->markdown().trimmed();
        const auto title = composeTitle->text().trimmed();
        const auto boardSlug = composeBoard->currentData().toString();
        const int limit = composingReply ? 10000 : 1000000;
        if (content.size() > limit) {
            editor->setStatus(QStringLiteral("正文超过源站允许的 %1 字符。").arg(limit));
            return;
        }
        if (content.isEmpty() || (!composingReply && (title.isEmpty() || boardSlug.isEmpty()))) {
            editor->setStatus(QStringLiteral("请填写正文。发布新话题还需填写标题并选择板块。"));
            return;
        }
        const bool reply = composingReply;
        const auto id = postId;
        const auto path = reply ? QString("/api/forum/posts/%1/comments").arg(id) : QString("/api/forum/posts");
        const auto payload = reply ? QJsonObject{{"content", content}}
                                   : QJsonObject{{"title", title}, {"board", boardSlug}, {"content", content}};
        sending = true;
        submit->setEnabled(false);
        composeTitle->setEnabled(false);
        composeBoard->setEnabled(false);
        editor->setLocked(true);
        editor->setStatus(QStringLiteral("正在发送…"));
        api(siteUrl(path), [this, reply, id](const QJsonObject &data, const QString &error) {
            sending = false;
            submit->setEnabled(true);
            composeTitle->setEnabled(true);
            composeBoard->setEnabled(true);
            editor->setLocked(false);
            if (!error.isEmpty()) { editor->setStatus(error); checkSession(); return; }
            detail->setCurrentWidget(reading);
            draftTimer->stop();
            if (reply) replies.remove(id);
            else QFile::remove(draftPath());
            loadPost(reply ? id : data.value("id").toInteger(), true);
            loadBoards();
            loadTopics();
            notice(reply ? QStringLiteral("回复已发送。") : QStringLiteral("话题已发布。"));
        }, payload, "POST");
    }

    void uploadImage(const QString &name, const QByteArray &bytes,
                     std::function<void(const QUrl &, const QString &)> done)
    {
        if (!loggedIn) { done({}, QStringLiteral("请先登录，再上传图片。")); return; }
        const auto mime = QMimeDatabase().mimeTypeForData(bytes);
        if (!mime.name().startsWith("image/") && mime.name() != "application/octet-stream") {
            done({}, QStringLiteral("请选择有效的图片文件。"));
            return;
        }
        const auto suffix = QFileInfo(name).suffix().toLower();
        const QStringList supported{"png", "jpeg", "jpg", "gif", "webp", "avif"};
        const auto fileName = supported.contains(suffix) ? "image." + suffix : QString("image.png");
        QHttpPart file;
        file.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"file[]\"; filename=\"%1\"").arg(fileName));
        file.setHeader(QNetworkRequest::ContentTypeHeader, mime.name());
        file.setBody(bytes);
        auto *form = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        form->append(file);
        api(siteUrl("/api/forum/uploads"), [done = std::move(done)](const QJsonObject &data, const QString &error) {
            if (!error.isEmpty()) { done({}, error); return; }
            const auto files = data.value("data").toObject().value("succMap").toObject();
            const auto url = files.isEmpty() ? QUrl() : CommunityWebView::home().resolved(QUrl(files.begin().value().toString()));
            if (!url.isValid() || url.scheme() != "https" || !url.userInfo().isEmpty())
                done({}, QStringLiteral("源站未接受该图片。"));
            else done(url, {});
        }, {}, "POST", form);
    }

    void setUser(const QJsonObject &session)
    {
        loggedIn = session.value("authenticated").toBool();
        if (accountInfo.value("id") != session.value("user").toObject().value("id")) {
            adminCookie.clear();
            adminCsrf.clear();
        }
        accountInfo = session.value("user").toObject();
        user = accountInfo.value("username").toString();
        csrf = session.value("csrf").toString();
        account->setText(loggedIn ? QStringLiteral("账号 · %1").arg(user) : QStringLiteral("登录"));
        updateControls();
    }

    void checkSession()
    {
        const auto generation = ++accountRequest;
        const auto existing = cookie;
        api(siteUrl("/api/forum/auth/session"), [this, generation, existing](const QJsonObject &data, const QString &error) {
            if (generation != accountRequest || existing != cookie || !error.isEmpty()) return;
            setUser(data);
            if (!loggedIn && !cookie.isEmpty()) {
                cookie.clear();
                web->clearSession();
                if (loginWeb) loginWeb->clearSession();
            }
            saveSession();
            if (loggedIn && loginDialog) loginDialog->accept();
        });
    }

    void saveSession()
    {
        auto settings = vsr::rendererSettings();
        if (!loggedIn || !settings.value(rememberLogin, false).toBool() || cookie.isEmpty()) {
            settings.remove(rememberedSession);
            return;
        }
        const auto encrypted = protectSession(cookie, true);
        if (encrypted.isEmpty()) notice(QStringLiteral("Windows 无法加密论坛会话，本次登录只保留在内存中。"));
        else settings.setValue(rememberedSession, encrypted);
    }

    void selectTab(int index)
    {
        notice({});
        if (index == Forum) {
            screens->setCurrentWidget(forum);
            if (postId && detail->currentWidget() == reading && currentPost.isEmpty()) loadPost(postId);
            loadTopics();
            checkSession();
        } else if (index == Docs) {
            openTutorials();
        } else if (index == Presets) {
            if (!presets) {
                presets = new PresetCommunityDialog(importPreset, screens);
                screens->addWidget(presets);
            }
            screens->setCurrentWidget(presets);
        } else if (index == About) {
            QMenu menu(page);
            for (const auto &entry : {std::pair{"关于站长", "/about"}, {"友链", "/friends"}, {"开源与引用", "/open-source"}})
                menu.addAction(QString::fromUtf8(entry.first), page, [this, path = entry.second] { openWeb(siteUrl(path)); });
            const auto choice = menu.exec(tabs->mapToGlobal(tabs->tabRect(About).bottomLeft()));
            if (!choice) tabs->setCurrentIndex(Forum);
        } else if (index == Download) {
            openWeb(siteUrl("/download"));
            api(siteUrl("/api/site-config"), [this](const QJsonObject &data, const QString &) {
                if (tabs->currentIndex() != Download || screens->currentWidget() != webPanel) return;
                const QUrl link(data.value("site").toObject().value("download_url").toString());
                if (CommunityWebView::isEmbeddedSite(link)) openWeb(link);
            });
        } else {
            const QStringList paths{"/", "/docs", "/resources", "/download", "/tools"};
            openWeb(siteUrl(paths.at(index)));
        }
    }

    void openWeb(const QUrl &url)
    {
        screens->setCurrentWidget(webPanel);
        web->open(url, cookie);
    }

    void showAccount()
    {
        if (loggedIn) { openWeb(siteUrl("/account")); return; }
        if (loginDialog) { loginDialog->raise(); return; }
        auto *dialog = new QDialog(page);
        loginDialog = dialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setObjectName("communityLoginDialog");
        dialog->setWindowTitle(QStringLiteral("登录论坛"));
        dialog->setWindowModality(Qt::WindowModal);
        dialog->resize(400, 240);
        auto *outer = new QVBoxLayout(dialog);
        auto *formWidget = new QWidget(dialog);
        outer->addWidget(formWidget);
        auto *form = new QFormLayout(formWidget);
        auto *name = new QLineEdit(formWidget);
        name->setObjectName("communityLoginName");
        name->setMaxLength(254);
        auto *password = new QLineEdit(formWidget);
        password->setObjectName("communityLoginPassword");
        password->setEchoMode(QLineEdit::Password);
        password->setMaxLength(2048);
        form->addRow(QStringLiteral("用户名或邮箱"), name);
        auto *passwordRow = new QHBoxLayout;
        passwordRow->addWidget(password, 1);
        auto *visibility = new QPushButton(QStringLiteral("显示密码"), formWidget);
        visibility->setObjectName("communityLoginShowPassword");
        visibility->setCheckable(true);
        visibility->setAutoDefault(false);
        passwordRow->addWidget(visibility);
        form->addRow(QStringLiteral("密码"), passwordRow);
        QObject::connect(visibility, &QPushButton::toggled, dialog, [password, visibility](bool show) {
            password->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
            visibility->setText(show ? QStringLiteral("隐藏密码") : QStringLiteral("显示密码"));
        });
        auto *remember = new QCheckBox(QStringLiteral("记住论坛登录"), formWidget);
        remember->setChecked(vsr::rendererSettings().value(rememberLogin, false).toBool());
        form->addRow(remember);
        auto *feedback = label(formWidget);
        feedback->setObjectName("communityLoginStatus");
        form->addRow(feedback);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, formWidget);
        auto *connectButton = buttons->addButton(QStringLiteral("登录"), QDialogButtonBox::AcceptRole);
        connectButton->setObjectName("communityLoginSubmit");
        connectButton->setDefault(true);
        auto *other = buttons->addButton(QStringLiteral("邮箱 / 第三方"), QDialogButtonBox::ActionRole);
        auto *registration = buttons->addButton(QStringLiteral("注册账号"), QDialogButtonBox::ActionRole);
        registration->setObjectName("communityLoginRegister");
        registration->setAutoDefault(false);
        form->addRow(buttons);
        QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
        const QPointer<QDialog> guard(dialog);
        const auto submitLogin = [this, guard, name, password, remember, feedback, connectButton, other, registration] {
            if (!connectButton->isEnabled()) return;
            const auto username = name->text().trimmed();
            if (username.isEmpty() || password->text().isEmpty()) {
                feedback->setText(QStringLiteral("请输入用户名或邮箱和密码。"));
                return;
            }
            connectButton->setEnabled(false);
            other->setEnabled(false);
            registration->setEnabled(false);
            feedback->setText(QStringLiteral("正在登录…"));
            const auto credentials = QJsonObject{{"username", username}, {"password", password->text()}};
            password->clear();
            api(siteUrl("/api/forum/auth/login"), {}, credentials, "POST", nullptr,
                [this, guard, password, remember, feedback, connectButton, other, registration](
                    const QJsonObject &data, const QByteArray &session, const QString &error) {
                    if (!guard || !guard->isVisible()) return;
                    if (!error.isEmpty()) {
                        feedback->setText(error);
                        connectButton->setEnabled(true);
                        other->setEnabled(true);
                        registration->setEnabled(true);
                        password->setFocus();
                        return;
                    }
                    ++accountRequest;
                    cookie = session;
                    web->setSession(cookie);
                    vsr::rendererSettings().setValue(rememberLogin, remember->isChecked());
                    setUser({{"authenticated", true}, {"user", data.value("user")}, {"csrf", data.value("csrf")}});
                    saveSession();
                    guard->accept();
                });
        };
        QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, submitLogin);
        QObject::connect(password, &QLineEdit::returnPressed, dialog, submitLogin);
        const auto openWebAuth = [this, dialog, formWidget, outer, remember, password, visibility](bool registerAccount) {
            password->clear();
            visibility->setChecked(false);
            formWidget->hide();
            const bool keep = remember->isChecked();
            auto *alternate = new QWidget(dialog);
            auto *layout = new QVBoxLayout(alternate);
            layout->setContentsMargins(0, 0, 0, 0);
            loginWeb = new CommunityWebView([this, keep](const QByteArray &session) {
                if (checkedSession(session).isEmpty()) return;
                vsr::rendererSettings().setValue(rememberLogin, keep);
                acceptWebSession(session);
            }, alternate);
            layout->addWidget(loginWeb, 1);
            auto *back = new QPushButton(QStringLiteral("返回密码登录"), alternate);
            layout->addWidget(back, 0, Qt::AlignRight);
            outer->addWidget(alternate, 1);
            QObject::connect(back, &QPushButton::clicked, dialog, [dialog, formWidget, alternate] {
                alternate->hide();
                alternate->deleteLater();
                formWidget->show();
                dialog->resize(400, 240);
            });
            dialog->resize(560, 650);
            loginWeb->open(siteUrl(registerAccount ? "/forum?vsr_login=register" : "/forum?vsr_login=1"), cookie);
        };
        QObject::connect(other, &QPushButton::clicked, dialog, [openWebAuth] { openWebAuth(false); });
        QObject::connect(registration, &QPushButton::clicked, dialog, [openWebAuth] { openWebAuth(true); });
        dialog->open();
        name->setFocus();
    }

    void acceptWebSession(const QByteArray &session)
    {
        const auto value = checkedSession(session);
        if (value == cookie) return;
        cookie = value;
        setUser({});
        web->setSession(cookie);
        vsr::rendererSettings().remove(rememberedSession);
        checkSession();
    }

    void accountMenu()
    {
        if (!loggedIn) { showAccount(); return; }
        QMenu menu(page);
        auto *identity = menu.addAction(QStringLiteral("%1  ·  %2").arg(user, accountInfo.value("role").toString()));
        identity->setEnabled(false);
        if (accountInfo.value("role").toString() == "admin")
            menu.addAction(QStringLiteral("帖子回收站…"), page, [this] { openRecycle(); });
        menu.addAction(QStringLiteral("网站账号设置…"), page, [this] { showAccount(); });
        menu.addSeparator();
        menu.addAction(QStringLiteral("退出论坛账号"), page, [this] {
            api(siteUrl("/api/forum/auth/logout"), [this](const QJsonObject &, const QString &error) {
                if (!error.isEmpty()) { notice(error); checkSession(); return; }
                page->forgetSession();
                notice(QStringLiteral("已退出论坛账号。"));
            }, {}, "POST");
        });
        menu.exec(account->mapToGlobal(QPoint(0, account->height())));
    }

    void openRecycle()
    {
        if (!adminCookie.isEmpty()) { recycleDialog(); return; }
        bool accepted = false;
        const auto password = QInputDialog::getText(page, QStringLiteral("站点后台管理员校验"),
            QStringLiteral("回收站使用独立的后台会话。请输入 %1 的网站密码。").arg(user),
            QLineEdit::Password, {}, &accepted);
        if (!accepted || password.isEmpty()) return;
        const auto accountId = accountInfo.value("id");
        api(siteUrl("/api/admin/login"), [this, accountId](const QJsonObject &, const QString &error) {
            if (accountInfo.value("id") != accountId) { adminCookie.clear(); adminCsrf.clear(); return; }
            if (!error.isEmpty()) { notice(error); return; }
            recycleDialog();
        }, {{"username", user}, {"password", password}}, "POST");
    }

    void recycleDialog()
    {
        auto *dialog = new QDialog(page);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setObjectName("communityRecycle");
        dialog->setWindowTitle(QStringLiteral("论坛帖子回收站"));
        dialog->resize(760, 460);
        auto *layout = new QVBoxLayout(dialog);
        auto *records = new QTreeWidget(dialog);
        records->setHeaderLabels({QStringLiteral("帖子"), QStringLiteral("板块"), QStringLiteral("删除时间")});
        records->setRootIsDecorated(false);
        records->setColumnWidth(0, 340);
        layout->addWidget(records, 1);
        auto *feedback = label(dialog);
        layout->addWidget(feedback);
        auto *row = new QHBoxLayout;
        layout->addLayout(row);
        const QPointer<QDialog> guard(dialog);
        const auto refresh = [this, guard, records, feedback] {
            feedback->setText(QStringLiteral("正在读取回收站…"));
            api(siteUrl("/api/admin/forum/recycle"), [guard, records, feedback](const QJsonObject &data, const QString &error) {
                if (!guard) return;
                records->clear();
                if (!error.isEmpty()) { feedback->setText(error); return; }
                for (const auto &entry : data.value("posts").toArray()) {
                    const auto item = entry.toObject();
                    auto *record = new QTreeWidgetItem(records, {item.value("title").toString(),
                        item.value("board_name").toString(), localDate(item, "deleted_at")});
                    record->setData(0, Qt::UserRole, item.value("id").toInteger());
                }
                feedback->setText(QStringLiteral("%1 篇帖子").arg(records->topLevelItemCount()));
            });
        };
        auto *reload = new QPushButton(QStringLiteral("刷新"), dialog);
        row->addWidget(reload);
        QObject::connect(reload, &QPushButton::clicked, dialog, refresh);
        for (const bool permanent : {false, true}) {
            auto *action = new QPushButton(permanent ? QStringLiteral("永久删除…") : QStringLiteral("恢复帖子"), dialog);
            row->addWidget(action);
            QObject::connect(action, &QPushButton::clicked, dialog, [this, guard, records, feedback, action, refresh, permanent] {
                const auto *selected = records->currentItem();
                if (!selected) return;
                const auto id = selected->data(0, Qt::UserRole).toLongLong();
                if (permanent && QMessageBox::question(guard, QStringLiteral("永久删除"),
                        QStringLiteral("永久删除选中的帖子及其回复？")) != QMessageBox::Yes) return;
                action->setEnabled(false);
                api(siteUrl(QString("/api/admin/forum/recycle/%1").arg(id)),
                    [this, guard, feedback, action, refresh](const QJsonObject &, const QString &error) {
                        if (!guard) return;
                        action->setEnabled(true);
                        if (!error.isEmpty()) { feedback->setText(error); return; }
                        refresh();
                        loadBoards();
                        loadTopics();
                    }, {}, permanent ? "DELETE" : "PUT");
            });
        }
        row->addStretch();
        dialog->open();
        refresh();
    }
};

CommunityPage::CommunityPage(ImportPreset importPreset, QWidget *parent,
                             std::unique_ptr<QNetworkAccessManager> transport)
    : QWidget(parent), impl_(std::make_unique<Impl>(this, std::move(importPreset), std::move(transport)))
{
    setObjectName("rendererCommunityPage");
}

CommunityPage::~CommunityPage()
{
    impl_->saveDraft();
    // Browser callbacks can arrive while the controller is closing.
    delete impl_->loginDialog.data();
    delete impl_->web;
}

void CommunityPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (impl_->initialized) return;
    impl_->initialized = true;
    QTimer::singleShot(0, this, [this] {
        impl_->loadBoards();
        impl_->loadTopics();
        impl_->checkSession();
    });
}

void CommunityPage::openAccount() { impl_->accountMenu(); }

void CommunityPage::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (impl_ && event->type() == QEvent::StyleChange) {
        const auto dark = vsr::rendererSettings().value("appearance/dark", false).toBool();
        impl_->body->documentTheme(dark);
        if (impl_->tutorialBody) impl_->tutorialBody->documentTheme(dark);
    }
}

void CommunityPage::setRememberLogin(bool enabled)
{
    vsr::rendererSettings().setValue(rememberLogin, enabled);
    impl_->saveSession();
}

void CommunityPage::forgetSession()
{
    vsr::rendererSettings().remove(rememberedSession);
    ++impl_->accountRequest;
    impl_->cookie.clear();
    impl_->web->clearSession();
    if (impl_->loginWeb) impl_->loginWeb->clearSession();
    impl_->setUser({});
    impl_->notice(QStringLiteral("已清除本地论坛会话。"));
}

void CommunityPage::resetPresetSource()
{
    if (!impl_->presets) return;
    const bool active = impl_->screens->currentWidget() == impl_->presets;
    delete impl_->presets;
    impl_->presets = nullptr;
    if (active) impl_->selectTab(Impl::Presets);
}

}
