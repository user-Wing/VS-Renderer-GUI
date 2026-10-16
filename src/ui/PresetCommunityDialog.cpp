#include "ui/PresetCommunityDialog.h"
#include "graph/PresetStore.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QTreeWidget>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace vsr {
QString PresetCommunityDialog::defaultSource(){return QStringLiteral("https://www.modelscope.cn/api/v1/datasets/ARXChem/Software-List/repo/tree?Revision=master&Root=ffmpegfreeui%2FPreset&PageSize=100&PageNumber=1");}
QString PresetCommunityDialog::directory() const{return QDir(PresetStore::directory()).filePath("Community");}
PresetCommunityDialog::PresetCommunityDialog(std::function<void(const QString &,bool)> import,QWidget *parent):QDialog(parent),import_(std::move(import)){
    setWindowTitle(QStringLiteral("预设社区"));setObjectName("rendererPresetCommunity");resize(900,580);
    source_=QSettings().value("community/sourceUrl",defaultSource()).toString();network_=new QNetworkAccessManager(this);network_->setTransferTimeout(45000);
    auto *layout=new QVBoxLayout(this);auto *source=new QLabel(source_,this);source->setWordWrap(true);layout->addWidget(source);
    auto *hint=new QLabel(QStringLiteral("下载后可导入全部 / 当前时间组。临时 3FUI v6 源仅用于下载和参数查看，不转换为 VS 滤镜。"),this);hint->setWordWrap(true);layout->addWidget(hint);
    files_=new QTreeWidget(this);files_->setObjectName("communityFiles");files_->setHeaderLabels({QStringLiteral("名称"),QStringLiteral("大小"),QStringLiteral("状态")});files_->header()->setSectionResizeMode(0,QHeaderView::Stretch);layout->addWidget(files_,1);
    status_=new QLabel(this);status_->setWordWrap(true);layout->addWidget(status_);auto *row=new QHBoxLayout;layout->addLayout(row);
    refresh_=new QPushButton(QStringLiteral("刷新资源"),this);row->addWidget(refresh_);connect(refresh_,&QPushButton::clicked,this,[this]{refresh();});
    const auto button=[&](const QString &text,auto action){auto *b=new QPushButton(text,this);row->addWidget(b);connect(b,&QPushButton::clicked,this,action);};
    button(QStringLiteral("下载选中"),[this]{enqueue(false);});button(QStringLiteral("下载全部"),[this]{enqueue(true);});
    button(QStringLiteral("导入所有时间组"),[this]{importSelected(false);});button(QStringLiteral("导入当前时间组"),[this]{importSelected(true);});
    connect(files_,&QTreeWidget::itemDoubleClicked,this,[this]{importSelected(false);});refresh();
}
void PresetCommunityDialog::refresh(int page){
    if(active_ || !queue_.isEmpty())return;
    if(page==1)files_->clear();
    refresh_->setEnabled(false);status_->setText(QStringLiteral("正在读取预设清单…"));
    QUrl url(source_);QUrlQuery query(url);const bool modelscope=url.path().endsWith("/tree");
    if(modelscope){query.removeAllQueryItems("PageNumber");query.addQueryItem("PageNumber",QString::number(page));url.setQuery(query);}
    auto *reply=network_->get(QNetworkRequest(url));
    connect(reply,&QNetworkReply::finished,this,[this,reply,modelscope,page]{
        reply->deleteLater();refresh_->setEnabled(true);
        if(reply->error()!=QNetworkReply::NoError){status_->setText(reply->errorString());return;}
        QJsonParseError error;const auto document=QJsonDocument::fromJson(reply->readAll(),&error);if(error.error!=QJsonParseError::NoError){status_->setText(error.errorString());return;}
        const auto root=document.object();const auto data=modelscope?root.value("Data").toObject():root;
        const auto entries=data.value(modelscope?"Files":"files").toArray();
        const auto unsafe=QRegularExpression("[<>:\"/\\\\|?*]");
        for(const auto &value:entries){const auto item=value.toObject();if(modelscope && item.value("Type").toString()!="blob")continue;
            const auto name=item.value(modelscope?"Name":"name").toString();if(!name.endsWith(".json",Qt::CaseInsensitive) && !name.endsWith(".vpy",Qt::CaseInsensitive))continue;
            if(name.isEmpty() || name!=QFileInfo(name).fileName() || name.contains(unsafe) || name.endsWith('.') || name.endsWith(' '))continue;
            QUrl download;
            if(modelscope){download=QUrl(source_);download.setPath(download.path().chopped(5));QUrlQuery q;q.addQueryItem("Revision",QUrlQuery(QUrl(source_)).queryItemValue("Revision"));q.addQueryItem("FilePath",item.value("Path").toString());download.setQuery(q);}
            else download=QUrl(source_).resolved(QUrl(item.value("url").toString()));
            const auto hash=item.value(modelscope?"Sha256":"sha256").toString().toLower();const auto bytes=qint64(item.value(modelscope?"Size":"size").toDouble());
            if(!QRegularExpression("^[0-9a-f]{64}$").match(hash).hasMatch() || bytes<0 || bytes>16*1024*1024 || (download.scheme()!="https" && download.scheme()!="http"))continue;
            QJsonObject record{{"name",name},{"url",download.toString()},{"sha256",hash},{"size",bytes}};
            auto *row=new QTreeWidgetItem(files_,{name,QString::number(bytes/1024.)+" KiB",QStringLiteral("未下载")});row->setData(0,Qt::UserRole,record);
            QFile local(QDir(directory()).filePath(name));if(local.open(QIODevice::ReadOnly) && QCryptographicHash::hash(local.readAll(),QCryptographicHash::Sha256).toHex()==hash.toLatin1())row->setText(2,QStringLiteral("已安装"));
        }
        if(modelscope && page*100<data.value("TotalCount").toInt()){refresh(page+1);return;}
        status_->setText(QStringLiteral("%1 个预设；安装目录：%2").arg(files_->topLevelItemCount()).arg(directory()));
    });
}
void PresetCommunityDialog::enqueue(bool all){
    for(int i=0;i<files_->topLevelItemCount();++i){auto *row=files_->topLevelItem(i);if((all || row==files_->currentItem()) && row->text(2)!=QStringLiteral("已安装") && row->text(2)!=QStringLiteral("下载中") && !queue_.contains(row))queue_<<row;}
    downloadNext();
}
void PresetCommunityDialog::downloadNext(){
    while(active_<3 && !queue_.isEmpty()){
        auto *row=queue_.takeFirst();const auto record=row->data(0,Qt::UserRole).toJsonObject();row->setText(2,QStringLiteral("下载中"));++active_;refresh_->setEnabled(false);
        auto *reply=network_->get(QNetworkRequest(QUrl(record.value("url").toString())));
        connect(reply,&QNetworkReply::downloadProgress,this,[reply](qint64 bytes,qint64){if(bytes>16*1024*1024)reply->abort();});
        connect(reply,&QNetworkReply::finished,this,[this,reply,row,record]{
            reply->deleteLater();--active_;QString failure;
            const auto bytes=reply->readAll();const auto name=record.value("name").toString();
            if(reply->error()!=QNetworkReply::NoError)failure=reply->errorString();
            else if(bytes.size()!=qint64(record.value("size").toDouble()) || QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=record.value("sha256").toString().toLatin1())failure=QStringLiteral("大小 / SHA256 校验失败");
            else if(name.endsWith(".json",Qt::CaseInsensitive) && !QJsonDocument::fromJson(bytes).isObject())failure=QStringLiteral("JSON 格式无效");
            if(failure.isEmpty()){
                QDir().mkpath(directory());const auto target=QDir(directory()).filePath(name);
                if(QFileInfo::exists(target) && !QFile::copy(target,target+QDateTime::currentDateTime().toString(".yyyyMMddHHmmsszzz.bak")))failure=QStringLiteral("保留原预设备份失败");
                if(failure.isEmpty()){QSaveFile file(target);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())failure=file.errorString();}
            }
            row->setText(2,failure.isEmpty()?QStringLiteral("已安装"):QStringLiteral("失败"));row->setToolTip(2,failure);if(!failure.isEmpty())status_->setText(failure);
            downloadNext();if(!active_ && queue_.isEmpty()){refresh_->setEnabled(true);status_->setText(QStringLiteral("下载完成；双击或使用导入按钮。"));}
        });
    }
}
void PresetCommunityDialog::importSelected(bool currentGroup){
    auto *row=files_->currentItem();if(!row)return;if(row->text(2)!=QStringLiteral("已安装")){enqueue(false);status_->setText(QStringLiteral("请在下载完成后导入。"));return;}
    const auto path=QDir(directory()).filePath(row->text(0));
    if(path.endsWith(".vpy",Qt::CaseInsensitive)){import_(path,currentGroup);return;}
    QFile file(path);if(!file.open(QIODevice::ReadOnly)){status_->setText(file.errorString());return;}const auto data=QJsonDocument::fromJson(file.readAll()).object();
    if(data.contains("nodes") || data.contains("timeGroups")){
        const auto target=path+".vpy";QString error;
        if(!PresetStore::write(target,PresetStore::create(PresetStore::timeGroups(data),static_cast<SourceFilter>(data.value("sourceFilter").toInt()),{},data.value("note").toString()),&error)){status_->setText(error);return;}
        import_(target,currentGroup);return;
    }
    QDialog view(this);view.setWindowTitle(QStringLiteral("3FUI 示例参数（独立编码预设）"));view.resize(800,600);auto *layout=new QVBoxLayout(&view);auto *text=new QPlainTextEdit(&view);text->setReadOnly(true);text->setPlainText(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Indented)));layout->addWidget(text);view.exec();
}
}
