#include "update/PortableUpdater.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
using namespace vsr;
namespace {
void write(const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(bytes);}
QString tool(const QString &name){return QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/"+name);}
void payload(const QString &root){for(const auto &name:QStringList{"VSRenderer.exe","vs-player.exe","FFF.Native.dll","Qt6Core.dll","runtime/python/python.exe","runtime/tools/aria2-next.exe","runtime/tools/7z.exe","runtime/tools/7z.dll","runtime/tools/apply-update.ps1","languages/en_US.json","languages/zh_CN.json"})write(QDir(root).filePath(name),"new-binary");write(QDir(root).filePath("release.json"),"{\"version\":\"1.0.3\"}");}
}
class TestUpdates : public QObject {
    Q_OBJECT
private slots:
    void versionsAndDiscovery(){QVERIFY(PortableUpdater::newer("1.0.10","1.0.2"));QVERIFY(!PortableUpdater::newer("1.0.1","1.0.2"));QVERIFY(!PortableUpdater::newer("1.0.2.0","1.0.2"));QVERIFY(!PortableUpdater::newer("bad","1.0.2"));
        QJsonArray entries{QJsonObject{{"Path","VS-GUI/1.0.10.7z"},{"Type","blob"},{"Size",100},{"Revision","abc"},{"Sha256",QString(64,'a')}},QJsonObject{{"Path","else/9.9.9.7z"},{"Type","blob"}},QJsonObject{{"Path","VS-GUI/9.9.9.7z"},{"Type","tree"}},QJsonObject{{"Path","VS-GUI/a/9.9.9.7z"},{"Type","blob"}}};
        const auto found=PortableUpdater::releases(entries);QCOMPARE(found.size(),1);QCOMPARE(found.first().version,QString("1.0.10"));QVERIFY(found.first().url.path().contains("/resolve/abc/VS-GUI/"));}
    void archivePaths(){const QString root="VS-Renderer-GUI-1.0.3-windows-x64";QVERIFY(PortableUpdater::safeArchiveListing("header\n----------\nPath = "+root+"\nPath = "+root+"/vs-player.exe\n"));
        QVERIFY(PortableUpdater::safeArchiveListing("----------\nPath = VS-Renderer-GUI-windows-x64\nPath = VS-Renderer-GUI-windows-x64/vs-player.exe\n"));
        QVERIFY(!PortableUpdater::safeArchiveListing("----------\nPath = VS-Renderer-GUI-anything-windows-x64/vs-player.exe\n"));
        for(const auto &path:QStringList{"/abs","C:/abs",root+"/../evil",root+"/x:stream",root+"/CON.txt",root+"/bad.",root+"/bad ",root+"//x"})QVERIFY2(!PortableUpdater::safeArchiveListing("----------\nPath = "+path+"\n"),qPrintable(path));
        QVERIFY(!PortableUpdater::safeArchiveListing("----------\nPath = "+root+"/x\nSymbolic Link = ../outside\n"));QVERIFY(!PortableUpdater::safeArchiveListing("----------\nPath = "+root+"/X\nPath = "+root+"/x\n"));}
    void downloadVerifyAndCancel(){QTemporaryDir dir;const auto root=dir.filePath("VS-Renderer-GUI-windows-x64");payload(root);
        QProcess zip;zip.setWorkingDirectory(dir.path());zip.start(tool("7z.exe"),{"a","-t7z",dir.filePath("test.7z"),QFileInfo(root).fileName(),"-mx=1"});QVERIFY(zip.waitForFinished(15000));QCOMPARE(zip.exitCode(),0);QFile file(dir.filePath("test.7z"));QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();
        QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost));connect(&server,&QTcpServer::newConnection,this,[&]{while(auto *socket=server.nextPendingConnection()){connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);connect(socket,&QTcpSocket::readyRead,socket,[socket,&bytes]{const auto request=socket->readAll();QByteArray response="HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(bytes.size())+"\r\nConnection: close\r\n\r\n";if(!request.startsWith("HEAD "))response+=bytes;socket->write(response);socket->disconnectFromHost();});}});
        PortableRelease release{"1.0.3",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()),QUrl(QString("http://127.0.0.1:%1/test.7z").arg(server.serverPort())),bytes.size()};
        PortableUpdater updater;QSignalSpy ready(&updater,&PortableUpdater::prepared),fail(&updater,&PortableUpdater::failed);updater.prepare(release);QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty() || !fail.isEmpty(),30000);QVERIFY2(fail.isEmpty(),fail.isEmpty()?"":qPrintable(fail.first().first().toString()));QCOMPARE(ready.size(),1);QVERIFY(QFileInfo(ready.first().first().toString()+"/release.json").exists());
        release.sha256=QString(64,'0');PortableUpdater invalid;QSignalSpy bad(&invalid,&PortableUpdater::failed),badReady(&invalid,&PortableUpdater::prepared);invalid.prepare(release);QTRY_COMPARE_WITH_TIMEOUT(bad.size(),1,15000);QCOMPARE(badReady.size(),0);QVERIFY(bad.first().first().toString().contains("SHA-256"));
        PortableUpdater cancelled;QSignalSpy stopped(&cancelled,&PortableUpdater::prepared);cancelled.prepare(release);cancelled.close();QTest::qWait(100);QCOMPARE(stopped.size(),0);
    }
    void fullDirectoryReplacement(){QTemporaryDir dir;const auto target=dir.filePath("portable with space"),source=dir.filePath("payload");payload(source);write(target+"/vs-player.exe","old");write(target+"/obsolete.dll","old");write(target+"/player.ini","user settings");write(target+"/vpy/custom.vpy","user vpy");write(target+"/vpy/builtin/Anime.vpy","old builtin");write(target+"/cache/indexes/x.lwi","old cache");write(source+"/vpy/builtin/Anime.vpy","new builtin");
        QProcess apply;apply.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",tool("apply-update.ps1"),"-ParentPid","0","-Source",source,"-Target",target,"-NoLaunch"});QVERIFY(apply.waitForFinished(20000));QVERIFY2(apply.exitCode()==0,apply.readAllStandardError().constData());
        QVERIFY(!QFile::exists(target+"/obsolete.dll"));QVERIFY(!QFile::exists(target+"/cache/indexes/x.lwi"));QFile ini(target+"/player.ini");QVERIFY(ini.open(QIODevice::ReadOnly));QCOMPARE(ini.readAll(),QByteArray("user settings"));QFile vpy(target+"/vpy/custom.vpy");QVERIFY(vpy.open(QIODevice::ReadOnly));QCOMPARE(vpy.readAll(),QByteArray("user vpy"));QFile builtin(target+"/vpy/builtin/Anime.vpy");QVERIFY(builtin.open(QIODevice::ReadOnly));QCOMPARE(builtin.readAll(),QByteArray("new builtin"));
        const auto backups=QDir(dir.path()).entryList({"portable with space.previous-*"},QDir::Dirs);QCOMPARE(backups.size(),1);QVERIFY(QFile::exists(dir.filePath(backups.first()+"/obsolete.dll")));
    }
    void blockedInstallKeepsOldDirectory(){QTemporaryDir dir;const auto target=dir.filePath("portable"),source=dir.filePath("payload");payload(source);write(target+"/vs-player.exe","old");QProcess apply;apply.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",tool("apply-update.ps1"),"-ParentPid",QString::number(QCoreApplication::applicationPid()),"-Source",source,"-Target",target,"-WaitSeconds","0","-NoLaunch"});QVERIFY(apply.waitForFinished(10000));QCOMPARE(apply.exitCode(),1);QFile old(target+"/vs-player.exe");QVERIFY(old.open(QIODevice::ReadOnly));QCOMPARE(old.readAll(),QByteArray("old"));QCOMPARE(QDir(dir.path()).entryList({"*.previous-*"},QDir::Dirs).size(),0);}
};
QTEST_MAIN(TestUpdates)
#include "TestUpdates.moc"
