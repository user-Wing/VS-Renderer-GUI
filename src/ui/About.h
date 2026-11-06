#pragma once
#include <QLabel>

namespace vsr {
inline QLabel *aboutLabel(const QString &program, QWidget *parent = nullptr)
{
    auto *label = new QLabel(QStringLiteral("%1\n%2\n\nthe VS-GUI Project (MIT)\n作者：user-Wing")
                            .arg(program, QStringLiteral(VSR_VERSION)), parent);
    label->setText(QStringLiteral("%1 %2<br><br>the VS-GUI Project (MIT)<br>作者：<a href=\"https://github.com/user-Wing/VS-Renderer-GUI\">user-Wing (ARXChem)</a><br><br>友链：<a href=\"https://github.com/Lake1059/FFF_Project\">3FPlayer 内核</a> · <a href=\"https://www1.arxchem.top\">博客教程</a> · <a href=\"https://github.com/lake1059/ffmpegfreeui\">3FUI</a> · <a href=\"https://github.com/maxzrb/VideoEnhancer\">VideoEnhancer</a>").arg(program.toHtmlEscaped(),QStringLiteral(VSR_VERSION)));
    label->setOpenExternalLinks(true);
    label->setObjectName("aboutProject");
    label->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
    label->setWordWrap(true);
    return label;
}
}
