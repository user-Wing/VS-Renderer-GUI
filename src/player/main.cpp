#include "runtime/PortableQtPlugins.h"
#include "runtime/PortableLayout.h"
#include "player/PlayerWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include "player/PlayerAssociations.h"
#include "player/PlayerLoginStartup.h"
#include "player/PlayerThumbnails.h"
#include "player/PlayerImage.h"
#include "player/PlayerImageTools.h"
#include <QTimer>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDataStream>
#include <QWinEventNotifier>
#include "update/SinglePlayerUpdater.h"
#include <qt_windows.h>
#include "player/PlayerStandbyWin.h"
#include <psapi.h>
#include <array>
#include <cstring>
#include <string>
#include <thread>

namespace {
#ifndef VSR_LITE_PLAYER
void refreshAssociations() {
    QSettings associations("HKEY_CURRENT_USER\\Software\\VSPlayer\\Capabilities", QSettings::NativeFormat);
    associations.beginGroup("FileAssociations");
    QStringList extensions;
    for (const auto &key : associations.childKeys())
        if (key.startsWith('.') && associations.value(key).toString().startsWith("VSPlayer."))
            extensions << key.mid(1);
    associations.endGroup();
    const auto path = QCoreApplication::applicationFilePath();
    if (!extensions.isEmpty() && associations.value("ApplicationIcon").toString() != QDir::toNativeSeparators(path) + ",-101")
        vsr::registerPlayerAssociations(extensions, path);
}
#endif

void prefetchPlaybackLibraries(std::stop_token stop, bool standby = false) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    std::array<unsigned char,128*1024> buffer;
    const auto readAhead = [&](const wchar_t *name) {
        if (stop.stop_requested()) return;
        wchar_t path[32768];
        const auto length=SearchPathW(nullptr,name,nullptr,32768,path,nullptr);
        if (!length || length>=32768) return;
        const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
            standby ? FILE_ATTRIBUTE_NORMAL : FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
        if (file==INVALID_HANDLE_VALUE) return;
        DWORD bytes=0;
        while (!stop.stop_requested() && ReadFile(file,buffer.data(),DWORD(buffer.size()),&bytes,nullptr) && bytes) {}
        CloseHandle(file);
    };
    if (standby) {
        for (auto *name : {L"vs-player.exe", L"Qt6Core.dll", L"Qt6Gui.dll", L"Qt6Widgets.dll", L"qwindows.dll", L"FFF.Native.dll"})
            readAhead(name);
    }
    for (auto *name : {L"avcodec-63.dll", L"avformat-63.dll", L"avutil-61.dll", L"swresample-7.dll", L"swscale-10.dll", L"avfilter-12.dll"})
        readAhead(name);
}

