#pragma once
#include <QLabel>

namespace vsr {
inline QLabel *aboutLabel(const QString &program, QWidget *parent = nullptr)
{
    auto *label = new QLabel(QStringLiteral("%1\n%2\n\nthe VS-GUI Project (MIT)\n作者：user-Wing")
                            .arg(program, QStringLiteral(VSR_VERSION)), parent);
    if(program=="VS Player"){
        label->setText(QStringLiteral("%1 %2<br><br>the VS-GUI Project (MIT)<br>作者：<a href=\"https://github.com/user-Wing/VS-Renderer-GUI\">user-Wing (ARXChem)</a>").arg(program.toHtmlEscaped(),QStringLiteral(VSR_VERSION)));
        label->setOpenExternalLinks(true);
    }
    label->setObjectName("aboutProject");
    label->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
    label->setWordWrap(true);
    return label;
}
}
