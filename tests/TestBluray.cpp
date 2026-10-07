#include "bluray/BlurayCatalog.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>
#include "bluray/BlurayWidget.h"
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QProcess>
#include <QJsonDocument>
#include <QtEndian>
using namespace vsr;
namespace {
void put16(QByteArray &d,int p,quint16 v){qToBigEndian(v,d.data()+p);}
void put32(QByteArray &d,int p,quint32 v){qToBigEndian(v,d.data()+p);}
QByteArray fixture(){
    QByteArray d(58,'\0');d.replace(0,8,"MPLS0200");put32(d,8,58);
    QByteArray list(10,'\0');put16(list,6,3);
    for(int i=0;i<3;++i){QByteArray part(34,'\0');put16(part,0,32);part.replace(2,5,QString("%1").arg(i,5,10,QChar('0')).toLatin1());part.replace(7,4,"M2TS");put32(part,14,27000000);put32(part,18,27000000+(i==2?45000:24*60*45000));list+=part;}
    put32(list,0,list.size()-4);put32(d,12,d.size()+list.size());QByteArray marks(6+28,'\0');put32(marks,0,marks.size()-4);put16(marks,4,2);for(int i=0;i<2;++i){marks[6+i*14+1]=1;put16(marks,6+i*14+2,i);put32(marks,6+i*14+4,27000000+45000);}
    return d+list+marks;
}
void write(const QString &path,const QByteArray &d){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(d),d.size());}
}
class TestBluray:public QObject {
    Q_OBJECT
private slots:
    void realRemux(){const auto source=qEnvironmentVariable("VSR_BD_TEST_SOURCE"),dest=qEnvironmentVariable("VSR_BD_REMUX_DIR");if(source.isEmpty() || dest.isEmpty())QSKIP("Set VSR_BD_TEST_SOURCE and VSR_BD_REMUX_DIR for real GUI remux");const auto output=QDir(dest).filePath("BD-GUI-EP03.mkv");QVERIFY(!QFileInfo::exists(output));BlurayWidget widget;widget.show();widget.open(source);auto *table=widget.findChild<QTreeWidget *>("blurayTitles");QTRY_VERIFY_WITH_TIMEOUT(table->topLevelItemCount()>0,15000);bool chosen=false;for(int i=0;i<table->topLevelItemCount();++i){auto *row=table->topLevelItem(i);const bool select=!chosen && row->checkState(0)==Qt::Checked;row->setCheckState(0,select?Qt::Checked:Qt::Unchecked);if(select){chosen=true;row->setText(0,"BD-GUI-EP03");}}QVERIFY(chosen);widget.findChild<QLineEdit *>("blurayOutput")->setText(dest);auto *run=widget.findChild<QPushButton *>("blurayRemuxStart");run->click();QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(),180000);QVERIFY(QFileInfo::exists(output));QVERIFY(!QFileInfo::exists(output+".partial.mkv"));QProcess identify;identify.start(BlurayCatalog::mkvmerge(),{"-J",output});QVERIFY(identify.waitForFinished(10000));QCOMPARE(identify.exitCode(),0);const auto info=QJsonDocument::fromJson(identify.readAllStandardOutput()).object();QVERIFY(!info.value("chapters").toArray().isEmpty());QVERIFY(info.value("tracks").toArray().size()>=2);qInfo()<<"GUI remux"<<QFileInfo(output).size()<<"tracks"<<info.value("tracks").toArray().size();}
    void parseAndSlice(){BlurayPlaylist p;QString error;QVERIFY2(BlurayCatalog::parse(fixture(),&p,&error),qPrintable(error));QCOMPARE(p.parts.size(),3);QCOMPARE(p.parts[0].out-p.parts[0].in,quint32(24*60*45000));QCOMPARE(p.marks.size(),2);BlurayPlaylist sliced;QVERIFY(BlurayCatalog::parse(BlurayCatalog::slice(p,1,1),&sliced,&error));QCOMPARE(sliced.parts.size(),1);QCOMPARE(sliced.parts[0].clip,QString("00001"));QCOMPARE(sliced.marks.size(),1);QCOMPARE(sliced.marks[0].item,0);QCOMPARE(sliced.marks[0].time,quint32(27045000));QVERIFY(BlurayCatalog::slice(p,4,1).isEmpty());}
    void rejectTruncation(){const auto d=fixture();QString error;BlurayPlaylist p;for(int n=0;n<d.size();++n)QVERIFY(!BlurayCatalog::parse(d.left(n),&p,&error));auto malformed=d;put32(malformed,8,0xffffffff);QVERIFY(!BlurayCatalog::parse(malformed,&p,&error));malformed=d;put16(malformed,64,65535);QVERIFY(!BlurayCatalog::parse(malformed,&p,&error));malformed=d;malformed.replace(68,5,"../00");QVERIFY(!BlurayCatalog::parse(malformed,&p,&error));}
    void interactiveMenuSubpath(){auto d=fixture();const int mark=qFromBigEndian<quint32>(d.constData()+12);QByteArray sub(10,'\0');put32(sub,0,6);sub[5]=3;put16(d,66,1);put32(d,58,qFromBigEndian<quint32>(d.constData()+58)+10);put32(d,12,mark+10);d.insert(mark,sub);BlurayPlaylist p;QString error;QVERIFY(BlurayCatalog::parse(d,&p,&error));QVERIFY(!p.subpaths);QVERIFY(!BlurayCatalog::slice(p,1,1).isEmpty());sub[5]=4;d.replace(mark,10,sub);QVERIFY(BlurayCatalog::parse(d,&p,&error));QVERIFY(p.subpaths);QVERIFY(BlurayCatalog::slice(p,1,1).isEmpty());}
    void concatPrerollKeepsOpeningKeyframe(){
        QTemporaryDir tmp;const QDir bd(QDir(tmp.path()).filePath("BDMV"));
        QVERIFY(QDir().mkpath(bd.filePath("PLAYLIST")));QVERIFY(QDir().mkpath(bd.filePath("STREAM")));QVERIFY(QDir().mkpath(bd.filePath("CLIPINF")));
        const quint32 in=27000000,out=27450450;
        QByteArray d(58,'\0');d.replace(0,8,"MPLS0200");put32(d,8,58);
        QByteArray list(10,'\0');put16(list,6,1);
        QByteArray part(34,'\0');put16(part,0,32);part.replace(2,5,"00000");part.replace(7,4,"M2TS");put32(part,14,in);put32(part,18,out);list+=part;
        put32(list,0,list.size()-4);put32(d,12,0);d+=list;
        write(bd.filePath("PLAYLIST/00000.mpls"),d);
        write(bd.filePath("STREAM/00000.m2ts"),"test");write(bd.filePath("CLIPINF/00000.clpi"),"test");
        BlurayTitle title;title.disc=bd.absolutePath();title.playlist=bd.filePath("PLAYLIST/00000.mpls");title.itemCount=1;title.clips<<QString("00000");
        QString error;const auto cached=BlurayCatalog::prepare(title,&error);QVERIFY2(!cached.isEmpty(),qPrintable(error));
        const auto input=BlurayCatalog::playbackInput(cached,&error);QVERIFY2(!input.isEmpty(),qPrintable(error));
        QFile file(input);QVERIFY(file.open(QIODevice::ReadOnly));const auto text=QString::fromUtf8(file.readAll());
        const auto value=[&](const QString &key){const auto match=QRegularExpression(key+" ([0-9.]+)").match(text);return match.hasMatch()?match.captured(1).toDouble():-1.;};
        const double start=value("inpoint"),stop=value("outpoint"),duration=value("duration");
        // The concat demuxer resumes after the seek result, so an inpoint equal to
        // the PlayItem IN drops the opening IDR and the segment decodes from
        // unreferenced frames. The generated script must start strictly earlier.
        QVERIFY(start>0.);
        QVERIFY(start<in/45000.);
        QVERIFY(qAbs((in/45000.-start)-0.001)<1e-9);
        QVERIFY(qAbs(stop-out/45000.)<1e-9);
        QVERIFY(qAbs(duration-(out-in)/45000.)<1e-9);
    }
    void scanAndTemplate(){QTemporaryDir tmp;const QDir root(tmp.path());QVERIFY(QDir().mkpath(root.filePath("Vol1/BDMV/PLAYLIST")));QVERIFY(QDir().mkpath(root.filePath("Vol1/BDMV/STREAM")));QVERIFY(QDir().mkpath(root.filePath("Vol1/BDMV/CLIPINF")));const QDir bd(root.filePath("Vol1/BDMV"));write(bd.filePath("PLAYLIST/00000.mpls"),fixture());write(bd.filePath("PLAYLIST/00001.mpls"),fixture());for(int i=0;i<3;++i){const auto name=QString("%1").arg(i,5,10,QChar('0'));write(bd.filePath("STREAM/"+name+".m2ts"),"test");write(bd.filePath("CLIPINF/"+name+".clpi"),"test");}auto scan=BlurayCatalog::scan(tmp.path());QCOMPARE(scan.titles.size(),3);QCOMPARE(scan.titles[0].suggested,true);QCOMPARE(scan.titles[1].suggested,true);QCOMPARE(scan.titles[2].suggested,false);QCOMPARE(scan.titles[1].chapters,1);
        QJsonObject profile{{"entries",QJsonArray{QJsonObject{{"playlist","00000.mpls"},{"firstItem",1},{"itemCount",1},{"label","Episode 9"}}}}};scan=BlurayCatalog::scan(tmp.path(),profile);QCOMPARE(scan.titles.size(),1);QCOMPARE(scan.titles[0].label,QString("Episode 9"));QCOMPARE(scan.titles[0].clips,QStringList{"00001"});QVERIFY(!BlurayCatalog::feedback(scan).value("metadata").toArray().isEmpty());}
    void realMetadata(){const auto source=qEnvironmentVariable("VSR_BD_TEST_SOURCE");if(source.isEmpty())QSKIP("Set VSR_BD_TEST_SOURCE for local/remote BD metadata audit");const auto scan=BlurayCatalog::scan(source);qInfo().noquote()<<"source:"<<source<<"titles:"<<scan.titles.size()<<"warnings:"<<scan.warnings;QVERIFY(!scan.titles.isEmpty());for(const auto &t:scan.titles)qInfo().noquote()<<t.label<<BlurayCatalog::time(t.ticks)<<t.clips.join(',')<<t.warning;const auto local=qEnvironmentVariable("VSR_BD_TEST_PREPARE");if(local=="1"){QString error;const auto playlist=BlurayCatalog::prepare(scan.titles.at(qEnvironmentVariableIntValue("VSR_BD_TEST_ITEM")),&error);QVERIFY2(!playlist.isEmpty(),qPrintable(error));qInfo().noquote()<<"prepared:"<<playlist;const auto metadata=BlurayCatalog::metadata(playlist);QVERIFY(!metadata.value("chapters").toArray().isEmpty());qInfo()<<"languages"<<metadata.value("languages");}}
    void realWidget(){const auto source=qEnvironmentVariable("VSR_BD_TEST_SOURCE");if(source.isEmpty())QSKIP("Set VSR_BD_TEST_SOURCE for BD UI smoke");BlurayWidget widget;widget.resize(1400,780);widget.show();widget.open(source);auto *table=widget.findChild<QTreeWidget *>("blurayTitles");QVERIFY(table);QTRY_VERIFY_WITH_TIMEOUT(table->topLevelItemCount()>0,15000);int checked=0;for(int i=0;i<table->topLevelItemCount();++i)checked+=table->topLevelItem(i)->checkState(0)==Qt::Checked;QVERIFY(checked>0);bool played=false;connect(&widget,&BlurayWidget::playRequested,&widget,[&](const QVector<BlurayTitle> &titles,int index){played=!titles.isEmpty() && index==0;});auto *button=widget.findChild<QPushButton *>("blurayRemuxStart");QVERIFY(button);for(auto *b:widget.findChildren<QPushButton *>())if(b->text()==QStringLiteral("播放所选节目"))b->click();QVERIFY(played);const auto shot=qEnvironmentVariable("VSR_BD_SCREENSHOT");if(!shot.isEmpty())QVERIFY(widget.grab().save(shot));}
};
QTEST_MAIN(TestBluray)
#include "TestBluray.moc"
