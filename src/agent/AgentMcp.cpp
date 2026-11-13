#include "agent/AgentMcp.h"
#include "agent/AgentSession.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QCryptographicHash>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace vsr {
namespace {
bool fail(QString *error, const QString &message) { if (error) *error = message; return false; }
QJsonObject failed(const QString &message) { return {{"ok",false},{"error",message}}; }
QByteArray protect(const QByteArray &bytes, bool encrypt)
{
    if (bytes.isEmpty()) return {};
#ifdef Q_OS_WIN
    DATA_BLOB input{DWORD(bytes.size()),reinterpret_cast<BYTE*>(const_cast<char*>(bytes.constData()))},output{};
    const bool ok=encrypt?CryptProtectData(&input,L"VS Renderer MCP",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)
                         :CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output);
    if(!ok)return {};const QByteArray result(reinterpret_cast<char*>(output.pbData),output.cbData);LocalFree(output.pbData);return result;
#else
    Q_UNUSED(encrypt)
    return bytes;
#endif
}
QString alias(const QString &id,const QString &tool)
{
    const auto source=QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(),QCryptographicHash::Sha256).toHex().left(12));
    QString name=tool;name.replace(QRegularExpression("[^A-Za-z0-9_-]"),"_");
    const auto suffix=QString::fromLatin1(QCryptographicHash::hash(tool.toUtf8(),QCryptographicHash::Sha256).toHex().left(8));
    return ("mcp_"+source+"_"+suffix+"_"+name).left(64);
}
}

bool AgentMcpConfig::validate(QString *error) const
{
    if(id.isEmpty() || name.trimmed().isEmpty())return fail(error,QStringLiteral("MCP 来源须有 ID 和名称。"));
    if(transport=="stdio"){
        if(command.trimmed().isEmpty() || command.contains(QChar(0)))return fail(error,QStringLiteral("请填写 MCP 可执行文件。"));
        for(const auto &arg:arguments)if(arg.contains(QChar(0)))return fail(error,QStringLiteral("MCP 参数不能包含空字符。"));
        if(!workingDirectory.isEmpty() && !QFileInfo(workingDirectory).isDir())return fail(error,QStringLiteral("MCP 工作目录不存在。"));
    }else if(transport=="http"){
        const QUrl address(url);if(!address.isValid() || address.host().isEmpty() || (address.scheme()!="http" && address.scheme()!="https") || !address.userInfo().isEmpty() || address.hasFragment())return fail(error,QStringLiteral("MCP HTTP 地址须为不含用户名、密码或片段的 HTTP/HTTPS URL。"));
    }else return fail(error,QStringLiteral("MCP 传输仅支持 stdio 或 http。"));
    for(const auto &map:{environment,headers})for(auto it=map.begin();it!=map.end();++it)
        if(!it.value().isString() || it.key().contains(QChar(0)) || it.value().toString().contains(QChar(0)))return fail(error,QStringLiteral("环境变量和请求头须为文本键值。"));
    for(auto it=headers.begin();it!=headers.end();++it)if(it.key().contains('\r') || it.key().contains('\n') || it.value().toString().contains('\r') || it.value().toString().contains('\n'))return fail(error,QStringLiteral("MCP 请求头不能包含换行。"));
    return true;
}

QJsonObject AgentMcpConfig::toJson(bool secrets) const
{
    QJsonArray args;for(const auto &arg:arguments)args.append(arg);
    QJsonObject object{{"id",id},{"name",name},{"transport",transport},{"command",command},{"arguments",args},{"workingDirectory",workingDirectory},{"url",url},{"enabled",enabled}};
    if(secrets){object.insert("environment",environment);object.insert("headers",headers);}return object;
}