void showPlayer(vsr::PlayerWindow &window, const QStringList &arguments, bool warmed = false) {
    if (warmed) window.resumeStandby();
    window.show();
    if (warmed) window.activateWindow();
#ifdef VSR_LITE_PLAYER
    auto *updater = new vsr::SinglePlayerUpdater(&window);
    QTimer::singleShot(5000, &window, [updater]{ updater->check(); });
#endif
    QTimer::singleShot(0, &window, [&window, arguments, warmed] {
        if (!arguments.value(1).startsWith("--")
#ifndef VSR_LITE_PLAYER
                && !qEnvironmentVariableIsSet("VSR_PLAYER_CONFIG")
#endif
        ) vsr::initializePlayerLoginStartup();
#ifndef VSR_LITE_PLAYER
        if (warmed) refreshAssociations();
        if (arguments.value(1) == "--renderer-preview") {
            if (arguments.size() > 3) window.loadPreset(arguments[3]);
            window.openFile(arguments[2]);
            return;
        }
#endif
        for (int i = 1; i < arguments.size(); ++i) {
            if (!arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.openFile(arguments[i]);
#ifndef VSR_LITE_PLAYER
            else window.loadPreset(arguments[i]);
#endif
        }
    });
}

int runWarmWindow(QApplication &app, const wchar_t *exe) {
    const auto closeHandle = [](void *handle) { CloseHandle(handle); };
    const DWORD owner = qEnvironmentVariableIntValue("VSR_PLAYER_WARM_PARENT");
    qunsetenv("VSR_PLAYER_WARM_PARENT");
    std::unique_ptr<void, decltype(closeHandle)> parent(
        OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, owner), closeHandle);
    wchar_t stopName[64];
#ifdef VSR_LITE_PLAYER
    const auto launcher = qEnvironmentVariable("VSR_SINGLE_PLAYER_PATH").toStdWString();
    if (launcher.empty()) return 1;
    vspStandbyEventName(stopName, launcher.c_str());
#else
    vspStandbyEventName(stopName, exe);
#endif
    std::unique_ptr<void, decltype(closeHandle)> stop(OpenEventW(SYNCHRONIZE, FALSE, stopName), closeHandle);
    const auto standbyRunning = [&] {
        return parent && stop && WaitForSingleObject(parent.get(), 0) == WAIT_TIMEOUT &&
            WaitForSingleObject(stop.get(), 0) == WAIT_TIMEOUT;
    };
    constexpr SIZE_T budget = 39500000;
    if (!standbyRunning() || !SetProcessWorkingSetSizeEx(GetCurrentProcess(), 128 * 1024, budget,
            QUOTA_LIMITS_HARDWS_MAX_ENABLE)) return 1;

    vsr::PlayerWindow window(true);
    window.ensurePolished();
    window.winId();
    {
        QImage canvas(window.size(), QImage::Format_ARGB32_Premultiplied);
        window.render(&canvas);
    }
    PROCESS_MEMORY_COUNTERS_EX memory{}, brokerMemory{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory), sizeof(memory)) ||
        !GetProcessMemoryInfo(parent.get(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&brokerMemory), sizeof(brokerMemory)) ||
        memory.PrivateUsage + brokerMemory.PrivateUsage >= 50000000 ||
        memory.WorkingSetSize > budget || !standbyRunning()) return 1;

    const auto config = vsr::playerConfigFile();
    const auto theme = QDir(QFileInfo(config).absolutePath()).filePath("themes.ini");
    const auto revisions = [&] { return std::array{QFileInfo(config).lastModified(), QFileInfo(theme).lastModified()}; };
    const auto prepared = revisions();
    const auto windowPreferences = [&] {
        QSettings settings(config, QSettings::IniFormat);
        settings.sync();
        QByteArray preferences;
        QDataStream stream(&preferences, QIODevice::WriteOnly);
        for (const auto &key : settings.allKeys()) {
            if (key.startsWith("positions/") || key.startsWith("tracks/") ||
                key == "subtitle/primary" || key == "subtitle/secondary") continue;
            // QSettings may cache typed defaults in this process, then read
            // the same INI scalars back as strings after another window saves.
            const auto value = settings.value(key);
            stream << key << (value.metaType().id() == QMetaType::QStringList
                ? value.toStringList() : QStringList{value.toString()});
        }
        return QCryptographicHash::hash(preferences, QCryptographicHash::Sha256);
    };
    const auto preparedPreferences = windowPreferences();
    bool claimed = false;
    QWinEventNotifier stopNotice(stop.get());
    QWinEventNotifier parentNotice(parent.get());
    const auto shutDown = [&] {
        parentNotice.setEnabled(false);
        stopNotice.setEnabled(false);
        parent.reset();
        stop.reset();
        if (!claimed) app.quit();
    };
    QObject::connect(&stopNotice, &QWinEventNotifier::activated, &app, shutDown);
    QObject::connect(&parentNotice, &QWinEventNotifier::activated, &app, shutDown);

    HWND listener = nullptr;
    auto open = [&](const wchar_t *arguments) -> BOOL {
        if (claimed || !standbyRunning()) return FALSE;
        const auto current = revisions();
        if (current[1] != prepared[1] || (current[0] != prepared[0] && windowPreferences() != preparedPreferences)) {
            QTimer::singleShot(0, &app, [&app] { app.exit(VSP_WARM_WINDOW_USED); });
            return FALSE;
        }
        const auto command = std::wstring(L"vsp ") + arguments;
        int count = 0;
        wchar_t **values = CommandLineToArgvW(command.c_str(), &count);
        if (!values) return FALSE;
        QStringList files;
        for (int i = 0; i < count; ++i) files << QString::fromWCharArray(values[i]);
        LocalFree(values);
        if (!SetProcessWorkingSetSizeEx(GetCurrentProcess(), 128 * 1024, budget,
                QUOTA_LIMITS_HARDWS_MAX_DISABLE)) return FALSE;
        claimed = true;
        QTimer::singleShot(0, &window, [&window, &listener, files] {
            DestroyWindow(listener);
            listener = nullptr;
            showPlayer(window, files, true);
        });
        return TRUE;
    };
    VspStandbyReceiver receiver{[](void *context, const wchar_t *arguments) {
        return (*static_cast<decltype(open) *>(context))(arguments);
    }, &open};
    listener = vspCreateStandbyReceiver(exe, VSP_WARM_WINDOW_CLASS, &receiver);
    if (!listener) return 1;
    const int result = app.exec();
    if (listener) DestroyWindow(listener);
    return claimed && standbyRunning() ? VSP_WARM_WINDOW_USED : result;
}
}
int main(int argc, char **argv) {
    const auto *nativeArguments = vspStartupArguments();
    wchar_t exe[4096];
    const auto length = GetModuleFileNameW(nullptr, exe, 4096);
    const bool warm = length && length < 4096 && !wcscmp(nativeArguments, L"--warm-window");
#ifndef VSR_LITE_PLAYER
    if (length && length < 4096) {
        if (!wcscmp(nativeArguments, L"--stop-standby")) {
            vspStopStandby(exe);
            return 0;
        }
        if (!wcscmp(nativeArguments, L"--standby")) {
            const HANDLE stop = vspBeginStandby(exe);
            if (!stop) return 0;
            const auto directory = std::wstring(exe, wcsrchr(exe, L'\\'));
            SetCurrentDirectoryW(directory.c_str());
            VspStandbyLaunch launch{exe, directory.c_str(), FALSE, FALSE};
            bool ready;
            {
                std::jthread warmup(prefetchPlaybackLibraries, true);
                ready = vspRunStandby(exe, stop, &launch, nullptr);
            }
            CloseHandle(stop);
            return ready ? 0 : 1;
        }
        if (!warm && !qEnvironmentVariableIsSet("VSR_PLAYER_CONFIG") && !qEnvironmentVariableIsSet("VSR_3FP_DLL")) {
            if (vspForwardToStandby(exe, nativeArguments, VSP_STANDBY_CLASS)) return 0;
            if ((argc < 2 || strncmp(argv[1], "--", 2) != 0) && !vspStandbyExists(exe)) {
                const auto directory = std::wstring(exe, wcsrchr(exe, L'\\'));
                if (vspKeepInSession(directory.c_str())) {
                    const VspStandbyLaunch launch{exe, directory.c_str(), FALSE, FALSE};
                    if (const auto broker = vspStartPlayer(&launch, L"--standby")) CloseHandle(broker);
                }
            }
        }
    }
#endif
    std::jthread prefetch;
    wchar_t disabled[2]{};
    if (!warm && (GetEnvironmentVariableW(L"FFF_NO_PREFETCH",disabled,2)!=1 || disabled[0]!=L'1'))
        if (argc<2 || QByteArray(argv[1])!="--photocraft-import") prefetch=std::jthread(prefetchPlaybackLibraries, false);
    vsr::registerPortableQtPlugins();
    QApplication app(argc, argv);
    vsr::migratePortableLayout();
    if(app.arguments().value(1)=="--install-thumbnails"){
        auto extensions=app.arguments().mid(2);if(extensions.isEmpty())extensions=vsr::thumbnailExtensions();
        QString error;const bool ok=vsr::installPlayerThumbnails(extensions,&error);if(!ok)qWarning().noquote()<<error;return ok?0:1;
    }
    if(app.arguments().value(1)=="--uninstall-thumbnails")return vsr::unregisterThumbnailProvider()?0:1;
    if(app.arguments().value(1)=="--photocraft-import" && app.arguments().size()==4){
        vsr::PlayerImage decoder;int result=1;
        QObject::connect(&decoder,&vsr::PlayerImage::loaded,&app,[&](const QImage &image){
            const auto error=vsr::PlayerImageTools::writeEditorImport(image,app.arguments()[3],QFileInfo(app.arguments()[2]).fileName());
            if(!error.isEmpty())qWarning().noquote()<<error;result=error.isEmpty()?0:1;app.quit();
        });
        QObject::connect(&decoder,&vsr::PlayerImage::failed,&app,[&](const QString &error){qWarning().noquote()<<error;app.quit();});
        QTimer::singleShot(0,&app,[&]{decoder.open(app.arguments()[2]);});app.exec();return result;
    }
    app.setApplicationName("VS Player"); app.setOrganizationName("VSRenderer"); app.setApplicationVersion(VSR_VERSION);
    app.setWindowIcon(QIcon(":/icons/player.ico"));
#ifndef VSR_LITE_PLAYER
    if (!warm) refreshAssociations();
#endif
    QFont font("Comic Sans MS"); font.setPixelSize(13); app.setFont(font);
    if (warm) return runWarmWindow(app, exe);
    const auto arguments = app.arguments();
#ifndef VSR_LITE_PLAYER
    std::unique_ptr<QTemporaryDir> previewSettings;
    if (arguments.value(1) == "--renderer-preview") {
        previewSettings = std::make_unique<QTemporaryDir>();
        if (!previewSettings->isValid() || arguments.size() < 3) {
            QMessageBox::warning(nullptr, QStringLiteral("VS Player"), QStringLiteral("无法准备 Renderer 预览会话。"));
            return 1;
        }
        const auto current = vsr::playerConfigFile();
        const auto preview = previewSettings->filePath("player.ini");
        if (QFileInfo::exists(current) && !QFile::copy(current, preview)) {
            QMessageBox::warning(nullptr, QStringLiteral("VS Player"), QStringLiteral("无法读取播放器预览配置。"));
            return 1;
        }
        QSettings settings(preview, QSettings::IniFormat);
        settings.setValue("player/preset", QString());
        settings.setValue("player/renderer", "VS");
        settings.setValue("player/interpolationAuto", false);
        settings.setValue("playback/remember", false);
        settings.remove("render/customShader");
        settings.sync();
        qputenv("VSR_PLAYER_CONFIG", preview.toUtf8());
    }
#endif
    vsr::PlayerWindow window;
    showPlayer(window, arguments);
    return app.exec();
}
