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
#include <QTreeWidgetItemIterator>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace vsr {
QString PresetCommunityDialog::defaultSource(){return QStringLiteral("https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity");}
QString PresetCommunityDialog::directory() const{return QDir(PresetStore::directory()).filePath("Community");}
PresetCommunityDialog::PresetCommunityDialog(std::function<void(const QString &,bool)> import,QWidget *parent):QDialog(parent),import_(std::move(import)){
    setWindowTitle(QStringLiteral("预设社区"));setObjectName("rendererPresetCommunity");resize(900,580);
    source_=QSettings().value("community/sourceUrl",defaultSource()).toString();
    if(source_.contains("Root=ffmpegfreeui%2FPreset") || source_.contains("Root=VS-GUI%2FPresetCommunity"))source_=defaultSource();
    listingUrl_=QUrl(source_);
    // Accept the repository browser link used in settings as well as the API URL.
    QUrl browser(source_);const auto marker=browser.path().indexOf("/tree/master/");
    if(browser.host().endsWith("modelscope.cn") && marker>=0){
        const auto parts=browser.path().split('/',Qt::SkipEmptyParts);
        if(parts.size()>=6){QUrl api("https://www.modelscope.cn/api/v1/datasets/"+parts[1]+"/"+parts[2]+"/repo/tree");
            QUrlQuery query;query.addQueryItem("Revision","master");query.addQueryItem("Root",browser.path().mid(marker+13));query.addQueryItem("PageSize","100");api.setQuery(query);listingUrl_=api;}
    }
    network_=new QNetworkAccessManager(this);network_->setTransferTimeout(45000);
    auto *layout=new QVBoxLayout(this);auto *source=new QLabel(source_,this);source->setWordWrap(true);layout->addWidget(source);
    auto *hint=new QLabel(QStringLiteral("自动扫描所有子目录；展开目录选择预设。下载后可导入全部 / 当前时间组。"),this);hint->setWordWrap(true);layout->addWidget(hint);
    files_=new QTreeWidget(this);files_->setObjectName("communityFiles");files_->setHeaderLabels({QStringLiteral("名称"),QStringLiteral("大小"),QStringLiteral("状态")});files_->header()->setSectionResizeMode(0,QHeaderView::Stretch);layout->addWidget(files_,1);
    status_=new QLabel(this);status_->setWordWrap(true);layout->addWidget(status_);auto *row=new QHBoxLayout;layout->addLayout(row);
    refresh_=new QPushButton(QStringLiteral("刷新资源"),this);row->addWidget(refresh_);connect(refresh_,&QPushButton::clicked,this,[this]{refresh();});
    const auto button=[&](const QString &text,auto action){auto *b=new QPushButton(text,this);row->addWidget(b);connect(b,&QPushButton::clicked,this,action);};
    button(QStringLiteral("下载选中"),[this]{enqueue(false);});button(QStringLiteral("下载全部"),[this]{enqueue(true);});
    button(QStringLiteral("导入所有时间组"),[this]{importSelected(false);});button(QStringLiteral("导入当前时间组"),[this]{importSelected(true);});
    connect(files_,&QTreeWidget::itemDoubleClicked,this,[this]{importSelected(false);});refresh();
}
void PresetCommunityDialog::refresh(){
    if(active_ || listing_ || !queue_.isEmpty())return;
    files_->clear();files_->setToolTip({});refresh_->setEnabled(false);status_->setText(QStringLiteral("正在扫描预设目录…"));
    scanDirectory(listingUrl_);
}
void PresetCommunityDialog::scanDirectory(const QUrl &directoryUrl,QTreeWidgetItem *parent,int page){
    QUrl url(directoryUrl);QUrlQuery query(url);const bool modelscope=url.path().endsWith("/tree");
    if(modelscope){query.removeAllQueryItems("PageNumber");query.addQueryItem("PageNumber",QString::number(page));url.setQuery(query);}
    ++listing_;auto *reply=network_->get(QNetworkRequest(url));
    connect(reply,&QNetworkReply::finished,this,[this,reply,directoryUrl,parent,modelscope,page]{
        reply->deleteLater();QString failure;
        if(reply->error()!=QNetworkReply::NoError)failure=reply->errorString();
        QJsonParseError error;const auto document=QJsonDocument::fromJson(reply->readAll(),&error);
        if(failure.isEmpty() && error.error!=QJsonParseError::NoError)failure=error.errorString();
        if(failure.isEmpty()){
            const auto root=document.object();const auto data=modelscope?root.value("Data").toObject():root;
            const auto entries=data.value(modelscope?"Files":"files").toArray();
            const QRegularExpression unsafe("[<>:\"/\\\\|?*]");
            for(const auto &value:entries){const auto item=value.toObject();const auto name=item.value(modelscope?"Name":"name").toString();
                if(name.isEmpty() || name=="." || name==".." || name.contains(unsafe) || name.endsWith('.') || name.endsWith(' '))continue;
                const auto relative=(parent?parent->data(0,Qt::UserRole+1).toString()+"/":QString())+name;
                if(modelscope && item.value("Type").toString()=="tree"){
                    const auto expected=QUrlQuery(directoryUrl).queryItemValue("Root",QUrl::FullyDecoded)+"/"+name;
                    if(item.value("Path").toString()!=expected)continue;
                    auto *folder=parent?new QTreeWidgetItem(parent,{name}):new QTreeWidgetItem(files_,{name});folder->setData(0,Qt::UserRole+1,relative);
                    folder->setExpanded(false);QUrl child(directoryUrl);QUrlQuery q(child);q.removeAllQueryItems("Root");q.addQueryItem("Root",expected);child.setQuery(q);scanDirectory(child,folder);continue;
                }
                if(modelscope && item.value("Type").toString()!="blob")continue;
                if(!name.endsWith(".json",Qt::CaseInsensitive) && !name.endsWith(".vpy",Qt::CaseInsensitive))continue;
                QUrl download;
                if(modelscope){download=directoryUrl;download.setPath(download.path().chopped(5));QUrlQuery q;q.addQueryItem("Revision",QUrlQuery(directoryUrl).queryItemValue("Revision"));q.addQueryItem("FilePath",item.value("Path").toString());download.setQuery(q);}
                else download=directoryUrl.resolved(QUrl(item.value("url").toString()));
                const auto hash=item.value(modelscope?"Sha256":"sha256").toString().toLower();const auto bytes=qint64(item.value(modelscope?"Size":"size").toDouble());
                if(!QRegularExpression("^[0-9a-f]{64}$").match(hash).hasMatch() || bytes<0 || bytes>16*1024*1024 || (download.scheme()!="https" && download.scheme()!="http"))continue;
                QJsonObject record{{"name",relative},{"url",download.toString()},{"sha256",hash},{"size",bytes}};
                const QStringList columns{name,QString::number(bytes/1024.)+" KiB",QStringLiteral("未下载")};
                auto *row=parent?new QTreeWidgetItem(parent,columns):new QTreeWidgetItem(files_,columns);row->setData(0,Qt::UserRole,record);
                QFile local(QDir(directory()).filePath(relative));if(local.open(QIODevice::ReadOnly) && QCryptographicHash::hash(local.readAll(),QCryptographicHash::Sha256).toHex()==hash.toLatin1())row->setText(2,QStringLiteral("已安装"));
            }
            if(modelscope && page*100<data.value("TotalCount").toInt())scanDirectory(directoryUrl,parent,page+1);
        }
        if(!failure.isEmpty()){files_->setToolTip(failure);status_->setText(failure);}
        if(!--listing_){refresh_->setEnabled(true);if(files_->toolTip().isEmpty())status_->setText(QStringLiteral("目录扫描完成；安装目录：%1").arg(directory()));}
    });
}
void PresetCommunityDialog::enqueue(bool all){
    if(listing_)return;
    const auto *selected=files_->currentItem();
    for(QTreeWidgetItemIterator it(files_);*it;++it){auto *row=*it;bool included=all;
        for(auto *ancestor=row;ancestor;ancestor=ancestor->parent())if(ancestor==selected)included=true;
        if(included && !row->data(0,Qt::UserRole).toJsonObject().isEmpty() && row->text(2)!=QStringLiteral("已安装") && row->text(2)!=QStringLiteral("下载中") && !queue_.contains(row))queue_<<row;
    }
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
                const auto target=QDir(directory()).filePath(name);QDir().mkpath(QFileInfo(target).absolutePath());
                if(QFileInfo::exists(target) && !QFile::copy(target,target+QDateTime::currentDateTime().toString(".yyyyMMddHHmmsszzz.bak")))failure=QStringLiteral("保留原预设备份失败");
                if(failure.isEmpty()){QSaveFile file(target);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())failure=file.errorString();}
            }
            row->setText(2,failure.isEmpty()?QStringLiteral("已安装"):QStringLiteral("失败"));row->setToolTip(2,failure);if(!failure.isEmpty())status_->setText(failure);
            downloadNext();if(!active_ && queue_.isEmpty()){refresh_->setEnabled(true);status_->setText(QStringLiteral("下载完成；双击或使用导入按钮。"));}
        });
    }
}
void PresetCommunityDialog::importSelected(bool currentGroup){
    auto *row=files_->currentItem();if(!row || row->data(0,Qt::UserRole).toJsonObject().isEmpty())return;if(row->text(2)!=QStringLiteral("已安装")){enqueue(false);status_->setText(QStringLiteral("请在下载完成后导入。"));return;}
    const auto path=QDir(directory()).filePath(row->data(0,Qt::UserRole).toJsonObject().value("name").toString());
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
