#include "player/PlayerWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include "player/PlayerAssociations.h"
#include "player/PlayerThumbnails.h"
#include "player/PlayerImage.h"
#include "player/PlayerImageTools.h"
#include <QTimer>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QTemporaryDir>
#include "update/SinglePlayerUpdater.h"
#include <qt_windows.h>
#include <array>
#include <thread>

namespace {
void prefetchPlaybackLibraries(std::stop_token stop) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    std::array<unsigned char,128*1024> buffer;
    for (const auto *name : {L"avcodec-63.dll",L"avformat-63.dll",L"avutil-61.dll",L"swresample-7.dll",L"swscale-10.dll",L"avfilter-12.dll"}) {
        if (stop.stop_requested()) return;
        wchar_t path[32768];
        const auto length=SearchPathW(nullptr,name,nullptr,32768,path,nullptr);
        if (!length || length>=32768) continue;
        const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
        if (file==INVALID_HANDLE_VALUE) continue;
        DWORD bytes=0;
        while (!stop.stop_requested() && ReadFile(file,buffer.data(),DWORD(buffer.size()),&bytes,nullptr) && bytes) {}
        CloseHandle(file);
    }
}
}
int main(int argc, char **argv) {
    std::jthread prefetch;
    wchar_t disabled[2]{};
    if (GetEnvironmentVariableW(L"FFF_NO_PREFETCH",disabled,2)!=1 || disabled[0]!=L'1')
        if (argc<2 || QByteArray(argv[1])!="--photocraft-import") prefetch=std::jthread(prefetchPlaybackLibraries);
    QApplication app(argc, argv);
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
    QSettings associations("HKEY_CURRENT_USER\\Software\\VSPlayer\\Capabilities", QSettings::NativeFormat);
    associations.beginGroup("FileAssociations");
    QStringList extensions;
    for (const auto &key : associations.childKeys())
        if (key.startsWith('.') && associations.value(key).toString().startsWith("VSPlayer."))
            extensions << key.mid(1);
    associations.endGroup();
    const auto iconPath=QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+",-101";
    if (!extensions.isEmpty() && associations.value("ApplicationIcon").toString()!=iconPath)
        vsr::registerPlayerAssociations(extensions, QCoreApplication::applicationFilePath());
#endif
    QFont font("Comic Sans MS"); font.setPixelSize(13); app.setFont(font);
    const auto arguments = app.arguments();
#ifndef VSR_LITE_PLAYER
    std::unique_ptr<QTemporaryDir> previewSettings;
    if (arguments.value(1) == "--renderer-preview") {
        previewSettings = std::make_unique<QTemporaryDir>();
        if (!previewSettings->isValid() || arguments.size() < 3) {
            QMessageBox::warning(nullptr, QStringLiteral("VS Player"), QStringLiteral("无法准备 Renderer 预览会话。"));
            return 1;
        }
        const auto current = qEnvironmentVariable("VSR_PLAYER_CONFIG",
            QDir(app.applicationDirPath()).filePath("player.ini"));
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
    vsr::PlayerWindow window; window.show();
#ifdef VSR_LITE_PLAYER
    auto *updater=new vsr::SinglePlayerUpdater(&window);QTimer::singleShot(5000,&window,[updater]{updater->check();});
#endif
    QTimer::singleShot(0,&window,[&window,arguments]{
#ifdef VSR_LITE_PLAYER
    for (int i = 1; i < arguments.size(); ++i)
        if (!arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.openFile(arguments[i]);
#else
    if (arguments.value(1) == "--renderer-preview") {
        if (arguments.size() > 3) window.loadPreset(arguments[3]);
        window.openFile(arguments[2]);
        return;
    }
    for (int i = 1; i < arguments.size(); ++i)
        if (arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.loadPreset(arguments[i]);
        else window.openFile(arguments[i]);
#endif
    });
    return app.exec();
}
