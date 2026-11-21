#include "ui/AnalysisPage.h"
#include "ui/CommunityPage.h"
#include "ui/ExportWindow.h"
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QTreeWidgetItemIterator>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QDialog>
#include <QTimer>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QListWidget>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include "app/MainWindow.h"
#include "app/RenderRoute.h"
#include "app/Style.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/FilterGraph.h"
#include "graph/PresetStore.h"
#include "ui/PresetDialog.h"
#include "ui/PresetCommunityDialog.h"
#include "ui/CompareView.h"
#include "ui/ParameterEditor.h"
#include "ui/PageTheme.h"
#include <QScopeGuard>
#include <QStackedWidget>
#include "graph/FilterCatalog.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QFont>
#include <QGridLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTabBar>
#include <QScopeGuard>
#include <QSettings>
#include <QStackedWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QUuid>
#include "graph/VpyScriptBuilder.h"
#include <QSignalSpy>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QMouseEvent>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QProcess>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QSplitter>
#include <QStandardPaths>
#include <QFileInfo>
#include <QTest>
#include <QToolButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <cstring>
#include <windows.h>

using namespace vsr;

namespace {
class CommunityFixtureReply final : public QNetworkReply {
public:
    CommunityFixtureReply(const QNetworkRequest &request, const QJsonObject &value, int status, QObject *parent,
                          const QList<QNetworkCookie> &cookies = {})
        : QNetworkReply(parent), body_(QJsonDocument(value).toJson(QJsonDocument::Compact))
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!cookies.isEmpty()) setHeader(QNetworkRequest::SetCookieHeader, QVariant::fromValue(cookies));
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QTimer::singleShot(0, this, [this] {
            setFinished(true);
            emit readyRead();
            emit finished();
        });
    }
    qint64 bytesAvailable() const override { return body_.size() - offset_ + QNetworkReply::bytesAvailable(); }
    void abort() override {}

protected:
    qint64 readData(char *destination, qint64 maximum) override
    {
        const auto count = std::min(maximum, body_.size() - offset_);
        if (!count) return -1;
        std::memcpy(destination, body_.constData() + offset_, count);
        offset_ += count;
        return count;
    }

private:
    QByteArray body_;
    qint64 offset_ = 0;
};

class CommunityFixtureNetwork final : public QNetworkAccessManager {
public:
    QJsonObject published, postedReply;
    QByteArray moderatorAction;
    bool rejectFirst = true;
    int publishAttempts = 0;
    QByteArray writeCsrf;
    bool recycleRead = false;
    bool restored = false;
    bool requireLogin = false;
    bool passwordAuthenticated = false;
    int detailReads = 0;

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoing) override
    {
        const auto method = request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
        const auto path = request.url().path();
        const QJsonObject board{{"slug", "dev.test"}, {"name", "Fixture"}, {"post_count", 1}};
        const QJsonObject post{{"id", 42}, {"user_id", 7}, {"title", "Fixture post"},
            {"username", "Fixture admin"}, {"content", "Formula $x^2$"}, {"pinned", 0}};
        const auto payload = outgoing ? QJsonDocument::fromJson(outgoing->readAll()).object() : QJsonObject();
        const auto reply = [&](const QJsonObject &value, int status = 200) {
            return new CommunityFixtureReply(request, value, status, this);
        };
        if (operation == CustomOperation && method != "GET") writeCsrf = request.rawHeader("X-Forum-CSRF");
        if (path == "/api/forum/auth/session" && requireLogin && !passwordAuthenticated)
            return reply({{"authenticated", false}});
        if (path == "/api/forum/auth/session")
            return reply({{"authenticated", true}, {"csrf", "fixture-csrf"},
                {"user", QJsonObject{{"id", 7}, {"username", "Fixture admin"}, {"role", "admin"}}}});
        if (path == "/api/forum/auth/login" && method == "POST") {
            if (payload.value("username") != QJsonValue("login-fixture") || payload.value("password") != QJsonValue("fixture-only"))
                return reply({{"error", "Fixture authentication rejected"}}, 401);
            passwordAuthenticated = true;
            return new CommunityFixtureReply(request, {{"ok", true}, {"csrf", "fixture-csrf"},
                {"user", QJsonObject{{"id", 7}, {"username", "Fixture admin"}, {"role", "admin"}}}}, 200, this,
                {QNetworkCookie("forum_session", "Fixture_native_login_1234567890ABCDE")});
        }
        if (path == "/api/forum/boards") return reply({{"boards", QJsonArray{board}}});
        auto otherPost = post;
        otherPost["id"] = 43;
        otherPost["user_id"] = 99;
        otherPost["title"] = "Second fixture post";
        if (path == "/api/forum/posts" && method == "GET") return reply({{"posts", QJsonArray{post, otherPost}}});
        if (path == "/api/forum/posts" && method == "POST") {
            ++publishAttempts;
            if (rejectFirst) { rejectFirst = false; return reply({{"error", "Fixture rejection"}}, 403); }
            published = payload;
            return reply({{"ok", true}, {"id", 42}}, 201);
        }
        if ((path == "/api/forum/posts/42" || path == "/api/forum/posts/43") && method == "GET") {
            ++detailReads;
            return reply({{"post", path.endsWith("42") ? post : otherPost}, {"comments", QJsonArray()}});
        }
        if (path == "/api/forum/posts/42/comments" && method == "POST") {
            postedReply = payload;
            return reply({{"ok", true}, {"id", 51}}, 201);
        }
        if (path == "/api/forum/posts/42/moderation" && method == "PUT") {
            moderatorAction = payload.value("action").toString().toLatin1();
            return reply({{"ok", true}});
        }
        if (path == "/api/admin/login" && method == "POST" && payload.value("password") == QJsonValue("fixture-only"))
            return new CommunityFixtureReply(request, {{"ok", true}, {"csrf", "fixture-admin-csrf"}}, 200, this,
                {QNetworkCookie("blog_session", "Fixture_only_blog_session_1234567890")});
        if (path.startsWith("/api/admin/forum/recycle") &&
            request.rawHeader("Cookie") == "blog_session=Fixture_only_blog_session_1234567890") {
            if (method == "GET") {
                recycleRead = true;
                return reply({{"posts", restored ? QJsonArray() : QJsonArray{post}}});
            }
            if (method == "PUT" && request.rawHeader("X-Blog-CSRF") == "fixture-admin-csrf") {
                restored = true;
                return reply({{"ok", true}});
            }
        }
        if (path == "/api/site-config") {
            const QJsonArray docs{
                QJsonObject{{"id", "folder"}, {"kind", "folder"}, {"title", "Fixture folder"}},
                QJsonObject{{"id", "guide"}, {"parent", "folder"}, {"kind", "page"}, {"title", "Fixture guide"}, {"slug", "fixture-guide"}},
                QJsonObject{{"id", "removed"}, {"parent", "folder"}, {"kind", "page"}, {"title", "Deleted guide"},
                            {"slug", "removed-guide"}, {"deleted", true}}
            };
            return reply({{"sections", QJsonObject{{"essay", QJsonObject{{"docs", docs}}}}}});
        }
        if (path == "/api/articles/fixture-guide")
            return reply({{"article", QJsonObject{{"title", "Fixture guide"}, {"content", "## First\n\n### Child\n\n## Second"}}}});
        return reply({{"error", "Unexpected fixture request"}}, 404);
    }
};
}

class ViewEventCounter final : public QObject {
public:
    int hides = 0;
    int resizes = 0;
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Hide) ++hides;
        if (event->type() == QEvent::Resize) ++resizes;
        return false;
    }
};

