#include "duplicatesmodel.h"
#include <QBrush>
#include <QColor>
#include "adifdb.h"

DuplicatesModel::DuplicatesModel(AdifModel *model, QObject *parent)
    : QAbstractItemModel(parent), m_model(model) {
    m_root = std::make_unique<DuplicateNode>();
    m_root->isGroup = true;
}

DuplicatesModel::~DuplicatesModel() = default;

auto DuplicatesModel::nodeFromIndex(const QModelIndex &idx) const -> DuplicateNode * {
    if (!idx.isValid()) return m_root.get();
    return static_cast<DuplicateNode *>(idx.internalPointer());
}

void DuplicatesModel::build(const std::vector<std::vector<QPersistentModelIndex>> &duplicates) {
    beginResetModel();

    m_root->children.clear();
    size_t groupIndex = 1;

    for (auto &group : duplicates) {
        auto groupNode = std::make_unique<DuplicateNode>();
        groupNode->isGroup = true;
        groupNode->parent = m_root.get();
        groupNode->status = DuplicateNode::GroupStatus::None;
        groupNode->groupIndex = groupIndex++;

        bool first = true;

        for (auto &index : group) {
            if (!index.isValid()) continue;

            auto child = std::make_unique<DuplicateNode>();
            child->isGroup = false;
            child->m_index = index;
            child->isMajor = first;
            child->parent = groupNode.get();

            first = false;
            groupNode->children.push_back(std::move(child));
        }

        if (!groupNode->children.empty()) m_root->children.push_back(std::move(groupNode));
    }

    endResetModel();
}

auto DuplicatesModel::index(int row, int col, const QModelIndex &parent) const -> QModelIndex {
    auto *parentNode = nodeFromIndex(parent);

    if (row >= (int)parentNode->children.size()) return {};

    return createIndex(row, col, parentNode->children[row].get());
}

auto DuplicatesModel::parent(const QModelIndex &child) const -> QModelIndex {
    if (!child.isValid()) return {};

    auto *node = nodeFromIndex(child);
    auto *parent = node->parent;

    if (!parent || parent == m_root.get()) return {};

    auto *grand = parent->parent;
    int row = 0;

    for (int i = 0; i < (int)grand->children.size(); ++i) {
        if (grand->children[i].get() == parent) {
            row = i;
            break;
        }
    }

    return createIndex(row, 0, parent);
}

auto DuplicatesModel::rowCount(const QModelIndex &parent) const -> int {
    return nodeFromIndex(parent)->children.size();
}

auto DuplicatesModel::data(const QModelIndex &idx, int role) const -> QVariant {
    if (!idx.isValid()) return {};

    auto *node = nodeFromIndex(idx);

    if (role == Qt::DisplayRole) {
        if (node->isGroup) {
            return tr("Group %1").arg(node->groupIndex);
        } else {
            return QString::fromStdString(m_model->records2StdString({node->m_index.row()}));
        }
    }

    if (role == Qt::BackgroundRole && !node->isGroup) {
        if (node->isMajor)
            return QBrush(QColor(0, 120, 215, 60));
        else
            return QBrush(QColor(240, 240, 240));
    }

    if (role == Qt::BackgroundRole && node->isGroup) {
        if (node->status == DuplicateNode::GroupStatus::Success)
            return QBrush(QColor(0, 255, 0, 60));
        else if (node->status == DuplicateNode::GroupStatus::Failed)
            return QBrush(QColor(255, 0, 0, 60));
    }

    return {};
}

void DuplicatesModel::setMajor(const QModelIndex &idx) {
    auto *node = nodeFromIndex(idx);
    if (node->isGroup) return;

    auto *n_parent = node->parent;

    for (auto &child : n_parent->children)
        child->isMajor = false;

    node->isMajor = true;

    emit dataChanged(index(0, 0, idx.parent()),
                     index(n_parent->children.size() - 1, 0, idx.parent()));
}

auto DuplicatesModel::collectMinors() const -> QModelIndexList {
    QModelIndexList result;

    for (auto &group : m_root->children) {
        for (auto &child : group->children) {
            if (!child->isMajor) result.push_back(child->m_index);
        }
    }
    return result;
}

auto DuplicatesModel::collectGroups() const -> std::vector<MergeGroup> {
    std::vector<MergeGroup> result;

    for (auto &group : m_root->children) {
        MergeGroup mg;

        for (auto &child : group->children) {
            if (child->isMajor)
                mg.major = child->m_index;
            else
                mg.minors.push_back(child->m_index);
        }

        if (mg.major.isValid() && !mg.minors.empty()) result.push_back(std::move(mg));
    }

    return result;
}

auto DuplicatesModel::groupCount() const -> size_t { return m_root->children.size(); }

void DuplicatesModel::updateGroupData(size_t groupIndex) {
    if (groupIndex >= m_root->children.size()) return;
    auto g_idx = index(groupIndex, 0, QModelIndex());
    auto g_size = nodeFromIndex(g_idx)->children.size();
    auto topleft = index(0, 0, g_idx);
    auto bottomright = index(g_size - 1, 0, g_idx);
    emit dataChanged(topleft, bottomright);
}

void DuplicatesModel::setGroupStatus(size_t groupIndex, DuplicateNode::GroupStatus status) {
    if (groupIndex >= m_root->children.size()) return;
    m_root->children[groupIndex]->status = status;
    auto idx = index(groupIndex, 0, QModelIndex());
    emit dataChanged(idx, idx);
}
