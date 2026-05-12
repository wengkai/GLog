#ifndef DUPLICATESMODEL_H
#define DUPLICATESMODEL_H

#include <QAbstractItemModel>
#include <QPersistentModelIndex>
#include <memory>
#include <vector>

#include "app_export.h"

class AdifModel;

struct DuplicateNode {
    bool isGroup = false;
    bool isMajor = false;

    enum class GroupStatus {
        None,
        Success,
        Failed,
    };

    GroupStatus status = GroupStatus::None;
    size_t groupIndex;

    QPersistentModelIndex m_index;

    DuplicateNode *parent = nullptr;
    std::vector<std::unique_ptr<DuplicateNode>> children;
};

class GLOGKIT_API DuplicatesModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    explicit DuplicatesModel(AdifModel *model, QObject *parent = nullptr);
    ~DuplicatesModel();

    void build(const std::vector<std::vector<QPersistentModelIndex>> &duplicates);

    QModelIndex index(int row, int col, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &) const override { return 1; }
    QVariant data(const QModelIndex &idx, int role) const override;

    void setMajor(const QModelIndex &idx);

    QModelIndexList collectMinors() const;

    struct MergeGroup {
        QPersistentModelIndex major;
        QModelIndexList minors;
    };
    std::vector<MergeGroup> collectGroups() const;
    size_t groupCount() const;
    void updateGroupData(size_t groupIndex);
    void setGroupStatus(size_t groupIndex, DuplicateNode::GroupStatus status);
    void beginUpdateAll();
    void endUpdateAll();

  private:
    DuplicateNode *nodeFromIndex(const QModelIndex &idx) const;

  private:
    AdifModel *m_model;
    std::unique_ptr<DuplicateNode> m_root;
};

#endif