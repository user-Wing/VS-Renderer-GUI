#include "ui/CommunityWebView.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLibrary>
#include <QPointer>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#ifdef VSR_WEBVIEW2
#include <windows.h>
#include <WebView2.h>
#endif

namespace vsr {
namespace {
class BasicCommunityDocument final : public QTextBrowser {
public:
    explicit BasicCommunityDocument(QWidget *parent) : QTextBrowser(parent)
    {
        setOpenLinks(false);
        auto font = this->font();
        font.setPixelSize(18);
        setFont(font);
        document()->setDocumentMargin(20);
    }
protected:
    QVariant loadResource(int, const QUrl &) override { return {}; }
};
}

QUrl CommunityWebView::home() { return QUrl(QStringLiteral("https://www1.arxchem.top/")); }

bool CommunityWebView::isSite(const QUrl &url)
{
    return url.isValid() && url.scheme() == "https" && url.host() == home().host()
        && url.userInfo().isEmpty() && url.port(443) == 443;
}

bool CommunityWebView::isEmbeddedSite(const QUrl &url)
{
    return isSite(url) || (url.scheme() == "https" && url.host() == "share.arxchem.top"
        && url.userInfo().isEmpty() && url.port(443) == 443);
}

#ifdef VSR_WEBVIEW2
namespace {

// The SDK callbacks are COM interfaces. Keep Qt object lifetimes independent of callbacks
// still held by the browser process after the community page closes.
template<class Interface, class... Args>
class Callback final : public Interface {
public:
    Callback(REFIID iid, std::function<HRESULT(Args...)> invoke) : iid_(iid), invoke_(std::move(invoke)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **out) override
    {
        *out = nullptr;
        if (id != iid_ && id != __uuidof(IUnknown)) return E_NOINTERFACE;
        *out = static_cast<Interface *>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto refs = --refs_;
        if (!refs) delete this;
        return refs;
    }
    HRESULT STDMETHODCALLTYPE Invoke(Args... args) override { return invoke_(args...); }

private:
    ULONG refs_ = 1;
    const IID iid_;
    std::function<HRESULT(Args...)> invoke_;
};

template<class Interface>
void releaseCom(Interface *object)
{
    if (object) object->Release();
}

template<class Interface>
using ComPtr = std::unique_ptr<Interface, decltype(&releaseCom<Interface>)>;

template<class Interface, class... Args>
ComPtr<Interface> callback(REFIID iid, std::function<HRESULT(Args...)> invoke)
{
    return {new Callback<Interface, Args...>(iid, std::move(invoke)), &releaseCom<Interface>};
}

template<class Interface>
ComPtr<Interface> query(IUnknown *source, REFIID iid)
{
    Interface *result = nullptr;
    source->QueryInterface(iid, reinterpret_cast<void **>(&result));
    return {result, &releaseCom<Interface>};
}

const wchar_t sessionName[] = L"forum_session";
const wchar_t siteHost[] = L"www1.arxchem.top";
const auto documentUrl = QUrl(QStringLiteral("https://vsr-markdown.invalid/index.html"));
const auto editorUrl = QUrl(QStringLiteral("https://vsr-markdown.invalid/editor.html"));

bool isLoginProvider(const QUrl &url)
{
    const auto host = url.host();
    return url.scheme() == "https" && url.userInfo().isEmpty() && url.port(443) == 443
        && (host == "github.com" || host == "modelscope.cn" || host == "www.modelscope.cn"
            || host == "account.aliyun.com" || host == "passport.aliyun.com");
}

// Keep the site's own account menu and application content. Navigation lives in Qt.
const wchar_t siteAppearance[] = LR"JS(
(() => {
    if (window.top !== window || location.origin !== 'https://www1.arxchem.top') return;
    document.addEventListener('DOMContentLoaded', async () => {
        const style = document.createElement('style');
        style.textContent = '.topbar .main-nav,.topbar .brand,.site-status-footer,#secondaryBar{display:none!important}'
            + ':root{--secondary-bar-height:0px!important}.topbar .header-actions{margin-left:auto}';
        document.head.append(style);
        const mode = new URLSearchParams(location.search).get('vsr_login');
        if (location.pathname === '/forum' && (mode === '1' || mode === 'register')) {
            style.textContent += 'body > :not(#modalRoot){display:none!important}body{min-height:100vh}';
            const session = await window.getForumSession?.();
            if (session?.authenticated) location.replace('/account');
            else if (window.openAuthModal) {
                await window.openAuthModal(mode === 'register' ? 'register' : 'login');
                history.replaceState(null, '', location.pathname);
            }
        }
    }, {once:true});
})()
)JS";

}
#endif

