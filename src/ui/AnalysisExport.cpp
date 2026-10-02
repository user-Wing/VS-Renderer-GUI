#include "ui/AnalysisPage.h"
#include "ui/ExportWindow.h"
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <limits>
#include <algorithm>

namespace vsr {
QSize AnalysisPage::compositionSize() const
{
    QSize output;
    for (int i=0;i<videoCount_;++i) {
        const auto snap=snapshot(i);
        if (qint64(snap.videoWidth)*snap.videoHeight>qint64(output.width())*output.height())
            output=QSize(snap.videoWidth,snap.videoHeight);
    }
    return output;
}
ScriptBuildResult AnalysisPage::compositionScript() const
{
    ScriptBuildResult result;
    const auto visible = visibleVideos();
    if(visible.isEmpty() || busy()) {result.errors << QStringLiteral("请等待视频加载与对齐完成。");return result;}
    const auto layout = static_cast<MultiCompareView::Layout>(layoutChoice_->currentData().toInt());
    const bool supported = (visible.size() == 2 && layout == MultiCompareView::Wipe) ||
        (visible.size() == 3 && (layout == MultiCompareView::ThreeLeft || layout == MultiCompareView::ThreeRight)) ||
        (visible.size() == 4 && (layout == MultiCompareView::Four || layout == MultiCompareView::FourWipe));
    if (!supported) {
        result.errors << QStringLiteral("当前布局不支持导出。仅支持 AB 滑块、ABC 2+1 和 ABCD 2×2；输出分辨率保持最大源视频分辨率。");
        return result;
    }
    if(scaler_->currentIndex()==4 || scaler_->currentIndex()==6 || chromaAlgorithm_==4 || chromaAlgorithm_==6 || chromaAlgorithm_>=9) {
        result.errors << QStringLiteral("当前 GPU Jinc/Super-XBR/双边核尚无一致的 VS 导出实现，请选用 Nearest/Bilinear/Bicubic/Lanczos/Spline36。导出不会静默替换算法。");return result;
    }
    const QSize output=compositionSize();
    if(output.isEmpty()) {result.errors << QStringLiteral("尚未取得视频分辨率。");return result;}
    double duration=std::numeric_limits<double>::max();
    QJsonArray sources;
    const bool wipe=view_->isWipe();
    const int canvasHeight=view_->height()-(wipe?28:0);
    for(int slot=0;slot<visible.size();++slot) {
        const int i=visible[slot]; const auto snap=snapshot(i);
        duration=std::min(duration,(snap.duration100ns-offsets_[i])/10000000.0);
        const QRect cell=view_->cellRect(slot);
        const auto pan=panes_[i]->pan();
        const auto viewport=panes_[i]->surface()->size();
        sources.append(QJsonObject{{"path",paths_[i]},{"offset",offsets_[i]/10000000.0},{"zoom",panes_[i]->zoom()},
            {"pan",QJsonArray{pan.x(),pan.y()}},{"viewport",QJsonArray{viewport.width(),viewport.height()}},
            {"region",QJsonArray{double(cell.left())/view_->width(),double(cell.top())/canvasHeight,
                double(cell.right()+1)/view_->width(),double(cell.bottom()+1)/canvasHeight}}});
    }
    const int audio=audio_->currentData().toInt();
    if(audio>=0) duration=std::min(duration,(snapshot(audio).duration100ns-offsets_[audio])/10000000.0);
    if(duration<=0) {result.errors << QStringLiteral("视频对齐后没有共同的可导出时长。");return result;}
    QJsonObject spec{{"width",output.width()},{"height",output.height()},{"duration",duration},{"wipe",wipe},
        {"sources",sources},{"scaler",scaler_->currentIndex()},{"chroma",chromaAlgorithm_}};
    QFile helper(QStringLiteral(":/export/comparison-export.py"));
    if(!helper.open(QIODevice::ReadOnly)) {result.errors << helper.errorString();return result;}
    // JSON booleans are parsed rather than injected as Python literals.
    result.script = QStringLiteral("import json\nspec=json.loads(%1)\n").arg(VpyScriptBuilder::pythonString(QString::fromUtf8(QJsonDocument(spec).toJson(QJsonDocument::Compact))))+QString::fromUtf8(helper.readAll());
    return result;
}
bool AnalysisPage::exportBusy() const {return exportWindow_ && exportWindow_->isBusy();}
void AnalysisPage::stopExport() {if(exportWindow_)exportWindow_->stopAll();}
void AnalysisPage::exportComparison()
{
    const auto script=compositionScript();
    if(!script.errors.isEmpty()) {status_->setText(script.errors.join('\n'));return;}
    if(!exportWindow_) {
        exportWindow_=std::make_unique<ExportWindow>();
        connect(exportWindow_.get(),&ExportWindow::idle,this,&AnalysisPage::exportIdle);
        connect(exportWindow_.get(),&ExportWindow::statusMessage,status_,&QLabel::setText);
    }
    const int audio=audio_->currentData().toInt();
    exportWindow_->setCurrentSource(paths_[audio<0?masterVideo():audio]);
    QStringList inputs; for(int i=0;i<videoCount_;++i) inputs << paths_[i];
    const auto output=compositionSize();
    exportWindow_->setComposition(script.script,inputs,audio<0?0:offsets_[audio]/10000000.0,audio<0,
        QStringLiteral("当前对比快照：%1 路，输出 %2×%3（最大源视频分辨率）；保留布局、切割、缩放、偏移和音频选择，从全局时间 0 导出。")
            .arg(visibleVideos().size()).arg(output.width()).arg(output.height()),output);
    exportWindow_->showNormal(); exportWindow_->raise(); exportWindow_->activateWindow();
}

}
