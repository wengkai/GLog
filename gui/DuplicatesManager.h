#ifndef DUPLICATESMANAGER_H
#define DUPLICATESMANAGER_H

#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QPersistentModelIndex>
#include <QTreeWidgetItem>
#include <unordered_map>
#include <vector>

class AdifModel;
class DuplicatesModel;

namespace Ui {
class DuplicatesManagerClass;
}

class DuplicatesManager : public QDialog {
    Q_OBJECT

  public:
    explicit DuplicatesManager(AdifModel *model, QWidget *parent = nullptr);
    ~DuplicatesManager();

    const QBrush kMajorBrush{QBrush(QColor(0, 120, 215, 60))};
    const QBrush kMinorBrush{QBrush(QColor(240, 240, 240, 255))};
    const QBrush kSuccessGroupBrush{QBrush(QColor(0, 255, 0, 60))};
    const QBrush kFailedGroupBrush{QBrush(QColor(255, 0, 0, 60))};

  public slots:
    void RefreshDuplicates();
    void RemoveMinors();
    void MergeToMajor();
    virtual int exec() override;

  private:
    Ui::DuplicatesManagerClass *ui;
    AdifModel *m_model;
    DuplicatesModel *m_duplicatesModel;
    std::unordered_map<QTreeWidgetItem *, QPersistentModelIndex> m_itemToIndex;
};

#endif
