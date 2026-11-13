#include "app/AgentVideoFrames.h"
#include "agent/AgentImageMessages.h"
#include "backend/VapourSynthFrameServer.h"
#include <QBuffer>
#include <QDir>
#include <QImage>
#include <QTimer>
#include <cmath>

namespace vsr {
namespace {
QJsonObject failure(const QString &error) { return {{"ok", false}, {"error", error}}; }
QJsonObject field(const char *type) { return {{"type", type}}; }
}

AgentVideoFrames::AgentVideoFrames(AgentSession *session, Script script) : QObject(session)
{
    const QJsonObject crop{{"type", "object"}, {"properties", QJsonObject{{"x", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"maximum", 2147483647}}}, {"y", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"maximum", 2147483647}}}, {"width", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 32768}}}, {"height", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 32768}}}}}, {"required", QJsonArray{"x", "y", "width", "height"}}, {"additionalProperties", false}};
    const QJsonObject schema{{"type", "object"}, {"properties", QJsonObject{
        {"frame", field("integer")}, {"seconds", field("number")},
        {"target", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"source", "processed"}}}}, {"crop", crop}}}, {"additionalProperties", false}};
    session->registerAsyncTool("inspect_video_frame", QStringLiteral(
        "直接从VapourSynth读取指定位置的真实帧并发送图像给视觉模型；不是屏幕截图。frame为0基帧号，seconds为秒，两者必须且只能选一个；按所选source原片或processed当前完整VS链的时间轴定位，默认source。"
        "可用crop指定原分辨率整数矩形，检查细微色带时先看全图再取局部；返回实际帧号、时间、尺寸和色彩信息。"
        "图像为有界无损PNG，最多1024像素边长；若缩小可能掩盖缺陷。成功仅代表图像已提供，需视觉判断。"
        "初检请抽查用户指定帧及多个时段，报告色带、噪点、色块、振铃、锯齿、模糊、过锐和压缩痕迹的位置/程度/置信度；先汇报再提出滤镜，不自动改链。单帧不能证明闪烁/鬼影或整片无瑕疵。HDR/宽色域请先显式转换为SDR BT.709。"),
        schema, [session, script](const QJsonObject &args, AgentSession::Completion completion) {
        if (args.contains("frame") == args.contains("seconds")) { completion(failure(QStringLiteral("frame和seconds必须且只能提供一个。"))); return; }
        const double position = args.value(args.contains("frame") ? "frame" : "seconds").toDouble(-1);
        if (!std::isfinite(position) || position < 0 || (args.contains("frame") && position != std::floor(position))) { completion(failure(QStringLiteral("位置必须是非负有限数，frame必须为整数。"))); return; }
        QString error; const bool processed = args.value("target").toString("source") == "processed";
        auto source = script(processed, &error);
        if (source.isEmpty() || !error.isEmpty()) { completion(failure(error.isEmpty() ? QStringLiteral("没有可用的源媒体。") : error)); return; }
        // FrameEval inspects each actual frame's properties, including variable metadata.
        source += QStringLiteral(R"(
import vapoursynth as _agent_vs
_agent_core = _agent_vs.core
_agent_node = _agent_vs.get_output(0)
if hasattr(_agent_node, 'clip'): _agent_node = _agent_node.clip
_agent_template = _agent_core.std.BlankClip(_agent_node, format=_agent_vs.RGB48)
def _agent_convert(n, f):
    p = f.props
    if p.get('_Transfer', 2) in (16, 18) or p.get('_Primaries', 2) not in (1, 2, 5, 6):
        raise ValueError('Visual inspection requires explicit SDR BT.709 conversion for HDR/wide-gamut frames')
    options = dict(format=_agent_vs.RGB48, dither_type='none')
    if _agent_node.format.color_family == _agent_vs.YUV:
        matrix = p.get('_Matrix', 2)
        if matrix == 2: matrix = 1 if _agent_node.height >= 720 else 6
        options['matrix_in'] = matrix
    return _agent_core.resize.Bicubic(_agent_node, **options)
_agent_core.std.FrameEval(_agent_template, _agent_convert, prop_src=_agent_node).set_output(0)
)");
        auto *job = new QObject(session);
        auto *server = new VapourSynthFrameServer(job, false);
        auto *timer = new QTimer(job); timer->setSingleShot(true);
        const auto provider = session->provider();
        auto done = std::make_shared<bool>(false);
        const auto finish = [job, done, completion](QJsonObject result) {
            if (*done) return; *done = true; completion(result); job->deleteLater();
        };
        QObject::connect(session, &AgentSession::toolExecutionCancelled, job, [finish] { finish({{"ok", false}, {"cancelled", true}, {"error", "Frame inspection cancelled."}}); });
        QObject::connect(session, &AgentSession::conversationReset, job, [finish] { finish(failure("Conversation changed.")); });
        QObject::connect(timer, &QTimer::timeout, job, [finish] { finish(failure("VapourSynth frame inspection timed out.")); });
        QObject::connect(server, &VapourSynthFrameServer::errorOccurred, job, [finish](const QString &message) { finish(failure(message)); });
        auto info = std::make_shared<VapourSynthClipInfo>();
        auto requested = std::make_shared<int>(-1);
        QObject::connect(server, &VapourSynthFrameServer::scriptLoaded, job, [=](const VapourSynthClipInfo &clip, const VapourSynthClipInfo &) {
            *info = clip;
            if (clip.fpsNumerator <= 0 || clip.fpsDenominator <= 0 || clip.totalFrames <= 0 || clip.width <= 0 || clip.height <= 0 || qint64(clip.width)*clip.height > 64*1024*1024) { finish(failure("Unsupported clip dimensions or timeline.")); return; }
            const double index = args.contains("frame") ? position : std::floor(position * clip.fpsNumerator / clip.fpsDenominator);
            if (index >= clip.totalFrames) { finish(failure("Requested position is outside the selected clip.")); return; }
            *requested = int(index); server->requestFrame(*requested);
        });
        QObject::connect(server, &VapourSynthFrameServer::frameReady, job, [=](const VapourSynthFrame &frame) {
            if (*done) return;
            if (frame.frameIndex != *requested || frame.format != ThreeFpExternalPixelFormat::GbrP16) { finish(failure("Unexpected VS frame or pixel format.")); return; }
            const auto current = session->provider();
            if (current.baseUrl != provider.baseUrl || current.model != provider.model || !agent_images::providerSupportsImages(current)) { finish(failure("Image-capable model changed during inspection.")); return; }
            QRect area(0, 0, frame.width, frame.height);
            if (args.contains("crop")) {
                const auto c = args.value("crop").toObject(); area = QRect(c.value("x").toInt(), c.value("y").toInt(), c.value("width").toInt(), c.value("height").toInt());
                if (area.isEmpty() || !QRect(0, 0, frame.width, frame.height).contains(area)) { finish(failure("Crop must be a positive rectangle inside the frame.")); return; }
            }
            QImage image(area.size(), QImage::Format_RGBX64);
            if (image.isNull()) { finish(failure("Cannot allocate frame image.")); return; }
            for (int y = 0; y < area.height(); ++y) {
                auto *row = reinterpret_cast<QRgba64 *>(image.scanLine(y));
                const auto sample = [&](int plane, int x) { return reinterpret_cast<const quint16 *>(frame.planes[plane].constData() + (y+area.y())*frame.strides[plane])[x+area.x()]; };
                // VS RGB planes are R/G/B; the bridge format name describes the FFmpeg equivalent.
                for (int x = 0; x < area.width(); ++x) row[x] = QRgba64::fromRgba64(sample(0,x), sample(1,x), sample(2,x), 65535);
            }
            const auto original = image.size();
            if (image.width() > 1024 || image.height() > 1024) image = image.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QByteArray bytes;
            for (;;) {
                bytes.clear(); QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly);
                if (!image.save(&buffer, "PNG")) { finish(failure("PNG encoding failed.")); return; }
                if (bytes.toBase64().size()+22 <= agent_images::maximumEncodedBytes) break;
                if (image.width() <= 32 || image.height() <= 32) { finish(failure("Frame exceeds image attachment budget.")); return; }
                image = image.scaled(qMax(1,image.width()*3/4), qMax(1,image.height()*3/4), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            const double seconds = double(frame.frameIndex)*info->fpsDenominator/info->fpsNumerator;
            const auto label = QString("VS %1 frame %2 (%3 s), crop %4,%5,%6,%7").arg(processed ? "processed" : "source").arg(frame.frameIndex).arg(seconds,0,'f',6).arg(area.x()).arg(area.y()).arg(area.width()).arg(area.height());
            const QJsonObject attachment{{"mime", "image/png"}, {"dataUrl", "data:image/png;base64,"+QString::fromLatin1(bytes.toBase64())}, {"path", label}, {"width", image.width()}, {"height", image.height()}};
            if (!session->queueToolImageAttachment(attachment, true)) { finish(failure("Cannot queue image outside an active visual tool turn or attachment budget.")); return; }
            finish({{"ok", true}, {"attachment_queued", true}, {"target", processed ? "processed" : "source"}, {"frame", double(frame.frameIndex)}, {"seconds", seconds}, {"fpsNumerator", double(info->fpsNumerator)}, {"fpsDenominator", double(info->fpsDenominator)}, {"totalFrames", info->totalFrames}, {"frameWidth", frame.width}, {"frameHeight", frame.height}, {"width", image.width()}, {"height", image.height()}, {"resized", image.size()!=original}, {"crop", QJsonObject{{"x",area.x()},{"y",area.y()},{"width",area.width()},{"height",area.height()}}}, {"encoding", "lossless PNG / RGB16"}, {"transfer", int(frame.colorTransfer)}, {"primaries", int(frame.colorPrimaries)}, {"scope", "Sampled frame only; resizing and RGB conversion can alter visible defects. Missing YUV matrix uses 709 for HD, 601 for SD. Use crops and adjacent frames to verify. No filters changed."}});
        });
        QObject::connect(server, &VapourSynthFrameServer::initialized, job, [=](bool ok) {
            if (ok) server->loadScript(source, QDir(session->workingDirectory()).filePath("agent-frame.vpy"));
            else finish(failure(server->errorString()));
        });
        timer->start(60000); server->setResourceLimits(2, 256); server->initialize();
    });
    session->setToolImageInputRequired("inspect_video_frame");
    session->setToolDelegationAllowed("inspect_video_frame", false);
}
}
