#include "update/PortableUpdater.h"
#include "update/ComponentDownloads.h"
#include <QTreeWidget>
#include <QLabel>
#include <QtEndian>
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
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
    void componentVersionsAndLayouts(){QVERIFY(ComponentDownloads::newer("102.0","99.0"));QVERIFY(!ComponentDownloads::newer("0.83","0.83.0"));QVERIFY(ComponentDownloads::newer("20261003","20260929"));QJsonArray files;for(const auto &path:QStringList{"aria2-next/v2.8.3/aria2-next-2.8.3-windows-x86_64.exe","aria2-next/v2.8.2/aria2-next-2.8.2-source.zip","aria2-next/v2.8.2/aria2-next-2.8.2-windows-x86_64.exe"})files<<QJsonObject{{"Path",path},{"Type","blob"},{"Size",100},{"Sha256",QString(64,'a')}};const auto versions=ComponentDownloads::releases("aria2-next",files);QCOMPARE(versions.size(),2);QCOMPARE(versions.first().version,QString("2.8.3"));QCOMPARE(ComponentDownloads::destination("mkvtoolnix"),QString("runtime/mkvtoolnix"));QVERIFY(ComponentDownloads::destination("bad").isEmpty());
        files={QJsonObject{{"Path","MadVR/madVR09217.7z"},{"Type","blob"},{"Size",100},{"Sha256",QString(64,'a')}}};QCOMPARE(ComponentDownloads::releases("MadVR",files).first().version,QString("0.92.17"));
        QTemporaryDir tmp;QByteArray pe(128,'\0');pe.replace(0,2,"MZ");qToLittleEndian<quint32>(64,pe.data()+60);pe.replace(64,4,QByteArray("PE\0\0",4));qToLittleEndian<quint16>(0x8664,pe.data()+68);write(tmp.filePath("wrapper/deep/mkvmerge.exe"),pe);QCOMPARE(ComponentDownloads::payloadDirectory("mkvtoolnix",tmp.path()),tmp.filePath("wrapper/deep"));write(tmp.filePath("duplicate/mkvmerge.exe"),pe);QVERIFY(ComponentDownloads::payloadDirectory("mkvtoolnix",tmp.path()).isEmpty());
    }
    void componentInstallPreservesPaths(){QTemporaryDir temp;const auto target=temp.filePath("portable with space"),cache=temp.filePath("cache"),source=cache+"/staging/wrapper/actual";write(target+"/vs-player.exe","player");write(target+"/runtime/tools/7z.exe","keep-7z");write(target+"/runtime/mkvtoolnix/mkvtoolnix-gui.ini","user");write(source+"/mkvmerge.exe","new-tool");write(source+"/mkvtoolnix-gui.ini","default");const auto plan=cache+"/plan.json";write(plan,QJsonDocument(QJsonArray{QJsonObject{{"id","mkvtoolnix"},{"version","102.0"},{"source",source}}}).toJson());QProcess helper;helper.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",tool("apply-components.ps1"),"-ParentPid","0","-Plan",plan,"-Target",target,"-NoLaunch"});QVERIFY(helper.waitForFinished(20000));QVERIFY2(helper.exitCode()==0,helper.readAllStandardError().constData());QFile installed(target+"/runtime/mkvtoolnix/mkvmerge.exe");QVERIFY(installed.open(QIODevice::ReadOnly));QCOMPARE(installed.readAll(),QByteArray("new-tool"));QFile config(target+"/runtime/mkvtoolnix/mkvtoolnix-gui.ini");QVERIFY(config.open(QIODevice::ReadOnly));QCOMPARE(config.readAll(),QByteArray("user"));QVERIFY(QFileInfo(target+"/runtime/tools/7z.exe").exists());
        write(plan,QJsonDocument(QJsonArray{QJsonObject{{"id","VS-GUI"},{"version","0.1"},{"source",source}}}).toJson());helper.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",tool("apply-components.ps1"),"-ParentPid","0","-Plan",plan,"-Target",target,"-NoLaunch"});QVERIFY(helper.waitForFinished(20000));QCOMPARE(helper.exitCode(),1);
    }
    void liveComponentDownload(){if(qEnvironmentVariable("VSR_COMPONENT_LIVE")!="1")QSKIP("Set VSR_COMPONENT_LIVE for real ModelScope GUI download");ComponentDownloads widget;widget.resize(980,650);widget.show();auto *tree=widget.findChild<QTreeWidget *>("componentVersions");QVERIFY(tree);QTreeWidgetItem *group=nullptr;for(int i=0;i<tree->topLevelItemCount();++i)if(tree->topLevelItem(i)->data(0,Qt::UserRole).toString()=="mkvtoolnix")group=tree->topLevelItem(i);QVERIFY(group);QTRY_VERIFY_WITH_TIMEOUT(group->childCount()>0,30000);group->setExpanded(true);tree->setCurrentItem(group->child(0));QSignalSpy ready(&widget,&ComponentDownloads::componentPrepared);widget.findChild<QPushButton *>("componentDownload")->click();QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty() || widget.findChild<QLabel *>("componentStatus")->text().startsWith(QStringLiteral("组件下载失败")),180000);QVERIFY2(!ready.isEmpty(),qPrintable(widget.findChild<QLabel *>("componentStatus")->text()));const auto payload=ready.first().at(2).toString();QVERIFY(QFileInfo(payload+"/mkvmerge.exe").isFile());QProcess info;info.start(payload+"/mkvmerge.exe",{"--version"});QVERIFY(info.waitForFinished(10000));QCOMPARE(info.exitCode(),0);qInfo()<<info.readAllStandardOutput();const auto shot=qEnvironmentVariable("VSR_COMPONENT_SCREENSHOT");if(!shot.isEmpty())QVERIFY(widget.grab().save(shot));}
    void versionsAndDiscovery(){QVERIFY(PortableUpdater::newer("1.0.10","1.0.2"));QVERIFY(!PortableUpdater::newer("1.0.1","1.0.2"));QVERIFY(!PortableUpdater::newer("1.0.2.0","1.0.2"));QVERIFY(!PortableUpdater::newer("bad","1.0.2"));
        QJsonArray entries{QJsonObject{{"Path","VS-GUI/1.0.10.7z"},{"Type","blob"},{"Size",100},{"Revision","abc"},{"Sha256",QString(64,'a')}},QJsonObject{{"Path","else/9.9.9.7z"},{"Type","blob"}},QJsonObject{{"Path","VS-GUI/9.9.9.7z"},{"Type","tree"}},QJsonObject{{"Path","VS-GUI/a/9.9.9.7z"},{"Type","blob"}}};
        const auto found=PortableUpdater::releases(entries);QCOMPARE(found.size(),1);QCOMPARE(found.first().version,QString("1.0.10"));QVERIFY(found.first().url.path().contains("/resolve/abc/VS-GUI/"));}
    void archivePaths(){const QString root="VS-Renderer-GUI-1.0.3-windows-x64";QVERIFY(PortableUpdater::safeArchiveListing("header\n----------\nPath = "+root+"\nPath = "+root+"/vs-player.exe\n"));
        QVERIFY(PortableUpdater::safeArchiveListing("----------\nPath = VS-Renderer-GUI-windows-x64\nPath = VS-Renderer-GUI-windows-x64/vs-player.exe\n"));
        QVERIFY(PortableUpdater::safeArchiveListing("----------\nPath = Arbitrary folder/vs-player.exe\n"));
        QVERIFY(PortableUpdater::safeArchiveListing("----------\nPath = vs-player.exe\nPath = runtime/python/python.exe\n"));
        for(const auto &path:QStringList{"/abs","C:/abs",root+"/../evil",root+"/x:stream",root+"/CON.txt",root+"/bad.",root+"/bad ",root+"//x"})QVERIFY2(!PortableUpdater::safeArchiveListing("----------\nPath = "+path+"\n"),qPrintable(path));
        QVERIFY(!PortableUpdater::safeArchiveListing("----------\nPath = "+root+"/x\nSymbolic Link = ../outside\n"));QVERIFY(!PortableUpdater::safeArchiveListing("----------\nPath = "+root+"/X\nPath = "+root+"/x\n"));}
    void releasedPackageCompatibility(){
        const auto archive=qEnvironmentVariable("VSR_TEST_UPDATE_ARCHIVE");if(archive.isEmpty())QSKIP("Set VSR_TEST_UPDATE_ARCHIVE to audit a release package");
        QProcess listing;listing.start(tool("7z.exe"),{"l","-slt","-sccUTF-8",archive});QVERIFY(listing.waitForFinished(15000));QCOMPARE(listing.exitCode(),0);const auto text=QString::fromUtf8(listing.readAllStandardOutput());QVERIFY(PortableUpdater::safeArchiveListing(text));
        const auto entries=text.mid(text.indexOf("----------")).split('\n');int paths=0;
        for(const auto &entry:entries)if(entry.startsWith("Path = ")){auto path=entry.mid(7).trimmed();path.replace('\\','/');QVERIFY2(QRegularExpression("^VS-Renderer-GUI-[0-9.]+-windows-x64$").match(path.section('/',0,0)).hasMatch(),qPrintable(path));++paths;}
        QVERIFY(paths>10);
    }
    void payloadLayouts(){
        QTemporaryDir flat;payload(flat.path());QCOMPARE(PortableUpdater::payloadDirectory(flat.path()),flat.path());
        QTemporaryDir wrapped;const auto child=wrapped.filePath("Any name 1.0.3");payload(child);QCOMPARE(PortableUpdater::payloadDirectory(wrapped.path()),child);
        payload(wrapped.filePath("second"));QVERIFY(PortableUpdater::payloadDirectory(wrapped.path()).isEmpty());
        QTemporaryDir deep;payload(deep.filePath("one/two"));QVERIFY(PortableUpdater::payloadDirectory(deep.path()).isEmpty());
        QFile::remove(flat.filePath("Qt6Core.dll"));QVERIFY(PortableUpdater::payloadDirectory(flat.path()).isEmpty());
    }
    void downloadVerifyAndCancel_data(){QTest::addColumn<QString>("wrapper");QTest::newRow("flat")<<QString();QTest::newRow("arbitrary-wrapper")<<QString("Any portable folder");}
    void downloadVerifyAndCancel(){QFETCH(QString,wrapper);QTemporaryDir dir;const auto root=wrapper.isEmpty()?dir.path():dir.filePath(wrapper);payload(root);
        QTemporaryDir archive;QProcess zip;zip.setWorkingDirectory(dir.path());zip.start(tool("7z.exe"),{"a","-t7z",archive.filePath("test.7z"),wrapper.isEmpty()?"*":wrapper,"-mx=1"});QVERIFY(zip.waitForFinished(15000));QCOMPARE(zip.exitCode(),0);QFile file(archive.filePath("test.7z"));QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();
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
