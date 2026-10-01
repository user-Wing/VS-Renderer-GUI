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
    if(file.fileName()=="Anime.vpy")return "Anime";
    if(file.fileName()=="Realistic.vpy")return "Realistic";
    return {};
}
void PlayerWindow::ensureProfiles() {
    const QDir shaders(QDir(QCoreApplication::applicationDirPath()).filePath("shaders"));QDir().mkpath(shaders.absolutePath());
    if(!QFileInfo::exists(shaders.filePath("anime4k-a-fast.glsl")))QFile::copy(":/filters/anime4k-a-fast.glsl",shaders.filePath("anime4k-a-fast.glsl"));
    const QDir builtin(QDir(PresetStore::directory()).filePath("builtin"));QDir().mkpath(builtin.absolutePath());
    for(const auto &mode:QStringList{"Anime","Realistic"}) {
        const auto path=builtin.filePath(mode+".vpy");if(QFileInfo::exists(path)) {
            QFile existing(path);if(existing.open(QIODevice::ReadOnly)){const auto original=existing.readAll();auto text=QString::fromUtf8(original);existing.close();
                const auto line=QStringLiteral("    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)");
                const auto conversion=QStringLiteral("    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n");
                if(text.startsWith("# VSR_PLAYER_PROFILE ") && text.contains(line) && !text.contains(conversion.trimmed())) {
                    text.replace(line,conversion+line);const auto backup=path+".before-bitdepth-fix";
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
            "_anime = %1 and clip.width < 3840 and clip.height < 2160 and _stage < 2\n"
            "if globals().get('_vsr_resize_before_enhance', False) and (_w < clip.width or _h < clip.height):\n"
            "    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
            "    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)\n"
            "if _anime:\n"
            "    if _stage == 0:\n"
            "        clip = _vsr_sharpen_chain(clip, [('sharpen_edges', 0.3, 2), ('enhance_detail', 0.3, 2)])\n"
            "    clip = core.resize.Spline36(clip, format=vs.YUV420P16)\n"
            "    clip = core.placebo.Shader(clip, shader=os.path.join(_vsr_directory, 'shaders', 'anime4k-a-fast.glsl'), width=_w, height=_h)\n"
            "elif clip.width != _w or clip.height != _h:\n"
            "    if clip.format.bits_per_sample not in (8, 16, 32): clip = core.resize.Point(clip, format=clip.format.replace(bits_per_sample=16))\n"
            "    clip = core.placebo.Resample(clip, width=_w, height=_h, filter='ewa_lanczos', antiring=0.5)\n"
            "import math\n"
            "_props = src.get_frame(0).props\n"
            "_sn = src.width * _h * int(_props.get('_SARNum', 1))\n"
            "_sd = src.height * _w * int(_props.get('_SARDen', 1))\n"
            "_g = math.gcd(_sn, _sd)\n"
            "clip = core.std.SetFrameProps(clip, _SARNum=_sn // _g, _SARDen=_sd // _g)\n"
            "clip.set_output(0)").arg(mode=="Anime"?"True":"False"));
        PresetStore::write(path,script);
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
    if(madvrMode() || (profile().isEmpty() && !networkSource()) || qualityStage_>=3 || !ready_ || !playing_ || seekPending_ || profileResize_->isActive() || !qualitySettling_.isValid() || qualitySettling_.elapsed()<2000) {qualityTimer_.invalidate();return;}
    const auto output=outputSnapshot();const auto dropped=skippedFrames_+output.droppedVideoFrames+output.coalescedVideoFrames;
    if(!qualityTimer_.isValid()){qualityTimer_.start();qualityDropped_=dropped;qualitySubmitted_=direct_?output.presentedVideoFrames:submittedFrames_;return;}
    if(qualityTimer_.elapsed()<5000)return;
    qualityTimer_.invalidate();
    if(dropped<qualityDropped_ || (direct_?output.presentedVideoFrames:submittedFrames_)<qualitySubmitted_)return;
    const auto missed=dropped-qualityDropped_;
    const auto frames=(direct_?output.presentedVideoFrames:submittedFrames_)-qualitySubmitted_;
    if(frames+missed<48 || double(missed)/(frames+missed)<=.05)return;
    if(direct_)qualityStage_=2;
    resumeAt_=position();autoPlay_=true;++qualityStage_;refreshScript();
    message_->setText(qualityStage_==1?tr("丢帧超过 5%：关闭细节增强，保留 Anime4K A/Fast。") :qualityStage_==2?tr("丢帧仍超过 5%：切换 Jinc 原生直通。"):tr("Jinc 丢帧超过 5%：切换 D3D11 原生直通。"));
}
void PlayerWindow::suspendQualityCheck() {qualityTimer_.invalidate();qualitySettling_.restart();}
void PlayerWindow::applyScaling() {
    auto *visible=direct_?clock_.get():output_.get();const bool native=qualityStage_>=3;
    visible->setAntiRinging(!native && settings_->value("render/antiring",true).toBool());
    visible->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(native?7:settings_->value("render/upscale",4).toInt()),static_cast<ThreeFpScalingAlgorithm>(native?7:settings_->value("render/downscale",4).toInt()));
}
}
