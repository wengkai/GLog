#ifndef MULTILINEDELEGATE_H
#define MULTILINEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPlainTextEdit>

class MultiLineDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	explicit MultiLineDelegate(QWidget *parent = NULL);

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif