#include "MultiLineDelegate.h"

MultiLineDelegate::MultiLineDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}

QWidget *MultiLineDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return new QPlainTextEdit(parent);
}

void MultiLineDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qobject_cast<QPlainTextEdit*>(editor)->setPlainText(index.data().toString());
}

void MultiLineDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    model->setData(index, qobject_cast<QPlainTextEdit*>(editor)->toPlainText());
}
