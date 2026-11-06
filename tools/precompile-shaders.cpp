#include "render/ShaderPackage.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QTextStream>
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);if(argc!=3 && argc!=4)return 2;
    const auto sourceRoot=QDir(QString::fromLocal8Bit(argv[1])).absolutePath();
    QJsonArray results;int passed=0;
    QDirIterator files(sourceRoot,{"*.glsl","*.hook","*.hlsl"},QDir::Files,QDirIterator::Subdirectories);
    while(files.hasNext()){
        const auto path=files.next();if(argc==4 && QFileInfo(path).canonicalFilePath()!=QFileInfo(QString::fromLocal8Bit(argv[3])).canonicalFilePath())continue;const auto relative=QDir(sourceRoot).relativeFilePath(path);const auto previous=QDir(QString::fromLocal8Bit(argv[2])).filePath(relative);QString input=path;
        if(QFileInfo(previous).absoluteFilePath()!=QFileInfo(path).absoluteFilePath() && QFileInfo::exists(previous+".vsrhlsl")){QDir().mkpath(QFileInfo(previous).absolutePath());QFile::remove(previous);if(QFile::copy(path,previous))input=previous;}
        const auto result=vsr::ShaderPackage::prepare(input,QString::fromLocal8Bit(argv[2])+"/cache");
        if(!result.packagePath.isEmpty()){
            const auto output=QDir(QString::fromLocal8Bit(argv[2])).filePath("mpv-shaders/"+QDir(sourceRoot).relativeFilePath(path)+".vsrhlsl");QDir().mkpath(QFileInfo(output).absolutePath());QFile::remove(output);if(!QFile::copy(result.packagePath,output))return 3;
            QFile source(path);source.open(QIODevice::ReadOnly);const auto hash=QCryptographicHash::hash(source.readAll(),QCryptographicHash::Sha256).toHex();
            QFile metadata(output+".json");if(!metadata.open(QIODevice::WriteOnly))return 3;metadata.write(QJsonDocument(QJsonObject{{"semantics","luma-main-prekernel-params-v8"},{"sha256",QString::fromLatin1(hash)}}).toJson());++passed;
        }
        results.append(QJsonObject{{"file",path},{"compiled",!result.packagePath.isEmpty()},{"reusedPrevious",result.packagePath==previous+".vsrhlsl"},{"error",result.error}});
    }
    QFile report(QString::fromLocal8Bit(argv[2])+"/shader-report.json");if(!report.open(QIODevice::WriteOnly))return 3;
    report.write(QJsonDocument(QJsonObject{{"compiled",passed},{"total",results.size()},{"results",results}}).toJson());QTextStream(stdout)<<passed<<" / "<<results.size()<<Qt::endl;return 0;
}