struct CommunityWebView::Impl {
    CommunityWebView *widget;
    SessionChanged sessionChanged;
    QLabel *hint;
    QUrl target;
    QByteArray initialCookie;
    DocumentLink documentLink;
    std::function<void(const QJsonArray &)> outlineChanged;
    std::function<void(bool)> editorReady;
    std::function<void(const QJsonObject &)> editorMessage;
    QJsonObject document;
    bool localDocument = false;
    bool documentFailed = false;
    BasicCommunityDocument *fallback = nullptr;
#ifdef VSR_WEBVIEW2
    ICoreWebView2Controller *controller = nullptr;
    ICoreWebView2 *view = nullptr;
    ICoreWebView2CookieManager *cookies = nullptr;
    bool comInitialized = false;
    bool opening = false;
    bool reading = false;
    bool documentReady = false;
    bool rendered = false;
    unsigned renderGeneration = 0;
    unsigned sessionGeneration = 0;
#endif

    void unavailable()
    {
        if (editorReady) { editorReady(false); return; }
        hint->setText(localDocument ? QObject::tr("基础 Markdown 预览。公式和高级排版需要 WebView2 与随程序提供的资源。")
                                    : QObject::tr("内嵌网页需要 Microsoft Edge WebView2 Runtime 和随程序提供的加载器。可使用上方的“浏览器打开”。"));
        if (localDocument) {
            documentFailed = true;
            if (!fallback) {
                fallback = new BasicCommunityDocument(widget);
                QObject::connect(fallback, &QTextBrowser::anchorClicked, widget, [this](const QUrl &url) {
                    const auto link = QUrl(document.value("base").toString()).resolved(url);
                    if (documentLink && (link.scheme() == "http" || link.scheme() == "https") && link.userInfo().isEmpty())
                        documentLink(link);
                });
                auto *layout = static_cast<QVBoxLayout *>(widget->layout());
                delete layout->takeAt(1);
                layout->addWidget(fallback, 1);
            }
            showFallback();
            hint->show();
        }
#ifdef VSR_WEBVIEW2
        opening = false;
        if (localDocument && controller) controller->put_IsVisible(FALSE);
#endif
    }

    void showFallback()
    {
        const auto features = QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub) | QTextDocument::MarkdownNoHTML;
        if (outlineChanged) {
            const auto blocks = document.value("blocks").toArray();
            const auto article = blocks.isEmpty() ? QJsonObject() : blocks.first().toObject();
            auto title = article.value("title").toString().simplified();
            for (const auto mark : QStringLiteral("\\`*_~<>[]{}#!.()+-=|"))
                title.replace(mark, QStringLiteral("\\") + mark);
            fallback->document()->setMarkdown(title + "\n\n" + article.value("markdown").toString(), features);
            QJsonArray headings;
            for (auto block = fallback->document()->begin(); block.isValid(); block = block.next())
                if (const int level = block.blockFormat().headingLevel(); level > 0 && !block.text().trimmed().isEmpty())
                    headings.append(QJsonObject{{"level", level}, {"title", block.text().trimmed()}});
            outlineChanged(headings);
            return;
        }
        fallback->clear();
        QTextCursor cursor(fallback->document());
        for (const auto &block : document.value("blocks").toArray()) {
            const auto data = block.toObject();
            cursor.insertText(data.value("title").toString());
            cursor.insertBlock();
            cursor.insertText(data.value("meta").toString());
            cursor.insertBlock();
            cursor.insertMarkdown(data.value("markdown").toString(), features);
            cursor.movePosition(QTextCursor::End);
            cursor.insertBlock();
        }
    }

#ifdef VSR_WEBVIEW2
    void size()
    {
        if (!controller) return;
        RECT bounds{};
        GetClientRect(reinterpret_cast<HWND>(widget->winId()), &bounds);
        controller->put_Bounds(bounds);
        controller->put_IsVisible(widget->isVisible() && !documentFailed);
    }

    void create()
    {
        if (!comInitialized) {
            comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            if (!comInitialized) { unavailable(); return; }
        }
        static auto *loader = new QLibrary(QCoreApplication::applicationDirPath() + "/WebView2Loader.dll");
        loader->setLoadHints(QLibrary::PreventUnloadHint);
        using Create = decltype(&CreateCoreWebView2EnvironmentWithOptions);
        auto createEnvironment = reinterpret_cast<Create>(loader->resolve("CreateCoreWebView2EnvironmentWithOptions"));
        if (!createEnvironment) { unavailable(); return; }
        opening = true;
        hint->setText(QObject::tr("正在打开网页…"));
        const QPointer<CommunityWebView> guard(widget);
        const std::function<HRESULT(HRESULT, ICoreWebView2Environment *)> onEnvironment =
            [guard](HRESULT result, ICoreWebView2Environment *environment) {
                if (!guard) return S_OK;
                auto &host = *guard->impl_;
                if (FAILED(result) || !environment) { host.unavailable(); return S_OK; }
                auto modern = query<ICoreWebView2Environment10>(environment, IID_ICoreWebView2Environment10);
                ICoreWebView2ControllerOptions *rawOptions = nullptr;
                if (!modern || FAILED(modern->CreateCoreWebView2ControllerOptions(&rawOptions))) {
                    host.unavailable();
                    return S_OK;
                }
                ComPtr<ICoreWebView2ControllerOptions> options(rawOptions, &releaseCom<ICoreWebView2ControllerOptions>);
                if (FAILED(options->put_IsInPrivateModeEnabled(TRUE))) { host.unavailable(); return S_OK; }
                if (host.localDocument && FAILED(options->put_ProfileName(L"VSRMarkdown"))) {
                    host.unavailable();
                    return S_OK;
                }
                const std::function<HRESULT(HRESULT, ICoreWebView2Controller *)> onController =
                    [guard](HRESULT status, ICoreWebView2Controller *controller) {
                        if (guard) {
                            if (SUCCEEDED(status) && controller) guard->impl_->attach(controller);
                            else guard->impl_->unavailable();
                        } else if (controller) {
                            controller->Close();
                        }
                        return S_OK;
                    };
                auto done = callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, onController);
                if (FAILED(modern->CreateCoreWebView2ControllerWithOptions(
                    reinterpret_cast<HWND>(guard->winId()), options.get(), done.get()))) host.unavailable();
                return S_OK;
            };
        auto done = callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, onEnvironment);
        const auto data = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/community-webview";
        if (FAILED(createEnvironment(nullptr, reinterpret_cast<LPCWSTR>(data.utf16()), nullptr, done.get()))) unavailable();
    }

    void attach(ICoreWebView2Controller *ready)
    {
        controller = ready;
        controller->AddRef();
        if (FAILED(controller->get_CoreWebView2(&view))) { unavailable(); return; }
        auto modern = query<ICoreWebView2_2>(view, IID_ICoreWebView2_2);
        if (modern && !localDocument) modern->get_CookieManager(&cookies);
        ICoreWebView2Settings *rawSettings = nullptr;
        if (SUCCEEDED(view->get_Settings(&rawSettings))) {
            ComPtr<ICoreWebView2Settings> settings(rawSettings, &releaseCom<ICoreWebView2Settings>);
            settings->put_IsWebMessageEnabled(FALSE);
            settings->put_AreHostObjectsAllowed(FALSE);
            settings->put_AreDevToolsEnabled(FALSE);
            if (localDocument) settings->put_IsZoomControlEnabled(FALSE);
            if (editorReady) settings->put_IsWebMessageEnabled(TRUE);
            auto password = query<ICoreWebView2Settings4>(settings.get(), IID_ICoreWebView2Settings4);
            if (password) {
                password->put_IsPasswordAutosaveEnabled(FALSE);
                password->put_IsGeneralAutofillEnabled(FALSE);
            }
        }
        EventRegistrationToken token{};
        const QPointer<CommunityWebView> guard(widget);
        if (localDocument) { attachDocument(); return; }
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *)> navigating =
            [](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args) {
                LPWSTR raw = nullptr;
                if (FAILED(args->get_Uri(&raw))) return S_OK;
                const QUrl url(QString::fromWCharArray(raw));
                CoTaskMemFree(raw);
                if (CommunityWebView::isEmbeddedSite(url) || isLoginProvider(url)) return S_OK;
                args->put_Cancel(TRUE);
                BOOL clicked = FALSE;
                args->get_IsUserInitiated(&clicked);
                if (clicked && (url.scheme() == "http" || url.scheme() == "https")) QDesktopServices::openUrl(url);
                return S_OK;
            };
        view->add_NavigationStarting(callback<ICoreWebView2NavigationStartingEventHandler>(
            IID_ICoreWebView2NavigationStartingEventHandler, navigating).get(), &token);
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *)> popup =
            [](ICoreWebView2 *view, ICoreWebView2NewWindowRequestedEventArgs *args) {
                args->put_Handled(TRUE);
                LPWSTR raw = nullptr;
                BOOL clicked = FALSE;
                args->get_IsUserInitiated(&clicked);
                if (!clicked || FAILED(args->get_Uri(&raw))) return S_OK;
                const QUrl url(QString::fromWCharArray(raw));
                if (CommunityWebView::isEmbeddedSite(url) || isLoginProvider(url)) view->Navigate(raw);
                else if (url.scheme() == "http" || url.scheme() == "https") QDesktopServices::openUrl(url);
                CoTaskMemFree(raw);
                return S_OK;
            };
        view->add_NewWindowRequested(callback<ICoreWebView2NewWindowRequestedEventHandler>(
            IID_ICoreWebView2NewWindowRequestedEventHandler, popup).get(), &token);
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *)> loaded =
            [guard](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                if (guard) guard->syncSession();
                return S_OK;
            };
        view->add_NavigationCompleted(callback<ICoreWebView2NavigationCompletedEventHandler>(
            IID_ICoreWebView2NavigationCompletedEventHandler, loaded).get(), &token);
        const std::function<HRESULT(HRESULT, LPCWSTR)> scriptReady = [guard](HRESULT, LPCWSTR) {
            if (!guard) return S_OK;
            auto &host = *guard->impl_;
            host.opening = false;
            host.restoreCookie();
            host.hint->hide();
            host.size();
            host.navigate();
            return S_OK;
        };
        if (FAILED(view->AddScriptToExecuteOnDocumentCreated(siteAppearance,
            callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                IID_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler, scriptReady).get())))
            scriptReady(E_FAIL, nullptr);
    }

    void restoreCookie()
    {
        ICoreWebView2Cookie *raw = nullptr;
        const auto value = QString::fromLatin1(initialCookie);
        if (cookies && !value.isEmpty() && SUCCEEDED(cookies->CreateCookie(sessionName,
            reinterpret_cast<LPCWSTR>(value.utf16()), siteHost, L"/", &raw))) {
            ComPtr<ICoreWebView2Cookie> cookie(raw, &releaseCom<ICoreWebView2Cookie>);
            cookie->put_IsHttpOnly(TRUE);
            cookie->put_IsSecure(TRUE);
            cookie->put_SameSite(COREWEBVIEW2_COOKIE_SAME_SITE_KIND_LAX);
            cookies->AddOrUpdateCookie(cookie.get());
        }
        initialCookie.clear();
    }

    void navigate()
    {
        const auto url = target.toString(QUrl::FullyEncoded);
        view->Navigate(reinterpret_cast<LPCWSTR>(url.utf16()));
    }

    void attachDocument()
    {
        auto local = query<ICoreWebView2_3>(view, IID_ICoreWebView2_3);
        const auto root = QDir(QCoreApplication::applicationDirPath()).filePath("community-markdown");
        if (!local || !QFileInfo(QDir(root).filePath("index.html")).isFile() ||
            FAILED(local->SetVirtualHostNameToFolderMapping(L"vsr-markdown.invalid",
                reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(root).utf16()), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY))) {
            unavailable();
            return;
        }
        const QPointer<CommunityWebView> guard(widget);
        EventRegistrationToken token{};
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *)> navigating =
            [guard](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args) {
                LPWSTR raw = nullptr;
                if (FAILED(args->get_Uri(&raw))) { args->put_Cancel(TRUE); return S_OK; }
                const QUrl url(QString::fromWCharArray(raw));
                CoTaskMemFree(raw);
                if (guard && url.matches(guard->impl_->target, QUrl::RemoveFragment)) return S_OK;
                args->put_Cancel(TRUE);
                BOOL clicked = FALSE;
                args->get_IsUserInitiated(&clicked);
                if (guard && clicked && (url.scheme() == "http" || url.scheme() == "https") && url.userInfo().isEmpty())
                    QTimer::singleShot(0, guard, [guard, url] { if (guard->impl_->documentLink) guard->impl_->documentLink(url); });
                return S_OK;
            };
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *)> popup =
            [](ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *args) { args->put_Handled(TRUE); return S_OK; };
        const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *)> loaded =
            [guard](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *args) {
                if (!guard || guard->impl_->documentReady) return S_OK;
                auto &host = *guard->impl_;
                BOOL success = FALSE;
                if (FAILED(args->get_IsSuccess(&success)) || !success) { host.unavailable(); return S_OK; }
                host.opening = false;
                host.documentReady = true;
                if (!host.editorReady) host.renderDocument();
                return S_OK;
            };
        if (editorReady) {
            const std::function<HRESULT(ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *)> receive =
                [guard](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) {
                    LPWSTR raw = nullptr;
                    if (!guard || FAILED(args->get_Source(&raw)) || !raw) return S_OK;
                    const QUrl url(QString::fromWCharArray(raw));
                    CoTaskMemFree(raw);
                    if (url != editorUrl || FAILED(args->get_WebMessageAsJson(&raw)) || !raw) return S_OK;
                    const auto data = QString::fromWCharArray(raw).toUtf8();
                    CoTaskMemFree(raw);
                    if (data.size() > 15 * 1024 * 1024) return S_OK;
                    const auto message = QJsonDocument::fromJson(data).object();
                    auto &host = *guard->impl_;
                    if (message.value("type") == "ready") {
                        host.opening = false;
                        host.documentReady = true;
                        host.hint->hide();
                        host.editorReady(true);
                    } else if (message.value("type") == "error") host.unavailable();
                    else if (host.editorMessage) host.editorMessage(message);
                    return S_OK;
                };
            if (FAILED(view->add_WebMessageReceived(callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    IID_ICoreWebView2WebMessageReceivedEventHandler, receive).get(), &token))) {
                unavailable();
                return;
            }
        }
        if (FAILED(view->add_NavigationStarting(callback<ICoreWebView2NavigationStartingEventHandler>(
                IID_ICoreWebView2NavigationStartingEventHandler, navigating).get(), &token)) ||
            FAILED(view->add_NewWindowRequested(callback<ICoreWebView2NewWindowRequestedEventHandler>(
                IID_ICoreWebView2NewWindowRequestedEventHandler, popup).get(), &token)) ||
            FAILED(view->add_NavigationCompleted(callback<ICoreWebView2NavigationCompletedEventHandler>(
                IID_ICoreWebView2NavigationCompletedEventHandler, loaded).get(), &token))) {
            unavailable();
            return;
        }
        size();
        navigate();
    }

    void execute(const QString &script)
    {
        if (documentReady) view->ExecuteScript(reinterpret_cast<LPCWSTR>(script.utf16()), nullptr);
    }

    void renderDocument()
    {
        const auto script = QStringLiteral("window.vsrRenderDocument(%1)%2;")
            .arg(QString::fromUtf8(QJsonDocument(document).toJson(QJsonDocument::Compact)),
                 outlineChanged ? QStringLiteral(" && window.vsrDocumentOutline()") : QString());
        const auto generation = ++renderGeneration;
        rendered = false;
        const QPointer<CommunityWebView> guard(widget);
        const std::function<HRESULT(HRESULT, LPCWSTR)> done = [guard, generation](HRESULT status, LPCWSTR result) {
            if (!guard || generation != guard->impl_->renderGeneration) return S_OK;
            auto &host = *guard->impl_;
            const auto value = result ? QString::fromWCharArray(result) : QString();
            const auto outline = host.outlineChanged ? QJsonDocument::fromJson(value.toUtf8()) : QJsonDocument();
            host.rendered = SUCCEEDED(status) && (host.outlineChanged ? outline.isArray() : value == "true");
            if (host.rendered && host.outlineChanged) host.outlineChanged(outline.array());
            if (host.rendered) host.hint->hide();
            else host.unavailable();
            return S_OK;
        };
        if (FAILED(view->ExecuteScript(reinterpret_cast<LPCWSTR>(script.utf16()),
                callback<ICoreWebView2ExecuteScriptCompletedHandler>(IID_ICoreWebView2ExecuteScriptCompletedHandler, done).get())))
            unavailable();
    }
#endif
};

CommunityWebView::CommunityWebView(SessionChanged sessionChanged, QWidget *parent)
    : QWidget(parent), impl_(std::make_unique<Impl>())
{
    impl_->widget = this;
    impl_->sessionChanged = std::move(sessionChanged);
    setObjectName("communityWebView");
    impl_->hint = new QLabel(this);
    impl_->hint->setWordWrap(true);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(impl_->hint, 0, Qt::AlignTop);
    layout->addStretch();
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    if (impl_->sessionChanged) {
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this] { if (isVisible()) syncSession(); });
        timer->start(1500);
    }
}

CommunityWebView::CommunityWebView(DocumentLink link, QWidget *parent)
    : CommunityWebView(SessionChanged{}, parent)
{
    impl_->localDocument = true;
    impl_->documentLink = std::move(link);
    setObjectName("communityMarkdownWeb");
}

CommunityWebView::~CommunityWebView()
{
#ifdef VSR_WEBVIEW2
    if (impl_->controller) impl_->controller->Close();
    releaseCom(impl_->cookies);
    releaseCom(impl_->view);
    releaseCom(impl_->controller);
    if (impl_->comInitialized) CoUninitialize();
#endif
}

void CommunityWebView::open(const QUrl &url, const QByteArray &cookie)
{
    if (impl_->localDocument || !isEmbeddedSite(url)) return;
    impl_->target = url;
    setSession(cookie);
#ifdef VSR_WEBVIEW2
    if (impl_->opening) return;
    if (impl_->view) { impl_->navigate(); return; }
    impl_->create();
#else
    Q_UNUSED(cookie)
    impl_->unavailable();
#endif
}