class TestAnalysis final : public QObject {
    Q_OBJECT
private slots:
    void nativeCommunityPostListSplitterStability()
    {
        CommunityPage page([](const QString &, bool) {}, nullptr, std::make_unique<CommunityFixtureNetwork>());
        page.setStyleSheet(applicationStyleSheet(true));
        page.resize(1150, 730);
        page.show();
        auto *topics = page.findChild<QListWidget *>("communityTopics");
        auto *reader = page.findChild<QWidget *>("communityPost");
        QVERIFY(topics && reader);
        QTRY_COMPARE(topics->count(), 2);
        topics->itemClicked(topics->item(0));
        QTRY_VERIFY(reader->isVisible());
        QSplitter *split = nullptr;
        for (auto *candidate : page.findChildren<QSplitter *>()) {
            if (candidate->orientation() == Qt::Horizontal && candidate->widget(0)->isAncestorOf(topics)) {
                split = candidate;
                break;
            }
        }
        QVERIFY(split);
        const auto original = topics->item(0)->text();
        topics->item(1)->setText(QStringLiteral("AI三字经\nVibe Coding · AI 辅助开发 · admin · 0 回复"));
        topics->item(0)->setText(original + QString(90, QLatin1Char('W')));
        split->setSizes({275, 850});
        topics->doItemsLayout();
        QCoreApplication::processEvents();
        QVERIFY(!topics->horizontalScrollBar()->isVisible());
        QVERIFY(topics->visualItemRect(topics->item(0)).right() < topics->viewport()->width());
        const auto rowHeight = topics->visualItemRect(topics->item(1)).height();
        QVERIFY(rowHeight >= 2 * topics->fontMetrics().height());
        split->setSizes({385, 740});
        topics->doItemsLayout();
        QCoreApplication::processEvents();
        QCOMPARE(topics->visualItemRect(topics->item(1)).height(), rowHeight);

        auto *handle = split->handle(1);
        const auto center = handle->rect().center();
        const auto target = center + QPoint(45, 0);
        const auto bodyWidth = reader->width();
        QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, center);
        QMouseEvent move(QEvent::MouseMove, target, handle->mapToGlobal(target),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(handle, &move);
        QCoreApplication::processEvents();
        const auto widthWhileDragging = reader->width();
        QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, target);
        QCOMPARE(widthWhileDragging, bodyWidth);
        QTRY_VERIFY(reader->width() < bodyWidth);
    }

    void nativeCommunityReplyToolbarStability()
    {
        auto fixture = std::make_unique<CommunityFixtureNetwork>();
        fixture->requireLogin = true;
        auto *network = fixture.get();
        CommunityPage page([](const QString &, bool) {}, nullptr, std::move(fixture));
        page.resize(1150, 730);
        page.show();
        auto *topics = page.findChild<QListWidget *>("communityTopics");
        auto *reply = page.findChild<QPushButton *>("communityReply");
        QVERIFY(topics && reply);
        QTRY_COMPARE(topics->count(), 2);
        topics->itemClicked(topics->item(0));
        QTRY_VERIFY(reply->isVisible() && reply->isEnabled());
        const auto position = reply->geometry();
        ViewEventCounter events;
        reply->installEventFilter(&events);

        topics->itemClicked(topics->item(1));
        QVERIFY(reply->isVisible());
        QVERIFY(!reply->isEnabled());
        topics->itemActivated(topics->item(1));
        QCOMPARE(network->detailReads, 2);
        QTRY_VERIFY(reply->isEnabled());
        QCOMPARE(events.hides, 0);
        QCOMPARE(reply->geometry(), position);
        const auto toolbarButtons = reply->parentWidget()->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY(toolbarButtons.size() >= 5);
        QCOMPARE(reply->font().family(), QStringLiteral("Microsoft YaHei UI"));
        for (const auto *button : toolbarButtons)
            QCOMPARE(button->font(), reply->font());
    }

    void nativeCommunityLoginAndRichEditor()
    {
        const auto organization = QCoreApplication::organizationName();
        const auto application = QCoreApplication::applicationName();
        const bool testMode = QStandardPaths::isTestModeEnabled();
        QCoreApplication::setOrganizationName("VSRendererTests");
        QCoreApplication::setApplicationName("CommunityRichEditorFixture");
        QStandardPaths::setTestModeEnabled(true);
        const auto draft = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath("community-post-draft.json");
        const auto restore = qScopeGuard([&] {
            QFile::remove(draft);
            QSettings().remove("community");
            QCoreApplication::setOrganizationName(organization);
            QCoreApplication::setApplicationName(application);
            QStandardPaths::setTestModeEnabled(testMode);
        });
        auto fixture = std::make_unique<CommunityFixtureNetwork>();
        auto *network = fixture.get();
        fixture->requireLogin = true;
        CommunityPage page([](const QString &, bool) {}, nullptr, std::move(fixture));
        page.resize(1150, 730);
        page.show();
        auto *create = page.findChild<QPushButton *>("communityNewPost");
        QVERIFY(create);
        create->click();
        auto *source = page.findChild<QPlainTextEdit *>("communityMarkdownSource");
        auto *preview = page.findChild<QWidget *>("communityMarkdownWeb");
        QVERIFY(source && preview && source->isVisible() && preview->isVisible());
        auto *split = page.findChild<QSplitter *>("communityLivePreviewSplit");
        QVERIFY(split && split->orientation() == Qt::Horizontal && split->count() == 2);
        source->setPlainText("colored");
        source->selectAll();
        QTimer::singleShot(20, &page, [] {
            if (auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget())) {
                dialog->setCurrentColor(QColor("#123abc"));
                dialog->accept();
            }
        });
        page.findChild<QPushButton *>("communityInsertColor")->click();
        QCOMPARE(source->toPlainText(), QString("{color:#123abc}colored{/color}"));
        source->setPlainText("hidden");
        source->selectAll();
        QTimer::singleShot(20, &page, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "communityBlackoutDialog") return;
            dialog->findChild<QLineEdit *>("communityBlackoutHint")->setText("warning");
            dialog->accept();
        });
        page.findChild<QPushButton *>("communityInsertBlackout")->click();
        QCOMPARE(source->toPlainText(), QString("{blackout:warning}hidden{/blackout}"));
        auto *submit = page.findChild<QPushButton *>("communitySubmitPost");
        submit->click();
        auto *login = page.findChild<QDialog *>("communityLoginDialog");
        QVERIFY(login && login->isVisible());
        login->findChild<QLineEdit *>("communityLoginName")->setText("login-fixture");
        auto *password = login->findChild<QLineEdit *>("communityLoginPassword");
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        auto *visibility = login->findChild<QPushButton *>("communityLoginShowPassword");
        QVERIFY(visibility && login->findChild<QPushButton *>("communityLoginRegister"));
        visibility->click();
        QCOMPARE(password->echoMode(), QLineEdit::Normal);
        visibility->click();
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        password->setText("incorrect");
        auto *signIn = login->findChild<QPushButton *>("communityLoginSubmit");
        signIn->click();
        QTRY_COMPARE(login->findChild<QLabel *>("communityLoginStatus")->text(), QString("Fixture authentication rejected"));
        QVERIFY(password->text().isEmpty());
        login->findChild<QCheckBox *>()->setChecked(true);
        password->setText("fixture-only");
        signIn->click();
        QTRY_VERIFY(network->passwordAuthenticated);
        QTRY_VERIFY(page.findChild<QPushButton *>("communityAccount")->text().contains("Fixture admin"));
        QTRY_VERIFY(!page.findChild<QDialog *>("communityLoginDialog"));
        QVERIFY(source->isVisible() && preview->isVisible());
        QCOMPARE(source->toPlainText(), QString("{blackout:warning}hidden{/blackout}"));
        const auto encrypted = QSettings().value("community/sessionDpapi").toByteArray();
        QVERIFY(!encrypted.isEmpty() && !encrypted.contains("Fixture_native_login"));
    }
    void nativeTutorialNavigation()
    {
        CommunityPage page([](const QString &, bool) {}, nullptr, std::make_unique<CommunityFixtureNetwork>());
        const auto previous = QSettings().value("community/lastTutorial");
        const auto restore = qScopeGuard([&] {
            if (previous.isValid()) QSettings().setValue("community/lastTutorial", previous);
            else QSettings().remove("community/lastTutorial");
        });
        page.resize(1100, 650);
        page.show();
        auto *tabs = page.findChild<QTabBar *>("communityTabs");
        QVERIFY(tabs);
        tabs->setCurrentIndex(1);
        auto *tree = page.findChild<QTreeWidget *>("communityTutorialTree");
        auto *outline = page.findChild<QTreeWidget *>("communityTutorialOutline");
        QVERIFY(tree && outline);
        QTRY_COMPARE(tree->topLevelItemCount(), 1);
        QCOMPARE(tree->topLevelItem(0)->childCount(), 1);
        QCOMPARE(tree->currentItem()->text(0), QString("Fixture guide"));
        QTRY_COMPARE(outline->topLevelItemCount(), 2);
        QCOMPARE(outline->topLevelItem(0)->text(0), QString("First"));
        QCOMPARE(outline->topLevelItem(0)->childCount(), 1);
        QCOMPARE(outline->topLevelItem(0)->child(0)->text(0), QString("Child"));
    }
    void nativeCommunityWriteFlows()
    {
        const auto organization = QCoreApplication::organizationName();
        const auto application = QCoreApplication::applicationName();
        const bool testMode = QStandardPaths::isTestModeEnabled();
        QCoreApplication::setOrganizationName("VSRendererTests");
        QCoreApplication::setApplicationName("CommunityWriteFixture");
        QStandardPaths::setTestModeEnabled(true);
        const auto draft = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath("community-post-draft.json");
        const auto restore = qScopeGuard([&] {
            QFile::remove(draft);
            QCoreApplication::setOrganizationName(organization);
            QCoreApplication::setApplicationName(application);
            QStandardPaths::setTestModeEnabled(testMode);
        });
        auto fixture = std::make_unique<CommunityFixtureNetwork>();
        auto *network = fixture.get();
        CommunityPage page([](const QString &, bool) {}, nullptr, std::move(fixture));
        page.resize(1150, 730);
        page.show();
        auto *account = page.findChild<QPushButton *>("communityAccount");
        QVERIFY(account);
        QTRY_VERIFY(account->text().contains("Fixture admin"));
        auto *create = page.findChild<QPushButton *>("communityNewPost");
        create->click();
        auto *title = page.findChild<QLineEdit *>("communityComposeTitle");
        auto *board = page.findChild<QComboBox *>("communityComposeBoard");
        auto *body = page.findChild<QPlainTextEdit *>("communityMarkdownSource");
        auto *submit = page.findChild<QPushButton *>("communitySubmitPost");
        QVERIFY(title && title->isVisible() && body && body->isVisible() && submit && board);
        QCOMPARE(board->currentData().toString(), QString("dev.test"));
        title->setText("New title");
        body->setPlainText("Draft $x^2$");
        submit->click();
        QTRY_VERIFY(submit->isEnabled());
        QCOMPARE(network->publishAttempts, 1);
        QVERIFY(title->isVisible());
        QCOMPARE(body->toPlainText(), QString("Draft $x^2$"));
        submit->click();
        QTRY_COMPARE(network->published.value("content").toString(), QString("Draft $x^2$"));
        QCOMPARE(network->published.value("board").toString(), QString("dev.test"));
        QCOMPARE(network->published.value("title").toString(), QString("New title"));
        QCOMPARE(network->writeCsrf, QByteArray("fixture-csrf"));
        auto *reply = page.findChild<QPushButton *>("communityReply");
        QTRY_VERIFY(reply && reply->isVisible() && reply->isEnabled());
        reply->click();
        QVERIFY(body->isVisible());
        QVERIFY(!title->isVisible());
        body->setPlainText("Reply $y$");
        submit->click();
        QTRY_COMPARE(network->postedReply.value("content").toString(), QString("Reply $y$"));
        auto *manage = page.findChild<QPushButton *>("communityManagePost");
        QTRY_VERIFY(manage && manage->isVisible());
        bool triggered = false;
        QTimer popupTimeout;
        popupTimeout.setSingleShot(true);
        connect(&popupTimeout, &QTimer::timeout, &page, [] {
            if (auto *popup = QApplication::activePopupWidget()) popup->close();
        });
        popupTimeout.start(1500);
        QTimer::singleShot(0, &page, [&] {
            auto *popup = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            if (!popup) return;
            for (auto *action : popup->actions()) {
                if (action->text() == QStringLiteral("置顶")) { action->trigger(); triggered = true; break; }
            }
            popup->close();
        });
        manage->click();
        popupTimeout.stop();
        QVERIFY(triggered);
        QTRY_COMPARE(network->moderatorAction, QByteArray("pin"));
        triggered = false;
        popupTimeout.start(1500);
        QTimer inputTimeout;
        inputTimeout.setSingleShot(true);
        connect(&inputTimeout, &QTimer::timeout, &page, [] {
            if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) input->reject();
        });
        inputTimeout.start(2000);
        QTimer::singleShot(0, &page, [&] {
            auto *popup = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            if (!popup) return;
            for (auto *action : popup->actions()) {
                if (!action->text().startsWith(QStringLiteral("帖子回收站"))) continue;
                QTimer::singleShot(0, &page, [] {
                    if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                        input->setTextValue("fixture-only");
                        input->accept();
                    }
                });
                action->trigger();
                triggered = true;
                break;
            }
            popup->close();
        });
        account->click();
        popupTimeout.stop();
        inputTimeout.stop();
        QVERIFY(triggered);
        QTRY_VERIFY(network->recycleRead);
        auto *recycle = page.findChild<QDialog *>("communityRecycle");
        QVERIFY(recycle);
        auto *records = recycle->findChild<QTreeWidget *>();
        QTRY_COMPARE(records->topLevelItemCount(), 1);
        records->setCurrentItem(records->topLevelItem(0));
        for (auto *button : recycle->findChildren<QPushButton *>())
            if (button->text() == QStringLiteral("恢复帖子")) button->click();
        QTRY_VERIFY(network->restored);
        QTRY_COMPARE(records->topLevelItemCount(), 0);
    }
    void playerRangeHandoff() {
        QTemporaryDir directory;const auto recovery=qgetenv("VSR_RECOVERY_ROOT");qputenv("VSR_RECOVERY_ROOT",directory.filePath("recovery").toUtf8());
        const auto restore=qScopeGuard([&]{if(recovery.isNull())qunsetenv("VSR_RECOVERY_ROOT");else qputenv("VSR_RECOVERY_ROOT",recovery);});
        const auto source=directory.filePath("loop.mkv");QProcess ff;
        ff.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=3","-c:v","ffv1","-y",source});
        QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        MainWindow window;window.show();QVERIFY(window.openPlayerSource(source,5000000,12500000));
        QTRY_COMPARE_WITH_TIMEOUT(window.timeGroups_.size(),3,10000);
        QCOMPARE(window.timeGroupIndex_,1);QCOMPARE(window.timeGroups_[0].startFrame,qint64(0));QCOMPARE(window.timeGroups_[1].startFrame,qint64(12));QCOMPARE(window.timeGroups_[2].startFrame,qint64(30));
        QVERIFY(window.timeGroups_[0].graph.nodes().isEmpty());QVERIFY(window.timeGroups_[2].graph.nodes().isEmpty());QVERIFY(window.isWindowModified());
        QString error;QVERIFY2(window.saveProjectTo(directory.filePath("range.vsr.json"),&error),qPrintable(error));
        MainWindow restored;QVERIFY2(restored.openProjectFrom(directory.filePath("range.vsr.json"),&error),qPrintable(error));
        QCOMPARE(restored.timeGroupIndex_,1);QCOMPARE(restored.timeGroups_[1].startFrame,qint64(12));QCOMPARE(restored.timeGroups_[2].startFrame,qint64(30));
        QVERIFY(!restored.openPlayerSource(source,10000000,5000000));QCOMPARE(restored.timeGroups_.size(),3);
        MainWindow whole;QVERIFY(whole.openPlayerSource(source));QCOMPARE(whole.timeGroups_.size(),1);QCOMPARE(whole.playerRangeIn_,qint64(-1));
        MainWindow zero;QVERIFY(zero.openPlayerSource(source,0,10000000));QTRY_COMPARE_WITH_TIMEOUT(zero.timeGroups_.size(),2,10000);QCOMPARE(zero.timeGroupIndex_,0);
    }
    void analysisOpenNguScaling()
    {
        const QDir runtime(QCoreApplication::applicationDirPath());
        if(!runtime.exists("shaders/OpenNGU-AA-Medium-BestMatch-2x-SDR.hlsl"))QSKIP("OpenNGU shader supplied separately");
        QTemporaryDir directory;
        const auto recovery=qgetenv("VSR_RECOVERY_ROOT");qputenv("VSR_RECOVERY_ROOT",directory.filePath("recovery").toUtf8());
        const auto restore=qScopeGuard([&]{if(recovery.isNull())qunsetenv("VSR_RECOVERY_ROOT");else qputenv("VSR_RECOVERY_ROOT",recovery);});
        const auto source=directory.filePath("ngu.mkv");
        QProcess encoder;encoder.start(runtime.filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24:duration=1","-c:v","ffv1","-pix_fmt","yuv444p10le","-colorspace","bt709","-color_primaries","bt709","-color_trc","bt709",source});
        QVERIFY(encoder.waitForFinished(15000));QCOMPARE(encoder.exitCode(),0);
        ThreeFpApi api;AnalysisPage page(api);page.resize(1200,750);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        QComboBox *scaler=nullptr;
        for(auto *combo:page.findChildren<QComboBox*>())if(combo->findText(QStringLiteral("放大：OpenNGU"))>=0)scaler=combo;
        QVERIFY(scaler);scaler->setCurrentIndex(scaler->findText(QStringLiteral("放大：OpenNGU")));
        QVERIFY(scaler->toolTip().contains(QStringLiteral("缩小默认回退 D3D11")));
        page.openFiles({source,source});QCOMPARE(page.videoCount(),2);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.snapshot(0).presentedVideoFrames>0,15000);page.pause();
        auto *view=page.findChild<MultiCompareView*>();QVERIFY(view);
        auto *surface=view->pane(0)->surface();surface->setFixedSize(256,144);
        auto capture=[&]{page.players_[0]->redraw();QTest::qWait(250);return page.players_[0]->capture();};
        const auto ngu=capture();QVERIFY(!ngu.isNull());
        scaler->setCurrentIndex(4);const auto jinc=capture();QVERIFY(!jinc.isNull());
        QVERIFY2(ngu!=jinc,"OpenNGU selection must change the actual comparison preview pixels");
        surface->setFixedSize(64,36);scaler->setCurrentIndex(9);const auto nguDown=capture();
        scaler->setCurrentIndex(8);const auto directDown=capture();
        QVERIFY(!nguDown.isNull());QCOMPARE(nguDown,directDown);
        scaler->setCurrentIndex(9);
        auto *layout=page.findChild<QComboBox*>("analysisLayout");QVERIFY(layout);
        layout->setCurrentIndex(layout->findData(MultiCompareView::Wipe));
        QVERIFY(page.compositionScript().errors.join(';').contains("OpenNGU"));

        page.hide();
        MainWindow window;window.resize(1200,750);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *previewScaler=window.findChild<QComboBox*>("rendererScaling");QVERIFY(previewScaler);
        previewScaler->setCurrentIndex(previewScaler->findData(15));
        QVERIFY(previewScaler->toolTip().contains(QStringLiteral("缩小默认回退 D3D11")));
        QVERIFY(window.sourcePlayer_->openFile(source));
        QVERIFY(window.processedPlayer_->openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(window.sourcePlayer_->snapshot().state==ThreeFpState::Ready && window.processedPlayer_->snapshot().state==ThreeFpState::Ready,15000);
        QVERIFY(window.sourcePlayer_->seekFrame(0));QVERIFY(window.processedPlayer_->seekFrame(0));
        QTRY_VERIFY_WITH_TIMEOUT(window.sourcePlayer_->snapshot().presentedVideoFrames>0 && window.processedPlayer_->snapshot().presentedVideoFrames>0,15000);
        window.sourcePlayer_->pause();window.processedPlayer_->pause();
        window.sourcePane_->surface()->setFixedSize(256,144);
        auto preview=[&]{window.sourcePlayer_->redraw();QTest::qWait(250);return window.sourcePlayer_->capture();};
        const auto previewNgu=preview();QVERIFY(!previewNgu.isNull());
        previewScaler->setCurrentIndex(previewScaler->findData(4));const auto previewJinc=preview();
        QVERIFY2(previewNgu!=previewJinc,"OpenNGU must change the workbench preview pixels");
        window.sourcePane_->surface()->setFixedSize(64,36);
        previewScaler->setCurrentIndex(previewScaler->findData(15));const auto previewDown=preview();
        previewScaler->setCurrentIndex(previewScaler->findData(7));QCOMPARE(previewDown,preview());
    }
    void projectExitDialog_data(){
        QTest::addColumn<QString>("language");QTest::addColumn<int>("choice");
        for(const auto &language:QStringList{"zh_CN","en_US"})for(int choice:{int(QMessageBox::Save),int(QMessageBox::Discard),int(QMessageBox::Cancel)})QTest::newRow(qPrintable(language+QString::number(choice)))<<language<<choice;
    }
    void projectExitDialog(){
        const auto oldOrganization=QCoreApplication::organizationName(),oldApplication=QCoreApplication::applicationName();
        const auto restoreNames=qScopeGuard([&]{QCoreApplication::setOrganizationName(oldOrganization);QCoreApplication::setApplicationName(oldApplication);});
        QCoreApplication::setOrganizationName("VSRendererTests");QCoreApplication::setApplicationName("renderer-project-exit-test");
        QFETCH(QString,language);QFETCH(int,choice);QSettings settings;const auto old=settings.value("appearance/language");
        const auto restore=qScopeGuard([&]{if(old.isValid())settings.setValue("appearance/language",old);else settings.remove("appearance/language");});settings.setValue("appearance/language",language);
        QTemporaryDir directory;MainWindow window;window.projectPath_=directory.filePath("saved.vsr.json");window.graph_.add("resize");window.checkpointProject();QVERIFY(window.isWindowModified());bool checked=false;
        QTimer::singleShot(0,&window,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(!box)return;const auto cancel=qScopeGuard([&]{if(!checked)box->done(QMessageBox::Cancel);});
            const bool english=language=="en_US";
            QCOMPARE(box->button(QMessageBox::Save)->text().remove('&'),english?QString("Save"):QStringLiteral("保存"));
            QCOMPARE(box->button(QMessageBox::Discard)->text().remove('&'),english?QString("Don't Save"):QStringLiteral("不保存"));
            QCOMPARE(box->button(QMessageBox::Cancel)->text().remove('&'),english?QString("Cancel"):QStringLiteral("取消"));
            QCOMPARE(box->windowTitle(),english?QString("Unsaved Project"):QStringLiteral("项目尚未保存"));checked=true;box->button(QMessageBox::StandardButton(choice))->click();});
        QCOMPARE(window.confirmProjectReplacement(),choice!=QMessageBox::Cancel);QVERIFY(checked);
        if(choice==QMessageBox::Save){QVERIFY(!window.isWindowModified());QVERIFY(QFileInfo::exists(window.projectPath_));}else QVERIFY(window.isWindowModified());
    }
    void startupTheme(){
        const auto path=QDir(QCoreApplication::applicationDirPath()).filePath("themes.ini");QFile original(path);const bool existed=original.exists();QByteArray bytes;if(original.open(QIODevice::ReadOnly))bytes=original.readAll();original.close();
        const auto restore=qScopeGuard([&]{if(existed){QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(bytes);}else QFile::remove(path);});
        QSettings skin(path,QSettings::IniFormat);skin.clear();skin.setValue("VSR-Main/color","#0fe621");skin.sync();
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));QTest::qWait(100);
        auto *stack=window.findChild<QStackedWidget *>();QVERIFY(stack);auto *page=stack->currentWidget();
        auto *theme=page->findChild<QWidget *>("pageTheme",Qt::FindDirectChildrenOnly);QVERIFY(theme);QVERIFY(theme->isVisible());QCOMPARE(theme->grab().toImage().pixelColor(4,4),QColor("#0fe621"));
        auto *hint=window.findChild<QLabel *>("rendererFrameBoundaryHint");QVERIFY(hint);QCOMPARE(hint->text(),QStringLiteral("精确帧通过源帧号实现"));QVERIFY(!hint->wordWrap());
        const auto visible=page->grab().toImage();visible.save(QDir(QCoreApplication::applicationDirPath()).filePath("startup-renderer-background.png"));
        QCOMPARE(visible.pixelColor(hint->mapTo(page,QPoint(hint->width()-3,hint->height()/2))*visible.devicePixelRatio()),QColor("#0fe621"));
    }
    void projectRecoveryAndFrameInputs(){
        QTemporaryDir temp;const auto old=qgetenv("VSR_RECOVERY_ROOT");qputenv("VSR_RECOVERY_ROOT",temp.path().toUtf8());const auto restore=qScopeGuard([&]{qputenv("VSR_RECOVERY_ROOT",old);});
        MainWindow window;window.sourceFpsNumerator_=24000;window.sourceFpsDenominator_=1001;window.sourceTotalFrames_=240;
        window.replaceTimeGroups({{"A",0,{},QString(),0},{"B",0,{},QString(),12}});window.refreshTimeGroups(1);
        auto *frames=window.findChild<QSpinBox*>("rendererTimeGroupFrame");QVERIFY(frames);QVERIFY(frames->isEnabled());frames->setValue(13);QCOMPARE(window.currentTimeGroups()[1].startFrame,qint64(13));window.checkpointProject();QVERIFY(window.isWindowModified());QVERIFY(QFileInfo::exists(window.recoveryPath_));
        const auto path=temp.filePath("project.vsr.json");QVERIFY(window.saveProjectTo(path));QVERIFY(!window.isWindowModified());window.graph_.add("resize");window.checkpointProject();QVERIFY(window.isWindowModified());window.stepProjectHistory(-1);QVERIFY(window.graph_.nodes().isEmpty());window.stepProjectHistory(1);QCOMPARE(window.graph_.nodes().size(),1);QVERIFY(window.saveProjectTo(path));QVERIFY(QFileInfo::exists(path+".bak"));
        QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));const auto data=QJsonDocument::fromJson(file.readAll()).object();QVERIFY(window.applyProjectSnapshot(data));QCOMPARE(window.currentTimeGroups()[0].startFrame,qint64(0));
    }

    void additionalRouteFallsBackToCompleteScript(){
        ThreeFpApi api;PreviewPane pane("route","VS");pane.resize(640,400);pane.show();RenderRoute route(api,&pane,nullptr);QTemporaryDir dir;
        const QString prefix="import vapoursynth as vs\nclip=vs.core.std.BlankClip(format=vs.YUV420P8,length=24,width=";
        const auto original=prefix+"160,height=90)\nclip.set_output()\n",fallback=prefix+"200,height=100)\nclip.set_output()\n";
        route.load(original,dir.filePath("route.vpy"),"unused-for-incompatible-aspect",{{"output_mode","指定分辨率"},{"width",200},{"height",100},{"keep_aspect",false}},fallback);
        ThreeFpSnapshot source{};source.position100ns=0;source.timelineGeneration=1;
        QTRY_VERIFY_WITH_TIMEOUT(([&]{route.sync(source);return route.snapshot().videoWidth==200;})(),15000);QCOMPARE(route.snapshot().videoHeight,100u);
    }
    void restoredRoutesSynchronizeFromEveryPane(){
        QTemporaryDir dir;const auto path=dir.filePath("restore.vsr.json");QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(R"({"schemaVersion":2,"routes":[{},{}],"editingRoute":1})");file.close();
        MainWindow window;window.syncView_->setChecked(true);QTimer chooser;chooser.setInterval(50);connect(&chooser,&QTimer::timeout,&window,[&]{for(auto *widget:QApplication::topLevelWidgets())if(auto *dialog=qobject_cast<QFileDialog*>(widget)){dialog->setOption(QFileDialog::DontUseNativeDialog);dialog->selectFile(path);if(auto *name=dialog->findChild<QLineEdit*>("fileNameEdit"))name->setText(path);QMetaObject::invokeMethod(dialog,"accept");}});
        const bool native=QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);chooser.start();window.openProject();chooser.stop();QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,native);
        auto *view=window.compareView_->findChild<MultiCompareView*>();QCOMPARE(window.additionalRoutes_.size(),size_t(1));emit view->pane(2)->viewChanged(3,.2f,.1f);QCOMPARE(window.sourcePane_->zoom(),3.f);QCOMPARE(window.processedPane_->zoom(),3.f);
    }
    void suppliedThemeAllPagesAndPreviewIsolation(){
        const auto image=qEnvironmentVariable("VSR_THEME_TEST_IMAGE");if(image.isEmpty())QSKIP("Set VSR_THEME_TEST_IMAGE");QVERIFY(QFileInfo::exists(image));
        const auto ini=QDir(QCoreApplication::applicationDirPath()).filePath("themes.ini");QFile original(ini);const bool existed=original.exists();QByteArray saved;if(original.open(QIODevice::ReadOnly))saved=original.readAll();original.close();const auto restore=qScopeGuard([&]{QFile file(ini);if(existed){file.open(QIODevice::WriteOnly);file.write(saved);}else file.remove();});
        QSettings settings(ini,QSettings::IniFormat);settings.clear();settings.sync();MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));const QStringList keys{"VSR-Main","VSR-Compare","VSR-Settings","VSR-BDRemux"};
        for(int i=0;i<keys.size();++i){window.selectPage(i);QTest::qWait(80);auto *page=window.pages_->widget(i);const auto before=page->grab().toImage();settings.setValue(keys[i]+"/image",image);settings.setValue(keys[i]+"/transparency",15);settings.setValue(keys[i]+"/brightness",80);settings.sync();applyPageTheme(page,ini,keys[i]);QTest::qWait(80);const auto after=page->grab().toImage();int changed=0,total=0;for(int y=0;y<before.height();y+=7)for(int x=0;x<before.width();x+=7){++total;if(before.pixel(x,y)!=after.pixel(x,y))++changed;}QVERIFY2(changed>total/50,qPrintable(keys[i]));
            for(auto *pane:page->findChildren<PreviewPane*>())QVERIFY(!pane->property("pageThemeContainer").toBool());
            const auto output=qEnvironmentVariable("VSR_THEME_ARTIFACTS");if(!output.isEmpty())window.grab().save(QDir(output).filePath(keys[i]+".png"));
        }
    }
    void routesWaitForManualPreview(){
        QTemporaryDir directory;const auto file=directory.filePath("manual.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        MainWindow window;window.show();QVERIFY(window.loadSource(file));const auto hint=window.processedPane_->findChild<QLabel*>("previewPlaceholder")->text();window.findChild<QPushButton*>("rendererShowRoute")->click();QTest::qWait(150);QVERIFY(!window.vsScriptReady_);
        QVERIFY(window.addRoute());QTest::qWait(700);QVERIFY(!window.vsScriptReady_);QCOMPARE(window.additionalRoutes_[0]->snapshot().presentedVideoFrames,0ull);
        auto *view=window.compareView_->findChild<MultiCompareView*>();QCOMPARE(view->pane(2)->findChild<QLabel*>("previewPlaceholder")->text(),hint);
        QVERIFY(window.addRoute());QTest::qWait(700);for(const auto &route:window.additionalRoutes_)QCOMPARE(route->snapshot().presentedVideoFrames,0ull);
        for(int i=1;i<4;++i){bool title=false;for(auto *label:view->pane(i)->findChildren<QLabel*>())title|=label->text()==QString("路%1").arg(i);QVERIFY(title);}
        window.validateScript();QTRY_VERIFY_WITH_TIMEOUT(window.additionalRoutes_[1]->snapshot().presentedVideoFrames>0,20000);QCOMPARE(window.additionalRoutes_[0]->snapshot().presentedVideoFrames,0ull);QVERIFY(!window.vsScriptReady_);
        QVERIFY(window.loadSource(file));QTest::qWait(700);for(int i=1;i<4;++i)QCOMPARE(view->pane(i)->findChild<QLabel*>("previewPlaceholder")->text(),hint);
        window.removeRoute();window.removeRoute();bool processed=false;for(auto *label:window.processedPane_->findChildren<QLabel*>())processed|=label->text()=="处理后";QVERIFY(processed);
    }
    void previewInitializationKeepsRequestedRoute(){
        QTemporaryDir directory;const auto file=directory.filePath("pending.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        MainWindow window;window.show();QVERIFY(window.loadSource(file));window.validateScript();QVERIFY(window.addRoute());
        QTRY_VERIFY_WITH_TIMEOUT(window.processedPlayer_->snapshot().presentedVideoFrames>0,5000);QCOMPARE(window.additionalRoutes_[0]->snapshot().presentedVideoFrames,0ull);QCOMPARE(window.routeIndex_,1);
    }
    void independentVideoRoutes(){
        QTemporaryDir directory;const auto file=directory.filePath("routes.mkv");QProcess ff;ff.start("ffmpeg",{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24","-t","3","-c:v","ffv1",file});QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        MainWindow window;window.show();QVERIFY(window.loadSource(file));
        auto resized=[](int width,int height){FilterGraph graph;const int node=graph.add("resize");graph.setParameter(node,"width",width);graph.setParameter(node,"height",height);return graph;};
        window.replaceTimeGroups({{QStringLiteral("One"),0,resized(64,36)}});window.validateScript();
        QTRY_VERIFY_WITH_TIMEOUT(window.processedPlayer_->snapshot().presentedVideoFrames>0,20000);
        QVERIFY(window.addRoute());window.replaceTimeGroups({{QStringLiteral("Two"),0,resized(48,24)}});window.validateScript();
        QVERIFY(window.addRoute());window.replaceTimeGroups({{QStringLiteral("Three"),0,resized(32,18)}});window.validateScript();QVERIFY(!window.addRoute());
        QTRY_VERIFY_WITH_TIMEOUT(window.additionalRoutes_[0]->snapshot().presentedVideoFrames>0,20000);
        QTRY_VERIFY_WITH_TIMEOUT(window.additionalRoutes_[1]->snapshot().presentedVideoFrames>0,20000);
        QCOMPARE(window.processedPlayer_->snapshot().videoWidth,64u);QCOMPARE(window.additionalRoutes_[0]->snapshot().videoWidth,48u);QCOMPARE(window.additionalRoutes_[1]->snapshot().videoWidth,32u);
        window.selectRoute(1);QVERIFY(window.currentScript().script.contains("width=48"));window.selectRoute(0);QVERIFY(window.currentScript().script.contains("width=64"));
        window.findChild<QPushButton*>("rendererHideRoute")->click();QVERIFY(!window.vsScriptReady_);window.validateScript();QVERIFY(!window.vsScriptReady_);window.findChild<QPushButton*>("rendererShowRoute")->click();
        window.removeRoute();QCOMPARE(window.routes_.size(),2);QVERIFY(window.currentScript().script.contains("width=48"));
    }

    void nativeImageZoomBeyondViewportBounds() {
        QTemporaryDir directory;QImage image(64,48,QImage::Format_RGB32);image.fill(Qt::green);
        for(int y=0;y<image.height();++y)image.setPixelColor(32,y,Qt::red);
        const auto path=directory.filePath("zoom.png");QVERIFY(image.save(path));
        ThreeFpApi api;QWidget surface;surface.resize(640,480);surface.show();
        QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpPlayer player(api,&surface);
        QVERIFY(player.openFile(path));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);
        for(float zoom:{128.f,1024.f,8192.f}){
            const auto before=player.snapshot().swapChainPresents;player.setView(zoom,0,0);
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>before,5000);
            VsrSourcePixelProbe a,b;QVERIFY(player.sampleSourcePixel(32,24,false,a));QVERIFY(player.sampleSourcePixel(33,24,false,b));
            qInfo()<<"Requested zoom"<<zoom<<"source pixel screen span"<<b.surfaceX-a.surfaceX;
            QVERIFY(std::abs((b.surfaceX-a.surfaceX)-zoom)<2);
            ThreeFpPixelProbe rgb{};const auto size=surface.size()*surface.devicePixelRatioF();
            const auto distance=std::min(int(zoom/4),size.width()/4);
            QVERIFY(player.samplePixel(size.width()/2+distance,size.height()/2,rgb));
            QVERIFY2(rgb.red>.8f && rgb.green<.1f && rgb.blue<.1f,"Magnified image became blank beyond D3D viewport bounds");
            QVERIFY(player.samplePixel(size.width()/2-distance,size.height()/2,rgb));
            QVERIFY2(rgb.green>.8f && rgb.red<.1f,"Clipped viewport sampled the wrong source pixel");
        }
    }
    void sourceYuvProbePreservesTenBitSubsampling() {
        ThreeFpApi api;QWidget surface;surface.resize(640,500);surface.show();
        QVERIFY(QTest::qWaitForWindowExposed(&surface));ThreeFpPlayer player(api,&surface);
        VapourSynthFrame frame;frame.width=64;frame.height=48;frame.format=ThreeFpExternalPixelFormat::Yuv420P10;
        frame.colorRange=1;frame.colorMatrix=1;frame.duration100ns=400000;frame.totalFrames=1;
        for(int c=0;c<3;++c){
            const int w=c?32:64,h=c?24:48;frame.strides[c]=w*2;frame.planes[c].resize(w*h*2);
            for(int y=0;y<h;++y)for(int x=0;x<w;++x){
                const quint16 value=100+c*200+x+y;
                memcpy(frame.planes[c].data()+(y*w+x)*2,&value,2);
            }
        }
        QVERIFY(player.submitFrame(frame));QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);
        VsrSourcePixelProbe sample;
        QVERIFY(player.sampleSourcePixel(19,13,false,sample));
        QCOMPARE(sample.x,19);QCOMPARE(sample.y,13);QCOMPARE(sample.family,1u);QCOMPARE(sample.bitDepth,10u);
        QCOMPARE(sample.values[0],132u);QCOMPARE(sample.values[1],315u);QCOMPARE(sample.values[2],515u);
        QCOMPARE(sample.chromaShiftX,1u);QCOMPARE(sample.chromaShiftY,1u);
        VsrSourcePixelProbe mapped;QVERIFY(player.sampleSourcePixel(sample.surfaceX,sample.surfaceY,true,mapped));
        QCOMPARE(mapped.x,19);QCOMPARE(mapped.y,13);
        QVERIFY(!player.sampleSourcePixel(-1,13,false,mapped));
        QVERIFY(!player.sampleSourcePixel(64,13,false,mapped));
        QVERIFY(!player.sampleSourcePixel(0,0,true,mapped)); // letterbox
    }
    void threeAvifDragLatency() {
        const auto directory=qEnvironmentVariable("VSR_AVIF_TEST_DIR");
        if(directory.isEmpty())QSKIP("Set VSR_AVIF_TEST_DIR to the Stray AVIF directory");
        QStringList paths;
        for(const auto &file:QDir(directory).entryList({"*.avif"},QDir::Files,QDir::Name))
            if(file.startsWith("[Stray]"))paths.append(QDir(directory).filePath(file));
        QCOMPARE(paths.size(),3);
        ThreeFpApi api;AnalysisPage page(api);page.resize(1600,950);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));page.openFiles(paths);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),5000);
        for(int i=0;i<3;++i)QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(i).presentedVideoFrames>0,10000);
        auto *view=page.findChild<MultiCompareView *>();auto *pane=view->pane(0);
        emit pane->viewChanged(3,0,0);pane->adoptView(3,0,0);QTest::qWait(200);
        auto *surface=pane->surface();const auto start=surface->rect().center();
        std::array<std::uint64_t,3> before{};for(int i=0;i<3;++i)before[i]=page.snapshot(i).swapChainPresents;
        QTest::mousePress(surface,Qt::LeftButton,Qt::NoModifier,start);
        QList<double> durations;
        for(int i=0;i<60;++i){
            const auto at=start+QPoint(i%30, i%20);
            QMouseEvent move(QEvent::MouseMove,at,surface->mapToGlobal(at),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QElapsedTimer timer;timer.start();QApplication::sendEvent(surface,&move);QApplication::processEvents();
            durations.append(timer.nsecsElapsed()/1e6);
            QTest::qWait(8);
        }
        QTest::mouseRelease(surface,Qt::LeftButton);std::sort(durations.begin(),durations.end());
        qInfo()<<"Three AVIF drag ms median"<<durations[30]<<"p95"<<durations[57];
        for(int i=0;i<3;++i){
            const auto updates=page.snapshot(i).swapChainPresents-before[i];
            qInfo()<<"Lane"<<i<<"drag presents"<<updates;
            QVERIFY2(updates>=15,"View changes are throttled to the still-image media frame rate");
        }
        QVERIFY2(durations[57]<30,"Three-lane dragging blocks the UI for more than 30 ms per update");
        auto *display=page.findChild<QComboBox *>("colorDisplay");QVERIFY(display);QCOMPARE(display->currentIndex(),0);
        const auto hover=[&](int lane){
            auto *s=view->pane(lane)->surface();const auto at=s->rect().center();
            QMouseEvent move(QEvent::MouseMove,at,s->mapToGlobal(at),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(s,&move);
        };
        hover(0);
        for(int i=0;i<3;++i)QTRY_VERIFY(view->pane(i)->findChild<QLabel *>("pixelReadout")->text().contains("RGB"));
        display->setCurrentIndex(1);
        const auto first=view->pane(0)->findChild<QLabel *>("pixelReadout")->text().section(')',0,0);
        for(int i=0;i<3;++i){
            const auto text=view->pane(i)->findChild<QLabel *>("pixelReadout")->text();
            QCOMPARE(text.section(')',0,0),first);QVERIFY(text.contains("YUV"));
            qInfo()<<"Source lane"<<i<<text;
        }
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::ThreeLeft));QTest::qWait(100);
        auto *s=view->pane(1)->surface();
        const auto at=s->mapFromGlobal(view->mapToGlobal(view->cellRect(1).center()+QPoint(0,28)));
        QMouseEvent move(QEvent::MouseMove,at,s->mapToGlobal(at),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(s,&move);QTest::qWait(100);
        const auto wipe=view->pane(1)->findChild<QLabel *>("pixelReadout")->text().section(')',0,0);
        for(int i=0;i<3;++i)QCOMPARE(view->pane(i)->findChild<QLabel *>("pixelReadout")->text().section(')',0,0),wipe);
        const auto anchor=s->rect().center();
        QWheelEvent wheel(anchor,s->mapToGlobal(anchor),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        for(int step=0;step<20;++step)QApplication::sendEvent(s,&wheel);
        for(int i=0;i<3;++i){QVERIFY(view->pane(i)->zoom()>128);QCOMPARE(view->pane(i)->zoom(),view->pane(1)->zoom());}
    }
    void rendererCubePreviewMediaFixture()
    {
        const auto source = qEnvironmentVariable("VSR_TEST_CUBE_MEDIA");
        const auto cube = qEnvironmentVariable("VSR_TEST_CUBE_LUT");
        if (source.isEmpty() || cube.isEmpty()) QSKIP("Set VSR_TEST_CUBE_MEDIA and VSR_TEST_CUBE_LUT to exercise real media");
        QVERIFY(QFileInfo(source).isFile() && QFileInfo(cube).isFile());
        QTemporaryDir recovery;
        QVERIFY(recovery.isValid());
        const auto previousRecovery = qgetenv("VSR_RECOVERY_ROOT");
        qputenv("VSR_RECOVERY_ROOT", recovery.path().toUtf8());
        const auto restore = qScopeGuard([&] {
            if (previousRecovery.isNull()) qunsetenv("VSR_RECOVERY_ROOT");
            else qputenv("VSR_RECOVERY_ROOT", previousRecovery);
        });
        MainWindow window;
        window.resize(1280, 760);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const int filter = qEnvironmentVariableIntValue("VSR_TEST_CUBE_SOURCE_FILTER");
        const int row = window.sourceFilter_->findData(filter);
        QVERIFY(row >= 0);
        window.sourceFilter_->setCurrentIndex(row);
        qInfo() << "Dropping Cube preview media with source selector" << filter;
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(source)});
        QDragEnterEvent enter(QPoint(420, 300), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&window, &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(420, 300), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&window, &drop);
        QVERIFY(drop.isAccepted());
        auto *catalog = window.findChild<QListWidget *>("rendererCatalog");
        QVERIFY(catalog);
        for (int i = 0; i < catalog->count(); ++i)
            if (catalog->item(i)->data(Qt::UserRole) == "cube_lut") catalog->setCurrentRow(i);
        QVERIFY(catalog->currentItem() && catalog->currentItem()->data(Qt::UserRole) == "cube_lut");
        for (auto *button : window.findChildren<QPushButton *>())
            if (button->text() == QStringLiteral("添加到处理链")) { button->click(); break; }
        QCOMPARE(window.graph_.nodes().size(), 1);
        auto *fileField = window.findChild<QWidget *>("parameter_file");
        QVERIFY(fileField);
        auto *fileEdit = fileField->findChild<QLineEdit *>();
        QVERIFY(fileEdit);
        fileEdit->setText(cube);
        QVERIFY(QMetaObject::invokeMethod(fileEdit, "editingFinished"));
        const auto imported = window.graph_.at(0)->parameters.value("file").toString();
        QVERIFY(imported.startsWith("luts/"));
        QVERIFY(QFileInfo(QDir(QCoreApplication::applicationDirPath()).filePath(imported)).isFile());
        QVERIFY(!window.graph_.at(0)->parameters.value("manual_space").toBool());
        auto *server = window.findChild<VapourSynthFrameServer *>();
        QVERIFY(server);
        QSignalSpy errors(server, &VapourSynthFrameServer::errorOccurred);
        qInfo() << "Validating Cube preview";
        window.validateScript();
        QTRY_VERIFY2_WITH_TIMEOUT(!errors.isEmpty() || window.lastVsFrame_ >= 0, qPrintable(window.taskStatus_->text()), 20000);
        QVERIFY2(errors.isEmpty(), qPrintable(window.taskStatus_->text()));
        QTRY_VERIFY2_WITH_TIMEOUT(window.processedPlayer_->snapshot().presentedVideoFrames > 0,
                                 qPrintable(window.processedPlayer_->lastError()), 5000);
        QVERIFY2(window.vsScriptReady_, qPrintable(window.taskStatus_->text()));
        const auto frame = window.processedPlayer_->snapshot();
        qInfo() << "Cube preview presented" << frame.videoWidth << "x" << frame.videoHeight;
    }

    void rendererThemedItemEditors()
    {
        QTemporaryDir files;
        const auto path = files.filePath("theme.ini");
        const auto originalStyle = qApp->styleSheet();
        const auto restoreStyle = qScopeGuard([&] { qApp->setStyleSheet(originalStyle); });
        qApp->setStyleSheet(applicationStyleSheet(true));
        MainWindow window;
        window.show();
        window.selectPage(2);
        auto *page = window.pages_->widget(2);
        auto *categories = page->findChild<QListWidget *>("rendererSettingsCategories");
        QVERIFY(categories);
        const auto setBackground = [page, path](const QString &color) {
            QSettings theme(path, QSettings::IniFormat);
            theme.setValue("fixture/color", color);
            theme.sync();
            applyPageTheme(page, path, "fixture");
            QCoreApplication::processEvents();
        };
        const auto sample = [](QWidget *editor, QWidget *viewport) {
            const auto p = editor->mapTo(viewport, QPoint(editor->width() * 3 / 4, editor->height() / 2));
            const auto image = viewport->grab().toImage();
            return image.pixelColor(QPointF(p * image.devicePixelRatio()).toPoint());
        };
        setBackground("#993300");
        categories->setCurrentItem(categories->findItems(QStringLiteral("快捷键"), Qt::MatchExactly).value(0));
        auto *table = page->findChild<QTableWidget *>("rendererShortcuts");
        QVERIFY(table && table->isVisible());
        table->setCurrentCell(0, 1);
        table->editItem(table->currentItem());
        auto *combo = table->findChild<QComboBox *>();
        QVERIFY(combo && combo->isVisible());
        QCoreApplication::processEvents();
        const auto comboColor = sample(combo, table->viewport());
        QCOMPARE(comboColor.alpha(), 255);
        setBackground("#006699");
        QCOMPARE(sample(combo, table->viewport()), comboColor);
        combo->showPopup();
        QTRY_VERIFY(combo->view()->isVisible());
        QCOMPARE(sample(combo, table->viewport()).alpha(), 255);
        combo->hidePopup();

        categories->setCurrentItem(categories->findItems(QStringLiteral("右键设置"), Qt::MatchExactly).value(0));
        auto *tree = page->findChild<QTreeWidget *>("rendererContextMenuSettings");
        QVERIFY(tree && tree->isVisible());
        for (auto *button : tree->parentWidget()->findChildren<QPushButton *>())
            if (button->text() == QStringLiteral("新建子菜单")) button->click();
        auto *name = tree->findChild<QLineEdit *>();
        QVERIFY(name && name->isVisible());
        QCoreApplication::processEvents();
        const auto nameColor = sample(name, tree->viewport());
        QCOMPARE(nameColor.alpha(), 255);
        setBackground("#993300");
        QCOMPARE(sample(name, tree->viewport()), nameColor);
    }

    void rendererWorkspaceShortcutsAndMenu()
    {
        const auto oldOrganization = QCoreApplication::organizationName();
        const auto oldApplication = QCoreApplication::applicationName();
        QCoreApplication::setOrganizationName("VSRendererTests");
        QCoreApplication::setApplicationName("renderer-shortcuts-menu-test");
        QSettings settings;
        settings.remove("renderer");
        const auto restore = qScopeGuard([&] {
            settings.remove("renderer");
            QCoreApplication::setOrganizationName(oldOrganization);
            QCoreApplication::setApplicationName(oldApplication);
        });
        MainWindow window;
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::keyClick(&window, Qt::Key_A);
        QCOMPARE(window.routes_.size(), 2);
        auto *search = window.findChild<QLineEdit *>("rendererCatalogSearch");
        QVERIFY(search);
        search->setFocus();
        QTest::keyClick(search, Qt::Key_A);
        QCOMPARE(window.routes_.size(), 2);
        window.selectPage(2);
        QTest::keyClick(&window, Qt::Key_A);
        QCOMPARE(window.routes_.size(), 2);
        auto *categories = window.findChild<QListWidget *>("rendererSettingsCategories");
        QVERIFY(categories);
        categories->setCurrentItem(categories->findItems(QStringLiteral("快捷键"), Qt::MatchExactly).value(0));
        auto *table = window.findChild<QTableWidget *>("rendererShortcuts");
        QVERIFY(table && table->isVisible());
        QCOMPARE(table->columnCount(), 3);
        QVERIFY(!(table->item(0, 2)->flags() & Qt::ItemIsEditable));
        auto *shortcutPage = table->parentWidget();
        const auto buttons = shortcutPage->findChildren<QPushButton *>();
        for (auto *button : buttons)
            if (button->text() == QStringLiteral("添加快捷键")) button->click();
        auto *newBinding = table->item(8, 0);
        QVERIFY(newBinding);
        auto *recorder = table->findChild<QKeySequenceEdit *>();
        QVERIFY(recorder);
        QTest::keyClick(recorder, Qt::Key_K, Qt::AltModifier);
        QTest::keyClick(recorder, Qt::Key_Return);
        QTRY_COMPARE(newBinding->text(), QStringLiteral("Alt+K"));
        for (auto *button : buttons)
            if (button->text() == QStringLiteral("保存")) button->click();
        QCOMPARE(settings.value("renderer/shortcuts").toList().last().toMap().value("key").toString(), QStringLiteral("Alt+K"));

        categories->setCurrentItem(categories->findItems(QStringLiteral("右键设置"), Qt::MatchExactly).value(0));
        auto *tree = window.findChild<QTreeWidget *>("rendererContextMenuSettings");
        QVERIFY(tree && tree->isVisible());
        auto *menuPage = tree->parentWidget();
        const auto click = [menuPage](const QString &label) {
            for (auto *button : menuPage->findChildren<QPushButton *>())
                if (button->text() == label) { button->click(); return true; }
            return false;
        };
        QVERIFY(click(QStringLiteral("新建子菜单")));
        auto *group = tree->topLevelItem(0);
        QVERIFY(group);
        auto *nameEdit = tree->findChild<QLineEdit *>();
        QVERIFY(nameEdit);
        nameEdit->setText(QStringLiteral("Old Name"));
        QTest::keyClick(nameEdit, Qt::Key_Return);
        QTRY_COMPARE(group->text(0), QStringLiteral("Old Name"));
        auto *catalog = menuPage->findChild<QComboBox *>();
        QVERIFY(catalog);
        catalog->setCurrentIndex(catalog->findData("resize"));
        QVERIFY(click(QStringLiteral("加入滤镜")));
        QCOMPARE(group->childCount(), 1);
        QStyleOptionViewItem itemStyle;
        itemStyle.initFrom(tree);
        itemStyle.font = tree->font();
        QStyledItemDelegate defaultSpacing;
        const auto normalHeight = defaultSpacing.sizeHint(itemStyle, tree->indexFromItem(group)).height();
        QTRY_COMPARE(tree->visualItemRect(group).height(), qRound(normalHeight * 1.3));
        QCOMPARE(tree->visualItemRect(group->child(0)).height(), tree->visualItemRect(group).height());
        tree->setCurrentItem(group->child(0));
        QVERIFY(click(QStringLiteral("重命名")));
        nameEdit = nullptr;
        for (auto *editor : tree->findChildren<QLineEdit *>())
            if (editor->isVisible()) nameEdit = editor;
        QVERIFY(nameEdit);
        nameEdit->setText(QStringLiteral("My Filters"));
        QTest::keyClick(nameEdit, Qt::Key_Return);
        QTRY_COMPARE(group->text(0), QStringLiteral("My Filters"));

        QTemporaryDir files;
        const auto profilePath = files.filePath("preview.json");
        const auto useQtDialogs = QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        const auto restoreDialogs = qScopeGuard([&] { QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, useQtDialogs); });
        const auto chooseFile = [&window, profilePath] {
            QTimer::singleShot(100, &window, [profilePath] {
                if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                    dialog->selectFile(profilePath);
                    if (auto *name = dialog->findChild<QLineEdit *>("fileNameEdit")) name->setText(profilePath);
                    QMetaObject::invokeMethod(dialog, "accept");
                }
            });
        };
        chooseFile();
        QVERIFY(click(QStringLiteral("导出")));
        QVERIFY(QFileInfo::exists(profilePath));
        QVERIFY(click(QStringLiteral("移除所选")));
        QCOMPARE(tree->topLevelItemCount(), 0);
        chooseFile();
        QVERIFY(click(QStringLiteral("导入")));
        QCOMPARE(tree->topLevelItemCount(), 1);

        window.selectPage(0);
        window.selectRoute(0);
        const auto source = files.filePath("timeline.mkv");
        QProcess ffmpeg;
        ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=24:duration=2",
             "-c:v","ffv1","-y",source});
        QVERIFY(ffmpeg.waitForFinished(10000));
        QCOMPARE(ffmpeg.exitCode(), 0);
        QVERIFY(window.loadSource(source));
        QTRY_COMPARE_WITH_TIMEOUT(window.sourceFpsNumerator_, 24, 8000);
        window.replaceTimeGroups({{"First",0,{}, {},0}, {"Second",0,{}, {},12}, {"Third",1,{}, {},24}});
        QVERIFY(window.sourcePlayer_->seekFrame(12));
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(window.sourcePlayer_->snapshot().position100ns - 5000000) < 1000, 8000);
        QCOMPARE(window.timeGroupIndex_, 0);
        window.setFocus();
        auto *pane = window.compareView_->processedPane(1);
        auto *surface = pane->surface();
        const auto point = surface->rect().center();
        QContextMenuEvent context(QContextMenuEvent::Mouse, point, surface->mapToGlobal(point));
        QApplication::sendEvent(surface, &context);
        auto *menu = window.findChild<QMenu *>("rendererPreviewMenu");
        QVERIFY(menu);
        QVERIFY(menu->testAttribute(Qt::WA_TranslucentBackground));
        auto *filters = menu->actions().last()->menu();
        QVERIFY(filters);
        QVERIFY(filters->testAttribute(Qt::WA_TranslucentBackground));
        QCOMPARE(filters->title(), QStringLiteral("My Filters"));
        filters->actions().first()->trigger();
        menu->close();
        QCOMPARE(window.routeIndex_, 0);
        QCOMPARE(window.timeGroupIndex_, 1);
        const auto groups = window.currentTimeGroups();
        QVERIFY(groups[0].graph.nodes().isEmpty());
        QCOMPARE(groups[1].graph.nodes().size(), 1);
        QVERIFY(groups[2].graph.nodes().isEmpty());
        QCOMPARE(window.graph_.nodes().size(), 1);
        QCOMPARE(window.graph_.nodes().first().definitionId, QStringLiteral("resize"));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        const auto duration = window.sourcePlayer_->snapshot().duration100ns;
        QVERIFY(duration > 5000000);
        window.timeline_->setValue(int(4900000 * qint64(window.timeline_->maximum()) / duration));
        window.seekTimeline(window.timeline_->value());
        QVERIFY(window.timelineSeekPending_);
        QContextMenuEvent beforeBoundary(QContextMenuEvent::Mouse, point, surface->mapToGlobal(point));
        QApplication::sendEvent(surface, &beforeBoundary);
        auto *earlierMenu = window.findChild<QMenu *>("rendererPreviewMenu");
        QVERIFY(earlierMenu);
        QVERIFY(earlierMenu->actions().last()->menu());
        earlierMenu->actions().last()->menu()->actions().first()->trigger();
        earlierMenu->close();
        QCOMPARE(window.timeGroupIndex_, 0);
        const auto earlierGroups = window.currentTimeGroups();
        QCOMPARE(earlierGroups[0].graph.nodes().size(), 1);
        QCOMPARE(earlierGroups[1].graph.nodes().size(), 1);
        QVERIFY(earlierGroups[2].graph.nodes().isEmpty());

        const auto reply = [](QMessageBox::StandardButton result) {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                if (auto *button = box->button(result)) button->click();
        };
        QTimer::singleShot(100, &window, [reply] { reply(QMessageBox::No); });
        window.confirmRemoveRoute();
        QCOMPARE(window.routes_.size(), 2);
        QTimer::singleShot(100, &window, [reply] { reply(QMessageBox::Yes); });
        window.confirmRemoveRoute();
        QCOMPARE(window.routes_.size(), 1);
    }

    void rendererKeyframeButtons_data()
    {
        QTest::addColumn<QString>("suffix");
        QTest::newRow("mp4") << QStringLiteral("mp4");
        QTest::newRow("mkv") << QStringLiteral("mkv");
    }

    void rendererKeyframeButtons()
    {
        QFETCH(QString, suffix);
        QTemporaryDir directory;
        const auto source = directory.filePath("gop." + suffix);
        QProcess ffmpeg;
        ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=12:duration=3",
             "-c:v","libx264","-g","12","-keyint_min","12","-sc_threshold","0","-y",source});
        QVERIFY(ffmpeg.waitForFinished(10000));
        QCOMPARE(ffmpeg.exitCode(), 0);
        MainWindow window;
        window.show();
        QVERIFY(window.loadSource(source));
        QTRY_VERIFY_WITH_TIMEOUT(window.sourcePlayer_->snapshot().presentedVideoFrames > 0, 10000);
        auto *next = window.findChild<QPushButton *>("rendererNextKeyframe");
        auto *previous = window.findChild<QPushButton *>("rendererPreviousKeyframe");
        QVERIFY(next && previous);
        const auto hasPresented = [&](qint64 framePts, qint64 position) {
            const auto snap = window.sourcePlayer_->snapshot();
            return snap.framePts != framePts && qAbs(snap.position100ns - position) < 1000000;
        };
        auto framePts = window.sourcePlayer_->snapshot().framePts;
        next->click();
        QTRY_VERIFY_WITH_TIMEOUT(hasPresented(framePts, 10000000), 8000);
        framePts = window.sourcePlayer_->snapshot().framePts;
        previous->click();
        QTRY_VERIFY_WITH_TIMEOUT(hasPresented(framePts, 0), 8000);
        framePts = window.sourcePlayer_->snapshot().framePts;
        QVERIFY(window.sourcePlayer_->seekFrame(11));
        QTRY_VERIFY_WITH_TIMEOUT(hasPresented(framePts, 9166667), 8000);
        framePts = window.sourcePlayer_->snapshot().framePts;
        next->click();
        QTRY_VERIFY_WITH_TIMEOUT(hasPresented(framePts, 10000000), 8000);
        framePts = window.sourcePlayer_->snapshot().framePts;
        next->click();
        QTRY_VERIFY_WITH_TIMEOUT(hasPresented(framePts, 20000000), 8000);
    }

    void rendererHlslPreviewAndGlslFallback(){
        QTemporaryDir directory;QVERIFY(directory.isValid());
        const auto source=directory.filePath("renderer-hlsl.mkv"),shader=directory.filePath("renderer.glsl");
        QProcess encoder;
        encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","color=c=blue:size=320x180:rate=24:duration=3",
             "-color_primaries","bt709","-color_trc","bt709","-colorspace","bt709","-c:v","ffv1","-y",source});
        QVERIFY(encoder.waitForFinished(10000));QVERIFY2(encoder.exitCode()==0,encoder.readAllStandardError().constData());
        QVERIFY(PresetStore::write(shader,"//!DESC Red output\n//!HOOK MAIN\n//!BIND HOOKED\n//!WIDTH OUTPUT.w\n//!HEIGHT OUTPUT.h\nvec4 hook(){return vec4(1.0,0.0,0.0,1.0);}\n"));
        FilterGraph graph;const int resize=graph.add("resize"),node=graph.add("anime4k");
        graph.setParameter(resize,"width",160);graph.setParameter(resize,"height",90);
        graph.setParameter(node,"mode","自定义 GLSL");graph.setParameter(node,"shader",shader);graph.setParameter(node,"hlsl",true);
        graph.setParameter(node,"output_mode","指定分辨率");graph.setParameter(node,"width",640);graph.setParameter(node,"height",360);graph.setParameter(node,"keep_aspect",true);
        MainWindow window;window.resize(1300,800);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.replaceTimeGroups({{QStringLiteral("全片"),0,graph}});
        QVERIFY(window.loadSource(source));
        QTRY_VERIFY2_WITH_TIMEOUT(window.sourcePlayer_->snapshot().presentedVideoFrames>0,qPrintable(window.taskStatus_->text()),15000);
        const auto fullScript=window.currentScript(source);QVERIFY(fullScript.errors.isEmpty());QVERIFY(fullScript.script.contains(".Shader(clip"));
        window.validateScript();
        QTRY_VERIFY2_WITH_TIMEOUT(window.vsScriptReady_ && window.lastVsFrame_>=0 && window.processedPlayer_->snapshot().presentedVideoFrames>0,qPrintable(window.taskStatus_->text()),25000);
        QVERIFY2(!window.hlslPreviewPackage_.isEmpty(),qPrintable(window.hlslPreviewFallback_));
        QCOMPARE(window.processedPlayer_->snapshot().videoWidth,160u); // Earlier VS resize still executes.
        QCOMPARE(window.hlslPreviewParameters_.value("width").toInt(),640);
        const auto surface=window.processedPane_->surface()->size()*window.processedPane_->surface()->devicePixelRatioF();
        ThreeFpPixelProbe pixel{};QVERIFY(window.processedPlayer_->samplePixel(surface.width()/2,surface.height()/2,pixel));
        QVERIFY2(pixel.red>.9f && pixel.green<.1f && pixel.blue<.1f,"Renderer did not execute the native red shader");
        window.sourcePlayer_->setView(2, .4f, .3f);
        window.processedPlayer_->setView(2, -.5f, .2f);
        window.copyPreviewFrame(-1);
        QCOMPARE(QApplication::clipboard()->image().size(), QSize(320, 180));
        window.copyPreviewFrame(0);
        const auto rendered = QApplication::clipboard()->image();
        QCOMPARE(rendered.size(), QSize(640, 360));
        QVERIFY(rendered.pixelColor(8, 8).red() > 220);
        window.sourcePlayer_->setView(1, 0, 0);
        window.processedPlayer_->setView(1, 0, 0);
        QFile preview(QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("preview.vpy"));
        QVERIFY(preview.open(QIODevice::ReadOnly));const auto previewScript=preview.readAll();preview.close();
        QVERIFY(!previewScript.contains(".Shader(clip"));
        // ExportWindow uses currentScript(source); preview extraction must not mutate that graph/script.
        QCOMPARE(window.currentScript(source).script,fullScript.script);
        const auto saved=PresetStore::create(window.currentTimeGroups(),SourceFilter::Ffms2,source,{});
        QVERIFY(saved.contains(".Shader(clip"));QVERIFY(saved.contains(VpyScriptBuilder::pythonString(shader)));
        QVERIFY(PresetStore::write(shader,"//!DESC Chroma passthrough\n//!HOOK CHROMA\n//!BIND HOOKED\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}\n"));
        window.validateScript();
        QTRY_VERIFY2_WITH_TIMEOUT(window.vsScriptReady_ && window.lastVsFrame_>=0 && window.processedPlayer_->snapshot().presentedVideoFrames>0,qPrintable(window.taskStatus_->text()),25000);
        QVERIFY(window.hlslPreviewPackage_.isEmpty());QVERIFY(window.hlslPreviewFallback_.contains("GLSL"));QVERIFY(window.hlslPreviewFallback_.contains("CHROMA"));
        QCOMPARE(window.processedPlayer_->snapshot().videoWidth,640u); // Original shader output dimensions are restored.
        QVERIFY(window.processedPlayer_->samplePixel(surface.width()/2,surface.height()/2,pixel));
        QVERIFY2(pixel.blue>.7f && pixel.red<.2f,"GLSL fallback did not restore the original blue source");
        window.copyPreviewFrame(0);
        const auto glslFrame = QApplication::clipboard()->image();
        QCOMPARE(glslFrame.size(), QSize(640, 360));
        QVERIFY(glslFrame.pixelColor(16, 16).blue() > 150);
        QVERIFY(preview.open(QIODevice::ReadOnly));QVERIFY(preview.readAll().contains(".Shader(clip"));
        QCOMPARE(window.currentScript(source).script,fullScript.script);
    }
    void rendererEditedVpyKeepsPython(){
        QTemporaryDir directory;FilterGraph existing,imported;existing.add("crop");imported.add("resize");
        const auto path=directory.filePath("edited.vpy");
        const auto script=PresetStore::create(imported,SourceFilter::Ffms2,{},{})+"\nclip = core.std.Invert(clip)\nclip.set_output(0)\n";
        QVERIFY(PresetStore::write(path,script));
        MainWindow window;window.replaceTimeGroups({{QStringLiteral("Existing"),0,existing}});
        window.loadPreset(path,true);
        QCOMPARE(window.currentTimeGroups().first().graph.nodes().first().definitionId,QString("crop"));
        QVERIFY(window.taskStatus_->text().contains(QStringLiteral("正文已修改")));
        window.loadPreset(path);
        QCOMPARE(window.activePreset_,path);
        QVERIFY(window.currentScript(directory.filePath("input.mkv")).script.contains("clip = core.std.Invert(clip)"));
    }
    void rendererTimeGroupLayout(){
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *groups=window.findChild<QListWidget*>("rendererTimeGroupList");auto *catalog=window.findChild<QListWidget*>("rendererCatalog");
        auto *pipeline=window.findChild<QListWidget*>("rendererPipeline");auto *view=window.findChild<CompareView*>();
        auto *input=window.findChild<QLineEdit*>("rendererSourcePath");auto *timeline=window.findChild<QSlider*>("rendererTimeline");
        QVERIFY(groups && catalog && pipeline && view && input && timeline);QCOMPARE(groups->count(),1);
        QVERIFY(groups->mapToGlobal(QPoint()).y()>catalog->mapToGlobal(QPoint()).y());
        QVERIFY(pipeline->mapToGlobal(QPoint()).x()>groups->mapToGlobal(QPoint()).x());
        QVERIFY(input->mapToGlobal(QPoint()).y()>timeline->mapToGlobal(QPoint()).y());
        auto *position=window.findChild<QLabel*>("rendererPosition");QVERIFY(position);
        QVERIFY(qAbs(position->mapToGlobal(position->rect().center()).y()-timeline->mapToGlobal(timeline->rect().center()).y())<5);QVERIFY(input->width()<=320);
        const auto width=view->width();auto *one=window.findChild<QAction*>("rendererToggleArea1"),*two=window.findChild<QAction*>("rendererToggleArea2");QVERIFY(one && two);
        one->trigger();QTest::qWait(80);QVERIFY(view->width()>width);two->trigger();QTest::qWait(80);QVERIFY(view->width()>width+300);
        one->trigger();two->trigger();QTest::qWait(80);QVERIFY(qAbs(view->width()-width)<10);
        QVERIFY(window.findChild<QLabel*>("rendererSourceMonitor"));QVERIFY(window.findChild<QLabel*>("rendererProcessedMonitor"));
        auto *browse=window.findChild<QPushButton*>("rendererBrowse");auto *controls=window.findChild<QWidget*>("rendererFrameControls");auto *mode=window.findChild<QComboBox*>("rendererCompareMode");
        QVERIFY(browse && controls && mode);QVERIFY(qAbs(input->mapToGlobal(input->rect().center()).y()-controls->mapToGlobal(controls->rect().center()).y())<6);
        QVERIFY(browse->mapToGlobal(QPoint()).x()<controls->mapToGlobal(QPoint()).x());QVERIFY(controls->mapToGlobal(QPoint()).x()<mode->mapToGlobal(QPoint()).x());
        window.grab().save("build/audit-20261017/renderer-timegroups.png");
    }
    void timeGroupPresetAndFrames(){
        FilterGraph first,second;const int node=first.add("resize");first.setParameter(node,"width",64);first.setParameter(node,"height",32);
        const int other=second.add("resize");second.setParameter(other,"width",64);second.setParameter(other,"height",32);
        QList<FilterTimeGroup> groups{{"Opening",0,first},{"Main",1,second},{"End",2,first}};
        groups[1].script=PresetStore::create(second,SourceFilter::Ffms2,{},{});
        groups[1].script+="\nassert os.path.isfile(os.path.join(_vsr_directory, 'runtime', 'python', 'python.exe'))\n";
        const auto preset=PresetStore::create(groups,SourceFilter::Ffms2,"input.mkv","roundtrip");const auto restored=PresetStore::timeGroups(PresetStore::metadata(preset));
        QCOMPARE(restored.size(),3);QCOMPARE(restored[1].startSeconds,1);QCOMPARE(restored[1].graph.nodes().first().parameters.value("width").toInt(),64);
        auto built=VpyScriptBuilder::buildGroups("input.mkv",SourceFilter::Ffms2,groups);QVERIFY(built.errors.isEmpty());
        built.script.replace(QRegularExpression("^src =.*$",QRegularExpression::MultilineOption),"src = core.std.BlankClip(width=64,height=32,format=vs.YUV420P8,length=101,fpsnum=24000,fpsden=1001)");
        VapourSynthFrameServer server;QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000);QVERIFY2(server.available(),qPrintable(server.errorString()));
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        QTemporaryDir directory;server.loadScript(built.script,directory.filePath("groups.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QCOMPARE(qvariant_cast<VapourSynthClipInfo>(loaded.first().first()).totalFrames,101);
        for(const int frame:{0,23,24,47,48,100}){frames.clear();server.requestFrame(frame);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),5000);QCOMPARE(qvariant_cast<VapourSynthFrame>(frames.first().first()).frameIndex,frame);}
    }
    void presetImportKeepsTimeGroups(){
        FilterGraph first,second;first.add("resize");second.add("crop");
        QList<FilterTimeGroup> groups{{"First",0,first},{"Second",12,second},{"Third",30,first}};
        MainWindow window;window.replaceTimeGroups(groups);window.refreshTimeGroups(1);
        QTemporaryDir dir;const auto single=dir.filePath("single.vpy"),multi=dir.filePath("multi.vpy");
        QVERIFY(PresetStore::write(single,PresetStore::create(first,SourceFilter::Ffms2,{},{})));
        QVERIFY(PresetStore::write(multi,PresetStore::create(groups,SourceFilter::Ffms2,{},{})));
        window.loadPreset(single,true);auto result=window.currentTimeGroups();
        QCOMPARE(result.size(),3);QCOMPARE(result[1].startSeconds,12);QCOMPARE(result[2].startSeconds,30);
        QCOMPARE(result[1].graph.nodes().first().definitionId,QString("resize"));
        window.loadPreset(multi,true);QCOMPARE(window.currentTimeGroups()[1].graph.nodes().first().definitionId,QString("resize"));
        window.loadPreset(multi);result=window.currentTimeGroups();QCOMPARE(result.size(),3);QCOMPARE(result[1].graph.nodes().first().definitionId,QString("crop"));
        // Script-backed builtins must survive import, boundaries, save and reload.
        const auto profile=dir.filePath("profile.vpy");const auto custom=PresetStore::create(first,SourceFilter::Ffms2,{},{})+"\nclip = core.std.Invert(clip)\nclip.set_output(0)\n";
        QVERIFY(PresetStore::write(profile,"# VSR_PLAYER_PROFILE Test\n"+custom));window.refreshTimeGroups(2);window.loadPreset(profile,true);
        result=window.currentTimeGroups();QVERIFY(result[2].script.contains("core.std.Invert"));QCOMPARE(result[2].startSeconds,30);
        const auto saved=PresetStore::create(result,SourceFilter::Ffms2,{},{});const auto restored=PresetStore::timeGroups(PresetStore::metadata(saved));
        QCOMPARE(restored[2].script,result[2].script);QCOMPARE(restored[1].startSeconds,12);
    }
    void communityExampleDownload(){
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));
        const auto filename="regression-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".vpy";
        const auto payload=PresetStore::create(FilterGraph{},SourceFilter::Ffms2,{},"HTTP regression fixture").toUtf8();
        connect(&server,&QTcpServer::newConnection,&server,[&]{
            auto *socket=server.nextPendingConnection();auto request=std::make_shared<QByteArray>();
            connect(socket,&QTcpSocket::readyRead,&server,[&,socket,request]{
                *request+=socket->readAll();if(!request->contains("\r\n\r\n"))return;
                const auto url=QUrl::fromEncoded(request->split(' ').value(1));QByteArray body=payload;
                if(url.path().endsWith("/tree")){
                    const auto root=QUrlQuery(url).queryItemValue("Root");QJsonObject entry;
                    if(root=="fixture")entry={{"Name","Builtin"},{"Type","tree"},{"Path","fixture/Builtin"}};
                    else entry={{"Name",filename},{"Type","blob"},{"Path","fixture/Builtin/"+filename},{"Size",payload.size()},{"Sha256",QString::fromLatin1(QCryptographicHash::hash(payload,QCryptographicHash::Sha256).toHex())}};
                    body=QJsonDocument(QJsonObject{{"Data",QJsonObject{{"Files",QJsonArray{entry}},{"TotalCount",1}}}}).toJson(QJsonDocument::Compact);
                }
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        });
        const auto previous=QSettings().value("community/sourceUrl");QSettings().setValue("community/sourceUrl",QString("http://127.0.0.1:%1/repo/tree?Root=fixture&Revision=master").arg(server.serverPort()));
        const auto restore=qScopeGuard([&]{if(previous.isValid())QSettings().setValue("community/sourceUrl",previous);else QSettings().remove("community/sourceUrl");QFile::remove(QDir(PresetStore::directory()).filePath("Community/Builtin/"+filename));});
        QString imported;bool current=false;
        PresetCommunityDialog dialog([&](const QString &path,bool group){imported=path;current=group;});dialog.show();auto *files=dialog.findChild<QTreeWidget*>("communityFiles");QVERIFY(files);
        QTRY_VERIFY_WITH_TIMEOUT(files->topLevelItemCount()>0 && files->topLevelItem(0)->childCount()>0,5000);
        QPushButton *refresh=nullptr,*download=nullptr;
        for(auto *button:dialog.findChildren<QPushButton *>()){if(button->text()==QStringLiteral("刷新资源"))refresh=button;if(button->text()==QStringLiteral("下载选中"))download=button;}
        QVERIFY(refresh&&download);QTRY_VERIFY_WITH_TIMEOUT(refresh->isEnabled(),5000);
        auto *folder=files->topLevelItem(0);QCOMPARE(folder->text(0),QString("Builtin"));QVERIFY(!folder->isExpanded());folder->setExpanded(true);
        QTreeWidgetItem *row=nullptr;for(int n=0;n<folder->childCount();++n)if(folder->child(n)->text(0).endsWith(".vpy",Qt::CaseInsensitive)){row=folder->child(n);break;}
        QVERIFY2(row,"Community fixture must select a VPY, not a JSON example that opens a modal viewer");files->setCurrentItem(row);
        QVERIFY(!row->data(0,Qt::UserRole).toJsonObject().isEmpty());QCOMPARE(files->currentItem(),row);QVERIFY(download->isEnabled());download->click();
        QVERIFY(row->text(2)!=QStringLiteral("未下载"));
        QTRY_COMPARE_WITH_TIMEOUT(row->text(2),QStringLiteral("已安装"),5000);
        for(auto *button:dialog.findChildren<QPushButton*>())if(button->text()==QStringLiteral("导入当前时间组"))button->click();
        QVERIFY(current);QVERIFY(imported.contains("/Community/Builtin/"));QVERIFY(!PresetStore::metadata(PresetStore::load(imported).script).isEmpty());
        dialog.grab().save("build/audit-20261017/preset-community.png");
    }
    void rendererStartupBudget() {
        const auto before=GetModuleHandleW(L"FFF.Native.dll");QElapsedTimer timer;timer.start();MainWindow window;window.show();
        qInfo()<<"renderer-startup-construction-ms"<<timer.elapsed();QCOMPARE(GetModuleHandleW(L"FFF.Native.dll"),before);
        auto *state=window.findChild<QLabel *>("rendererWarmupState");QVERIFY(state);QCOMPARE(state->text(),QStringLiteral("就绪"));
    }
    void dual4kPerformance() {
        const auto path=qEnvironmentVariable("VSR_COMPARE_SOURCE");if(path.isEmpty())QSKIP("Set VSR_COMPARE_SOURCE for real comparison performance");
        ThreeFpApi api;AnalysisPage page(api);page.resize(1920,1080);page.show();page.openFiles({path,path});
        QTRY_VERIFY_WITH_TIMEOUT(page.videoCount()==2 && !page.busy(),30000);page.togglePlayback();QTRY_COMPARE_WITH_TIMEOUT(page.snapshot(0).state,ThreeFpState::Playing,10000);
        QTest::qWait(2000);const auto first=page.snapshot(0),second=page.snapshot(1);QElapsedTimer elapsed;elapsed.start();int worstTick=0;
        while(elapsed.elapsed()<10000){QElapsedTimer tick;tick.start();QTest::qWait(50);worstTick=std::max(worstTick,int(tick.elapsed()));}
        const auto a=page.snapshot(0),b=page.snapshot(1);const double seconds=elapsed.elapsed()/1000.;
        qInfo()<<"dual-performance"<<"wall"<<seconds<<"accepted-fps"<<(a.presentedVideoFrames-first.presentedVideoFrames)/seconds<<(b.presentedVideoFrames-second.presentedVideoFrames)/seconds<<"dropped"<<a.droppedVideoFrames-first.droppedVideoFrames<<b.droppedVideoFrames-second.droppedVideoFrames<<"worst-ui-tick-ms"<<worstTick<<"timeline"<<a.position100ns/1e7<<b.position100ns/1e7<<"mode"<<a.decodeMode<<b.decodeMode;
        page.pause();QVERIFY(a.presentedVideoFrames>first.presentedVideoFrames);QVERIFY(b.presentedVideoFrames>second.presentedVideoFrames);QVERIFY(worstTick<500);QVERIFY(qAbs((a.position100ns-first.position100ns)/1e7-seconds)<.5);QVERIFY(qAbs(a.position100ns-b.position100ns)<1000000);
        const double minimum=qEnvironmentVariable("VSR_COMPARE_MIN_FPS").toDouble();
        QVERIFY2((a.presentedVideoFrames-first.presentedVideoFrames)/seconds>=minimum && (b.presentedVideoFrames-second.presentedVideoFrames)/seconds>=minimum,"Both comparison lanes must meet the requested realtime frame rate");
    }
    void rendererLayoutAndParameterSizing()
    {
        const auto oldFont=qApp->font();const auto oldStyle=qApp->styleSheet();
        const auto restore=qScopeGuard([&]{qApp->setFont(oldFont);qApp->setStyleSheet(oldStyle);});
        QFont font("Segoe UI Variable");font.setPixelSize(13);qApp->setFont(font);qApp->setStyleSheet(applicationStyleSheet());
        MainWindow window;QVERIFY(window.width()>=1440);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *view=window.findChild<CompareView *>();auto *transport=window.findChild<QWidget *>("rendererTransport");
        auto *frames=window.findChild<QWidget *>("rendererFrameControls");auto *mode=window.findChild<QComboBox *>("rendererCompareMode");
        auto *position=window.findChild<QLabel *>("rendererPosition");auto *timeline=window.findChild<QSlider *>("rendererTimeline");
        auto *source=window.findChild<QLineEdit *>("rendererSourcePath");auto *browse=window.findChild<QPushButton *>("rendererBrowse");
        auto *catalog=window.findChild<QListWidget *>("rendererCatalog");auto *sidebar=window.findChild<QWidget *>("processingSidebar");
        auto *parameters=window.findChild<QWidget *>("rendererParameterPanel");
        auto *scroll=window.findChild<QScrollArea *>("rendererParameters");
        QVERIFY(view && transport && frames && mode && position && timeline && source && browse && catalog && sidebar);
        QVERIFY(parameters && scroll);QVERIFY(!sidebar->isAncestorOf(parameters));
        auto *toggle=window.findChild<QAction *>("navigationToggle");QVERIFY(toggle);
        for(auto size:{QSize(1440,900),QSize(1280,820),QSize(1700,950)}) {
            window.resize(size);toggle->trigger();QTest::qWait(80);
            QCOMPARE(transport->mapToGlobal(QPoint(0,0)).x(),view->mapToGlobal(QPoint(0,0)).x());
            QCOMPARE(transport->width(),view->width());
            QVERIFY(frames->mapToGlobal(QPoint(frames->width(),0)).x()<=mode->mapToGlobal(QPoint()).x());
            QVERIFY(position->mapToGlobal(QPoint(position->width(),0)).x()<=timeline->mapToGlobal(QPoint()).x());
            QVERIFY(std::abs(frames->mapToGlobal(frames->rect().center()).y()-mode->mapToGlobal(mode->rect().center()).y())<=2);
            QVERIFY(timeline->mapTo(transport,QPoint(timeline->width(),0)).x()<=transport->width());
            QCOMPARE(source->mapToGlobal(source->rect().center()).y(),browse->mapToGlobal(browse->rect().center()).y());
            QVERIFY(catalog->height()>=190);QVERIFY(sidebar->width()<360);
            QVERIFY(sidebar->mapToGlobal(QPoint(sidebar->width(),0)).x()<=parameters->mapToGlobal(QPoint()).x());
            QVERIFY(parameters->mapToGlobal(QPoint(parameters->width(),0)).x()<=view->mapToGlobal(QPoint()).x());
            QVERIFY(scroll->height()>190);
        }
        for(int i=0;i<catalog->count();++i)
            QVERIFY2(catalog->fontMetrics().horizontalAdvance(catalog->item(i)->text())+12<=catalog->viewport()->width(),qPrintable(catalog->item(i)->text()));
        auto *warmup=window.findChild<QLabel *>("rendererWarmupState");QVERIFY(warmup);
        QVERIFY(warmup->parentWidget()!=source->parentWidget());
        QCOMPARE(warmup->text(),QStringLiteral("就绪"));
        for(auto *action:window.findChildren<QAction *>())QVERIFY(action->text()!=QStringLiteral("实验设置"));
        for(auto *button:window.findChildren<QPushButton *>())QVERIFY(button->text()!=QStringLiteral("查看生成的 VPY"));
        window.grab().save(QCoreApplication::applicationDirPath()+"/renderer-ui-layout.png");
        QCOMPARE(scroll->viewport()->grab().toImage().pixelColor(10,scroll->viewport()->height()-10),QColor(Qt::white));
        ParameterEditor editor;editor.resize(scroll->viewport()->width(),600);editor.show();
        for(const auto &definition:FilterCatalog::all()) {
            FilterGraph graph;graph.add(definition.id);editor.setNode(&definition,&graph.nodes().first());QCoreApplication::processEvents();
            for(const auto &parameter:definition.parameters) {
                if(definition.id=="anime4k" && parameter.id=="dimension_axis")continue;
                auto *label=editor.findChild<QLabel *>("parameterLabel_"+parameter.id);auto *field=editor.findChild<QWidget *>("parameter_"+parameter.id);
                QVERIFY(label && field);
                QCOMPARE(label->isVisible(), field->isVisible());
                if (!field->isVisible()) continue;
                QVERIFY2(!label->geometry().intersects(field->geometry()),qPrintable(definition.id+":"+parameter.id));
                if(parameter.type==ParameterType::Boolean)QVERIFY(std::abs(field->geometry().right()-(editor.width()-9))<=1);
                if(auto *combo=qobject_cast<QComboBox *>(field)) {
                    QVERIFY(combo->width()>=70 && combo->width()<=120);
                    if(!combo->currentText().isEmpty())QVERIFY(!combo->toolTip().isEmpty());
                }
            }
            if(definition.id=="mvtools" || definition.id=="rife")editor.grab().save(QCoreApplication::applicationDirPath()+"/renderer-parameters-"+definition.id+".png");
            const auto image=editor.grab().toImage();
            QCOMPARE(image.pixelColor(4,editor.height()-4),QColor(Qt::white));
            auto *description=editor.findChildren<QLabel *>().first();
            QCOMPARE(image.pixelColor(description->geometry().right(),description->geometry().top()),QColor(Qt::white));
        }
    }
    void exportTabsShowSelection()
    {
        ExportWindow window;window.show();auto *tabs=window.findChild<QTabWidget *>("exportTabs");QVERIFY(tabs);QCOMPARE(tabs->count(),3);
        const auto sample=[&](int selected,int tab){tabs->setCurrentIndex(selected);QTest::qWait(40);const auto rectangle=tabs->tabBar()->tabRect(tab);return tabs->tabBar()->grab().toImage().pixelColor(rectangle.left()+10,rectangle.top()+10);};
        const auto selected=sample(0,0),idle=sample(1,0);QVERIFY(selected!=idle);QVERIFY(selected.blue()>selected.red());
        tabs->setCurrentIndex(0);QCoreApplication::processEvents();
        auto *command=window.findChild<QPlainTextEdit *>("exportCommand");QVERIFY(command);
        QCOMPARE(tabs->tabBar()->mapToGlobal(tabs->tabBar()->tabRect(0).topLeft()).x(),command->mapToGlobal(QPoint()).x());
        tabs->setCurrentIndex(2);QCoreApplication::processEvents();
        auto *queue=window.findChild<QTreeWidget *>("encodingQueue");QVERIFY(queue);
        const auto previous=qApp->styleSheet();const auto restore=qScopeGuard([&]{qApp->setStyleSheet(previous);});
        for(bool dark:{false,true}){
            qApp->setStyleSheet(applicationStyleSheet(dark));QTest::qWait(60);
            const auto header=queue->header()->grab().toImage().scaled(queue->header()->size(),Qt::IgnoreAspectRatio,Qt::FastTransformation);
            QCOMPARE(header.pixelColor(queue->header()->sectionSize(0)-12,header.height()/2),QColor(dark?"#25272b":"#eeeeee"));
            for(int section=0;section<4;++section) {
                const int boundary=queue->header()->sectionViewportPosition(section)+queue->header()->sectionSize(section)-1;
                if(boundary<header.width()) QCOMPARE(header.pixelColor(boundary,8),QColor(dark?"#50535a":"#d1d1d1"));
            }
        }
        tabs->setCurrentIndex(2);window.grab().save(QCoreApplication::applicationDirPath()+"/renderer-export-tabs.png");
    }
    void parameterEditorFollowsTheme() {
        const auto previous=qApp->styleSheet();const auto restore=qScopeGuard([&]{qApp->setStyleSheet(previous);});
        FilterGraph graph;const int id=graph.add("anime4k");ParameterEditor editor;editor.setNode(FilterCatalog::find("anime4k"),graph.at(id));editor.resize(620,850);editor.show();
        for(bool dark:{true,false}){qApp->setStyleSheet(applicationStyleSheet(dark));QTest::qWait(100);const auto image=editor.grab().toImage();QVERIFY(!image.isNull());const auto background=image.pixelColor(image.width()-4,image.height()-4);QVERIFY(dark?background.lightness()<80:background.lightness()>220);auto *label=editor.findChild<QLabel *>();QVERIFY(label);const auto text=label->palette().color(QPalette::WindowText);QVERIFY(dark?text.lightness()>180:text.lightness()<80);editor.grab().save(QCoreApplication::applicationDirPath()+QString(dark?"/renderer-parameters-dark.png":"/renderer-parameters-light.png"));}
    }
    void themedCubeLabelsFollowBackground() {
        const auto style = qApp->styleSheet();
        const auto restore = qScopeGuard([&] { qApp->setStyleSheet(style); });
        qApp->setStyleSheet(applicationStyleSheet(true));
        QTemporaryDir fixture;
        const auto path = fixture.filePath("themes.ini");
        MainWindow window;
        window.show();
        QCoreApplication::processEvents();
        const auto sample = [&](const QColor &color) {
            QSettings theme(path, QSettings::IniFormat);
            theme.setValue("VSR-Main/color", color.name());
            theme.sync();
            auto *page = window.pages_->widget(0);
            applyPageTheme(page, path, "VSR-Main");
            FilterGraph graph;
            graph.add("cube_lut");
            window.parameterEditor_->setNode(FilterCatalog::find("cube_lut"), &graph.nodes().first());
            QCoreApplication::processEvents();
            auto *label = window.parameterEditor_->findChild<QLabel *>("parameterLabel_file");
            if (!label) return QColor();
            const auto position = label->mapTo(page, QPoint(label->width() - 4, label->height() / 2));
            const auto image = page->grab().toImage();
            return image.pixelColor(position * image.devicePixelRatio());
        };
        const auto warm = sample(QColor("#bb3300"));
        const auto cool = sample(QColor("#0033bb"));
        QVERIFY(warm.isValid() && cool.isValid());
        QVERIFY2(warm.red() > cool.red() + 12 && cool.blue() > warm.blue() + 12,
            qPrintable(warm.name() + " / " + cool.name()));
    }
    void rendererSettingsPageAndVpyWarning()
    {
        const auto oldOrganization=QCoreApplication::organizationName(),oldApplication=QCoreApplication::applicationName();
        const auto restoreNames=qScopeGuard([&]{QCoreApplication::setOrganizationName(oldOrganization);QCoreApplication::setApplicationName(oldApplication);});
        QCoreApplication::setOrganizationName("VSRendererTests");QCoreApplication::setApplicationName("renderer-settings-page-test");
        QSettings settings;const auto saved=settings.value("sourceFilter");
        const auto restore=qScopeGuard([&]{if(saved.isValid())settings.setValue("sourceFilter",saved);else settings.remove("sourceFilter");});
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *pages=window.findChild<QStackedWidget *>();auto *button=window.findChild<QToolButton *>("applicationSettings");
        auto *page=window.findChild<QWidget *>("rendererSettingsPage");QVERIFY(pages && button && page);
        button->click();QCOMPARE(pages->currentWidget(),page);QVERIFY(button->isChecked());QVERIFY(!QApplication::activeModalWidget());
        auto *categories=page->findChild<QListWidget*>("rendererSettingsCategories");
        QVERIFY(categories);
        QCOMPARE(categories->count(),7);
        QVERIFY(page->findChild<QComboBox*>("rendererLanguage")->isVisible());
        categories->setCurrentRow(1);
        auto *themeEditor=page->findChild<QWidget*>("pageThemeEditor");
        QVERIFY(themeEditor && themeEditor->isVisible());
        QVERIFY(!QApplication::activeModalWidget());
        auto *theme=window.findChild<QComboBox*>("rendererTheme");QVERIFY(theme);const auto previousStyle=qApp->styleSheet();const auto previousDark=settings.value("appearance/dark");
        const auto restoreTheme=qScopeGuard([&]{qApp->setStyleSheet(previousStyle);if(previousDark.isValid())settings.setValue("appearance/dark",previousDark);else settings.remove("appearance/dark");});
        theme->setCurrentIndex(1);QVERIFY(qApp->styleSheet().contains("#202124"));page->grab().save(QCoreApplication::applicationDirPath()+"/renderer-settings-dark.png");theme->setCurrentIndex(0);
        categories->setCurrentRow(2);
        QVERIFY(page->findChild<QLineEdit*>("rendererFfmpegExecutable")->isVisible());
        categories->setCurrentRow(3);
        QVERIFY(page->findChild<QCheckBox*>("rendererForumRememberLogin")->isVisible());
        QVERIFY(page->findChild<QLineEdit*>("rendererCommunitySource")->isVisible());
        categories->setCurrentRow(0);
        auto *source=window.findChild<QComboBox *>("rendererSettingsSource");
        auto *apply=window.findChild<QPushButton *>("rendererApplySettings");QVERIFY(source && apply);
        const int choice=source->currentIndex()==0?1:0;source->setCurrentIndex(choice);apply->click();
        QCOMPARE(window.findChild<QComboBox *>("sourceFilter")->currentIndex(),choice);QCOMPARE(settings.value("sourceFilter").toInt(),choice);
        for(auto *nav:window.findChild<QWidget *>("pageNavigation")->findChildren<QToolButton *>())
            if(nav->text()==QStringLiteral("图像分析比对"))nav->click();
        QVERIFY(qobject_cast<AnalysisPage *>(pages->currentWidget()));QCOMPARE(pages->currentIndex(),1);QVERIFY(!button->isChecked());
        button->click();QCOMPARE(pages->currentWidget(),page);QCOMPARE(pages->currentIndex(),2);
        page->grab().save(QCoreApplication::applicationDirPath()+"/renderer-settings-page.png");
        for(auto *nav:window.findChild<QWidget *>("pageNavigation")->findChildren<QToolButton *>())
            if(nav->text()==QStringLiteral("VS 实时渲染"))nav->click();
        QCOMPARE(pages->currentIndex(),0);
        bool warningSeen=false;int warningWidth=0;bool titleFits=false;
        QTimer::singleShot(0,&window,[&] {
            auto *warning=window.findChild<QMessageBox *>("rendererVpyWarning");
            if(warning) {
                warningSeen=true;warningWidth=warning->width();
                titleFits=warning->fontMetrics().horizontalAdvance(warning->windowTitle())+100<warningWidth;
                warning->grab().save(QCoreApplication::applicationDirPath()+"/renderer-vpy-warning.png");
                warning->accept();
            }
        });
        bool actionFound=false;
        for(auto *action:window.findChildren<QAction *>())if(action->text()==QStringLiteral("查看 VPY")){actionFound=true;action->trigger();break;}
        QVERIFY(actionFound);QVERIFY(warningSeen);QVERIFY(warningWidth>=520);QVERIFY(titleFits);
    }
    void rendererLoadsPresetDuringStartup()
    {
        QTemporaryDir dir; const auto source = dir.filePath("source.mkv"); QVERIFY(QFile::copy(":/startup/warmup.mkv",source));
        FilterGraph graph; const int row = graph.add("resize"); graph.setParameter(row,"width",160); graph.setParameter(row,"height",90);
        const auto path = dir.filePath("preset.vpy"); QVERIFY(PresetStore::write(path,PresetStore::create(graph,SourceFilter::Ffms2,source,"test")));
        MainWindow window; window.show(); QMimeData mime; mime.setUrls({QUrl::fromLocalFile(path)});
        QDragEnterEvent enter(QPoint(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(&window,&enter); QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(&window,&drop); QVERIFY(drop.isAccepted());
        const auto rendered = [&window] { for (auto *player : window.findChildren<ThreeFpPlayer *>()) if (player->snapshot().videoWidth == 160 && player->snapshot().presentedVideoFrames > 0) return true; return false; };
        QTest::qWait(200);QVERIFY(!rendered());window.validateScript();
        QTRY_VERIFY_WITH_TIMEOUT(rendered(),15000);
    }
    void presetManagementActions()
    {
        const QString name = "__test_" + QUuid::createUuid().toString(QUuid::Id128);
        const QString target = QDir(PresetStore::directory()).filePath(name + ".vpy");
        const QString renamed = QDir(PresetStore::directory()).filePath(name + "_renamed.vpy");
        QString loaded;
        PresetDialog dialog([](const QString &note) { return PresetStore::create(FilterGraph(), SourceFilter::Ffms2, "Z:/missing.mkv", note); },
                            [&loaded](const QString &path) { loaded = path; }, [] {});
        dialog.show(); auto *input = dialog.findChild<QLineEdit *>("presetName"); QVERIFY(input); input->setText(name);
        const auto click = [&dialog](const QString &title) {
            for (auto *button : dialog.findChildren<QPushButton *>()) if (button->text() == title) { button->click(); return true; }
            return false;
        };
        QVERIFY(click(QStringLiteral("保存当前"))); QVERIFY(QFileInfo::exists(target));
        auto *list = dialog.findChild<QTreeWidget *>("presetList"); QVERIFY(list);
        for (int i = 0; i < list->topLevelItemCount(); ++i) if (list->topLevelItem(i)->text(0) == name) list->setCurrentItem(list->topLevelItem(i));
        QVERIFY(click(QStringLiteral("读取"))); QCOMPARE(loaded,target);
        input->setText(name + "_renamed"); QVERIFY(click(QStringLiteral("变更名称"))); QVERIFY(QFileInfo::exists(renamed)); QVERIFY(!QFileInfo::exists(target));
        QVERIFY(!PresetStore::metadata(PresetStore::load(renamed).script).isEmpty());
        dialog.grab().save("build/mingw-release/preset-management-layout.png");
        QVERIFY(QFile::remove(renamed));
    }
    void presetTreeSubdirectories(){
        const auto folder="__tree_"+QUuid::createUuid().toString(QUuid::Id128);const QDir root(PresetStore::directory());
        const auto path=root.filePath(folder+"/child/original.vpy"),renamed=root.filePath(folder+"/child/renamed.vpy");
        QVERIFY(PresetStore::write(path,PresetStore::create(FilterGraph(),SourceFilter::Ffms2,{},{})));
        const auto cleanup=qScopeGuard([&]{QFile::remove(path);QFile::remove(renamed);QDir().rmdir(root.filePath(folder+"/child"));QDir().rmdir(root.filePath(folder));});
        PresetDialog dialog([](const QString&){return QString();},[](const QString&){},[]{});auto *tree=dialog.findChild<QTreeWidget*>("presetList");QVERIFY(tree);
        QTreeWidgetItem *leaf=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).toString()==path)leaf=*it;
        QVERIFY(leaf && leaf->parent() && leaf->parent()->parent());QCOMPARE(leaf->parent()->text(0),QString("child"));QVERIFY(!leaf->parent()->isExpanded());leaf->parent()->setExpanded(true);QVERIFY(leaf->parent()->isExpanded());
        tree->setCurrentItem(leaf);dialog.findChild<QLineEdit*>("presetName")->setText("renamed");
        for(auto *button:dialog.findChildren<QPushButton*>())if(button->text()==QStringLiteral("变更名称"))button->click();
        QVERIFY(QFileInfo::exists(renamed));QVERIFY(!QFileInfo::exists(path));
    }
    void portablePreset()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath("source.mkv");
        QVERIFY(QFile::copy(":/startup/warmup.mkv", source));
        FilterGraph graph;
        const int row = graph.add("resize");
        graph.setParameter(row, "width", 160); graph.setParameter(row, "height", 90);
        const auto text = PresetStore::create(graph, SourceFilter::Ffms2, "Z:/missing.mkv", QStringLiteral("测试预设"));
        const QString preset = dir.filePath("滤镜.vpy");
        QVERIFY(PresetStore::write(preset, text));
        const auto loaded = PresetStore::load(preset, source);
        QCOMPARE(PresetStore::metadata(loaded.script).value("note").toString(), QStringLiteral("测试预设"));
        QCOMPARE(PresetStore::graph(PresetStore::metadata(loaded.script)).nodes().size(), 1);
        VapourSynthFrameServer server;
        QSignalSpy ready(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY(server.available());
        server.loadScript(loaded.script, preset);
        QTRY_VERIFY_WITH_TIMEOUT(ready.count() || errors.count(), 15000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto info = qvariant_cast<VapourSynthClipInfo>(ready.first().first());
        QCOMPARE(info.width, 160); QCOMPARE(info.height, 90);
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() || errors.count(), 10000);
        QVERIFY(errors.isEmpty()); QVERIFY(!frames.isEmpty());
    }
    void directComparison()
    {
        QTemporaryDir dir;
        QString ffmpeg = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
        if (!QFileInfo::exists(ffmpeg)) ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg.exe"));
        if (ffmpeg.isEmpty()) ffmpeg = QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe");
        for (int i = 0; i < 2; ++i) {
            QProcess process;
            process.start(ffmpeg, {"-v", "error", "-f", "lavfi", "-i", QString("testsrc2=size=320x180:rate=%1:duration=6").arg(i ? 30 : 24), "-c:v", "libx264", "-preset", "ultrafast", "-y", dir.filePath(QString("%1.mkv").arg(i))});
            QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
            QVERIFY(process.waitForFinished(15000));
            QCOMPARE(process.exitCode(), 0);
        }
        ThreeFpApi api;
        QVERIFY2(api.available(), qPrintable(api.errorString()));
        AnalysisPage page(api);
        page.resize(1280, 700);
        page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        page.openFiles({dir.filePath("0.mkv"), dir.filePath("1.mkv")});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 15000);
        QVERIFY(page.snapshot(0).presentedVideoFrames > 0);
        QVERIFY(page.snapshot(1).presentedVideoFrames > 0);
        page.seekGlobal(20000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 10000);
        QVERIFY(std::abs(page.snapshot(0).position100ns - 20000000) < 500000);
        const auto left = page.snapshot(0).position100ns;
        page.alignVideo(1, 1, false);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1) > 0, 10000);
        QCOMPARE(page.snapshot(0).position100ns, left);
        QVERIFY(page.offset(1) > 200000 && page.offset(1) < 500000);
        page.alignVideo(1, -1, false);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && std::abs(page.offset(1)) < 10000, 10000);
        page.alignVideo(1, 1, true);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1) > 9000000, 10000);
        QCOMPARE(page.snapshot(0).position100ns, left);
        const auto offset = page.offset(1);
        page.seekGlobal(10000000);
        page.seekGlobal(15000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && std::abs(page.snapshot(0).position100ns - 15000000) < 500000, 10000);
        QVERIFY(std::abs(page.snapshot(1).position100ns - page.snapshot(0).position100ns - offset) < 500000);
        page.stepGlobal(1);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.snapshot(0).position100ns > 15300000, 10000);
        QCOMPARE(page.offset(1), offset);
        page.findChild<QSlider *>("analysisTimeline1")->setValue(60000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.snapshot(1).position100ns > 35000000, 10000);
        QVERIFY(std::abs(page.snapshot(1).position100ns - page.snapshot(0).position100ns - offset) < 500000);
        page.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).state == ThreeFpState::Playing, 5000);
        QTest::qWait(600);
        page.pause();
        QTRY_VERIFY(page.snapshot(0).state == ThreeFpState::Paused);
        QTRY_VERIFY(page.snapshot(1).state == ThreeFpState::Paused);
        auto *view = page.findChild<MultiCompareView *>();
        const auto handleA = view->pane(0)->surface()->winId();
        page.findChild<QComboBox *>("analysisLayout")->setCurrentIndex(1);
        view->setDivision(0.23,0.5);
        page.resize(1100, 620);
        QTest::qWait(150);
        const auto presents = page.snapshot(0).swapChainPresents;
        view->pane(0)->adoptView(2.0f, 0, 0);
        emit view->pane(0)->viewChanged(2.0f, 0, 0);
        QTRY_VERIFY(page.snapshot(0).swapChainPresents > presents);
        QCOMPARE(view->pane(0)->surface()->winId(), handleA);
        page.showFullScreen();
        QTest::qWait(150);
        page.showNormal();
        page.resize(1280, 700);
        QTest::qWait(150);
        page.grab().save(QCoreApplication::applicationDirPath() + "/analysis-page.png");
        page.seekGlobal(59000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 10000);
        page.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).state != ThreeFpState::Playing && page.snapshot(1).state != ThreeFpState::Playing, 5000);
    }
    void multipleSourcesAndLayouts()
    {
        QTemporaryDir dir;
        const QString mp4=dir.filePath("source.mp4");
        QProcess process;
        process.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=4","-c:v","libx264","-preset","ultrafast","-y",mp4});
        QVERIFY(process.waitForFinished(15000)); QCOMPARE(process.exitCode(),0);
        ThreeFpApi api;
        AnalysisPage page(api); page.resize(1440,900); page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        for(auto *slider:page.findChildren<QSlider *>()) QVERIFY(!slider->isVisible());
        QStringList paths; for(int i=0;i<9;++i) paths.append(mp4);
        page.openFiles(paths);
        QCOMPARE(page.videoCount(),9);
        QCOMPARE(page.visibleVideos().size(),9);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),20000);
        for(int i=0;i<9;++i) QVERIFY(page.snapshot(i).presentedVideoFrames>0);
        int visibleRows=0;for(auto *slider:page.findChildren<QSlider *>())if(slider->isVisible())++visibleRows;
        QCOMPARE(visibleRows,4);
        page.alignVideo(8,1,true);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(8)>9000000,10000);
        const auto offset=page.offset(8);
        page.setMode(2);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        page.selectSource(0,8);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(page.visibleVideos().first(),8);QCOMPARE(page.offset(8),offset);
        page.selectSource(1,8);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(page.visibleVideos(),QList<int>({1,8}));QCOMPARE(page.offset(8),offset);
        page.setMode(3);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        auto *view=page.findChild<MultiCompareView *>();
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::ThreeLeft));
        const auto before=view->cellRect(0);
        auto *center=view->findChild<QWidget *>("comparisonDivider4");
        QVERIFY(center->isVisible());
        const QPoint local=center->rect().center();
        QTest::mousePress(center,Qt::LeftButton,Qt::NoModifier,local);
        const QPoint target=view->mapToGlobal(QPoint(view->width()*.6,view->height()*.7));
        QMouseEvent move(QEvent::MouseMove,center->mapFromGlobal(target),target,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(center,&move);QTest::mouseRelease(center,Qt::LeftButton);
        QVERIFY(view->cellRect(0).width()>before.width());
        QVERIFY(view->cellRect(1).height()>view->height()*.6);
        page.setMode(4);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        view->setDivision(.4,.6);QCOMPARE(view->cellRect(0).right()+1,view->cellRect(1).left());
        layout->setCurrentIndex(layout->findData(MultiCompareView::FourWipe));
        QTest::qWait(100);
        for (int slot = 0; slot < 4; ++slot) {
            auto *pane = view->pane(page.visibleVideos()[slot]);
            auto *surface = pane->surface();
            const QPoint target = view->cellRect(slot).center() + QPoint(0,28);
            const QPoint global = view->mapToGlobal(target);
            QMouseEvent hover(QEvent::MouseMove, surface->mapFromGlobal(global), global, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(surface, &hover);
            auto *label = pane->findChild<QLabel *>("pixelReadout");
            QTRY_VERIFY(label->isVisible());
            QVERIFY(label->text().contains("RGB"));
            QVERIFY(view->cellRect(slot).translated(0,28).contains(QRect(label->mapTo(view,QPoint(0,0)),label->size())));
        }
        auto *color = page.findChild<QComboBox *>("colorProcessing");
        color->setCurrentIndex(0); QCOMPARE(color->currentData().toInt(),0);
        QVERIFY(color->toolTip().contains("Nearest"));
        auto *retained = view->pane(8);
        const auto hwnd = retained->surface()->winId();
        QTest::mouseClick(view->findChild<QToolButton *>("removeVideo0"),Qt::LeftButton);
        QCOMPARE(page.videoCount(),8);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(view->pane(7),retained);
        QCOMPARE(retained->surface()->winId(),hwnd);
        QCOMPARE(page.offset(7),offset);
        page.openFiles({mp4});
        QCOMPARE(page.videoCount(),9);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QVERIFY(page.snapshot(8).presentedVideoFrames>0);
        page.setMode(9);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QTest::qWait(100);page.grab().save(QCoreApplication::applicationDirPath()+"/nine-way-analysis.png");
        while(page.videoCount()) page.removeVideo(0);
        QCOMPARE(page.visibleVideos().size(),0);
        for(auto *slider:page.findChildren<QSlider *>()) QVERIFY(!slider->isVisible());
        page.openFiles({mp4});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QVERIFY(page.snapshot(0).presentedVideoFrames>0);
    }
    void enhancementFiltersExecute()
    {
        QTemporaryDir dir;const QString input=dir.filePath("input.mkv");
        QVERIFY(QFile::copy(QStringLiteral(":/startup/warmup.mkv"),input));
        FilterGraph graph;
        for(const auto &id:{"deband","deblock","nlmeans","dering","sharpen_edges","crispen_edges","thin_edges","enhance_detail"}) QVERIFY(graph.add(id)>=0);
        const auto script=VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph);
        QVERIFY(script.errors.isEmpty());
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000);QVERIFY(server.available());
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        server.loadScript(script.script,dir.filePath("filters.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QVERIFY(!frames.isEmpty());
    }
    void expandedFiltersExecute()
    {
        QTemporaryDir dir; const QString input=dir.filePath("input.mkv");
        QVERIFY(QFile::copy(QStringLiteral(":/startup/warmup.mkv"),input));
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000); QVERIFY(server.available());
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded), errors(&server,&VapourSynthFrameServer::errorOccurred), frames(&server,&VapourSynthFrameServer::frameReady);
        for(const auto &id:{"temporal_median","flux_t","flux_st","smart_median","iq_mean","degrain_median","cnr4","ccd","dct_filter","temporal_soften","vertical_cleaner","clahe","descale","rife"}) {
            FilterGraph graph; const int row=graph.add(id); QVERIFY(row>=0);
            if(QString(id)=="descale") {graph.setParameter(row,"width",64);graph.setParameter(row,"height",36);}
            const auto script=VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph);
            QVERIFY(script.errors.isEmpty());
            loaded.clear();errors.clear();frames.clear();
            server.loadScript(script.script,dir.filePath("filters.vpy"));
            QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
            QVERIFY2(errors.isEmpty(),qPrintable(QString(id)+": "+(errors.isEmpty()?QString():errors.first().first().toString())));
            server.requestFrame(1);
            QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),20000);
            QVERIFY2(errors.isEmpty(),qPrintable(QString(id)+": "+(errors.isEmpty()?QString():errors.first().first().toString())));
            QVERIFY(!frames.isEmpty());
            qInfo()<<"Verified filter"<<id;
            if(QString(id)=="rife") {
                const auto info=qvariant_cast<VapourSynthClipInfo>(loaded.first().first()); const auto sourceInfo=qvariant_cast<VapourSynthClipInfo>(loaded.first().at(1)); QCOMPARE(info.totalFrames,sourceInfo.totalFrames*2);
                graph.setParameter(row,"model",QStringLiteral("4.26 Heavy"));
                loaded.clear();errors.clear();frames.clear();
                server.loadScript(VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph).script,dir.filePath("heavy.vpy"));
                QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                server.requestFrame(1);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),20000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                QVERIFY(!frames.isEmpty());
            }
        }
    }
    void compositionExportsCanvas()
    {
        QTemporaryDir dir; QStringList paths;
        const QString ffmpeg=QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe");
        const QStringList colors{"red","lime","blue","white"};
        for(int i=0;i<4;++i) {
            const QString path=dir.filePath(QString("%1.mkv").arg(i));paths<<path;
            QProcess process;process.start(ffmpeg,{"-v","error","-f","lavfi","-i",QString("color=%1:size=%2:rate=24:duration=2").arg(colors[i],i==3?"320x180":"160x90"),"-c:v","ffv1","-y",path});
            QVERIFY(process.waitForFinished(15000));QCOMPARE(process.exitCode(),0);
        }
        ThreeFpApi api;AnalysisPage page(api);page.resize(1700,1000);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));page.openFiles(paths);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),15000);
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::FourWipe));
        auto *view=page.findChild<MultiCompareView *>();view->setDivision(.35,.65);
        page.alignVideo(1,1,true);QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1)>9000000,10000);
        QVERIFY(!page.compositionScript().errors.isEmpty()); // D3D11 preview cannot be reproduced by VS.
        QComboBox *scaler=nullptr;for(auto *combo:page.findChildren<QComboBox*>())if(combo->findText(QStringLiteral("放大：Lanczos 4"))>=0)scaler=combo;QVERIFY(scaler);scaler->setCurrentIndex(7); // Lanczos 4 must map to a supported export kernel.
        const auto script=page.compositionScript();QVERIFY2(script.errors.isEmpty(),qPrintable(script.errors.join(";")));
        QFile saved(dir.filePath("composition.vpy"));QVERIFY(saved.open(QIODevice::WriteOnly));saved.write(script.script.toUtf8());saved.close();
        QProcess pipe;pipe.start(QCoreApplication::applicationDirPath()+"/runtime/python/Lib/site-packages/vapoursynth/vspipe.exe",{"--info",saved.fileName(),"-"});
        QVERIFY(pipe.waitForFinished(15000));QVERIFY2(pipe.exitCode()==0,pipe.readAllStandardError().constData());
        QVERIFY(pipe.readAllStandardOutput().contains("Width: 320"));
        QTest::mouseClick(page.findChild<QPushButton *>("exportComparison"),Qt::LeftButton);
        ExportWindow *window=nullptr;
        for(auto *widget:QApplication::topLevelWidgets()) if(auto *candidate=qobject_cast<ExportWindow *>(widget)) window=candidate;
        QVERIFY(window);
        QString output=dir.filePath("canvas.mkv");
        QString overwriteError;
        QVERIFY(!window->addJob("ffmpeg -y -i <输入文件> -c:v ffv1 <输出文件>",paths[2],paths[0],script.script,1,&overwriteError));
        QVERIFY(overwriteError.contains(QStringLiteral("覆盖")));
        for(const auto &parameter:QStringList{"-vf scale=160:90","-filter:v:0 crop=160:90","-s 160x90","-filter scale=160:90","-vf scale\\_cuda=160:90"}) {
            QString error;QVERIFY(!window->addJob("ffmpeg -y -i <输入文件> "+parameter+" -c:v ffv1 <输出文件>",output,paths[2],script.script,1,&error));
            QVERIFY(error.contains(QStringLiteral("分辨率")));
        }
        window->findChild<QPlainTextEdit *>("exportCommand")->setPlainText("ffmpeg -y -i <输入文件> -map 0:v -c:v ffv1 <输出文件>");
        window->findChild<QLineEdit *>("singleOutput")->setText(output);
        QTest::mouseClick(window->findChild<QPushButton *>("exportSingle"),Qt::LeftButton);
        output=dir.filePath(QStringLiteral("canvas_路1.mkv"));
        QCOMPARE(window->jobsStatus().size(),1);
        QCOMPARE(window->jobsStatus().first().toObject().value("output").toString(),output);
        QTRY_VERIFY_WITH_TIMEOUT(!page.exportBusy() && QFileInfo(output).size()>1000,30000);
        auto *table=window->findChild<QTreeWidget *>();QCOMPARE(table->topLevelItem(0)->text(2),QStringLiteral("100%"));
        QProcess decode;decode.start(ffmpeg,{"-v","error","-i",output,"-frames:v","1","-pix_fmt","rgb24","-f","rawvideo","pipe:1"});
        QVERIFY(decode.waitForFinished(15000));QCOMPARE(decode.exitCode(),0);
        const QByteArray rgb=decode.readAllStandardOutput();QCOMPARE(rgb.size(),320*180*3);
        const QList<QPoint> probes{{50,60},{240,60},{50,145},{240,145}};
        for(int i=0;i<4;++i) {
            const int index=(probes[i].y()*320+probes[i].x())*3;
            const int r=uchar(rgb[index]),g=uchar(rgb[index+1]),b=uchar(rgb[index+2]);
            QVERIFY2(i==0?r>200&&g<40&&b<40:i==1?g>200&&r<40&&b<40:i==2?b>200&&r<40&&g<40:r>200&&g>200&&b>200,qPrintable(QString("slot %1: %2 %3 %4").arg(i).arg(r).arg(g).arg(b)));
        }
        page.grab().save(QCoreApplication::applicationDirPath()+"/comparison-export-layout.png");
        auto *trackSource=page.findChild<QComboBox *>("analysisSource0");
        const int shortWidth=trackSource->width();
        const QString longName=QString(90,QLatin1Char('W'))+QStringLiteral(".mkv");
        trackSource->setItemText(trackSource->currentIndex(),longName);
        page.refreshLayout();
        QVERIFY(trackSource->width()>shortWidth);
        QVERIFY(page.findChild<QSlider *>("analysisTimeline0")->width()>=160);
        QCOMPARE(trackSource->toolTip(),longName);
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000);
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        for(int count:{2,3,4,9}) {
            if(count==9)page.openFiles({paths[0],paths[1],paths[2],paths[3],paths[0]});
            page.setMode(count);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
            for(int option=0;option<layout->count();++option) {
                layout->setCurrentIndex(option);
                QTest::qWait(50);
                auto *pane=view->pane(page.visibleVideos().first());
                pane->adoptView(1.25f,.2f,-.1f);
                const auto composition=page.compositionScript();
                const int mode=layout->currentData().toInt();
                const bool supported=mode==MultiCompareView::Wipe || mode==MultiCompareView::ThreeLeft || mode==MultiCompareView::ThreeRight || mode==MultiCompareView::Four || mode==MultiCompareView::FourWipe;
                if(!supported) {
                    QVERIFY(composition.errors.join(';').contains(QStringLiteral("不支持导出")));
                    QTest::mouseClick(page.findChild<QPushButton *>("exportComparison"),Qt::LeftButton);
                    bool visibleError=false;for(auto *label:page.findChildren<QLabel *>())if(label->isVisible() && label->text().contains(QStringLiteral("不支持导出")))visibleError=true;
                    QVERIFY(visibleError);continue;
                }
                QVERIFY2(composition.errors.isEmpty(),qPrintable(composition.errors.join(";")));
                loaded.clear();errors.clear();frames.clear();
                server.loadScript(composition.script,dir.filePath("layout.vpy"));
                QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                server.requestFrame(1);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                const auto frame=qvariant_cast<VapourSynthFrame>(frames.first().first());
                QCOMPARE(frame.width,320);QCOMPARE(frame.height,180);
            }
        }
        window->close();
    }
    void firstFrame2160p()
    {
        QTemporaryDir dir;const QString input=dir.filePath("2160p.mp4");QProcess process;
        process.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=3840x2160:rate=24:duration=1","-c:v","libx264","-preset","ultrafast","-y",input});
        QVERIFY(process.waitForFinished(20000));QCOMPARE(process.exitCode(),0);
        ThreeFpApi api; AnalysisPage page(api);page.resize(1200,800);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        QElapsedTimer elapsed;elapsed.start();page.openFiles({input});
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).presentedVideoFrames>0,15000);
        qInfo()<<"2160p first frame ms:"<<elapsed.elapsed();
        QVERIFY(elapsed.elapsed()<5000);
        page.openFiles({input});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        auto *view=page.findChild<MultiCompareView *>();
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::Wipe));
        QTest::qWait(150);
        auto *handle=view->findChild<QWidget *>("comparisonDivider0");
        ViewEventCounter counter;
        handle->installEventFilter(&counter);
        view->pane(0)->surface()->installEventFilter(&counter);
        view->pane(1)->surface()->installEventFilter(&counter);
        const auto size=view->pane(0)->surface()->size();
        const auto sourceFrame=page.snapshot(0).framePts;
        QTest::mousePress(handle,Qt::LeftButton,Qt::NoModifier,handle->rect().center());
        elapsed.restart();
        for(int step=0;step<100;++step) {
            const QPoint global=view->mapToGlobal(QPoint(200+step*5,200));
            QMouseEvent move(QEvent::MouseMove,handle->mapFromGlobal(global),global,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(handle,&move);
            QCoreApplication::processEvents();
            QCOMPARE(QWidget::mouseGrabber(),handle);
        }
        qInfo()<<"2160p AB 100 drag updates ms:"<<elapsed.elapsed();
        QVERIFY(elapsed.elapsed()<2000);
        QTest::mouseRelease(handle,Qt::LeftButton);
        QCOMPARE(counter.hides,0);QCOMPARE(counter.resizes,0);
        QCOMPARE(view->pane(0)->surface()->size(),size);
        QCOMPARE(view->pane(1)->surface()->size(),size);
        QCOMPARE(page.snapshot(0).framePts,sourceFrame);
    }
    void chromaAndScalingRender()
    {
        ThreeFpApi api;
        PreviewPane pane(QStringLiteral("Shader test"),QStringLiteral("Test"));pane.resize(640,480);pane.setSurfaceActive(true);pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        ThreeFpPlayer player(api,pane.surface());QVERIFY(player.ready());
        VapourSynthFrame frame;frame.width=64;frame.height=64;frame.format=ThreeFpExternalPixelFormat::Yuv420P8;
        frame.colorRange=2;frame.colorMatrix=1;frame.totalFrames=1;frame.duration100ns=400000;
        frame.planes[0]=QByteArray(64*64,char(128));frame.strides[0]=64;
        for(int p=1;p<3;++p){frame.planes[p].resize(32*32);frame.strides[p]=32;
            for(int y=0;y<32;++y)for(int x=0;x<32;++x)frame.planes[p][y*32+x]=char(((x+y+p)%2)?80:180);}
        QVERIFY(player.submitFrame(frame));
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);
        double nearest=0,linear=0;
        for(int kernel=0;kernel<=10;++kernel){
            const auto presents=player.snapshot().swapChainPresents;
            QVERIFY2(player.setChromaAlgorithm(kernel),qPrintable(player.lastError()));
            QVERIFY(player.submitFrame(frame));
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>presents,5000);
            double value=0;
            for(int x=201;x<210;++x){ThreeFpPixelProbe probe{};QVERIFY(player.samplePixel(x,203,probe));value+=probe.red+probe.blue*2;}
            if(kernel==0)nearest=value;if(kernel==1)linear=value;
        }
        QVERIFY(std::abs(nearest-linear)>0.001);
        for(auto algorithm:{ThreeFpScalingAlgorithm::Spline36,ThreeFpScalingAlgorithm::SuperXbrSinglePass}){
            const auto presents=player.snapshot().swapChainPresents;
            QVERIFY(player.setScalingAlgorithms(algorithm,ThreeFpScalingAlgorithm::Lanczos3));
            player.setView(2,0,0);QVERIFY(player.submitFrame(frame));
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>presents,5000);
        }
    }
    void wipeUsesSharedCanvasWithoutResizingOrLosingCapture()
    {
        MultiCompareView view;
        view.resize(960,628);
        view.setSources({"A","B","C","D"});
        for (int i=0;i<4;++i) view.pane(i)->setSurfaceActive(true);
        view.setSlots({0,1,2},MultiCompareView::ThreeNormal);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QCOMPARE(view.cellRect(0).width(),320);
        QCOMPARE(view.cellRect(1).width(),320);
        QCOMPARE(view.cellRect(2).width(),320);
        for (auto layout : {MultiCompareView::Wipe, MultiCompareView::ThreeRow,
                            MultiCompareView::ThreeColumn, MultiCompareView::ThreeLeft,
                            MultiCompareView::ThreeRight, MultiCompareView::FourWipe}) {
            const int count = layout == MultiCompareView::Wipe ? 2 : layout == MultiCompareView::FourWipe ? 4 : 3;
            QList<int> assignments;
            for (int i=0;i<count;++i) assignments.append(i);
            view.setSlots(assignments,layout);
            QTest::qWait(60);
            for (int i=0;i<count;++i) {
                QCOMPARE(view.pane(i)->surface()->size(),QSize(960,600));
                QCOMPARE(view.pane(i)->surface()->mapTo(&view,QPoint()),QPoint(0,28));
            }
            const int handleId = layout == MultiCompareView::ThreeColumn ? 2 :
                (layout == MultiCompareView::ThreeLeft || layout == MultiCompareView::ThreeRight || layout == MultiCompareView::FourWipe) ? 4 : 0;
            auto *handle = view.findChild<QWidget *>(QStringLiteral("comparisonDivider%1").arg(handleId));
            QVERIFY(handle->isVisible());
            ViewEventCounter counter;
            handle->installEventFilter(&counter);
            QList<WId> handles;
            for (int i=0;i<count;++i) {
                view.pane(i)->surface()->installEventFilter(&counter);
                handles.append(view.pane(i)->surface()->winId());
            }
            QTest::mousePress(handle,Qt::LeftButton,Qt::NoModifier,handle->rect().center());
            QCOMPARE(QWidget::mouseGrabber(),handle);
            QElapsedTimer elapsed;elapsed.start();
            for (int step=0;step<100;++step) {
                const QPoint global=view.mapToGlobal(QPoint(250+step*2,200+step));
                QMouseEvent move(QEvent::MouseMove,handle->mapFromGlobal(global),global,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
                QApplication::sendEvent(handle,&move);
                QCOMPARE(QWidget::mouseGrabber(),handle);
            }
            qInfo()<<"Wipe mode"<<int(layout)<<"100 drag updates ms:"<<elapsed.elapsed();
            QVERIFY(elapsed.elapsed()<1500);
            QTest::mouseRelease(handle,Qt::LeftButton);
            QCOMPARE(counter.hides,0);
            QCOMPARE(counter.resizes,0);
            int area=0;
            for (int i=0;i<count;++i) {
                QCOMPARE(view.pane(i)->surface()->size(),QSize(960,600));
                QCOMPARE(view.pane(i)->surface()->mapTo(&view,QPoint()),QPoint(0,28));
                QCOMPARE(view.pane(i)->surface()->winId(),handles[i]);
                area += view.cellRect(i).width()*view.cellRect(i).height();
                for (int j=0;j<i;++j) QVERIFY(!view.cellRect(i).intersects(view.cellRect(j)));
            }
            QCOMPARE(area,960*600);
        }
        view.setSlots({0,1,2,3},MultiCompareView::Four);
        QVERIFY(!view.isWipe());
        QCOMPARE(view.cellRect(0).size(),QSize(480,314));
    }

    void rifePreviewPresentsIntermediateFrames()
    {
        QTemporaryDir dir;const QString input=dir.filePath("rife.mkv");
        QProcess encode;encode.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=4","-c:v","libx264","-preset","ultrafast","-y",input});
        QVERIFY(encode.waitForFinished(15000));QCOMPARE(encode.exitCode(),0);
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        QMimeData mime;mime.setUrls({QUrl::fromLocalFile(input)});
        QDragEnterEvent enter(QPoint(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&window,&enter);QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&window,&drop);QVERIFY(drop.isAccepted());
        bool found=false;
        for(auto *list:window.findChildren<QListWidget *>()) for(int i=0;i<list->count();++i)
            if(list->item(i)->data(Qt::UserRole).toString()=="rife") {list->setCurrentRow(i);found=true;}
        QVERIFY(found);
        for(auto *button:window.findChildren<QPushButton *>()) if(button->text()==QStringLiteral("添加到处理链"))button->click();
        auto *server=window.findChild<VapourSynthFrameServer *>();QVERIFY(server);
        QSignalSpy loaded(server,&VapourSynthFrameServer::scriptLoaded),frames(server,&VapourSynthFrameServer::frameReady),errors(server,&VapourSynthFrameServer::errorOccurred);
        for(auto *action:window.findChildren<QAction *>()) if(action->text()==QStringLiteral("生成并验证")) action->trigger();
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),15000);
        QPushButton *play=nullptr;
        for(auto *button:window.findChildren<QPushButton *>()) if(button->text()==QStringLiteral("播放"))play=button;
        QVERIFY(play);frames.clear();play->click();QTest::qWait(700);play->click();
        int intermediate=0;
        for(const auto &entry:frames) if(qvariant_cast<VapourSynthFrame>(entry.first()).frameIndex%2==1)++intermediate;
        qInfo()<<"RIFE preview submitted frames"<<frames.size()<<"intermediate frames"<<intermediate;
        QVERIFY(frames.size()>8);QVERIFY(intermediate>2);
        QTimer::singleShot(0,&window,[]{if(auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))box->done(QMessageBox::Discard);});
        window.close();
        QVERIFY(!window.isVisible());
    }
    void navigation()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *rail = window.findChild<QWidget *>("pageNavigation");
        QVERIFY(rail);
        QCOMPARE(rail->width(), 44);
        auto *toggle = window.findChild<QAction *>("navigationToggle");
        QVERIFY(toggle);
        auto *toggleButton = rail->findChild<QToolButton *>("navigationToggleButton");
        QVERIFY(toggleButton);
        QCOMPARE(toggleButton->defaultAction(), toggle);
        QCOMPARE(rail->layout()->itemAt(0)->widget(), toggleButton);
        toggle->trigger();
        QCOMPARE(rail->width(), 180);
        QCOMPARE(rail->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly).size(),
            window.navigationButtons_.size() + 1);
        for (auto *button : window.navigationButtons_) {
            QCOMPARE(button->size(), toggleButton->size());
            QCOMPARE(button->iconSize(), toggleButton->iconSize());
        }
        QVERIFY(!window.findChild<QComboBox *>("sourceFilter")->isVisible());
        window.findChild<QToolButton *>("applicationSettings")->click();
        auto *settingsSource=window.findChild<QComboBox *>("rendererSettingsSource");
        QVERIFY(settingsSource && settingsSource->isVisible());
        window.navigationButtons_[1]->click();
        QVERIFY(window.findChild<AnalysisPage *>()->isVisible());
        window.navigationButtons_[0]->click();
        QVERIFY(!window.findChild<AnalysisPage *>()->isVisible());
        toggle->trigger();
        QCOMPARE(rail->width(), 44);
        window.navigationButtons_[1]->click();
        toggle->trigger();
        QTest::qWait(150);
        window.grab().save(QCoreApplication::applicationDirPath() + "/analysis-navigation.png");
    }
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setStyleSheet(vsr::applicationStyleSheet());
    TestAnalysis test;
    return QTest::qExec(&test, argc, argv);
}
#include "TestAnalysis.moc"
