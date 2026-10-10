#include "player/PlayerWindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include "player/PlayerAssociations.h"
#include "player/PlayerImage.h"
#include "player/PlayerImageTools.h"
#include <QTimer>
#include <QFileInfo>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
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
    vsr::PlayerWindow window; window.show();
    const auto arguments = app.arguments();
    QTimer::singleShot(0,&window,[&window,arguments]{
#ifdef VSR_LITE_PLAYER
    for (int i = 1; i < arguments.size(); ++i)
        if (!arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.openFile(arguments[i]);
#else
    for (int i = 1; i < arguments.size(); ++i)
        if (arguments[i].endsWith(".vpy", Qt::CaseInsensitive)) window.loadPreset(arguments[i]);
        else window.openFile(arguments[i]);
#endif
    });
    return app.exec();
}
