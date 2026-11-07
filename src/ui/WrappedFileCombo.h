#pragma once
#include <QComboBox>
#include <QtMath>
#include <QListView>
#include <QPainter>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QTextLayout>
namespace vsr {
class FileNameDelegate final : public QStyledItemDelegate {
    QListView *view_;
    static qreal layoutText(QTextLayout &layout,int width){
        QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(option);layout.beginLayout();qreal height=0;
        while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(40,width));line.setPosition(QPointF(0,height));height+=line.height();}layout.endLayout();return height;
    }
public:
    explicit FileNameDelegate(QListView *view):QStyledItemDelegate(view),view_(view){}
    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const override{
        QTextLayout text(index.data().toString(),option.font);return QSize(0,qCeil(layoutText(text,qMax(view_->viewport()->width(),view_->minimumWidth()-24)-16))+12);
    }
    void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index) const override{
        QStyleOptionViewItem background(option);initStyleOption(&background,index);background.text.clear();if(option.state&QStyle::State_Selected)p->fillRect(option.rect,option.palette.color(QPalette::Highlight));else view_->style()->drawControl(QStyle::CE_ItemViewItem,&background,p,view_);
        QTextLayout text(index.data().toString(),option.font);layoutText(text,option.rect.width()-16);p->save();p->setClipRect(option.rect);p->setPen(option.palette.color(option.state&QStyle::State_Selected?QPalette::HighlightedText:QPalette::Text));text.draw(p,option.rect.topLeft()+QPoint(8,6));p->restore();
    }
};
class WrappedFileCombo final : public QComboBox {
public:
    explicit WrappedFileCombo(QWidget *parent):QComboBox(parent){auto *list=new QListView;setView(list);list->setWordWrap(true);list->setTextElideMode(Qt::ElideNone);list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list->setUniformItemSizes(false);list->setItemDelegate(new FileNameDelegate(list));setMaxVisibleItems(8);connect(this,&QComboBox::currentTextChanged,this,&QWidget::setToolTip);}
    void showPopup() override{view()->setMinimumWidth(qMin(720,screen()->availableGeometry().width()-40));QComboBox::showPopup();}
};
}