void CommunityWebView::setSession(const QByteArray &cookie)
{
    if (impl_->localDocument) return;
    impl_->initialCookie = cookie;
#ifdef VSR_WEBVIEW2
    ++impl_->sessionGeneration;
    if (impl_->cookies) {
        if (cookie.isEmpty()) impl_->cookies->DeleteCookiesWithDomainAndPath(sessionName, siteHost, L"/");
        else impl_->restoreCookie();
    }
#endif
}

void CommunityWebView::clearSession() { setSession({}); }

void CommunityWebView::syncSession()
{
#ifdef VSR_WEBVIEW2
    if (!impl_->cookies || impl_->opening || impl_->reading) return;
    impl_->reading = true;
    const QPointer<CommunityWebView> guard(this);
    const auto generation = impl_->sessionGeneration;
    const std::function<HRESULT(HRESULT, ICoreWebView2CookieList *)> read =
        [guard, generation](HRESULT status, ICoreWebView2CookieList *list) {
            if (!guard) return S_OK;
            auto &host = *guard->impl_;
            host.reading = false;
            if (FAILED(status) || !list || generation != host.sessionGeneration) return S_OK;
            UINT count = 0;
            list->get_Count(&count);
            QByteArray value;
            for (UINT i = 0; i < count; ++i) {
                ICoreWebView2Cookie *raw = nullptr;
                if (FAILED(list->GetValueAtIndex(i, &raw))) continue;
                ComPtr<ICoreWebView2Cookie> cookie(raw, &releaseCom<ICoreWebView2Cookie>);
                LPWSTR name = nullptr, domain = nullptr, path = nullptr;
                cookie->get_Name(&name);
                cookie->get_Domain(&domain);
                cookie->get_Path(&path);
                const bool valid = name && domain && path && QString::fromWCharArray(name) == "forum_session"
                    && QString::fromWCharArray(domain) == "www1.arxchem.top" && QString::fromWCharArray(path) == "/";
                CoTaskMemFree(name);
                CoTaskMemFree(domain);
                CoTaskMemFree(path);
                if (!valid) continue;
                LPWSTR token = nullptr;
                if (SUCCEEDED(cookie->get_Value(&token))) {
                    value = QString::fromWCharArray(token).toLatin1();
                    CoTaskMemFree(token);
                }
                break;
            }
            host.sessionChanged(value);
            return S_OK;
        };
    if (FAILED(impl_->cookies->GetCookies(L"https://www1.arxchem.top/",
        callback<ICoreWebView2GetCookiesCompletedHandler>(
            IID_ICoreWebView2GetCookiesCompletedHandler, read).get()))) impl_->reading = false;
#endif
}