AgentMcpConfig AgentMcpConfig::fromJson(const QJsonObject &object,const QString &inputId)
{
    AgentMcpConfig config;config.id=object.value("id").toString(inputId);if(config.id.isEmpty())config.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.name=object.value("name").toString(inputId.isEmpty()?config.id:inputId);config.transport=object.value("transport").toString(object.contains("url")?"http":"stdio");
    if(config.transport=="streamable-http" || config.transport=="streamableHttp")config.transport="http";
    config.command=object.value("command").toString();config.workingDirectory=object.value("workingDirectory").toString(object.value("cwd").toString());config.url=object.value("url").toString();
    for(const auto &arg:object.value(object.contains("arguments")?"arguments":"args").toArray())config.arguments.append(arg.toString());
    config.environment=object.value(object.contains("environment")?"environment":"env").toObject();config.headers=object.value("headers").toObject();config.enabled=object.value("enabled").toBool(false);return config;
}

AgentMcpManager::AgentMcpManager(const QString &path,const QString &runtimeDirectory,QObject *parent)
    :QObject(parent),configPath_(path),runtime_(runtimeDirectory,this),bridge_(this)
{
    connect(&bridge_,&QProcess::started,this,[this]{
#ifdef Q_OS_WIN
        if(processJob_){CloseHandle(static_cast<HANDLE>(processJob_));processJob_=nullptr;}
        HANDLE job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        HANDLE process=OpenProcess(PROCESS_SET_QUOTA|PROCESS_TERMINATE,FALSE,DWORD(bridge_.processId()));
        const bool safe=job && process && SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)) && AssignProcessToJobObject(job,process);
        if(process)CloseHandle(process);
        if(!safe){if(job)CloseHandle(job);emit errorOccurred(QStringLiteral("无法为 MCP 建立进程清理边界，未启动任何 MCP 子程序。"));bridge_.kill();return;}
        processJob_=job;
#endif
        bridge_.write("{\"op\":\"host-ready\"}\n");
    });
    connect(&bridge_,&QProcess::readyReadStandardOutput,this,&AgentMcpManager::readBridge);
    connect(&bridge_,&QProcess::readyReadStandardError,this,[this]{bridge_.readAllStandardError();});
    const auto stopped=[this]{
#ifdef Q_OS_WIN
        if(processJob_){CloseHandle(static_cast<HANDLE>(processJob_));processJob_=nullptr;}
#endif
        const auto ids=pending_.keys();for(const auto id:ids)complete(id,failed(QStringLiteral("MCP 桥接进程已停止。")));
        for(auto it=states_.begin();it!=states_.end();++it){it.value()="disconnected";emit serverChanged(it.key());}tools_.clear();syncTools();
    };
    connect(&bridge_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[stopped](int,QProcess::ExitStatus){stopped();});
    connect(&bridge_,&QProcess::errorOccurred,this,[this,stopped](QProcess::ProcessError error){if(error==QProcess::FailedToStart){stopped();emit errorOccurred(QStringLiteral("MCP Node 桥接进程无法启动。"));}});
}

AgentMcpManager::~AgentMcpManager()
{
    bridge_.disconnect(this);bridge_.closeWriteChannel();const bool stopped=bridge_.waitForFinished(5000);
#ifdef Q_OS_WIN
    if(processJob_){CloseHandle(static_cast<HANDLE>(processJob_));processJob_=nullptr;}
#endif
    if(!stopped){bridge_.kill();bridge_.waitForFinished(1000);}
}

bool AgentMcpManager::load(QString *error)
{
    QFile file(configPath_);if(!file.exists())return true;
    if(!file.open(QIODevice::ReadOnly) || file.size()>4*1024*1024)return fail(error,QStringLiteral("无法读取 MCP 来源配置。"));
    QJsonParseError parse;const auto document=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError || !document.isObject() || document.object().value("schemaVersion").toInt()!=1 || !document.object().value("servers").isArray())return fail(error,QStringLiteral("MCP 来源配置格式不正确。"));
    QList<AgentMcpConfig> configs;
    for(const auto &value:document.object().value("servers").toArray()){
        auto object=value.toObject();const auto encrypted=object.value("secretsProtected").toString();
        if(!encrypted.isEmpty()){
            const auto plain=protect(QByteArray::fromBase64(encrypted.toLatin1()),false);if(plain.isEmpty())return fail(error,QStringLiteral("无法解密 MCP 来源的环境变量或请求头，请在当前 Windows 用户下重新填写。"));
            const auto secrets=QJsonDocument::fromJson(plain).object();object.insert("environment",secrets.value("environment"));object.insert("headers",secrets.value("headers"));
        }
        const auto config=AgentMcpConfig::fromJson(object);if(!config.validate(error))return false;configs.append(config);
    }
    return setServers(configs,error);
}

