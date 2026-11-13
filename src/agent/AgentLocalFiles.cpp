#include "agent/AgentLocalFiles.h"
#include "agent/AgentSession.h"
#include <QDateTime>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStringConverter>
#include <QtConcurrentRun>
#include <functional>

namespace vsr {
namespace {
constexpr qint64 textLimit=32*1024*1024;
constexpr int outputLimit=64*1024;
using Cancel=std::shared_ptr<std::atomic_bool>;
QJsonObject failure(const QString &message) { return {{"ok",false},{"error",message}}; }
QJsonObject cancelled() { return {{"ok",false},{"cancelled",true},{"error","Operation cancelled; completed filesystem actions are not undone."}}; }
QString resolve(const QString &path,const QString &cwd)
{
    return QDir::cleanPath(QFileInfo(QDir(cwd).filePath(QDir::fromNativeSeparators(path))).absoluteFilePath());
}
QString pathKey(const QString &path)
{
    QString identity=QDir::cleanPath(path);
    // Qt canonicalFilePath does not resolve Windows directory junctions.
    for(int links=0;links<32;++links){QString ancestor=identity,suffix;bool changed=false;
        while(true){const QFileInfo info(ancestor);const auto target=info.isJunction()?info.junctionTarget():info.isSymbolicLink()?info.symLinkTarget():QString();
            if(!target.isEmpty()){identity=QDir::cleanPath(target+suffix);changed=true;break;}
            const auto parent=info.absolutePath();if(parent==ancestor)break;suffix='/'+info.fileName()+suffix;ancestor=parent;
        }
        if(!changed)break;
    }
#ifdef Q_OS_WIN
    return identity.toCaseFolded();
#else
    return identity;
#endif
}
QJsonObject metadata(const QFileInfo &info)
{
    return {{"path",info.absoluteFilePath()},{"name",info.fileName()},{"exists",info.exists() || info.isSymLink()},
        {"is_file",info.isFile()},{"is_directory",info.isDir()},{"is_symlink",info.isSymLink()},
        {"size",double(info.size())},{"last_modified",info.lastModified().toUTC().toString(Qt::ISODateWithMs)},
        {"readable",info.isReadable()},{"writable",info.isWritable()}};
}
struct Text {
    QByteArray bytes,bom;QString value;
    QStringConverter::Encoding encoding=QStringConverter::Utf8;
    QByteArray encode(const QString &text) const { QStringEncoder encoder(encoding);return bom+encoder(text); }
};
bool decode(const QByteArray &bytes,Text &text,QString &error)
{
    text.bytes=bytes;QByteArray body=bytes;
    if(body.startsWith("\xFF\xFE")){text.encoding=QStringConverter::Utf16LE;text.bom=body.left(2);body.remove(0,2);}
    else if(body.startsWith("\xFE\xFF")){text.encoding=QStringConverter::Utf16BE;text.bom=body.left(2);body.remove(0,2);}
    else if(body.startsWith("\xEF\xBB\xBF")){text.bom=body.left(3);body.remove(0,3);}
    QStringDecoder decoder(text.encoding,QStringConverter::Flag::Stateless);text.value=decoder(body);
    if(decoder.hasError() || text.value.contains(QChar(0))){error="File is not valid UTF-8 or BOM-marked UTF-16 text.";return false;}
    return true;
}
bool load(const QString &path,QByteArray &bytes,QString &error)
{
    QFile file(path);if(!QFileInfo(path).isFile() || !file.open(QIODevice::ReadOnly)){error="Cannot read file: "+path+": "+file.errorString();return false;}
    if(file.size()>textLimit){error="Mutation/UTF-16 input exceeds 32 MiB: "+path;return false;}
    bytes=file.readAll();if(file.error()!=QFileDevice::NoError){error=file.errorString();return false;}return true;
}
bool save(const QString &path,const QByteArray &bytes,QString &error)
{
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()){error="Cannot write "+path+": "+file.errorString();return false;}return true;
}

// UTF-8 pages stream. Very long lines are consumed once and explicitly truncated.
QJsonObject readFile(const QString &path,const QJsonObject &args,const Cancel &stop)
{
    QFile file(path);if(!QFileInfo(path).isFile() || !file.open(QIODevice::ReadOnly))return failure("Cannot open regular file: "+path);
    const int offset=args.value("offset").toInt(1),limit=args.value("limit").toInt(200);int line=0,bytes=0;bool clipped=false,more=false;
    QJsonArray lines;QString numbered;QStringList utf16;const auto prefix=file.peek(3);
    if(prefix.startsWith("\xFF\xFE") || prefix.startsWith("\xFE\xFF")){
        if(file.size()>textLimit)return failure("UTF-16 reads above 32 MiB are unsupported.");Text text;QString error;if(!decode(file.readAll(),text,error))return failure(error);utf16=text.value.split('\n');if(text.value.isEmpty())utf16.clear();else if(text.value.endsWith('\n'))utf16.removeLast();
    }
    const bool wide=!utf16.isEmpty() || prefix.startsWith("\xFF\xFE") || prefix.startsWith("\xFE\xFF");
    while(wide?line<utf16.size():!file.atEnd()){
        if(stop->load())return cancelled();QString value;bool longLine=false;
        if(wide)value=utf16[line];else{
            auto raw=file.readLine(1024*1024);if(file.error()!=QFileDevice::NoError)return failure(file.errorString());longLine=!raw.endsWith('\n') && !file.atEnd();
            while(longLine && !raw.endsWith('\n') && !file.atEnd()){if(stop->load())return cancelled();const auto rest=file.readLine(1024*1024);if(file.error()!=QFileDevice::NoError)return failure(file.errorString());if(rest.endsWith('\n') || file.atEnd())break;}
            if(line==0 && raw.startsWith("\xEF\xBB\xBF"))raw.remove(0,3);QStringDecoder decoder(QStringConverter::Utf8,longLine?QStringConverter::Flags{}:QStringConverter::Flag::Stateless);value=decoder(raw);if(decoder.hasError() || value.contains(QChar(0)))return failure("File is not valid UTF-8 text: "+path);
        }
        ++line;if(value.endsWith('\n'))value.chop(1);if(value.endsWith('\r'))value.chop(1);if(line<offset)continue;
        longLine|=value.size()>8000;value=value.left(8000);const int size=value.toUtf8().size();
        if(bytes+size>outputLimit && !lines.isEmpty()){more=true;break;}bytes+=size;clipped|=longLine;lines.append(QJsonObject{{"line",line},{"text",value},{"truncated",longLine}});numbered+=QString::number(line)+": "+value+"\n";
        if(lines.size()>=limit){more=wide?line<utf16.size():!file.atEnd();break;}
    }
    QJsonObject result{{"ok",true},{"path",path},{"offset",offset},{"lines",lines},{"content",numbered},{"has_more",more},{"truncated",more || clipped}};
    if(more)result.insert("next_offset",offset+lines.size());else result.insert("total_lines",line);return result;
}

struct Scan {
    Cancel stop;QElapsedTimer timer;int visited=0;bool truncated=false;QString reason;
    explicit Scan(Cancel flag):stop(std::move(flag)){timer.start();}
    bool proceed(){if(stop->load())return false;if(++visited>100000 || timer.elapsed()>5000){truncated=true;reason="Search budget reached; narrow path/pattern or use pagination.";return false;}return true;}
};
void walk(const QString &root,bool recursive,Scan &scan,const std::function<bool(const QFileInfo&)> &visit)
{
    if(QFileInfo(root).isFile()){if(scan.proceed())visit(QFileInfo(root));return;}
    QStringList directories{root};
    while(!directories.isEmpty()){
        QDirIterator entries(directories.takeLast(),QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);
        while(entries.hasNext()){
            if(!scan.proceed())return;entries.next();const auto info=entries.fileInfo();
            if(info.isDir() && (info.fileName()==".git" || info.fileName()==".hg" || info.fileName()==".svn"))continue;
            if(!visit(info))return;if(recursive && info.isDir() && !info.isSymLink())directories.append(info.absoluteFilePath());
        }
    }
}
QRegularExpression globPattern(const QString &pattern)
{
    const auto value=QDir::fromNativeSeparators(pattern);QString regex="^";
    for(int i=0;i<value.size();++i){const auto c=value[i];
        if(c=='*' && i+1<value.size() && value[i+1]=='*'){++i;if(i+1<value.size() && value[i+1]=='/'){++i;regex+="(?:.*/)?";}else regex+=".*";}
        else if(c=='*')regex+="[^/]*";else if(c=='?')regex+="[^/]";else regex+=QRegularExpression::escape(QString(c));
    }
    return QRegularExpression(regex+"$");
}
QJsonObject discover(const QString &name,const QString &path,const QJsonObject &args,const Cancel &stop)
{
    const QFileInfo root(path);if(!root.exists() || (!root.isFile() && !root.isDir()))return failure("Search path does not exist: "+path);if(name=="list_directory" && !root.isDir())return failure("list_directory requires a directory.");
    const int limit=args.value("limit").toInt(name=="list_directory"?500:name=="glob"?500:250),offset=args.value("offset").toInt();int skipped=0,matched=0,bytes=0;QJsonArray found;
    const auto pattern=args.value("pattern").toString(),include=args.value("include").toString();const auto glob=globPattern(name=="glob"?pattern:include);
    auto regex=QRegularExpression(args.value("literal").toBool()?QRegularExpression::escape(pattern):pattern,args.value("case_sensitive").toBool(true)?QRegularExpression::NoPatternOption:QRegularExpression::CaseInsensitiveOption);
    if(name!="list_directory" && name!="glob" && !regex.isValid())return failure("Invalid regular expression: "+regex.errorString());
    Scan scan(stop);const auto keep=[&](QJsonObject item){if(matched++<offset)return true;const int size=QJsonDocument(item).toJson(QJsonDocument::Compact).size();if(found.size()>=limit || bytes+size>outputLimit){scan.truncated=true;scan.reason="Result limit reached; use next_offset.";return false;}bytes+=size;found.append(item);return true;};
    walk(path,name!="list_directory" || args.value("recursive").toBool(),scan,[&](const QFileInfo &info){
        if(name=="list_directory")return keep(metadata(info));if(!info.isFile())return true;const auto relative=QDir(root.isDir()?path:root.absolutePath()).relativeFilePath(info.absoluteFilePath());
        if(name=="glob"){if(glob.match(pattern.contains('/') || pattern.contains('\\')?relative:info.fileName()).hasMatch())return keep(QJsonObject{{"path",info.absoluteFilePath()},{"relative_path",relative}});return true;}
        if(!include.isEmpty() && !glob.match(include.contains('/') || include.contains('\\')?relative:info.fileName()).hasMatch())return true;
        if(info.size()>textLimit){++skipped;return true;}QByteArray raw;QString error;Text text;if(!load(info.absoluteFilePath(),raw,error) || !decode(raw,text,error)){++skipped;return true;}
        const auto lines=text.value.split('\n');for(int i=0;i<lines.size();++i){if(!scan.proceed())return false;if(regex.match(lines[i]).hasMatch()){auto line=lines[i];if(line.endsWith('\r'))line.chop(1);if(!keep(QJsonObject{{"path",info.absoluteFilePath()},{"relative_path",relative},{"line",i+1},{"text",line.left(8000)},{"truncated",line.size()>8000}}))return false;}}return true;
    });
    if(stop->load())return cancelled();QJsonObject result{{"ok",true},{"path",path},{"entries",found},{"truncated",scan.truncated},{"scanned_units",scan.visited},{"skipped_files",skipped}};
    if(scan.truncated){result.insert("reason",scan.reason);result.insert("next_offset",offset+found.size());}return result;
}

struct Hunk { QString context;QStringList lines;bool eof=false; };
struct Patch { QString kind,path,destination;QList<Hunk> hunks;QString added; };
bool parsePatch(const QString &input,QList<Patch> &operations,QString &error)
{
    auto lines=QString(input).replace("\r\n","\n").split('\n');while(!lines.isEmpty() && lines.first().trimmed().isEmpty())lines.removeFirst();while(!lines.isEmpty() && lines.last().trimmed().isEmpty())lines.removeLast();
    if(lines.size()<2 || lines.first()!="*** Begin Patch" || lines.last()!="*** End Patch"){error="Expected *** Begin Patch / *** End Patch markers.";return false;}
    for(int i=1;i<lines.size()-1;){
        const auto header=lines[i++];Patch op;
        if(header.startsWith("*** Add File: ")){op.kind="add";op.path=header.mid(14);}
        else if(header.startsWith("*** Delete File: ")){op.kind="delete";op.path=header.mid(17);}
        else if(header.startsWith("*** Update File: ")){op.kind="update";op.path=header.mid(17);}
        else {error="Invalid patch operation: "+header;return false;}
        if(op.path.trimmed().isEmpty()){error="Patch path is empty.";return false;}
        if(op.kind=="update" && i<lines.size()-1 && lines[i].startsWith("*** Move to: ")){op.destination=lines[i++].mid(13);if(op.destination.trimmed().isEmpty()){error="Move destination is empty.";return false;}}
        while(i<lines.size()-1 && !lines[i].startsWith("*** Add File: ") && !lines[i].startsWith("*** Update File: ") && !lines[i].startsWith("*** Delete File: ")){
            const auto line=lines[i++];
            if(op.kind=="add"){if(!line.startsWith('+')){error="Added lines must start with +.";return false;}op.added+=line.mid(1)+'\n';continue;}
            if(op.kind=="delete"){error="Delete File cannot contain a body.";return false;}
            if(line=="@@" || line.startsWith("@@ ")){if(!op.hunks.isEmpty() && op.hunks.last().lines.isEmpty()){error="Empty update hunk.";return false;}op.hunks.append(Hunk{line.mid(3),{},false});continue;}
            if(line=="*** End of File"){if(op.hunks.isEmpty()){error="End of File needs a hunk.";return false;}op.hunks.last().eof=true;continue;}
            if(op.hunks.isEmpty())op.hunks.append(Hunk{});if(op.hunks.last().eof || (!line.isEmpty() && !QString(" +-").contains(line[0]))){error="Invalid update hunk line: "+line;return false;}op.hunks.last().lines.append(line.isEmpty()?QString(" "):line);
        }
        if(op.kind=="update" && ((op.hunks.isEmpty() && op.destination.isEmpty()) || (!op.hunks.isEmpty() && op.hunks.last().lines.isEmpty()))){error="Update File needs a nonempty hunk.";return false;}
        operations.append(op);if(operations.size()>128){error="Patch exceeds 128 operations.";return false;}
    }
    if(operations.isEmpty()){error="Patch contains no operations.";return false;}return true;
}
QString plainLine(QString value) { if(value.endsWith('\r'))value.chop(1);return value; }
bool updateText(const Text &text,const QList<Hunk> &hunks,QString &output,QString &error)
{
    auto lines=text.value.split('\n');const bool newline=text.value.endsWith('\n');if(text.value.isEmpty())lines.clear();else if(newline)lines.removeLast();int cursor=0;const QString cr=text.value.contains("\r\n")?"\r":"";
    for(const auto &hunk:hunks){
        if(!hunk.context.isEmpty()){bool found=false;for(int i=cursor;i<lines.size();++i)if(plainLine(lines[i])==hunk.context){cursor=i+1;found=true;break;}if(!found){error="Patch @@ context was not found: "+hunk.context;return false;}}
        QStringList old;for(const auto &line:hunk.lines)if(!line.startsWith('+'))old.append(line.mid(1));
        int match=-1,count=0;
        if(old.isEmpty()){match=hunk.eof?lines.size():hunk.context.isEmpty()?lines.size():cursor;count=1;}
        else for(int i=cursor;i+old.size()<=lines.size();++i){if(hunk.eof && i+old.size()!=lines.size())continue;bool equal=true;for(int j=0;equal && j<old.size();++j)equal=plainLine(lines[i+j])==old[j];if(equal){match=i;++count;}}
        if(count!=1){error=count?"Patch hunk is ambiguous; add more context.":"Patch hunk did not match the file exactly.";return false;}
        QStringList replacement;int consumed=0;for(const auto &line:hunk.lines){if(line.startsWith('+'))replacement.append(line.mid(1)+cr);else if(line.startsWith('-'))++consumed;else replacement.append(lines[match+consumed++]);}
        for(int i=0;i<old.size();++i)lines.removeAt(match);for(int i=0;i<replacement.size();++i)lines.insert(match+i,replacement[i]);cursor=match+replacement.size();
    }
    output=lines.join('\n');if(!lines.isEmpty() && (newline || text.value.isEmpty()))output+='\n';return true;
}
struct Change { QString path;bool beforeExists=false,afterExists=false;QByteArray before,after;QFileDevice::Permissions permissions{}; };
bool prepareParent(const QString &path,QStringList &created,QString &error)
{
    const auto parent=QFileInfo(path).absolutePath();QString next=parent;QStringList missing;
    while(!QFileInfo(next).exists()){missing.prepend(next);const auto up=QFileInfo(next).absolutePath();if(up==next)break;next=up;}
    if(!QFileInfo(next).isDir()){error="Parent path is not a directory: "+next;return false;}
    for(const auto &dir:missing){if(!QDir().mkdir(dir)){error="Cannot create directory: "+dir;return false;}created.append(dir);}return true;
}
void cleanupDirectories(const QStringList &created) { for(auto it=created.crbegin();it!=created.crend();++it)QDir().rmdir(*it); }
QJsonObject applyPatch(const QString &input,const QString &cwd,const Cancel &stop)
{
    if(input.toUtf8().size()>4*1024*1024)return failure("Patch exceeds 4 MiB.");QList<Patch> operations;QString error;if(!parsePatch(input,operations,error))return failure(error);
    QMap<QString,Change> changes;QSet<QString> used;QJsonArray affected;
    const auto admit=[&](const QString &path){const auto key=pathKey(path);for(const auto &other:used)if(key==other || key.startsWith(other+'/') || other.startsWith(key+'/')){error="Patch targets resolve to the same file or a file/parent conflict: "+path;return false;}used.insert(key);const QFileInfo info(path);if(info.isSymLink() || (info.exists() && !info.isFile())){error="Patch targets must be regular files, not directories/symlinks: "+path;return false;}return true;};
    for(const auto &op:operations){
        if(stop->load())return cancelled();const auto path=resolve(op.path,cwd),destination=op.destination.isEmpty()?path:resolve(op.destination,cwd);if(!op.destination.isEmpty() && pathKey(path)==pathKey(destination))return failure("Move source and destination resolve to the same file.");if(!admit(path) || (pathKey(path)!=pathKey(destination) && !admit(destination)))return failure(error);
        Change change;change.path=path;change.beforeExists=QFileInfo::exists(path);change.permissions=QFileInfo(path).permissions();
        if(op.kind=="add"){if(change.beforeExists)return failure("Add File destination already exists: "+path);change.afterExists=true;change.after=op.added.toUtf8();}
        else {if(!load(path,change.before,error))return failure(error);if(op.kind=="update"){Text text;if(!decode(change.before,text,error))return failure(error);QString modified;if(!updateText(text,op.hunks,modified,error))return failure(path+": "+error);change.afterExists=true;change.after=text.encode(modified);}}
        if(pathKey(destination)!=pathKey(path)){if(QFileInfo::exists(destination))return failure("Move destination already exists: "+destination);Change moved;moved.path=destination;moved.afterExists=true;moved.after=change.after;moved.permissions=change.permissions;changes.insert(destination,moved);change.afterExists=false;affected.append(QJsonObject{{"kind","move"},{"path",path},{"destination",destination}});}
        else affected.append(QJsonObject{{"kind",op.kind},{"path",path}});changes.insert(path,change);
    }
    // Stage all new contents before replacing anything. Admission failure cannot partially write files.
    QStringList created;QList<std::shared_ptr<QSaveFile>> staged;QList<QString> stagedPaths;
    for(auto it=changes.cbegin();it!=changes.cend();++it){if(stop->load()){staged.clear();cleanupDirectories(created);return cancelled();}if(!it->afterExists)continue;
        if(!prepareParent(it->path,created,error)){staged.clear();cleanupDirectories(created);return failure(error);}auto file=std::make_shared<QSaveFile>(it->path);if(!file->open(QIODevice::WriteOnly) || file->write(it->after)!=it->after.size()){error=file->errorString();file.reset();staged.clear();cleanupDirectories(created);return failure(error);}if(it->permissions)file->setPermissions(it->permissions);staged.append(file);stagedPaths.append(it->path);
    }
    for(const auto &change:changes){QByteArray current;if(change.beforeExists){if(!load(change.path,current,error) || current!=change.before){staged.clear();cleanupDirectories(created);return failure("File changed during patch preflight: "+change.path);}}else if(QFileInfo::exists(change.path)){staged.clear();cleanupDirectories(created);return failure("Destination appeared during patch preflight: "+change.path);}}
    if(stop->load()){staged.clear();cleanupDirectories(created);return cancelled();}
    QStringList completed;
    for(int i=0;i<staged.size();++i){if(!staged[i]->commit()){error="Patch commit failed: "+staged[i]->errorString();break;}completed.append(stagedPaths[i]);}
    if(error.isEmpty())for(const auto &change:changes)if(!change.afterExists){if(!QFile::remove(change.path)){error="Cannot delete patch source: "+change.path;break;}completed.append(change.path);}
    if(!error.isEmpty()){
        staged.clear();QJsonArray rollbackErrors;for(auto it=completed.crbegin();it!=completed.crend();++it){const auto change=changes.value(*it);QString reason;bool restored=change.beforeExists?save(change.path,change.before,reason):QFile::remove(change.path);if(restored && change.beforeExists)QFile::setPermissions(change.path,change.permissions);if(!restored)rollbackErrors.append(change.path+": "+reason);}cleanupDirectories(created);
        return {{"ok",false},{"error",error},{"rolled_back",rollbackErrors.isEmpty()},{"partial",!rollbackErrors.isEmpty()},{"rollback_errors",rollbackErrors}};
    }
    return {{"ok",true},{"operations",affected},{"atomic_preflight",true},{"cancelled_after_commit",stop->load()}};
}

QJsonObject perform(const QString &name,const QJsonObject &args,const QString &directory,const Cancel &stop)
{
    if(stop->load())return cancelled();const auto cwd=resolve(args.value("cwd").toString("."),directory);if(!QFileInfo(cwd).isDir())return failure("cwd is not an existing directory: "+cwd);
    const auto raw=args.value("path").toString(".");if(raw.trimmed().isEmpty())return failure("path must be non-empty.");const auto path=resolve(raw,cwd);
    if(name=="read_file")return readFile(path,args,stop);
    if(name=="list_directory" || name=="glob" || name=="grep" || name=="search_files")return discover(name,path,args,stop);
    if(name=="file_info"){auto result=metadata(QFileInfo(path));result.insert("ok",true);return result;}
    if(name=="apply_patch")return applyPatch(args.value("patch").toString(),cwd,stop);
    if(name=="create_directory")return QDir().mkpath(path)?QJsonObject{{"ok",true},{"path",path}}:failure("Cannot create directory: "+path);
    if(name=="delete_file"){if(!QFileInfo(path).isFile() && !QFileInfo(path).isSymLink())return failure("delete_file accepts a file or symlink, not a directory: "+path);return QFile::remove(path)?QJsonObject{{"ok",true},{"path",path}}:failure("Cannot delete file: "+path);}
    QString error;
    if(name=="copy_file" || name=="move_file"){
        const auto source=resolve(args.value("source").toString(),cwd),destination=resolve(args.value("destination").toString(),cwd);if(!QFileInfo(source).isFile())return failure("Source is not a regular file: "+source);if(pathKey(source)==pathKey(destination))return failure("Source and destination must differ.");
        if((QFileInfo::exists(destination) || QFileInfo(destination).isSymLink()) && !args.value("overwrite").toBool())return failure("Destination exists; set overwrite=true to replace it.");if(QFileInfo(destination).isDir())return failure("Destination is a directory.");
        if(name=="move_file" && !QFileInfo::exists(destination))return QFile::rename(source,destination)?QJsonObject{{"ok",true},{"source",source},{"destination",destination}}:failure("Move failed; check destination directory and permissions.");
        QFile input(source);if(!input.open(QIODevice::ReadOnly))return failure(input.errorString());QSaveFile output(destination);if(!output.open(QIODevice::WriteOnly))return failure(output.errorString());while(!input.atEnd()){if(stop->load())return cancelled();const auto chunk=input.read(1024*1024);if(input.error()!=QFileDevice::NoError || output.write(chunk)!=chunk.size())return failure("File copy failed.");}output.setPermissions(input.permissions());if(!output.commit())return failure(output.errorString());
        if(name=="move_file" && !QFile::remove(source))return QJsonObject{{"ok",false},{"partial",true},{"destination_written",true},{"error","Destination was written but the move source could not be deleted."}};return {{"ok",true},{"source",source},{"destination",destination}};
    }
    if(name=="write_file"){
        if((QFileInfo::exists(path) || QFileInfo(path).isSymLink()) && !args.value("overwrite").toBool())return failure("File exists; set overwrite=true to replace it.");if(QFileInfo(path).isDir())return failure("Destination is a directory.");const auto bytes=args.value("content").toString().toUtf8();if(bytes.size()>textLimit)return failure("Write exceeds 32 MiB.");QStringList created;if(!prepareParent(path,created,error)){cleanupDirectories(created);return failure(error);}if(stop->load()){cleanupDirectories(created);return cancelled();}if(!save(path,bytes,error)){cleanupDirectories(created);return failure(error);}return {{"ok",true},{"path",path},{"bytes_written",bytes.size()}};
    }
    if(name=="edit_file"){
        QByteArray bytes;Text text;if(!load(path,bytes,error) || !decode(bytes,text,error))return failure(error);auto old=args.value("old_string").toString(),replacement=args.value("new_string").toString();if(old.isEmpty() || old==replacement)return failure("old_string must be non-empty and differ from new_string.");
        if(text.value.contains("\r\n")){old.replace("\r\n","\n");old.replace("\n","\r\n");replacement.replace("\r\n","\n");replacement.replace("\n","\r\n");}
        int count=0,from=0;while((from=text.value.indexOf(old,from))>=0){++count;from+=old.size();}if(!count)return failure("old_string was not found exactly.");if(!args.value("replace_all").toBool() && count!=1)return failure("old_string matches more than once; use replace_all or a more specific match.");if(args.contains("expected_matches") && count!=args.value("expected_matches").toInt())return failure("Match count differs from expected_matches.");
        auto modified=text.value;modified.replace(old,replacement);if(stop->load())return cancelled();if(!save(path,text.encode(modified),error))return failure(error);return {{"ok",true},{"path",path},{"replacements",count}};
    }
    return failure("Unknown filesystem tool.");
}
QJsonObject field(const QString &type,const QString &description={}) { QJsonObject result{{"type",type}};if(!description.isEmpty())result.insert("description",description);return result; }
QJsonObject integer(int minimum,int maximum) { return {{"type","integer"},{"minimum",minimum},{"maximum",maximum}}; }
QJsonObject schema(QJsonObject properties,const QJsonArray &required={})
{
    properties.insert("cwd",field("string","Optional operation working directory, resolved from this Session's working directory. Does not change permissions."));return {{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",false}};
}
}

AgentLocalFiles::AgentLocalFiles(const QString &workingDirectory,QObject *parent):QObject(parent),directory_(QDir::currentPath())
{
    if(!workingDirectory.isEmpty())setWorkingDirectory(workingDirectory);
}
AgentLocalFiles::~AgentLocalFiles() { cancelJobs(); }
void AgentLocalFiles::cancelJobs() { for(const auto &weak:jobs_)if(const auto job=weak.lock())job->store(true); }
QString AgentLocalFiles::workingDirectory() const { return session_?session_->workingDirectory():directory_; }
bool AgentLocalFiles::setWorkingDirectory(const QString &path,QString *error)
{
    const auto directory=resolve(path,workingDirectory());if(!QFileInfo(directory).isDir()){if(error)*error="Working directory does not exist: "+directory;return false;}if(session_ && !session_->setWorkingDirectory(directory)){if(error)*error="Session rejected working directory.";return false;}directory_=directory;return true;
}
void AgentLocalFiles::registerTools(AgentSession *session)
{
    if(!session || session_)return;session_=session;connect(session,&AgentSession::toolExecutionCancelled,this,[this]{cancelJobs();});
    const auto add=[this,session](const QString &name,const QString &description,const QJsonObject &parameters,bool writes){
        const QPointer<AgentLocalFiles> self(this);session->registerAsyncTool(name,description,parameters,[self,name](const QJsonObject &arguments,AgentSession::Completion completion){
            if(!self){completion(failure("Filesystem module is unavailable."));return;}auto stop=std::make_shared<std::atomic_bool>(false);self->jobs_.removeIf([](const auto &weak){return weak.expired();});self->jobs_.append(stop);const auto cwd=self->workingDirectory();auto *watcher=new QFutureWatcher<QJsonObject>(self);
            connect(watcher,&QFutureWatcher<QJsonObject>::finished,self,[watcher,completion]{const auto result=watcher->result();watcher->deleteLater();completion(result);});watcher->setFuture(QtConcurrent::run([name,arguments,cwd,stop]{return perform(name,arguments,cwd,stop);}));
        });session->setToolEnvironmentAccess(name,true,writes);session->setToolDelegationAllowed(name,false);
    };
    const auto path=field("string","Absolute path or path relative to cwd. Full-environment mode permits local paths outside the software directory.");
    add("read_file","Read a UTF-8/BOM UTF-16 text file with 1-based offset/limit line pagination. Large UTF-8 files stream; output and long lines are explicitly truncated.",schema({{"path",path},{"offset",integer(1,1000000000)},{"limit",integer(1,2000)}},{"path"}),false);
    add("list_directory","List local directory entries and metadata; recursive traversal does not follow directory symlinks. Results are capped and paginated.",schema({{"path",path},{"offset",integer(0,1000000000)},{"limit",integer(1,2000)},{"recursive",field("boolean")}}),false);
    add("glob","Find local files by *, ?, ** glob pattern without requiring rg. Includes hidden files, skips VCS metadata and directory symlinks; use offset for further results.",schema({{"pattern",field("string")},{"path",path},{"offset",integer(0,1000000000)},{"limit",integer(1,2000)}},{"pattern"}),false);
    const auto search=schema({{"pattern",field("string")},{"path",path},{"include",field("string","Optional single positive file glob.")},{"literal",field("boolean")},{"case_sensitive",field("boolean")},{"offset",integer(0,1000000000)},{"limit",integer(1,1000)}},{"pattern"});
    for(const auto &name:{"grep","search_files"})add(name,"Search local text files using a Qt regular expression, or literal=true. Returns 1-based line numbers; reports skipped binary/large files and budget/output truncation.",search,false);
    add("file_info","Read existence, size, type, timestamps and access metadata for a local path.",schema({{"path",path}},{"path"}),false);
    add("write_file","Create a UTF-8 file; overwrite=true explicitly permits replacement. Writes atomically per file and creates missing parent directories.",schema({{"path",path},{"content",field("string")},{"overwrite",field("boolean")}},{"path","content"}),true);
    add("edit_file","Apply an exact literal text replacement. Match must be unique unless replace_all=true; expected_matches additionally checks the count. Preserves BOM/encoding and CRLF style.",schema({{"path",path},{"old_string",field("string")},{"new_string",field("string")},{"replace_all",field("boolean")},{"expected_matches",integer(1,1000000)}},{"path","old_string","new_string"}),true);
    add("apply_patch","Apply Codex *** Begin Patch Add/Update/Delete/Move syntax. All paths/hunks are validated and new content staged before commit; commit failures attempt rollback and explicitly report partial effects. Patch targets must be regular files, not symlinks.",schema({{"patch",field("string")}},{"patch"}),true);
    for(const auto &name:{"copy_file","move_file"})add(name,"Copy/move a regular local file; destination parent must exist. Existing destination requires overwrite=true. A move reports partial=true if destination succeeds but source deletion fails.",schema({{"source",path},{"destination",path},{"overwrite",field("boolean")}},{"source","destination"}),true);
    add("delete_file","Delete one local file or symlink. Directories are refused. Completed deletion cannot be undone by stopping the Agent.",schema({{"path",path}},{"path"}),true);
    add("create_directory","Create a local directory and missing parents.",schema({{"path",path}},{"path"}),true);
}
}
