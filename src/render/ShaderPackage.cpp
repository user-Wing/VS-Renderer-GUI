#include "render/ShaderPackage.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cmath>
#include <algorithm>

namespace vsr {
namespace {
struct Pass {
    QString hook="MAIN", save="MAIN", width="HOOKED.w", height="HOOKED.h", when="1", body;
    QStringList bindings;
    QByteArray bytecode;
    int components=4;
};
QByteArray read(const QString &path) { QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray(); }
bool write(const QString &path,const QByteArray &data) { QFile file(path);return file.open(QIODevice::WriteOnly) && file.write(data)==data.size(); }
QString toolPath(const QString &name) {
    const QDir app(QCoreApplication::applicationDirPath());
    for(const auto &directory:QStringList{app.filePath("runtime/shader-tools"),app.filePath("../../.deps/color/ucrt64/bin"),app.filePath(".deps/color/ucrt64/bin")}) {
        const auto path=QDir(directory).filePath(name+".exe");if(QFileInfo::exists(path))return QFileInfo(path).absoluteFilePath();
    }
    return {};
}
bool run(const QString &program,const QStringList &arguments,QString &error) {
    QProcess process;process.setProcessChannelMode(QProcess::MergedChannels);process.start(program,arguments);
    if(!process.waitForStarted(5000)){error=process.errorString();return false;}
    if(!process.waitForFinished(30000)){process.kill();process.waitForFinished();error="Shader conversion timed out.";return false;}
    if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0){error=QString::fromUtf8(process.readAll()).trimmed();return false;}
    return true;
}
bool expression(const QString &text,const QSet<QString> &names,QString &error) {
    int depth=0;
    for(const auto &token:text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) {
        if(token=="!"){if(depth<1){error="Invalid unary expression: "+text;return false;}continue;}
        if(QStringList{"+","-","*","/",">","<","="}.contains(token)){if(depth<2){error="Invalid expression: "+text;return false;}--depth;continue;}
        bool number=false;const double value=token.toDouble(&number);
        if(number && std::isfinite(value)){++depth;continue;}
        const auto match=QRegularExpression("^([A-Za-z_][A-Za-z_0-9]*)\\.(w|h|width|height)$").match(token);
        if(!match.hasMatch() || !names.contains(match.captured(1))){error="Unknown expression value: "+token;return false;}
        ++depth;
    }
    if(depth!=1){error="Invalid expression: "+text;return false;}return true;
}
bool parse(const QString &text,QList<Pass> &passes,QString &error) {
    struct Block {QMap<QString,QStringList> directives;QString body;};
    QList<Block> blocks;Block current;bool body=false;
    const auto uncomment=[](QString value){value.remove(QRegularExpression("/\\*[\\s\\S]*?\\*/"));value.remove(QRegularExpression("//[^\\n]*"));return value.trimmed();};
    const auto flush=[&]{if(!current.body.trimmed().isEmpty()){if(!current.directives.isEmpty() || !uncomment(current.body).isEmpty())blocks.append(current);current={};}body=false;};
    for(const auto &line:text.split('\n')) {
        if(line.startsWith("//!")) {
            if(body)flush();
            const auto match=QRegularExpression("^//!(\\w+)(?:\\s+(.*))?$").match(line.trimmed());
            if(!match.hasMatch()){error="Invalid mpv directive: "+line;return false;}
            current.directives[match.captured(1)].append(match.captured(2).trimmed());
        } else {
            current.body+=line+'\n';
            if(!line.trimmed().isEmpty() && !line.trimmed().startsWith("//"))body=true;
        }
    }
    flush();
    if(!current.directives.isEmpty()){error="Shader directives have no body.";return false;}
    QMap<QString,QString> defaults;QString definitions;QList<Block> hooks;
    for(const auto &block:blocks){
        if(!block.directives.contains("PARAM")){hooks.append(block);continue;}
        const auto &d=block.directives;const auto name=d.value("PARAM").value(0),type=d.value("TYPE").value(0);const auto literal=uncomment(block.body);bool numeric=false;const double value=literal.toDouble(&numeric);
        for(auto it=d.cbegin();it!=d.cend();++it)if(!QStringList{"PARAM","DESC","TYPE","MINIMUM","MAXIMUM"}.contains(it.key())){error="Unsupported parameter directive: "+it.key();return false;}
        if(d.value("PARAM").size()!=1 || !QRegularExpression("^[A-Za-z_][A-Za-z_0-9]*$").match(name).hasMatch() || defaults.contains(name) || !numeric || !std::isfinite(value) || !QStringList{"float","int","CONSTANT float","CONSTANT int"}.contains(type) || (type.endsWith("int") && value!=std::trunc(value))){error="Unsupported mpv parameter declaration: "+name;return false;}
        defaults.insert(name,QString::number(value,'g',16));definitions+="#define "+name+" ("+(type.endsWith("int")?QString("int"):QString("float"))+"("+literal+"))\n";
    }
    blocks=hooks;
    const auto resolve=[&](QString value){for(auto it=defaults.cbegin();it!=defaults.cend();++it)value.replace(QRegularExpression("\\b"+QRegularExpression::escape(it.key())+"\\b"),it.value());return value;};
    if(blocks.isEmpty() || blocks.size()>128){error="Expected 1 to 128 mpv pixel shader passes.";return false;}
    // mpv runs each hook stage in order, retaining file order within that stage.
    // Bundled Anime4K places its PREKERNEL clamp before the later MAIN CNN passes.
    std::stable_sort(blocks.begin(),blocks.end(),[](const Block &a,const Block &b){return (a.directives.value("HOOK").value(0)=="LUMA"?0:a.directives.value("HOOK").value(0)=="PREKERNEL"?2:1)<(b.directives.value("HOOK").value(0)=="LUMA"?0:b.directives.value("HOOK").value(0)=="PREKERNEL"?2:1);});
    QSet<QString> names{"LUMA","MAIN","HOOKED","OUTPUT","NATIVE"};bool prekernel=false;
    for(const auto &block:blocks) {
        const auto &d=block.directives;
        for(auto it=d.begin();it!=d.end();++it) {
            if(!QStringList{"DESC","HOOK","BIND","SAVE","WIDTH","HEIGHT","WHEN","COMPONENTS"}.contains(it.key())) {error="Unsupported mpv directive: "+it.key();return false;}
            if(it.key()!="BIND" && it.key()!="DESC" && it.value().size()!=1){error="Repeated mpv directive: "+it.key();return false;}
        }
        Pass pass;pass.hook=d.value("HOOK").value(0);
        if(pass.hook!="LUMA" && pass.hook!="MAIN" && pass.hook!="PREKERNEL"){error="Unsupported mpv hook: "+pass.hook+" (supports LUMA, MAIN and PREKERNEL).";return false;}
        if(pass.hook=="MAIN" && prekernel){error="MAIN after PREKERNEL requires a hook scheduler not supported by this importer.";return false;}
        prekernel|=pass.hook=="PREKERNEL";if(prekernel)names.insert("PREKERNEL");
        pass.save=d.value("SAVE").value(0,pass.hook);if(pass.save=="HOOKED")pass.save=pass.hook;
        pass.width=d.value("WIDTH").value(0,"HOOKED.w");pass.height=d.value("HEIGHT").value(0,"HOOKED.h");pass.when=d.value("WHEN").value(0,"1");
        if(!QRegularExpression("^[A-Za-z_][A-Za-z_0-9]*$").match(pass.save).hasMatch() || pass.save=="OUTPUT" || pass.save=="NATIVE"){error="Unsupported SAVE name: "+pass.save;return false;}
        if(pass.hook=="PREKERNEL" && pass.save=="PREKERNEL" && (d.contains("WIDTH") || d.contains("HEIGHT"))){error="PREKERNEL cannot resize the hooked image.";return false;}
        if(d.contains("COMPONENTS")){bool ok=false;pass.components=d.value("COMPONENTS").first().toInt(&ok);if(!ok || pass.components<1 || pass.components>4){error="COMPONENTS must be between 1 and 4.";return false;}}
        pass.bindings=d.value("BIND");if(pass.bindings.size()>8){error="At most eight texture bindings are supported.";return false;}
        // NATIVE geometry is known, but its pre-conversion pixels are not exposed.
        // Anime4K declares this binding only for NATIVE.w/h in WHEN expressions.
        if(pass.bindings.contains("NATIVE")){
            if(QRegularExpression("\\bNATIVE(?:_\\w+)?\\b").match(block.body).hasMatch()){error="Sampling NATIVE pixels is not supported by the RGB importer.";return false;}
            pass.bindings.removeAll("NATIVE");
        }
        QSet<QString> seen;
        for(const auto &name:pass.bindings){if(!names.contains(name) || name=="OUTPUT" || seen.contains(name)){error="Unknown or repeated BIND texture: "+name;return false;}seen.insert(name);}
        pass.width=resolve(pass.width);pass.height=resolve(pass.height);pass.when=resolve(pass.when);
        if(!expression(pass.width,names,error) || !expression(pass.height,names,error) || !expression(pass.when,names,error))return false;
        // Only the explicit mpv subset is injected. Unimplemented builtins fail compilation.
        pass.body=definitions+block.body;passes.append(pass);names.insert(pass.save);
    }
    return true;
}
QString glsl(const Pass &pass) {
    QString source="#version 450\nlayout(location=0) in vec2 uv;\nlayout(location=0) out vec4 outColor;\n";
    for(int i=0;i<pass.bindings.size();++i){const auto &name=pass.bindings[i];source+=QString(
        "layout(set=0,binding=%1) uniform sampler2D %2_raw;\n"
        "#define %2_size vec2(textureSize(%2_raw,0))\n#define %2_pt (1.0 / %2_size)\n#define %2_pos uv\n"
        "#define %2_tex(p) textureLod(%2_raw,(p),0.0)\n#define %2_texOff(o) %2_tex(uv+(o)*%2_pt)\n#define %2_gather(p,c) textureGather(%2_raw,(p),c)\n").arg(i).arg(name);}
    const auto alias=[&](const QString &name,const QString &bound){
        for(const auto &suffix:QStringList{"raw","size","pt","pos"})source+="#define "+name+'_'+suffix+' '+bound+'_'+suffix+'\n';
        source+="#define "+name+"_tex(p) "+bound+"_tex(p)\n#define "+name+"_texOff(o) "+bound+"_texOff(o)\n#define "+name+"_gather(p,c) "+bound+"_gather(p,c)\n";
    };
    if(pass.bindings.contains("HOOKED") && !pass.bindings.contains(pass.hook))alias(pass.hook,"HOOKED");
    if(pass.bindings.contains(pass.hook) && !pass.bindings.contains("HOOKED"))alias("HOOKED",pass.hook);
    const QStringList output{"", "vec4(v.r,0,0,1)", "vec4(v.rg,0,1)", "vec4(v.rgb,1)", "v"};
    return source+pass.body+"\nvoid main(){vec4 v=hook();outColor="+output.at(pass.components)+";}\n";
}
bool compile(Pass &pass,const QByteArray &source,bool native,QString &error) {
    ID3DBlob *code=nullptr,*messages=nullptr;
    const auto result=D3DCompile(source.constData(),size_t(source.size()),"custom-shader",nullptr,nullptr,"main","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&messages);
    if(FAILED(result)){error=messages?QString::fromUtf8(static_cast<const char*>(messages->GetBufferPointer()),int(messages->GetBufferSize())):"HLSL compilation failed.";if(messages)messages->Release();return false;}
    if(messages)messages->Release();
    ID3D11ShaderReflection *reflection=nullptr;
    const auto reflected=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_ID3D11ShaderReflection,reinterpret_cast<void**>(&reflection));
    bool valid=SUCCEEDED(reflected);D3D11_SHADER_DESC desc{};if(valid)valid=SUCCEEDED(reflection->GetDesc(&desc));
    for(UINT i=0;valid && i<desc.BoundResources;++i) {
        D3D11_SHADER_INPUT_BIND_DESC binding{};reflection->GetResourceBindingDesc(i,&binding);
        if(binding.BindCount!=1)valid=false;
        else if(binding.Type==D3D_SIT_TEXTURE)valid=binding.Dimension==D3D_SRV_DIMENSION_TEXTURE2D && binding.ReturnType==D3D_RETURN_TYPE_FLOAT && binding.BindPoint<UINT(pass.bindings.size());
        else if(binding.Type==D3D_SIT_SAMPLER)valid=binding.BindPoint<UINT(pass.bindings.size()) && !(binding.uFlags&D3D_SIF_COMPARISON_SAMPLER);
        else if(binding.Type==D3D_SIT_CBUFFER && native && binding.BindPoint==0){
            auto* cb=reflection->GetConstantBufferByName(binding.Name);D3D11_SHADER_BUFFER_DESC buffer{};
            valid=SUCCEEDED(cb->GetDesc(&buffer)) && buffer.Size==32 && buffer.Variables==2;
            for(UINT v=0;valid && v<2;++v){D3D11_SHADER_VARIABLE_DESC variable{};D3D11_SHADER_TYPE_DESC type{};auto* entry=cb->GetVariableByIndex(v);
                valid=SUCCEEDED(entry->GetDesc(&variable)) && SUCCEEDED(entry->GetType()->GetDesc(&type)) && variable.StartOffset==v*16 && variable.Size==16 && type.Class==D3D_SVC_VECTOR && type.Type==D3D_SVT_FLOAT && type.Rows==1 && type.Columns==4 && type.Elements==0;}
        }
        else valid=false;
    }
    for(UINT i=0;valid && i<desc.InputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC input{};reflection->GetInputParameterDesc(i,&input);const auto name=QByteArray(input.SemanticName).toUpper();valid=input.ComponentType==D3D_REGISTER_COMPONENT_FLOAT32 && (name=="SV_POSITION" || (name=="TEXCOORD" && input.SemanticIndex==0 && input.Mask<=3));}
    for(UINT i=0;valid && i<desc.OutputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC output{};reflection->GetOutputParameterDesc(i,&output);valid=output.ComponentType==D3D_REGISTER_COMPONENT_FLOAT32 && QByteArray(output.SemanticName).toUpper()=="SV_TARGET" && output.SemanticIndex==0;}
    if(reflection)reflection->Release();
    if(valid)pass.bytecode=QByteArray(static_cast<const char*>(code->GetBufferPointer()),int(code->GetBufferSize()));
    else error="Unsupported HLSL bindings. Use main/ps_5_0, Texture2D t0, SamplerState s0, TEXCOORD0 and optional 32-byte b0 (sourceSize, outputSize).";
    code->Release();return valid;
}
void put(QDataStream &stream,const QByteArray &text){stream<<quint32(text.size());stream.writeRawData(text.constData(),text.size());}
bool validCache(const QByteArray &bytes) {
    if(bytes.size()<12 || bytes.size()>64*1024*1024 || !bytes.startsWith("VSRHLSL1"))return false;
    qsizetype at=8;
    const auto integer=[&](quint32 &value){if(at+4>bytes.size())return false;const auto* p=reinterpret_cast<const unsigned char*>(bytes.constData()+at);value=quint32(p[0])|(quint32(p[1])<<8)|(quint32(p[2])<<16)|(quint32(p[3])<<24);at+=4;return true;};
    const auto field=[&](bool code){quint32 size=0;if(!integer(size) || size>(code?16u*1024*1024:4096u) || at+size>bytes.size())return false;if(code && (size<4 || bytes.mid(at,4)!="DXBC"))return false;at+=size;return true;};
    quint32 passes=0;if(!integer(passes) || passes==0 || passes>128)return false;
    for(quint32 i=0;i<passes;++i){for(int f=0;f<5;++f)if(!field(false))return false;quint32 bindings=0;if(!integer(bindings) || bindings>8)return false;for(quint32 b=0;b<bindings;++b)if(!field(false))return false;if(!field(true))return false;}
    return at==bytes.size();
}
}