bool AgentMcpManager::save(QString *error) const
{
    QJsonArray servers;for(const auto &config:configs_){
        if(!config.validate(error))return false;auto object=config.toJson();
        if(!config.environment.isEmpty() || !config.headers.isEmpty()){
            const auto encrypted=protect(QJsonDocument(QJsonObject{{"environment",config.environment},{"headers",config.headers}}).toJson(QJsonDocument::Compact),true);
            if(encrypted.isEmpty())return fail(error,QStringLiteral("无法保护 MCP 凭证，旧配置保持不变。"));object.insert("secretsProtected",QString::fromLatin1(encrypted.toBase64()));
        }servers.append(object);
    }
    QDir().mkpath(QFileInfo(configPath_).absolutePath());QSaveFile file(configPath_);const auto bytes=QJsonDocument(QJsonObject{{"schemaVersion",1},{"servers",servers}}).toJson();
    return file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit()?true:fail(error,QStringLiteral("无法保存 MCP 来源配置。"));
}

bool AgentMcpManager::setServers(const QList<AgentMcpConfig> &servers,QString *error)
{
    QSet<QString> ids;for(const auto &config:servers){if(!config.validate(error))return false;if(ids.contains(config.id))return fail(error,QStringLiteral("MCP 来源 ID 重复。"));ids.insert(config.id);}
    for(const auto &old:configs_){bool changed=true;for(const auto &config:servers)if(config.id==old.id && config.toJson(true)==old.toJson(true)){changed=false;break;}if(changed)disconnectServer(old.id);}
    configs_=servers;emit toolsChanged();return true;
}

bool AgentMcpManager::importConfiguration(const QString &path,QString *error)
{
    QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()>4*1024*1024)return fail(error,QStringLiteral("无法读取 MCP 导入文件。"));
    QJsonParseError parse;const auto document=QJsonDocument::fromJson(file.readAll(),&parse);if(parse.error!=QJsonParseError::NoError || !document.isObject())return fail(error,QStringLiteral("MCP 导入文件须为 JSON 对象。"));
    QList<AgentMcpConfig> additions;const auto object=document.object();
    if(object.value("mcpServers").isObject()){
        const auto servers=object.value("mcpServers").toObject();for(auto it=servers.begin();it!=servers.end();++it)additions.append(AgentMcpConfig::fromJson(it.value().toObject(),it.key()));
    }else if(object.value("servers").isArray())for(const auto &server:object.value("servers").toArray())additions.append(AgentMcpConfig::fromJson(server.toObject()));
    else return fail(error,QStringLiteral("文件中没有 mcpServers 对象或 servers 数组。"));
    auto combined=configs_;for(auto config:additions){config.id=QUuid::createUuid().toString(QUuid::WithoutBraces);config.enabled=false;if(!config.validate(error))return false;combined.append(config);}
    if(!setServers(combined,error))return false;return save(error);
}

bool AgentMcpManager::exportConfiguration(const QString &path,QString *error) const
{
    QJsonArray servers;for(const auto &config:configs_){auto object=config.toJson();object.insert("enabled",false);servers.append(object);}
    const auto bytes=QJsonDocument(QJsonObject{{"schemaVersion",1},{"servers",servers}}).toJson();QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit()?true:fail(error,QStringLiteral("无法导出 MCP 来源配置。"));
}