void CommunityWebView::goBack()
{
#ifdef VSR_WEBVIEW2
    BOOL canGoBack = FALSE;
    if (impl_->view && SUCCEEDED(impl_->view->get_CanGoBack(&canGoBack)) && canGoBack) impl_->view->GoBack();
#endif
}

QUrl CommunityWebView::currentUrl() const
{
#ifdef VSR_WEBVIEW2
    LPWSTR raw = nullptr;
    if (impl_->view && SUCCEEDED(impl_->view->get_Source(&raw)) && raw) {
        const QUrl url(QString::fromWCharArray(raw));
        CoTaskMemFree(raw);
        if (isEmbeddedSite(url)) return url;
    }
#endif
    return impl_->target;
}

void CommunityWebView::showDocument(const QJsonObject &document)
{
    if (!impl_->localDocument) return;
    impl_->document = document;
    if (impl_->documentFailed) { impl_->showFallback(); return; }
#ifdef VSR_WEBVIEW2
    impl_->target = documentUrl;
    if (impl_->documentReady) impl_->renderDocument();
    else if (!impl_->opening) impl_->create();
#else
    impl_->unavailable();
#endif
}

void CommunityWebView::documentZoom(int step)
{
    if (auto *fallback = impl_->fallback) {
        if (step > 0) fallback->zoomIn();
        else if (step < 0) fallback->zoomOut();
        else {
            auto font = fallback->font();
            font.setPixelSize(18);
            fallback->setFont(font);
        }
        return;
    }
#ifdef VSR_WEBVIEW2
    if (impl_->localDocument) impl_->execute(QStringLiteral("window.vsrDocumentZoom(%1);").arg(step));
#else
    Q_UNUSED(step)
#endif
}

