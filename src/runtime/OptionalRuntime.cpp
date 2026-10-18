#include "runtime/OptionalRuntime.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace vsr {
QString optionalRuntimeExecutable(const QString &component, const QString &program, bool extract, QString *error) {
    const QDir app(QCoreApplication::applicationDirPath());
    const auto loose=app.filePath("runtime/"+component+"/"+program);
    if(QFileInfo::exists(loose))return loose;
    const auto archive=app.filePath("runtime/"+component+".zip");
    QFile input(archive);if(!input.open(QIODevice::ReadOnly))return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);if(!hash.addData(&input))return {};
    const auto parent=QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)+"/VSRenderer/runtime/"+component;
    const auto cache=parent+"/"+QString::fromLatin1(hash.result().toHex());
    const auto executable=cache+"/"+program;
    if(!extract || (QFileInfo::exists(cache+"/.ready") && QFileInfo::exists(executable)))return executable;
    const auto fail=[&](const QString &message){if(error)*error=message;return QString();};
    if(!QDir().mkpath(parent))return fail("Cannot create optional runtime cache");
    QLockFile lock(cache+".lock");if(!lock.tryLock(30000))return fail("Optional runtime extraction is busy");
    if(QFileInfo::exists(cache+"/.ready") && QFileInfo::exists(executable))return executable;
    QTemporaryDir staging(parent+"/extract-XXXXXX");if(!staging.isValid())return fail("Cannot stage optional runtime");
    QProcess process;
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    process.start(app.filePath("runtime/tools/7z.exe"),{"x",archive,"-o"+staging.path(),"-y","-bb0"});
    if(!process.waitForStarted() || !process.waitForFinished(120000)){
        process.kill();process.waitForFinished();return fail("Cannot unpack optional runtime: "+process.errorString());
    }
    if(process.exitCode()!=0 || !QFileInfo::exists(staging.filePath(program)))return fail("Incomplete optional runtime: "+QString::fromUtf8(process.readAllStandardError()));
    QFile ready(staging.filePath(".ready"));if(!ready.open(QIODevice::WriteOnly))return fail(ready.errorString());ready.close();
    if(QFileInfo::exists(cache) && !QDir(cache).removeRecursively())return fail("Cannot replace incomplete optional runtime cache");
    if(!QDir().rename(staging.path(),cache))return fail("Cannot commit optional runtime cache");
    staging.setAutoRemove(false);return executable;
}
}
