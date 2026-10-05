#include "player/PlayerWindow.h"
#include "backend/ThreeFpPlayer.h"
#include <QSettings>
#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QScrollArea>
#include <QTimer>

namespace vsr {
void PlayerWindow::applyColorSettings() {
    auto c=VsrColorDefaultSettings();
    const auto integer=[&](const char* key,uint32_t fallback,uint32_t maximum){return qBound(0,settings_->value(QString("color/")+key,fallback).toInt(),int(maximum));};
    c.engine=integer("engine",0,1);c.output=integer("output",0,2);c.tone=integer("tone",0,5);
    c.gamut=integer("gamut",0,4);c.quality=integer("quality",1,1);c.icc=integer("icc",1,2);
    c.peakDetect=settings_->value("color/peakDetect",true).toBool();c.dither=settings_->value("color/dither",true).toBool();
    c.inverseTone=settings_->value("color/inverseTone",false).toBool();
    c.sdrPeak=qBound(1.0,settings_->value("color/sdrPeak",100).toDouble(),10000.0);
    c.displayPeak=qBound(0.0,settings_->value("color/displayPeak",0).toDouble(),10000.0);
    c.paperWhite=qBound(1.0,settings_->value("color/paperWhite",203).toDouble(),10000.0);
    c.contrastRecovery=qBound(0.0,settings_->value("color/contrastRecovery",0).toDouble(),1.0);
    qstrncpy(c.iccPath,settings_->value("color/iccPath").toString().toUtf8().constData(),sizeof(c.iccPath));
    qstrncpy(c.lutPath,settings_->value("color/lutPath").toString().toUtf8().constData(),sizeof(c.lutPath));
    clock_->setColorSettings(c);output_->setColorSettings(c);
}

void PlayerWindow::showColorSettings() {
    QDialog dialog(this);dialog.setObjectName("playerColorSettings");dialog.setWindowTitle(tr("3FP 解码配置 · 色彩管理"));dialog.resize(720,650);
    auto *outer=new QVBoxLayout(&dialog);auto *scroll=new QScrollArea(&dialog);scroll->setWidgetResizable(true);auto *content=new QWidget(scroll);scroll->setWidget(content);outer->addWidget(scroll,1);auto *form=new QFormLayout(content);form->setVerticalSpacing(10);
    auto *engine=new QComboBox(&dialog);engine->setObjectName("colorEngine");
    engine->addItems({tr("默认：3FP 原生(709 / 2020 简易模式)"),tr("自定义：libplacebo 高级色彩管理")});
    engine->setCurrentIndex(settings_->value("color/engine",0).toInt()==1?1:0);form->addRow(tr("色彩引擎"),engine);
    auto *explanation=new QLabel(tr("高级模式只处理解码 / VS 输出后的显示色彩，不修改 VPY、滤镜顺序或 VS 输出。\n默认自动映射目标显示器，自动色域映射、动态峰值检测与末端抖动；失败继续使用原生输出。madVR 模式由 madVR 管理颜色。"),&dialog);
    explanation->setWordWrap(true);form->addRow(explanation);
    auto *advanced=new QWidget(&dialog);auto *a=new QFormLayout(advanced);a->setContentsMargins(0,0,0,0);a->setVerticalSpacing(10);form->addRow(advanced);
    const auto combo=[&](const char* key,const QString& label,const QStringList& options,int fallback){auto *w=new QComboBox(advanced);w->setObjectName(QString("color_")+key);w->addItems(options);w->setCurrentIndex(qBound(0,settings_->value(QString("color/")+key,fallback).toInt(),int(options.size())-1));a->addRow(label,w);return w;};
    auto *output=combo("output",tr("目标输出"),{tr("自动：HDR 源 + HDR 显示器输出 scRGB，否则 SDR"),tr("始终 SDR"),tr("强制 HDR scRGB(测试；需 HDR 环境)")},0);
    auto *tone=combo("tone",tr("色调映射"),{tr("自动(SDR 原值 / Spline / HDR10+ ST2094-40)"),"Spline","ST2094-40","BT.2390",tr("Clip(对照测试)"),tr("Linear(对照测试)")},0);
    auto *gamut=combo("gamut",tr("色域映射"),{tr("自动(HDR / 广色域感知；普通 SDR 相对色度)"),tr("感知"),"Softclip","Relative",tr("Clip(对照测试)")},0);
    auto *quality=combo("quality",tr("色彩质量"),{tr("均衡"),tr("高质量(默认)")},1);
    const auto number=[&](const char* key,const QString& label,double value,double min,double max,int decimals){auto *w=new QDoubleSpinBox(advanced);w->setObjectName(QString("color_")+key);w->setRange(min,max);w->setDecimals(decimals);w->setValue(settings_->value(QString("color/")+key,value).toDouble());a->addRow(label,w);return w;};
    auto *sdr=number("sdrPeak",tr("SDR 参考峰值(nit)"),100,1,10000,0);
    auto *peak=number("displayPeak",tr("显示峰值(nit；0 自动检测)"),0,0,10000,0);
    auto *white=number("paperWhite",tr("字幕参考白(nit)"),203,1,10000,0);
    auto *contrast=number("contrastRecovery",tr("对比恢复(0 关闭)"),0,0,1,2);
    const auto check=[&](const char* key,const QString& label,bool value){auto *w=new QCheckBox(label,advanced);w->setObjectName(QString("color_")+key);w->setChecked(settings_->value(QString("color/")+key,value).toBool());a->addRow(w);return w;};
    auto *detect=check("peakDetect",tr("动态峰值检测(正常播放帧；暂停复用色彩结果)"),true);
    auto *dither=check("dither",tr("整数输出末端抖动(FP16 HDR 不额外抖动)"),true);
    auto *inverse=check("inverseTone",tr("逆色调映射(默认关闭)"),false);
    auto *icc=combo("icc",tr("显示 ICC"),{tr("关闭：标准目标色域"),tr("自动：当前显示器系统 ICC"),tr("自定义 ICC")},1);
    const auto path=[&](const char* key,const QString& label,const QString& filter){auto *row=new QWidget(advanced);auto *layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto *w=new QLineEdit(settings_->value(QString("color/")+key).toString(),row);w->setObjectName(QString("color_")+key);auto *button=new QPushButton(tr("浏览…"),row);layout->addWidget(w,1);layout->addWidget(button);a->addRow(label,row);connect(button,&QPushButton::clicked,&dialog,[&,w,filter]{const auto name=QFileDialog::getOpenFileName(&dialog,tr("选择色彩配置"),w->text(),filter);if(!name.isEmpty())w->setText(name);});return w;};
    auto *iccPath=path("iccPath",tr("ICC 文件"),"ICC (*.icc *.icm)");
    auto *lutPath=path("lutPath",tr("输出 3D LUT(SDR .cube)"),"Cube LUT (*.cube)");
    auto *limits=new QLabel(tr("HDR scRGB 跳过 SDR ICC / .cube，按显示器 HDR 色域映射。Dolby Vision 仅在可用元数据支持的情况下重塑；FEL 不重建。HDR Vivid 当前只映射 PQ / HLG 底层，未执行其动态曲线。"),advanced);limits->setWordWrap(true);a->addRow(limits);
    auto *status=new QLabel(&dialog);status->setWordWrap(true);outer->addWidget(status);
    const auto refresh=[&]{const auto s=(direct_?clock_:output_)->colorStatus();status->setText(tr("当前引擎：%1\n%2").arg(QString::fromUtf8(s.engine),QString::fromUtf8(s.fallback)));};refresh();
    advanced->setEnabled(engine->currentIndex()==1);connect(engine,&QComboBox::currentIndexChanged,advanced,[advanced](int value){advanced->setEnabled(value==1);});
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply,&dialog);outer->addWidget(buttons);
    const auto store=[&]{
        const auto save=[&](const char* key,const QVariant& value){settings_->setValue(QString("color/")+key,value);};
        save("engine",engine->currentIndex());save("output",output->currentIndex());save("tone",tone->currentIndex());save("gamut",gamut->currentIndex());save("quality",quality->currentIndex());
        save("sdrPeak",sdr->value());save("displayPeak",peak->value());save("paperWhite",white->value());save("contrastRecovery",contrast->value());save("peakDetect",detect->isChecked());save("dither",dither->isChecked());save("inverseTone",inverse->isChecked());
        save("icc",icc->currentIndex());save("iccPath",iccPath->text());save("lutPath",lutPath->text());settings_->sync();applyColorSettings();refresh();QTimer::singleShot(100,&dialog,refresh);
    };
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,store);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{store();dialog.accept();});connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.exec();
}
}