ShaderPackageResult ShaderPackage::prepare(const QString &sourcePath,const QString &cacheDirectory) {
    if(!QFileInfo::exists(sourcePath)){
        QDir directory(QFileInfo(sourcePath).absolutePath());
        while(!QFileInfo::exists(directory.filePath("mpv-shaders.7z"))){if(!directory.cdUp())return {{},"Cannot find shader: "+sourcePath};}
        const auto archive=directory.filePath("mpv-shaders.7z"),member=directory.relativeFilePath(sourcePath);
        const auto root=cacheDirectory.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/hlsl":cacheDirectory;
        const auto output=QDir(root).filePath("bundled/"+QString::fromLatin1(QCryptographicHash::hash(read(archive),QCryptographicHash::Sha256).toHex())+"/"+member);
        QDir().mkpath(QFileInfo(output).absolutePath());
        if(!QFileInfo::exists(output))for(const auto &suffix:QStringList{"",".vsrhlsl",".vsrhlsl.json"}){
            QProcess process;process.start(QDir(QCoreApplication::applicationDirPath()).filePath("runtime/tools/7z.exe"),{"x","-so","-bd","-bsp0","-spd",archive,member+suffix});
            if(!process.waitForStarted() || !process.waitForFinished(30000)){process.kill();process.waitForFinished();return {{},"Cannot extract bundled shader."};}
            if(process.exitCode()==0){if(!write(output+suffix,process.readAllStandardOutput()))return {{},"Cannot save bundled shader."};}
            else if(suffix.isEmpty())return {{},"Bundled shader is missing: "+member};
        }
        return prepare(output,cacheDirectory);
    }
    ShaderPackageResult result;const auto source=read(sourcePath);const auto suffix=QFileInfo(sourcePath).suffix().toLower();
    if(source.isEmpty() || source.size()>16*1024*1024){result.error="Cannot read shader (limit 16 MiB): "+sourcePath;return result;}
    const bool native=suffix=="hlsl";
    const bool multipass=native && QRegularExpression("^//!HOOK\\s",QRegularExpression::MultilineOption).match(QString::fromUtf8(source)).hasMatch();
    if(!native && suffix!="glsl" && suffix!="hook"){result.error="Expected .hlsl, .glsl or .hook.";return result;}
    if(QRegularExpression("^\\s*#\\s*include\\b",QRegularExpression::MultilineOption).match(QString::fromUtf8(source)).hasMatch()){result.error="Shader #include is not supported; import a self-contained shader.";return result;}
    const auto packaged=sourcePath+".vsrhlsl";
    const auto metadata=QJsonDocument::fromJson(read(packaged+".json")).object();
    if(QStringList{"luma-main-prekernel-v7","luma-main-prekernel-params-v8"}.contains(metadata.value("semantics").toString()) && metadata.value("sha256").toString()==QString::fromLatin1(QCryptographicHash::hash(source,QCryptographicHash::Sha256).toHex()) && validCache(read(packaged))){result.packagePath=packaged;return result;}
    const auto glslc=toolPath("glslc"),cross=toolPath("spirv-cross");
    if(!native && (glslc.isEmpty() || cross.isEmpty())){result.error="GLSL conversion tools are missing (runtime/shader-tools/glslc.exe and spirv-cross.exe).";return result;}
    QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData("VSR-HLSL-1.0.17-format1-main-prekernel-native-multipass-params-v8");hash.addData(suffix.toUtf8());hash.addData(source);
    if(!native){hash.addData(read(glslc));hash.addData(read(cross));}
    wchar_t compilerPath[MAX_PATH]{};if(GetModuleFileNameW(GetModuleHandleW(L"d3dcompiler_47.dll"),compilerPath,MAX_PATH))hash.addData(read(QString::fromWCharArray(compilerPath)));
    const auto cache=cacheDirectory.isEmpty()?QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("hlsl"):cacheDirectory;
    if(!QDir().mkpath(cache)){result.error="Cannot create shader cache: "+cache;return result;}
    const auto path=QDir(cache).filePath(QString::fromLatin1(hash.result().toHex())+".vsrhlsl");
    const auto existing=read(path);
    if(validCache(existing)){result.packagePath=path;return result;}
    QList<Pass> passes;
    if(native && !multipass){Pass pass;pass.width="OUTPUT.w";pass.height="OUTPUT.h";pass.bindings={"HOOKED"};passes.append(pass);}
    else if(!parse(QString::fromUtf8(source),passes,result.error))return result;
    QTemporaryDir temporary(QDir(cache).filePath("convert-XXXXXX"));if(!temporary.isValid()){result.error="Cannot create conversion workspace.";return result;}
    for(int i=0;i<passes.size();++i){auto &pass=passes[i];QByteArray hlsl=multipass?pass.body.toUtf8():source;
        if(!native){const auto base=QDir(temporary.path()).filePath(QString::number(i));
            if(!write(base+".frag",glsl(pass).toUtf8())){result.error="Cannot write GLSL conversion input.";return result;}
            if(!run(glslc,{"-fshader-stage=frag",base+".frag","-o",base+".spv"},result.error) ||
               !run(cross,{base+".spv","--hlsl","--shader-model","50","--output",base+".hlsl"},result.error))return result;
            hlsl=read(base+".hlsl");
        }
        if(!compile(pass,hlsl,native,result.error)){result.error=QString("Pass %1: %2").arg(i+1).arg(result.error);return result;}
    }
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)){result.error=file.errorString();return result;}
    QDataStream stream(&file);stream.setByteOrder(QDataStream::LittleEndian);stream.writeRawData("VSRHLSL1",8);stream<<quint32(passes.size());
    for(const auto &pass:passes){for(const auto &text:QStringList{pass.hook,pass.save,pass.width,pass.height,pass.when})put(stream,text.toUtf8());stream<<quint32(pass.bindings.size());for(const auto &binding:pass.bindings)put(stream,binding.toUtf8());put(stream,pass.bytecode);}
    if(stream.status()!=QDataStream::Ok || !file.commit()){result.error="Cannot save compiled shader cache: "+file.errorString();return result;}
    result.packagePath=path;return result;
}
}
