#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/PresetStore.h"
#include "ui/PreviewPane.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QLabel>
#include <QSettings>
#include <QPushButton>
#include <QTimer>
#include <algorithm>
#include <numeric>

namespace vsr {
QString PlayerWindow::profile() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return {};
    if(file.fileName()=="Anime.vpy" || fixedAnimeStage()>=0)return "Anime";
    if(file.fileName()=="Realistic.vpy")return "Realistic";
    if(interpolationStage()>=0)return "Interpolation";
    return {};
}
int PlayerWindow::fixedAnimeStage() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return -1;
    const QStringList names{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy"};
    return names.indexOf(file.fileName());
}
int PlayerWindow::initialQualityStage() const {
    if(interpolationStage()>=0)return 4;
    const int fixed=fixedAnimeStage();
    return fixed>=0?fixed:profile()=="Anime"?std::clamp(settings_->value("player/animeStage",0).toInt(),0,5):0;
}
QStringList PlayerWindow::qualityNames() const {
    return {tr("Anime4K CNN + 额外增强"),tr("Anime4K CNN"),tr("Anime4K no CNN + 额外增强"),tr("Anime4K no CNN"),tr("Jinc 直通"),tr("D3D11 原生直通")};
}
int PlayerWindow::interpolationStage() const {
    const QFileInfo file(preset_);
    if(file.absolutePath()!=QDir(PresetStore::directory()).filePath("builtin"))return -1;
    const QStringList names{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};
    return names.indexOf(file.fileName());
}
QStringList PlayerWindow::interpolationNames() const {
    return {tr("RIFE 4.26 · 4queue"),tr("RIFE 4.26 · 4queue · 半宽高"),tr("MVTools · 最高质量"),tr("MVTools · 低质量"),tr("关闭")};
}
void PlayerWindow::setInterpolation(int stage,bool automatic) {
    if(madvrMode() || imageMode_ || networkSource())return;
    const QStringList names{"Interpolation-0-RIFE.vpy","Interpolation-1-RIFE-Half.vpy","Interpolation-2-MVTools-HQ.vpy","Interpolation-3-MVTools.vpy"};
    preset_=QDir(PresetStore::directory()).filePath("builtin/"+(stage>=0 && stage<names.size()?names[stage]:QString("Anime.vpy")));
    interpolationAuto_=automatic && stage>=0 && stage<names.size();qualityStage_=interpolationStage()>=0?4:0;
    settings_->setValue("player/preset",preset_);settings_->setValue("player/interpolationAuto",interpolationAuto_);
    if(!source_.isEmpty())refreshScript();
}
bool PlayerWindow::advanceInterpolation() {
    const int stage=interpolationStage();if(stage<0 || !interpolationAuto_)return false;
    if(stage>=3){showInterpolationWarning(true);return false;}
    if(resumeAt_<0)resumeAt_=position();autoPlay_=playing_ || autoPlay_;
    setInterpolation(stage+1,true);message_->setText(tr("自动补帧：切换到 %1。").arg(interpolationNames().at(stage+1)));return true;
}
void PlayerWindow::showInterpolationWarning(bool visible) {
    interpolationWarning_->setVisible(visible);if(visible){interpolationWarning_->adjustSize();interpolationWarning_->raise();}
}
void PlayerWindow::ensureProfiles() {
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
        "    clip = core.placebo.Shader(clip, shader_s=_shader, width=_w, height=_h)\n");
    const QDir builtin(QDir(PresetStore::directory()).filePath("builtin"));QDir().mkpath(builtin.absolutePath());
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
    const bool opened=!media_.isEmpty();const auto at=position();
    const auto state=clock_->snapshot().state;if(state==ThreeFpState::Ready || state==ThreeFpState::Playing || state==ThreeFpState::Paused || state==ThreeFpState::Ended)clock_->stop();
    clock_.swap(output_);direct_=enabled;
    output_->setMuted(true);clock_->setMuted(mute_->isChecked());
    clock_->setDecodeMode(settings_->value("decode/mode",2).toUInt());
    auto *visible=direct_?clock_.get():output_.get();
    applyScaling();
    visible->setView(pane_->zoom(),pane_->pan().x(),pane_->pan().y());
    if(opened){clock_->openFile(mediaInput_);rateApplied_=false;resumeAt_=at;}
}
void PlayerWindow::updateProfile() {
    if((interpolationStage()>=0 && !interpolationAuto_ && interpolationStage()!=3) || (fixedAnimeStage()>=0 && !profileFallback_) || madvrMode() || (profile().isEmpty() && !networkSource()) || qualityStage_>=5 || !ready_ || !playing_ || seekPending_ || profileResize_->isActive() || !qualitySettling_.isValid() || qualitySettling_.elapsed()<2000) {qualityTimer_.invalidate();return;}
    const auto output=outputSnapshot();const auto dropped=skippedFrames_+output.droppedVideoFrames+output.coalescedVideoFrames;
    if(!qualityTimer_.isValid()){qualityTimer_.start();qualityDropped_=dropped;qualitySubmitted_=direct_?output.presentedVideoFrames:submittedFrames_;return;}
    if(qualityTimer_.elapsed()<5000)return;
    qualityTimer_.invalidate();
    if(dropped<qualityDropped_ || (direct_?output.presentedVideoFrames:submittedFrames_)<qualitySubmitted_)return;
    const auto missed=dropped-qualityDropped_;
    const auto frames=(direct_?output.presentedVideoFrames:submittedFrames_)-qualitySubmitted_;
    if(frames+missed<48)return;
    const bool overloaded=double(missed)/(frames+missed)>.05;
    if(interpolationStage()==3){showInterpolationWarning(overloaded);return;}
    if(!overloaded)return;
    if(advanceInterpolation())return;
    if(direct_)qualityStage_=4;
    resumeAt_=position();autoPlay_=true;++qualityStage_;refreshScript();
    message_->setText(tr("丢帧超过 5%：切换到 %1。").arg(qualityNames().at(qualityStage_)));
}
void PlayerWindow::suspendQualityCheck() {qualityTimer_.invalidate();qualitySettling_.restart();}
void PlayerWindow::applyScaling() {
    auto *visible=direct_?clock_.get():output_.get();const bool native=qualityStage_>=5;
    visible->setAntiRinging(!native && settings_->value("render/antiring",true).toBool());
    visible->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(native?7:qualityStage_==4?4:settings_->value("render/upscale",4).toInt()),static_cast<ThreeFpScalingAlgorithm>(native?7:qualityStage_==4?4:settings_->value("render/downscale",4).toInt()));
}
}