void AgentMcpManager::ensureBridge()
{
    if(bridge_.state()!=QProcess::NotRunning)return;buffer_.clear();auto environment=QProcessEnvironment::systemEnvironment();environment.insert("VSR_AGENT_WAIT_FOR_HOST","1");bridge_.setProcessEnvironment(environment);bridge_.setWorkingDirectory(runtime_.directory());bridge_.start(runtime_.nodePath(),{runtime_.bridgePath()});
}

quint64 AgentMcpManager::request(const QJsonObject &body,Completion callback,QObject *owner)
{
    if(!runtime_.ready()){callback(failed(QStringLiteral("MCP npm 组件未安装，请打开 MCP 设置安装运行组件。")));return 0;}
    ensureBridge();const auto id=++nextId_;auto message=body;message.insert("id",qint64(id));pending_.insert(id,{std::move(callback),owner,owner,owner!=nullptr,body.value("server").toString()});
    if(bridge_.write(QJsonDocument(message).toJson(QJsonDocument::Compact)+'\n')<0){complete(id,failed(QStringLiteral("无法写入 MCP 桥接进程。")));return id;}
    QTimer::singleShot(135000,this,[this,id]{if(!pending_.contains(id))return;QJsonObject cancel{{"id",qint64(++nextId_)},{"op","cancel"},{"target",qint64(id)}};bridge_.write(QJsonDocument(cancel).toJson(QJsonDocument::Compact)+'\n');complete(id,failed(QStringLiteral("MCP 请求超时。")));});return id;
}

void AgentMcpManager::readBridge()
{
    buffer_+=bridge_.readAllStandardOutput();if(buffer_.size()>16*1024*1024){bridge_.kill();return;}
    while(true){const int end=buffer_.indexOf('\n');if(end<0)break;const auto line=buffer_.left(end);buffer_.remove(0,end+1);const auto object=QJsonDocument::fromJson(line).object();
        if(object.value("event").toString()=="disconnected"){const auto id=object.value("server").toString();states_[id]="disconnected";tools_.remove(id);syncTools();emit serverChanged(id);}
        else if(object.contains("id"))complete(quint64(object.value("id").toInteger()),object.value("result").toObject());
    }
}

void AgentMcpManager::complete(quint64 id,const QJsonObject &result)
{
    const auto it=pending_.find(id);if(it==pending_.end())return;const auto pending=it.value();pending_.erase(it);if(!pending.owned || pending.owner)pending.callback(result);
}

void AgentMcpManager::startEnabled(){for(const auto &config:configs_)if(config.enabled)connectServer(config.id);}

void AgentMcpManager::connectServer(const QString &id)
{
    if(state(id)=="connecting" || state(id)=="connected")return;
    for(const auto &config:configs_)if(config.id==id){
        const auto generation=++serverGenerations_[id];states_[id]="connecting";emit serverChanged(id);
        request({{"op","connect"},{"server",id},{"config",config.toJson(true)}},[this,id,generation](const QJsonObject &result){
            if(serverGenerations_.value(id)!=generation)return;
            const bool ok=result.value("ok").toBool();states_[id]=ok?"connected":"error";if(ok)tools_[id]=result.value("tools").toArray();else{tools_.remove(id);emit errorOccurred(result.value("error").toString());}syncTools();emit serverChanged(id);
        });return;
    }emit errorOccurred(QStringLiteral("MCP 来源不存在。"));
}

void AgentMcpManager::disconnectServer(const QString &id)
{
    ++serverGenerations_[id];
    const auto requests=pending_.keys();for(const auto requestId:requests){if(pending_.value(requestId).server!=id)continue;
        if(bridge_.state()!=QProcess::NotRunning){const QJsonObject cancel{{"id",qint64(++nextId_)},{"op","cancel"},{"target",qint64(requestId)}};bridge_.write(QJsonDocument(cancel).toJson(QJsonDocument::Compact)+'\n');}
        complete(requestId,failed(QStringLiteral("MCP 来源已断开。")));
    }
    states_[id]="disconnected";tools_.remove(id);syncTools();emit serverChanged(id);
    if(bridge_.state()!=QProcess::NotRunning)request({{"op","disconnect"},{"server",id}},[](const QJsonObject &){});
}

