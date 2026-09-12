#include "ui/CollapsibleSection.h"

#include <QToolButton>
#include <QVBoxLayout>

namespace vsr {

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *content, QWidget *parent)
    : QWidget(parent), content_(content)
{
    header_ = new QToolButton(this);
    header_->setText(title);
    header_->setCheckable(true);
    header_->setChecked(true);
    header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header_->setArrowType(Qt::DownArrow);
    header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    header_->setFixedHeight(32);
    header_->setStyleSheet(QStringLiteral(
        "QToolButton { border: 0; border-radius: 0; background: #eaeaea; font-weight: 600; text-align: left; padding: 0 8px; }"
        "QToolButton:hover { background: #dfdfdf; }"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header_);
    layout->addWidget(content_);

    connect(header_, &QToolButton::toggled, this, [this](bool checked) {
        content_->setVisible(checked);
        header_->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        emit expandedChanged(checked);
    });
}

bool CollapsibleSection::isExpanded() const { return header_->isChecked(); }
void CollapsibleSection::setExpanded(bool expanded) { header_->setChecked(expanded); }

}