void CommunityWebView::onOutline(std::function<void(const QJsonArray &)> changed)
{
    impl_->outlineChanged = std::move(changed);
}

void CommunityWebView::scrollToHeading(int index)
{
    if (auto *fallback = impl_->fallback) {
        for (auto block = fallback->document()->begin(); block.isValid(); block = block.next()) {
            if (block.blockFormat().headingLevel() > 0 && !block.text().trimmed().isEmpty() && index-- == 0) {
                fallback->setTextCursor(QTextCursor(block));
                fallback->ensureCursorVisible();
                return;
            }
        }
        return;
    }
#ifdef VSR_WEBVIEW2
    impl_->execute(QStringLiteral("window.vsrScrollToHeading(%1);").arg(index));
#else
    Q_UNUSED(index)
#endif
}

void CommunityWebView::openEditor(std::function<void(bool)> ready, std::function<void(const QJsonObject &)> message)
{
    impl_->editorReady = std::move(ready);
    impl_->editorMessage = std::move(message);
#ifdef VSR_WEBVIEW2
    impl_->target = editorUrl;
    impl_->create();
#else
    impl_->unavailable();
#endif
}

void CommunityWebView::editorCommand(const QJsonObject &command)
{
#ifdef VSR_WEBVIEW2
    impl_->execute(QStringLiteral("window.vsrEditorCommand(%1);")
        .arg(QString::fromUtf8(QJsonDocument(command).toJson(QJsonDocument::Compact))));
#else
    Q_UNUSED(command)
#endif
}

void CommunityWebView::documentTheme(bool dark)
{
    if (!impl_->localDocument) return;
    impl_->document.insert("dark", dark);
#ifdef VSR_WEBVIEW2
    impl_->execute(dark ? QStringLiteral("window.vsrDocumentTheme(true);") : QStringLiteral("window.vsrDocumentTheme(false);"));
#endif
}

void CommunityWebView::exportDocumentPdf(const QString &path, std::function<void(bool)> finished)
{
#ifdef VSR_WEBVIEW2
    if (impl_->localDocument && impl_->rendered) {
        auto pdf = query<ICoreWebView2_7>(impl_->view, IID_ICoreWebView2_7);
        if (pdf) {
            const QPointer<CommunityWebView> guard(this);
            const std::function<HRESULT(HRESULT, BOOL)> done = [guard, finished](HRESULT status, BOOL success) {
                if (guard) finished(SUCCEEDED(status) && success);
                return S_OK;
            };
            if (SUCCEEDED(pdf->PrintToPdf(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()), nullptr,
                callback<ICoreWebView2PrintToPdfCompletedHandler>(IID_ICoreWebView2PrintToPdfCompletedHandler, done).get()))) return;
        }
    }
#else
    Q_UNUSED(path)
#endif
    finished(false);
}

bool CommunityWebView::event(QEvent *event)
{
    const bool handled = QWidget::event(event);
#ifdef VSR_WEBVIEW2
    if (impl_ && (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::Hide))
        impl_->size();
#endif
    return handled;
}

}
