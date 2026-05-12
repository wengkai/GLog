#include "MultiLineDelegate.h"

MultiLineDelegate::MultiLineDelegate(QWidget *parent) : QStyledItemDelegate(parent) {}

auto MultiLineDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const -> QWidget * {
    auto *editor = new QPlainTextEdit(parent);
    editor->installEventFilter(const_cast<MultiLineDelegate *>(this));
    return editor;
}

void MultiLineDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    qobject_cast<QPlainTextEdit *>(editor)->setPlainText(index.data().toString());
}

void MultiLineDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                     const QModelIndex &index) const {
    model->setData(index, qobject_cast<QPlainTextEdit *>(editor)->toPlainText());
}

auto MultiLineDelegate::eventFilter(QObject *obj, QEvent *event) -> bool {
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Check if modifiers like Shift or Ctrl are pressed
            // If they are, we allow the new line. If not, we commit.
            if (!(keyEvent->modifiers() & Qt::ShiftModifier) &&
                !(keyEvent->modifiers() & Qt::ControlModifier)) {

                auto *editor = qobject_cast<QPlainTextEdit *>(obj);
                if (editor != nullptr) {
                    emit commitData(editor);
                    emit closeEditor(editor);
                }
                return true; // Event handled
            }
        }
    }
    return QStyledItemDelegate::eventFilter(obj, event);
}
