#ifndef DROPABLETABLEVIEW_H
#define DROPABLETABLEVIEW_H

#include <QItemSelection>
#include <QMimeData>
#include <QTableView>
#include "GHeaderView.h"
#include "app_export.h"

class AdifModel;

class GLOGKIT_API DropAbleTableView : public QTableView {
    Q_OBJECT

  private:
    GHeaderView *headerview;
    AdifModel *m_model;

  public:
    explicit DropAbleTableView(AdifModel *model, QWidget *parent = nullptr);
    void setAdifModel(AdifModel *model);
    void tryDeleteSelectedRows();
    void tryNewSelectedRowsView();
    void tryPasteRows();
    void tryCopySelectedRows();
    GHeaderView *getHeaderView();

  public slots:
    void customContextMenu(const QPoint &pos);
    void findNext(const QString &key, const QString &value, bool isReg);
    void foundNext(QModelIndex index);
    void selectRows(const QList<int> &rows);
    void deselectRows(const QList<int> &rows);

  signals:
    void findNextSignal(QModelIndex current, QString key, QString value, bool isReg);
    void selected();
    void userInformation(const QString &title, const QString &text);

  protected:
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

  public:
    void proceedDeleteSelectedRows(const QModelIndexList &rows);
    void proceedNewViewWithRows(const QModelIndexList &rows);
    void proceedPasteRows(const QMimeData *mimeData);

  private:
    void applyRowRangeSelectionAsync(const QList<int> &rows,
                                     QItemSelectionModel::SelectionFlags mergeFlags);
    void applySelectionOnGuiThread(QItemSelection selection,
                                   QItemSelectionModel::SelectionFlag command);
};

#endif