void AgentMcpManager::refreshTools(const QString &id)
{
    if(state(id)!="connected")return;request({{"op","tools"},{"server",id}},[this,id](const QJsonObject &result){if(result.value("ok").toBool()){tools_[id]=result.value("tools").toArray();syncTools();emit serverChanged(id);}else emit errorOccurred(result.value("error").toString());});
}

QString AgentMcpManager::state(const QString &id) const{return states_.value(id,"disconnected");}
QJsonArray AgentMcpManager::serverTools(const QString &id) const{return tools_.value(id);}

void AgentMcpManager::callTool(const QString &id,const QString &name,const QJsonObject &arguments,Completion callback,QObject *owner)
{
    if(state(id)!="connected"){callback(failed(QStringLiteral("MCP 来源未连接。")));return;}
    request({{"op","call"},{"server",id},{"name",name},{"arguments",arguments}},std::move(callback),owner);
}

void AgentMcpManager::registerTools(AgentSession *session)
{
    if(!session)return;for(const auto &known:sessions_)if(known==session)return;sessions_.append(session);
    connect(session,&AgentSession::toolExecutionCancelled,this,[this,session]{cancelOwner(session);});
    connect(session,&QObject::destroyed,this,[this,session]{registered_.remove(session);toolPreferences_.remove(session);cancelOwner(session);});syncTools();
}

void AgentMcpManager::syncTools()
{
    for(auto it=sessions_.begin();it!=sessions_.end();){if(!*it){it=sessions_.erase(it);continue;}auto *session=it->data();++it;
        QMap<QString,bool> existing;for(const auto &value:session->allTools()){const auto name=value.toObject().value("function").toObject().value("name").toString();existing[name]=session->toolEnabled(name);}
        for(const auto &name:registered_.value(session)){toolPreferences_[session][name]=session->toolEnabled(name);session->unregisterTool(name);}QStringList names;
        for(const auto &config:configs_){if(state(config.id)!="connected")continue;for(const auto &value:tools_.value(config.id)){
            const auto tool=value.toObject();const auto original=tool.value("name").toString();if(original.isEmpty())continue;const auto name=alias(config.id,original);if(names.contains(name))continue;
            names.append(name);const QPointer<AgentMcpManager> guard(this);const QPointer<AgentSession> owner(session);
            session->registerAsyncTool(name,QStringLiteral("MCP [%1] %2").arg(config.name,tool.value("description").toString()),tool.value("inputSchema").toObject(),[guard,owner,id=config.id,original](const QJsonObject &arguments,Completion callback){if(!guard || !owner){callback(failed(QStringLiteral("MCP 连接已关闭。")));return;}guard->callTool(id,original,arguments,std::move(callback),owner);});
            session->setToolEnvironmentAccess(name,true,true);
            if(existing.contains(name))session->setToolEnabled(name,existing.value(name));else if(toolPreferences_[session].contains(name))session->setToolEnabled(name,toolPreferences_[session].value(name));
        }}registered_[session]=names;
    }emit toolsChanged();
}

void AgentMcpManager::cancelOwner(QObject *owner)
{
    const auto ids=pending_.keys();for(const auto id:ids){const auto pending=pending_.value(id);if(!pending.owned || pending.ownerIdentity!=owner)continue;
        if(bridge_.state()==QProcess::Running){const QJsonObject cancel{{"id",qint64(++nextId_)},{"op","cancel"},{"target",qint64(id)}};bridge_.write(QJsonDocument(cancel).toJson(QJsonDocument::Compact)+'\n');}
        complete(id,failed(QStringLiteral("MCP 请求已取消。")));
    }
}

}
