#include "player/PlayerWindow.h"
#include "player/PlayerDiscMenu.h"
#include "player/PlayerAiSubtitles.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"
#include "ui/PreviewPane.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QSettings>
#include <QPushButton>
#include <QTimer>
#include <QThread>
#include <algorithm>
#include <numeric>

namespace vsr {
QString PlayerWindow::profile() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return {};
    if(file.fileName()=="Anime.vpy" || fixedAnimeStage()>=0)return "Anime";
    if(file.fileName()=="Realistic.vpy")return "Realistic";
    if(interpolationStage()>=0)return "Interpolation";
    if(superInterpolationStage()>=0)return "SuperInterpolation";
    return {};
}
bool PlayerWindow::animeSourceSupported() const {
    for(const auto &entry:media_.value("streams").toArray()){
        const auto stream=entry.toObject();
        if(stream.value("type").toString()=="video")return stream.value("height").toInt()<2160;
    }
    return true;
}
int PlayerWindow::fixedAnimeStage() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return -1;
    const QStringList names{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy"};
    return names.indexOf(file.fileName());
}
int PlayerWindow::initialQualityStage() const {
#ifdef VSR_LITE_PLAYER
    return std::clamp(settings_->value("player/animeStage",5).toInt(),4,5);
#endif
    if(interpolationStage()>=0)return 4;
    const int fixed=fixedAnimeStage();
    if(fixed>=0)return fixed;
    if(profile()!="Anime")return 0;
    const QList<int> order{0,1,2,3,6,7,4,5};
    const auto allowed=settings_->value("player/animeStages",QStringList{"0","1","2","3","6","7","4","5"}).toStringList();
    const int start=order.indexOf(std::clamp(settings_->value("player/animeStage",0).toInt(),0,7));
    for(int n=start;n<order.size();++n)if(order[n]==5 || allowed.contains(QString::number(order[n])))return order[n];
    return 5;
}
QStringList PlayerWindow::qualityNames() const {
    return {tr("Anime4K CNN + 额外增强"),tr("Anime4K CNN"),tr("Anime4K no CNN + 额外增强"),tr("Anime4K no CNN"),tr("Jinc 直通"),tr("D3D11 原生直通"),tr("Anime4K 1X · Jinc"),tr("Anime4K 1X · D3D11")};
}
int PlayerWindow::superInterpolationStage() const {
    const QFileInfo file(preset_);if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return -1;
    return QStringList{"Super-0-A4K-RIFE-A4K.vpy","Super-1-A4K-RIFE-Jinc.vpy","Super-2-A4K-RIFE-D3D11.vpy","Super-3-RIFE-A4K.vpy"}.indexOf(file.fileName());
}
void PlayerWindow::updateAlgorithmBadges(){
    message_->setVisible(!message_->text().isEmpty());
    const bool video=!imageMode_ && !audioMode_ && !madvrMode() && !(discMenu_ && discMenu_->active());algorithmBadge_->setVisible(video);interpolationBadge_->hide();if(!video)return;
    const int interpolation=interpolationStage();
    if(superInterpolationStage()>=0 && !direct_){algorithmBadge_->setText(superInterpolationStage()==2?"D3D11":superInterpolationStage()==1?"Jinc":"A4K");interpolationBadge_->setText("RIFE 960×540");interpolationBadge_->show();}
    else if(interpolation>=0 && !direct_){algorithmBadge_->setText(interpolationRenderer()?"D3D11":"Jinc");interpolationBadge_->setText(QStringList{"RIFE+","RIFE","MVT+","MVT"}.at(interpolation));interpolationBadge_->show();}
    else if(!direct_ && profile()=="Anime")algorithmBadge_->setText(QStringList{"A4KCNN+","A4KCNN","A4K+","A4K","Jinc","D3D11","A4K-1X-Jinc","A4K-1X-D3D11"}.at(std::clamp(qualityStage_,0,7)));
    else algorithmBadge_->setText(QStringList{"Nearest","Bilinear","Bicubic","Lanczos3","Jinc","Spline36","SuperXbr","D3D11","Lanczos4"}.value(scalingAlgorithm(),"Jinc"));
}
int PlayerWindow::interpolationStage() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return -1;
    const QStringList names{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};
    auto name=file.fileName();name.replace("-D3D11.vpy",".vpy");return names.indexOf(name);
}
int PlayerWindow::interpolationRenderer() const {return QFileInfo(preset_).fileName().endsWith("-D3D11.vpy")?1:0;}
QStringList PlayerWindow::interpolationStages(int renderer) const {
    QStringList defaults;for(int stage=std::clamp(settings_->value("player/interpolationStart",0).toInt(),0,3);stage<4;++stage)defaults<<QString::number(stage);
    return settings_->value(renderer?"player/interpolationStagesD3D11":"player/interpolationStagesJinc",defaults).toStringList();
}
QStringList PlayerWindow::interpolationNames() const {
    return {tr("RIFE 4.26 · 4queue"),tr("RIFE 4.26 · 4queue · 半宽高"),tr("MVTools · 最高质量"),tr("MVTools · 低质量"),tr("关闭")};
}
void PlayerWindow::setInterpolation(int stage,bool automatic,int renderer) {
#ifdef VSR_LITE_PLAYER
    return;
#endif
    if(madvrMode() || imageMode_ || networkSource())return;
    const QStringList names{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};
    if(stage>=4){stage-=4;renderer=1;}
    if(automatic){const auto allowed=interpolationStages(renderer);while(stage<4 && !allowed.contains(QString::number(stage)))++stage;if(stage>=4)return;}
    auto name=stage>=0 && stage<names.size()?names[stage]:QString("Anime.vpy");
    if(stage>=0 && renderer==1)name.replace(".vpy","-D3D11.vpy");
    preset_=QDir(PresetStore::directory()).filePath("builtin/"+name);
    interpolationAuto_=automatic && stage>=0 && stage<names.size();qualityStage_=interpolationStage()>=0?4:0;
    settings_->setValue("player/preset",preset_);settings_->setValue("player/interpolationAuto",interpolationAuto_);
    settings_->setValue("player/interpolationRenderer",renderer);
    timer_->setInterval(interpolationStage()>=0?4:10);
    usage_=resources_.sample();const auto cache=usage_.totalMemoryMiB*(settings_->value("performance/ram",50).toInt()-usage_.ram)/100.;
    server_->setResourceLimits(qMax(1,QThread::idealThreadCount()),std::clamp(int(cache),256,8192));
    if(!source_.isEmpty())refreshScript();
}
bool PlayerWindow::advanceInterpolation() {
    const int stage=interpolationStage();if(stage<0 || !interpolationAuto_)return false;
    const auto allowed=interpolationStages(interpolationRenderer());int next=stage+1;while(next<4 && !allowed.contains(QString::number(next)))++next;
    if(next>=4){showInterpolationWarning(true);return false;}
    if(resumeAt_<0)resumeAt_=position();autoPlay_=playing_ || autoPlay_;
    setInterpolation(next,true,interpolationRenderer());message_->setText(tr("自动补帧：切换到 %1。").arg(interpolationNames().at(next)));return true;
}
void PlayerWindow::showInterpolationWarning(bool visible) {
    interpolationWarning_->setVisible(visible);if(visible){interpolationWarning_->adjustSize();interpolationWarning_->raise();}
}
void PlayerWindow::ensureProfiles() {
#ifdef VSR_LITE_PLAYER
    // Only the two user-selected pass-through presets are shipped by Lite.
    return;
#endif
    const QDir shaders(QDir(QCoreApplication::applicationDirPath()).filePath("shaders"));QDir().mkpath(shaders.absolutePath());
    if(!QFileInfo::exists(shaders.filePath("anime4k-a-fast.glsl")))QFile::copy(":/filters/anime4k-a-fast.glsl",shaders.filePath("anime4k-a-fast.glsl"));
    if(!QFileInfo::exists(shaders.filePath("anime4k-no-cnn.glsl")))QFile::copy(":/filters/anime4k-no-cnn.glsl",shaders.filePath("anime4k-no-cnn.glsl"));
    const auto oldProcessing=QStringLiteral(
        "    if _stage == 0:\n"
        "        clip = _vsr_sharpen_chain(clip, [('sharpen_edges', 0.3, 2), ('enhance_detail', 0.3, 2)])\n"
        "    clip = core.resize.Spline36(clip, format=vs.YUV420P16)\n"
        "    clip = core.placebo.Shader(clip, shader=os.path.join(_vsr_directory, 'shaders', 'anime4k-a-fast.glsl'), width=_w, height=_h)\n");
    const auto processing=QStringLiteral(
        "    _shader_name = 'anime4k-a-fast.glsl' if _stage < 2 else 'anime4k-no-cnn.glsl'\n"
        "    with open(os.path.join(_vsr_directory, 'shaders', _shader_name), encoding='utf-8') as _file: _shader = _file.read()\n"
        "    if _stage in (0, 2):\n"
        "        _shader = '\\n'.join(_vsr_sharpen_shader(*s) for s in [('sharpen_edges', 0.3, 2), ('enhance_detail', 0.3, 2)]) + '\\n' + _shader\n"
        "    if clip.format.bits_per_sample != 16: clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
        "    clip = getattr(core, 'vsrplacebo', core.placebo).Shader(clip, shader_s=_shader, width=_w, height=_h)\n");
    const QDir builtin(QDir(PresetStore::directory()).filePath("builtin"));QDir().mkpath(builtin.absolutePath());
    const QStringList superFiles{"Super-0-A4K-RIFE-A4K.vpy","Super-1-A4K-RIFE-Jinc.vpy","Super-2-A4K-RIFE-D3D11.vpy","Super-3-RIFE-A4K.vpy"};
    for(int stage=0;stage<superFiles.size();++stage){
        const auto path=builtin.filePath(superFiles[stage]);if(QFileInfo::exists(path))continue;
        FilterGraph graph;
        const auto anime=[&]{const int node=graph.add("anime4k");graph.setParameter(node,"mode","anime4k-a-fast.glsl");graph.setParameter(node,"output_mode","指定分辨率");graph.setParameter(node,"width",3840);graph.setParameter(node,"height",2160);graph.setParameter(node,"keep_aspect",false);};
        if(stage<3)anime();
        if(stage<3){const int resize=graph.add("resize");graph.setParameter(resize,"width",960);graph.setParameter(resize,"height",540);graph.setParameter(resize,"kernel","Bicubic");}
        const int rife=graph.add("rife");graph.setParameter(rife,"threads",1);
        if(stage==3)graph.setParameter(rife,"inference_scale","2 - 半宽半高");
        if(stage==0 || stage==3)anime();
        auto script=PresetStore::create(graph,SourceFilter::Ffms2,{},{});
        if(stage==3)script.replace("shader_s=_shader_text, ","shader_s=_shader_text, **({'subsampling': 1} if hasattr(core, 'vsrplacebo') else {}), ");
        PresetStore::write(path,"# VSR_PLAYER_SUPER_INTERPOLATION\n"+script);
    }
    const QStringList interpolationFiles{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};
    for(int stage=0;stage<interpolationFiles.size();++stage) {
        const auto path=builtin.filePath(interpolationFiles[stage]);if(QFileInfo::exists(path))continue;
        FilterGraph graph;const int node=graph.add(stage<2?"rife":"mvtools");
        if(stage<2){graph.setParameter(node,"threads",4);graph.setParameter(node,"inference_scale",stage==0?"1 - 原始":"2 - 半宽半高");}
        else if(stage==2){graph.setParameter(node,"block","8");graph.setParameter(node,"pel","2");graph.setParameter(node,"overlap",true);graph.setParameter(node,"chroma",true);}
        auto script=PresetStore::create(graph,SourceFilter::Ffms2,{},interpolationNames().at(stage));
        script.replace("clip = src","clip = core.resize.Point(src, format=vs.YUV444P16, **({'matrix': _vsr_matrix(src)} if src.format.color_family == vs.RGB else {}))");
        PresetStore::write(path,script);
    }
    for(const auto &name:interpolationFiles){auto target=name;target.replace(".vpy","-D3D11.vpy");const auto path=builtin.filePath(target);if(!QFileInfo::exists(path))QFile::copy(builtin.filePath(name),path);}
    for(const auto &mode:QStringList{"Anime","Realistic"}) {
        const auto path=builtin.filePath(mode+".vpy");if(QFileInfo::exists(path)) {
            QFile existing(path);if(existing.open(QIODevice::ReadOnly)){const auto original=existing.readAll();auto text=QString::fromUtf8(original);existing.close();
                const auto line=QStringLiteral("    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)");
                const auto conversion=QStringLiteral("    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n");
                if(text.startsWith("# VSR_PLAYER_PROFILE ") && text.contains(line) && !text.contains(conversion.trimmed())) {
                    text.replace(line,conversion+line);const auto backup=path+".before-bitdepth-fix";
                    if(QFile::exists(backup) || QFile::copy(path,backup))PresetStore::write(path,text);
                }
                if(mode=="Anime" && text.startsWith("# VSR_PLAYER_PROFILE Anime\n") && text.contains(oldProcessing) && text.contains("_stage < 2\n")) {
                    text.replace(oldProcessing,processing);text.replace("_stage < 2\n","_stage < 4\n");const auto backup=path+".before-six-stage";
                    if(QFile::exists(backup) || QFile::copy(path,backup))PresetStore::write(path,text);
                }
            }continue;
        }
        QString script="# VSR_PLAYER_PROFILE "+mode+'\n'+PresetStore::create(FilterGraph(),SourceFilter::Ffms2,{},mode);
        QFile helper(":/filters/gpu-sharpen.py");if(mode=="Anime" && helper.open(QIODevice::ReadOnly))script.replace("clip = src",QString::fromUtf8(helper.readAll())+"\nclip = src");
        script.replace("clip.set_output(0)",QStringLiteral(
            "# Editable profile. Player supplies the viewport target and quality stage.\n"
            "_tw = max(2, int(globals().get('_vsr_target_width', 3840)))\n"
            "_th = max(2, int(globals().get('_vsr_target_height', 2160)))\n"
            "_fit = min(_tw / clip.width, _th / clip.height, 3840 / clip.width, 2160 / clip.height)\n"
            "_w = max(2, int(clip.width * _fit) // 2 * 2)\n"
            "_h = max(2, int(clip.height * _fit) // 2 * 2)\n"
            "import math\n"
            "_div = math.gcd(src.width, src.height)\n"
            "_rw, _rh = src.width // _div, src.height // _div\n"
            "_mul = min(_w // _rw, _h // _rh)\n"
            "if _rw % 2 or _rh % 2: _mul = _mul // 2 * 2\n"
            "if _mul > 0: _w, _h = _rw * _mul, _rh * _mul\n"
            "_stage = int(globals().get('_vsr_quality_stage', 0))\n"
            "_anime = %1 and clip.width < 3840 and clip.height < 2160 and _stage < 4\n"
            "if globals().get('_vsr_resize_before_enhance', False) and (_w < clip.width or _h < clip.height):\n"
            "    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
            "    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)\n"
            "if _anime:\n"
            "%2"
            "elif clip.width != _w or clip.height != _h:\n"
            "    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
            "    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)\n"
            "import math\n"
            "_props = src.get_frame(0).props\n"
            "_sn = src.width * _h * int(_props.get('_SARNum', 1))\n"
            "_sd = src.height * _w * int(_props.get('_SARDen', 1))\n"
            "_g = math.gcd(_sn, _sd)\n"
            "clip = core.std.SetFrameProps(clip, _SARNum=_sn // _g, _SARDen=_sd // _g)\n"
            "clip.set_output(0)").arg(mode=="Anime"?"True":"False",processing));
        PresetStore::write(path,script);
    }
    QFile anime(builtin.filePath("Anime.vpy"));
    if(anime.open(QIODevice::ReadOnly)) {
        const auto automatic=QString::fromUtf8(anime.readAll());
        const QStringList names{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy"};
        for(int stage=0;stage<names.size();++stage) {
            const auto path=builtin.filePath(names[stage]);if(QFileInfo::exists(path))continue;
            auto script=automatic;
            script.replace("# VSR_PLAYER_PROFILE Anime\n",QString("# VSR_PLAYER_PROFILE Anime\n# VSR_PLAYER_FIXED_STAGE %1\n").arg(stage));
            script.replace("_stage = int(globals().get('_vsr_quality_stage', 0))",QString("_stage = %1").arg(stage));
            script.replace("_anime = True and clip.width < 3840 and clip.height < 2160 and _stage < 4","_anime = _stage < 4");
            PresetStore::write(path,script);
        }
    }
    for(int stage:{6,7}) {
        const auto path=builtin.filePath(stage==6?"Anime-6-1X-Jinc.vpy":"Anime-7-1X-D3D11.vpy");
        if(QFileInfo::exists(path))continue;
        auto script=PresetStore::create(FilterGraph(),SourceFilter::Ffms2,{},{});
        script.prepend(QString("# VSR_PLAYER_PROFILE Anime\n# VSR_PLAYER_FIXED_STAGE %1\n").arg(stage));
        script.replace("clip.set_output(0)",QStringLiteral(
            "if clip.format.bits_per_sample != 16: clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
            "clip = getattr(core, 'vsrplacebo', core.placebo).Shader(clip, shader=os.path.join(_vsr_directory, 'shaders', 'anime4k-a-fast.glsl'), width=clip.width, height=clip.height)\n"
            "clip.set_output(0)"));
        PresetStore::write(path,script);
    }
    const auto descriptions=QDir(PresetStore::directory()).filePath("preset-descriptions.json");
    QFile descriptionFile(descriptions);QJsonObject entries;bool changed=!descriptionFile.exists();
    if(descriptionFile.open(QIODevice::ReadOnly)){
        const auto document=QJsonDocument::fromJson(descriptionFile.readAll());
        if(!document.isObject())return;
        entries=document.object();descriptionFile.close();
    }else if(descriptionFile.exists())return;
    for(const auto &name:builtin.entryList({"*.vpy"},QDir::Files))if(!entries.contains("builtin/"+name)){entries.insert("builtin/"+name,QString());changed=true;}
    if(changed)PresetStore::write(descriptions,QString::fromUtf8(QJsonDocument(entries).toJson(QJsonDocument::Indented)));
}
QSize PlayerWindow::profileTarget() const {
    QSize source(clip_.width,clip_.height);
    for(const auto &entry:media_.value("streams").toArray()){const auto stream=entry.toObject();if(stream.value("type").toString()=="video"){source=QSize(stream.value("width").toInt(),stream.value("height").toInt());break;}}
    if(source.isEmpty())return QSize(1920,1080);
    const auto available=(pane_->surface()->size()*pane_->surface()->devicePixelRatioF()).boundedTo(QSize(3840,2160));
    const auto fitted=source.scaled(available,Qt::KeepAspectRatio);
    const int divisor=std::gcd(source.width(),source.height()),w=source.width()/divisor,h=source.height()/divisor;
    int multiple=qMin(fitted.width()/w,fitted.height()/h);if(w%2 || h%2)multiple=multiple/2*2;
    if(multiple>0)return QSize(w*multiple,h*multiple);
    return QSize(qMax(2,fitted.width()/2*2),qMax(2,fitted.height()/2*2));
}
void PlayerWindow::setDirectMode(bool enabled) {
    if(direct_==enabled)return;
    if(aiSubtitles_)aiSubtitles_->stop();
    const bool opened=!media_.isEmpty();const auto at=position();
    const auto state=clock_->snapshot().state;if(state==ThreeFpState::Ready || state==ThreeFpState::Playing || state==ThreeFpState::Paused || state==ThreeFpState::Ended)clock_->stop();
    clock_.swap(output_);direct_=enabled;
    clock_->setClockOnly(!enabled);
    output_->setMuted(true);clock_->setMuted(mute_->isChecked());
    clock_->setDecodeMode(settings_->value("decode/mode",2).toUInt());
    auto *visible=direct_?clock_.get():output_.get();
    applyScaling();
    visible->setView(pane_->zoom(),pane_->pan().x(),pane_->pan().y());
    if(opened){clock_->openFile(mediaInput_);rateApplied_=false;if(resumeAt_<0)resumeAt_=at;}
}
void PlayerWindow::updateProfile() {
    if(superInterpolationStage()>=0 || (interpolationStage()>=0 && !interpolationAuto_ && interpolationStage()!=3) || (fixedAnimeStage()>=0 && !profileFallback_) || madvrMode() || (profile().isEmpty() && !networkSource()) || qualityStage_==5 || !ready_ || !playing_ || seekPending_ || profileResize_->isActive() || !qualitySettling_.isValid() || qualitySettling_.elapsed()<2000) {qualityTimer_.invalidate();return;}
    const auto output=outputSnapshot();const auto dropped=skippedFrames_+output.droppedVideoFrames+output.coalescedVideoFrames;
    if(!qualityTimer_.isValid()){qualityTimer_.start();qualityDropped_=dropped;qualitySubmitted_=direct_?output.presentedVideoFrames:submittedFrames_;return;}
    if(qualityTimer_.elapsed()<5000)return;
    qualityTimer_.invalidate();
    if(dropped<qualityDropped_ || (direct_?output.presentedVideoFrames:submittedFrames_)<qualitySubmitted_)return;
    const auto missed=dropped-qualityDropped_;
    const auto frames=(direct_?output.presentedVideoFrames:submittedFrames_)-qualitySubmitted_;
    if(frames+missed<48)return;
    const double lag=clip_.fpsNumerator>0?position()/1e7-lastFrame_*double(clip_.fpsDenominator)/clip_.fpsNumerator:0;
    const bool overloaded=double(missed)/(frames+missed)>.05 || (!direct_ && lag>.2);
    if(interpolationStage()>=0){if(!interpolationAuto_)showInterpolationWarning(overloaded);else if(overloaded)advanceInterpolation();else showInterpolationWarning(false);return;}
    if(!overloaded)return;
    if(direct_)qualityStage_=4;
    const QList<int> order{0,1,2,3,6,7,4,5};
    int next=order.indexOf(qualityStage_)+1;
    const auto allowed=settings_->value("player/animeStages",QStringList{"0","1","2","3","6","7","4","5"}).toStringList();
    while(next<order.size()-1 && !allowed.contains(QString::number(order[next])))++next;
    resumeAt_=position();autoPlay_=true;qualityStage_=order.value(next,5);refreshScript();
    message_->setText(tr("丢帧超过 5%：切换到 %1。").arg(qualityNames().at(qualityStage_)));
}
void PlayerWindow::suspendQualityCheck() {qualityTimer_.invalidate();qualitySettling_.restart();}
int PlayerWindow::scalingAlgorithm(bool downscale) const {
    if(direct_ && qualityStage_==5)return std::clamp(settings_->value(downscale?"render/d3d11Downscale":"render/d3d11Upscale",7).toInt(),0,8);
    if(qualityStage_==7 || superInterpolationStage()==2 || (interpolationStage()>=0 && interpolationRenderer()==1))return 7;
    return 4;
}
void PlayerWindow::applyScaling() {
    auto *visible=direct_?clock_.get():output_.get();const bool native=scalingAlgorithm()==7;
    visible->setVideoEnhancement(false,settings_->value("render/rtxVsr",false).toBool());
    visible->setVideoEnhancement(true,direct_ && native && settings_->value("render/trueHdr",false).toBool());
    visible->setAntiRinging(!native && settings_->value("render/antiring",true).toBool());
    visible->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(scalingAlgorithm()),static_cast<ThreeFpScalingAlgorithm>(scalingAlgorithm(true)));
}
}
