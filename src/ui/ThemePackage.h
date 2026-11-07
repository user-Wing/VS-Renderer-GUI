#pragma once
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QMap>
#include <QFileInfo>
#include <QImageReader>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
namespace vsr::ThemePackage {
inline bool archive(const QStringList &arguments,const QString &directory,QByteArray &output,QString &error){
    auto tool=QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/7z.exe");
    if(!QFileInfo::exists(tool))tool=QStandardPaths::findExecutable("7z.exe");
    if(tool.isEmpty()){error=QObject::tr("找不到主题压缩工具 7z.exe");return false;}
    QProcess process;process.setWorkingDirectory(directory);process.start(tool,arguments);
    if(!process.waitForStarted() || !process.waitForFinished(30000)){process.kill();process.waitForFinished();error=QObject::tr("主题压缩工具启动失败或超时");return false;}
    output=process.readAllStandardOutput();if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0){error=QString::fromUtf8(process.readAllStandardError());return false;}return true;
}
inline bool safeName(QString name){name.replace('\\','/');return !name.isEmpty() && !name.startsWith('/') && !name.contains(':') && !name.split('/').contains("..");}
inline QString locate(const QString &directory){
    QString result;QDirIterator it(directory,{"skin.ini"},QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext()){if(!result.isEmpty())return {};result=it.next();}return result;
}
inline bool importPack(const QString &input,const QString &configuration,QString &installed,QString &error){
    QTemporaryDir temporary;QString directory=input;QByteArray output;
    if(QFileInfo(input).isFile()){
        if(!archive({"l","-slt","-sccUTF-8",input},{},output,error))return false;
        const auto listing=QString::fromUtf8(output).section("----------",1);qint64 bytes=0;int files=0;
        for(const auto &line:listing.split('\n')){
            const auto text=line.trimmed();
            if(text.startsWith("Path = ") && (!safeName(text.mid(7)) || ++files>512)){error=QObject::tr("主题包路径无效或文件过多");return false;}
            if(text.startsWith("Size = "))bytes+=text.mid(7).toLongLong();
            if(text.startsWith("Symbolic Link =") || text.startsWith("Hard Link =")){error=QObject::tr("主题包不能包含链接");return false;}
        }
        if(files==0 || bytes>256*1024*1024){error=QObject::tr("主题包为空或超过 256 MiB");return false;}
        if(!archive({"x","-y","-o"+temporary.path(),input},{},output,error))return false;
        directory=temporary.path();
    }
    const auto ini=locate(directory);if(ini.isEmpty()){error=QObject::tr("主题包必须包含唯一的 skin.ini");return false;}
    QSettings source(ini,QSettings::IniFormat);QMap<QString,QVariant> values;
    QMap<QString,QString> images;QCryptographicHash hash(QCryptographicHash::Sha256);
    QFile data(ini);if(!data.open(QIODevice::ReadOnly))return false;hash.addData(data.readAll());
    for(const auto &key:source.allKeys()){
        auto value=source.value(key);
        if(key.endsWith("/image") && !value.toString().isEmpty()){
            const auto name=value.toString();const auto path=QDir(QFileInfo(ini).absolutePath()).filePath(name);
            const QFileInfo info(path);QFile image(path);
            if(!safeName(name) || info.isSymLink() || info.size()>64*1024*1024 || !image.open(QIODevice::ReadOnly) || !QImageReader(path).canRead()){error=QObject::tr("主题图片无效：%1").arg(name);return false;}
            const auto content=image.readAll();hash.addData(content);const auto file=QString::fromLatin1(QCryptographicHash::hash(content,QCryptographicHash::Sha256).toHex())+"."+info.suffix();
            images.insert(file,path);value=file;
        }
        values.insert(key,value);
    }
    const auto relative="themes/"+QString::fromLatin1(hash.result().toHex().left(16));
    const auto target=QDir(QFileInfo(configuration).absolutePath()).filePath(relative);
    if(!QDir().mkpath(target)){error=QObject::tr("无法创建主题目录");return false;}
    for(auto it=images.cbegin();it!=images.cend();++it){const auto file=QDir(target).filePath(it.key());if(!QFileInfo::exists(file) && !QFile::copy(it.value(),file)){error=QObject::tr("无法保存主题图片");return false;}}
    installed=QDir(target).filePath("skin.ini");QSettings destination(installed,QSettings::IniFormat);destination.clear();for(auto it=values.cbegin();it!=values.cend();++it)destination.setValue(it.key(),it.value());destination.sync();return destination.status()==QSettings::NoError;
}
inline bool exportPack(const QString &configuration,const QString &output,QString &error){
    QTemporaryDir temporary;QDir().mkpath(temporary.filePath("skin"));
    const auto ini=temporary.filePath("skin/skin.ini");QSettings source(configuration,QSettings::IniFormat),destination(ini,QSettings::IniFormat);
    for(const auto &key:source.allKeys()){
        auto value=source.value(key);
        if(key.endsWith("/image") && !value.toString().isEmpty()){
            const auto path=QDir(QFileInfo(configuration).absolutePath()).absoluteFilePath(value.toString());QFile image(path);
            if(!image.open(QIODevice::ReadOnly)){error=QObject::tr("无法读取主题图片：%1").arg(path);return false;}
            const auto name=QString::fromLatin1(QCryptographicHash::hash(image.readAll(),QCryptographicHash::Sha256).toHex())+"."+QFileInfo(path).suffix();const auto file=temporary.filePath("skin/"+name);
            if(!QFileInfo::exists(file) && !QFile::copy(path,file))return false;value=name;
        }
        destination.setValue(key,value);
    }
    destination.sync();QByteArray ignored;if(!archive({"a","-tzip",temporary.filePath("theme.zip"),"skin"},temporary.path(),ignored,error))return false;
    QFile zip(temporary.filePath("theme.zip"));QSaveFile result(output);if(!zip.open(QIODevice::ReadOnly) || !result.open(QIODevice::WriteOnly)){error=QObject::tr("无法写入主题包");return false;}
    while(!zip.atEnd()){const auto block=zip.read(1024*1024);if(block.isEmpty() || result.write(block)!=block.size())return false;}return result.commit();
}
}
