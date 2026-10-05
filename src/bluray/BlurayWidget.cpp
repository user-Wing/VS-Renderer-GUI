#include "bluray/BlurayWidget.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QUrl>
#include <QtConcurrentRun>

namespace vsr {
BlurayWidget::BlurayWidget(QWidget *parent):QWidget(parent) {
    setObjectName("blurayRemux");auto *layout=new QVBoxLayout(this);
    auto *intro=new QLabel(tr("BD 一键 Remux · 读取 MPLS 顺序、起止时间与章节，原样保留音视频及字幕。候选集号需核对；双击预览，勾选后导出 MKV。"),this);intro->setWordWrap(true);layout->addWidget(intro);
    auto *input=new QHBoxLayout;source_=new QLineEdit(this);source_->setPlaceholderText(tr("光盘根目录 / BDMV / 包含多个 BD 的目录"));source_->setObjectName("bluraySource");input->addWidget(source_,1);
    auto *folder=new QPushButton(tr("打开 BD 文件夹…"),this);input->addWidget(folder);auto *iso=new QPushButton(tr("装载光盘镜像…"),this);input->addWidget(iso);layout->addLayout(input);
    connect(folder,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getExistingDirectory(this,tr("打开 BD 文件夹"),source_->text());if(!path.isEmpty())open(path);});connect(iso,&QPushButton::clicked,this,[this]{mountImage();});
    auto *options=new QHBoxLayout;mode_=new QComboBox(this);mode_->addItems({tr("自动 · 动画剧集 15–45 分钟"),tr("完整 MPLS · 电影 / 连续节目"),tr("自定义 JSON 模板")});options->addWidget(mode_);
    auto *load=new QPushButton(tr("加载模板…"),this);options->addWidget(load);auto *rescan=new QPushButton(tr("重新识别"),this);options->addWidget(rescan);
    auto *manual=new QPushButton(tr("手动添加文件…"),this);options->addWidget(manual);
    connect(manual,&QPushButton::clicked,this,[this]{const auto paths=QFileDialog::getOpenFileNames(this,tr("手动选择 M2TS；章节/裁切信息请优先通过 MPLS 模板指定"),source_->text(),"Blu-ray stream (*.m2ts)");for(const auto &path:paths){BlurayTitle t;t.playlist=path;t.disc=QFileInfo(path).absolutePath();t.label=QFileInfo(path).completeBaseName();t.clips={QFileInfo(path).fileName()};t.suggested=true;t.warning=tr("手动 M2TS：保留整段视频；没有 MPLS 章节/裁切信息");scan_.titles<<t;}display();});
    auto *profile=new QPushButton(tr("保存所选模式…"),this);options->addWidget(profile);auto *feedback=new QPushButton(tr("导出结构反馈…"),this);options->addWidget(feedback);options->addStretch();layout->addLayout(options);
    connect(rescan,&QPushButton::clicked,this,&BlurayWidget::scan);connect(mode_,&QComboBox::currentIndexChanged,this,[this]{if(!source_->text().isEmpty())scan();});
    connect(load,&QPushButton::clicked,this,[this]{const auto file=QFileDialog::getOpenFileName(this,tr("加载 BD 模板"),{},"JSON (*.json)");if(file.isEmpty())return;QFile f(file);if(!f.open(QIODevice::ReadOnly)){status_->setText(f.errorString());return;}QJsonParseError error;const auto doc=QJsonDocument::fromJson(f.readAll(),&error);if(!doc.isObject() || error.error!=QJsonParseError::NoError || doc.object().value("schemaVersion").toInt()!=1){status_->setText(tr("模板须为 schemaVersion=1 的 JSON 对象"));return;}profile_=doc.object();if(mode_->currentIndex()==2)scan();else mode_->setCurrentIndex(2);});
    const auto saveJson=[this](const QJsonObject &object,const QString &name){const auto path=QFileDialog::getSaveFileName(this,name,{},"JSON (*.json)");if(path.isEmpty())return;QSaveFile f(path);const auto bytes=QJsonDocument(object).toJson();if(!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit())status_->setText(tr("保存失败：%1").arg(f.errorString()));else status_->setText(tr("已保存：%1").arg(path));};
    connect(feedback,&QPushButton::clicked,this,[this,saveJson]{saveJson(BlurayCatalog::feedback(scan_),tr("保存 BD 结构反馈（含元数据，不含视频）"));});
    connect(profile,&QPushButton::clicked,this,[this,saveJson]{QJsonArray entries;for(const auto &t:selected()){if(t.itemCount==0)continue;auto match=QDir(scan_.source).relativeFilePath(t.disc);if(match==".")match.clear();entries<<QJsonObject{{"discContains",match},{"playlist",QFileInfo(t.playlist).fileName()},{"firstItem",t.firstItem},{"itemCount",t.itemCount},{"label",t.label}};}if(entries.isEmpty()){status_->setText(tr("先勾选 MPLS 节目再保存模板"));return;}saveJson({{"schemaVersion",1},{"entries",entries}},tr("保存 BD 匹配模板"));});
    table_=new QTreeWidget(this);table_->setObjectName("blurayTitles");table_->setHeaderLabels({tr("节目 / 输出名称（可编辑）"),tr("时长"),tr("章节"),tr("MPLS / PlayItem"),tr("视频片段"),tr("识别说明")});table_->header()->setSectionResizeMode(0,QHeaderView::Stretch);table_->setColumnWidth(1,105);table_->setColumnWidth(2,55);table_->setColumnWidth(3,165);table_->setColumnWidth(4,170);table_->setColumnWidth(5,270);table_->setTextElideMode(Qt::ElideMiddle);layout->addWidget(table_,1);
    const auto play=[this]{const auto items=selected();if(items.isEmpty()){status_->setText(tr("先勾选需要播放的节目"));return;}emit playRequested(items,0);};
    connect(table_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *item,int column){if(column==0)return;auto titles=selected();auto t=scan_.titles.at(item->data(0,Qt::UserRole).toInt());t.label=item->text(0);int index=-1;for(int i=0;i<titles.size();++i)if(titles[i].playlist==t.playlist && titles[i].firstItem==t.firstItem && titles[i].itemCount==t.itemCount)index=i;if(index<0){titles={t};index=0;}emit playRequested(titles,index);});
    auto *dest=new QHBoxLayout;dest->addWidget(new QLabel(tr("输出目录"),this));output_=new QLineEdit(QSettings().value("bluray/output").toString(),this);output_->setObjectName("blurayOutput");dest->addWidget(output_,1);auto *pick=new QPushButton(tr("浏览…"),this);dest->addWidget(pick);layout->addLayout(dest);connect(pick,&QPushButton::clicked,this,[this]{const auto dir=QFileDialog::getExistingDirectory(this,tr("MKV 输出目录"),output_->text());if(!dir.isEmpty())output_->setText(dir);});
    auto *tools=new QHBoxLayout;tools->addWidget(new QLabel("mkvmerge",this));tool_=new QLineEdit(QSettings().value("bluray/mkvmerge",BlurayCatalog::mkvmerge()).toString(),this);tools->addWidget(tool_,1);auto *browse=new QPushButton(tr("选择工具…"),this);tools->addWidget(browse);layout->addLayout(tools);connect(browse,&QPushButton::clicked,this,[this]{const auto file=QFileDialog::getOpenFileName(this,tr("选择 mkvmerge"),tool_->text(),"mkvmerge (mkvmerge.exe)");if(!file.isEmpty())tool_->setText(file);});
    auto *actions=new QHBoxLayout;auto *preview=new QPushButton(tr("播放所选节目"),this);actions->addWidget(preview);connect(preview,&QPushButton::clicked,this,play);run_=new QPushButton(tr("一键 Remux 所选节目"),this);run_->setObjectName("blurayRemuxStart");actions->addWidget(run_);connect(run_,&QPushButton::clicked,this,&BlurayWidget::remux);stop_=new QPushButton(tr("停止队列"),this);stop_->setEnabled(false);actions->addWidget(stop_);connect(stop_,&QPushButton::clicked,this,[this]{stopped_=true;queue_.clear();process_.terminate();});auto *download=new QPushButton(tr("通过 ModelScope Manager 下载 BD…"),this);actions->addWidget(download);connect(download,&QPushButton::clicked,this,&BlurayWidget::download);actions->addStretch();layout->addLayout(actions);
    status_=new QLabel(tr("请选择 BD 文件夹。支持无加密 BDMV；加密盘、多角度与 BD-J 菜单需另外处理。"),this);status_->setWordWrap(true);layout->addWidget(status_);log_=new QPlainTextEdit(this);log_->setReadOnly(true);log_->setMaximumBlockCount(1500);log_->setMaximumHeight(140);layout->addWidget(log_);
    process_.setProcessChannelMode(QProcess::MergedChannels);connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{log_->appendPlainText(QString::fromUtf8(process_.readAllStandardOutput()));});
    connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart){status_->setText(process_.errorString());queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);}});
    connect(&process_,&QProcess::finished,this,[this](int code,QProcess::ExitStatus state){
        if(stopped_ || state!=QProcess::NormalExit || code>1){status_->setText(tr("任务停止/失败；未完成文件保留为 .partial.mkv，原文件不覆盖。"));queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);return;}
        const auto partial=activeOutput_+".partial.mkv";
        if(!QFile::rename(partial,activeOutput_)){status_->setText(tr("完成输出重命名失败：%1").arg(partial));queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);return;}
        log_->appendPlainText(tr("%1：%2").arg(code==1?tr("已完成（工具有警告，请查看日志）"):tr("已完成"),activeOutput_));next();
    });
}
void BlurayWidget::open(const QString &path){if(QFileInfo(path).isFile() && QStringList{"iso","img","vhd","vhdx"}.contains(QFileInfo(path).suffix().toLower())){mountImage(path);return;}source_->setText(path);scan();}
void BlurayWidget::scan(){
    if(mode_->currentIndex()==2 && profile_.isEmpty()){status_->setText(tr("请先加载自定义模板"));return;}
    const auto path=source_->text().trimmed();if(path.isEmpty())return;
    const int generation=++scanGeneration_;status_->setText(tr("正在读取 BD 元数据…"));table_->clear();
    auto *watcher=new QFutureWatcher<BlurayScan>(this);const auto profile=mode_->currentIndex()==2?profile_:QJsonObject{{"splitPlayItems",mode_->currentIndex()==0}};
    connect(watcher,&QFutureWatcher<BlurayScan>::finished,this,[this,watcher,generation]{if(generation==scanGeneration_){scan_=watcher->result();display();}watcher->deleteLater();});
    watcher->setFuture(QtConcurrent::run([path,profile]{return BlurayCatalog::scan(path,profile);}));
}
void BlurayWidget::display(){
    table_->clear();for(int i=0;i<scan_.titles.size();++i){const auto &t=scan_.titles[i];auto *row=new QTreeWidgetItem(table_);row->setData(0,Qt::UserRole,i);row->setFlags(row->flags()|Qt::ItemIsUserCheckable|Qt::ItemIsEditable);row->setCheckState(0,t.suggested && t.warning.isEmpty()?Qt::Checked:Qt::Unchecked);row->setText(0,t.label);row->setText(1,BlurayCatalog::time(t.ticks));row->setText(2,QString::number(t.chapters));row->setText(3,QString("%1 · %2–%3").arg(QFileInfo(t.playlist).fileName()).arg(t.firstItem+1).arg(t.firstItem+t.itemCount));row->setText(4,t.clips.join(", "));row->setText(5,t.warning.isEmpty()?(t.suggested?tr("候选正片；集号按卷/播放顺序推断"):tr("完整节目 / 花絮；手动勾选")):t.warning);row->setToolTip(0,t.disc);row->setToolTip(3,t.playlist);row->setToolTip(5,row->text(5));}
    status_->setText(tr("识别 %1 个节目；默认勾选候选正片。播放请双击时长列或使用按钮。%2").arg(scan_.titles.size()).arg(scan_.warnings.join('\n')));
}
QVector<BlurayTitle> BlurayWidget::selected()const{QVector<BlurayTitle> titles;for(int i=0;i<table_->topLevelItemCount();++i){auto *row=table_->topLevelItem(i);if(row->checkState(0)!=Qt::Checked)continue;auto t=scan_.titles.at(row->data(0,Qt::UserRole).toInt());t.label=row->text(0);titles<<t;}return titles;}
void BlurayWidget::remux(){
    if(process_.state()!=QProcess::NotRunning)return;
    queue_=selected();if(queue_.isEmpty()){status_->setText(tr("请勾选节目"));return;}
    if(!QFileInfo(tool_->text()).isFile() || output_->text().trimmed().isEmpty() || !QDir().mkpath(output_->text())){status_->setText(tr("请选择可用 mkvmerge 与输出目录"));queue_.clear();return;}
    queueOutput_=output_->text();queueTool_=tool_->text();QSettings().setValue("bluray/output",queueOutput_);QSettings().setValue("bluray/mkvmerge",queueTool_);pending_=0;stopped_=false;run_->setEnabled(false);stop_->setEnabled(true);next();
}
void BlurayWidget::next(){
    if(pending_>=queue_.size()){status_->setText(tr("Remux 队列已完成；音视频未重新编码，轨道与章节保留。"));queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);return;}
    const auto title=queue_[pending_++];QString error;const auto playlist=BlurayCatalog::prepare(title,&error);
    if(playlist.isEmpty()){status_->setText(error);queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);return;}
    auto name=title.label;name.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");name=name.trimmed();while(name.endsWith('.') || name.endsWith(' '))name.chop(1);if(name.isEmpty())name="BD-title";
    activeOutput_=QDir(queueOutput_).filePath(name+".mkv");
    if(QFileInfo::exists(activeOutput_) || QFileInfo::exists(activeOutput_+".partial.mkv")){status_->setText(tr("输出已存在，请改名或换目录：%1").arg(activeOutput_));queue_.clear();run_->setEnabled(true);stop_->setEnabled(false);return;}
    status_->setText(tr("正在 Remux %1/%2：%3").arg(pending_).arg(queue_.size()).arg(title.label));
    const QStringList args{"--ui-language","en","-o",activeOutput_+".partial.mkv",playlist};log_->appendPlainText(queueTool_+" "+args.join(' '));process_.start(queueTool_,args);
}
void BlurayWidget::mountImage(const QString &image){
    const auto file=image.isEmpty()?QFileDialog::getOpenFileName(this,tr("装载 BD 镜像"),{},"Disc image (*.iso *.img *.vhd *.vhdx)"):image;if(file.isEmpty())return;
    auto path=QDir::toNativeSeparators(file);path.replace("'","''");const auto script=QString("$ErrorActionPreference='Stop'; $i=Mount-DiskImage -ImagePath '%1' -PassThru; $i | Get-Volume | Where-Object DriveLetter | ForEach-Object { $_.DriveLetter + ':\\' }").arg(path);
    auto *process=new QProcess(this);connect(process,&QProcess::finished,this,[this,process](int code){const auto root=QString::fromUtf8(process->readAllStandardOutput()).trimmed();if(code==0 && QFileInfo(root).isDir())open(root);else status_->setText(tr("装载失败：%1").arg(QString::fromUtf8(process->readAllStandardError())));process->deleteLater();});connect(process,&QProcess::errorOccurred,this,[this,process]{status_->setText(process->errorString());process->deleteLater();});process->start("powershell.exe",{"-NoProfile","-NonInteractive","-EncodedCommand",QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(script.utf16()),script.size()*2).toBase64())});
}
void BlurayWidget::download(){
    const auto manager=QFileDialog::getExistingDirectory(this,tr("选择 ModelScope Manager 程序目录（含 runtime/python.exe）"));if(manager.isEmpty())return;
    const auto dest=QFileDialog::getExistingDirectory(this,tr("选择本地 BD 下载目录"));if(dest.isEmpty())return;
    QString prefix=QDir::fromNativeSeparators(source_->text());const QString marker="/datasets/ARXChem/Animations-List/";const int index=prefix.indexOf(marker,0,Qt::CaseInsensitive);if(index<0){status_->setText(tr("下载入口支持 ARXChem/Animations-List 网络挂载路径；其它来源请在 Manager 中选择下载。"));return;}prefix=prefix.mid(index+marker.size());
    const auto script=QDir(QCoreApplication::applicationDirPath()).filePath("tools/download-bd.py");
    if(!QFileInfo(script).isFile() || !QFileInfo(QDir(manager).filePath("runtime/python.exe")).isFile()){status_->setText(tr("缺少下载脚本或 Manager Python 运行时"));return;}
    const QStringList args{"-u",script,"--manager",manager,"--prefix",prefix,"--destination",dest};
    if(QProcess::startDetached(QDir(manager).filePath("runtime/pythonw.exe"),args)){status_->setText(tr("完整 BD 下载已启动，可在下载目录查看 bd-download.log 与 .download-state。完成后打开下载后的根目录；重复启动可断点续传。"));}else status_->setText(tr("无法启动 Manager 下载引擎"));
}
}
