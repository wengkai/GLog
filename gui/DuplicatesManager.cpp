#include "DuplicatesManager.h"
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <algorithm>
#include <cassert>
#include "Concurrent.h"
#include "adifdb.h"
#include "duplicatesmodel.h"
#include "ui_DuplicatesManager.h"

DuplicatesManager::DuplicatesManager(AdifModel *model, QWidget *parent)
    : QDialog(parent), m_model(model) {
    ui = new Ui::DuplicatesManagerClass;
    ui->setupUi(this);
    m_duplicatesModel = new DuplicatesModel(model, this);
    auto *header = ui->treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeView->setModel(m_duplicatesModel);
    ui->treeView->setHeaderHidden(true);
    ui->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->treeView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(ui->refreshButton, &QPushButton::clicked, this, &DuplicatesManager::RefreshDuplicates);
    connect(ui->removeMinorsButton, &QPushButton::clicked, this, &DuplicatesManager::RemoveMinors);
    connect(ui->mergeToMajorButton, &QPushButton::clicked, this, &DuplicatesManager::MergeToMajor);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto index = ui->treeView->indexAt(pos);
        if (!index.isValid()) return;

        QMenu menu(this);
        auto *setMajorAction = menu.addAction(tr("Set as major"));

        if (menu.exec(ui->treeView->viewport()->mapToGlobal(pos)) == setMajorAction) {
            m_duplicatesModel->setMajor(index);
        }
    });
}

DuplicatesManager::~DuplicatesManager() { delete ui; }

void DuplicatesManager::RefreshDuplicates() {
    auto fields = ui->fieldsEdit->text().split(',', Qt::SkipEmptyParts);
    for (auto &field : fields) {
        field = field.trimmed();
    }
    if (fields.isEmpty()) return;
    std::vector<std::string> fieldNames;
    fieldNames.reserve(fields.size());
    for (auto &field : fields) {
        fieldNames.push_back(field.toStdString());
    }
    auto duplicates = m_model->findDuplicates(fieldNames);
    m_duplicatesModel->build(duplicates);
    ui->treeView->expandAll();
}

void DuplicatesManager::RemoveMinors() {
    if (QMessageBox::question(this, tr("Confirm"),
                              tr("Are you sure to delete all minor records?")) != QMessageBox::Yes)
        return;

    auto minors = m_duplicatesModel->collectMinors();
    if (minors.empty()) return;
    ui->removeMinorsButton->setEnabled(false);
    m_model->deleteRowsAsync(std::move(minors)).then(this, [this](size_t count) {
        if (count == 0) return;
        QMessageBox::information(this, tr("Done"), tr("Deleted %1 rows").arg(count));
        RefreshDuplicates();
        ui->removeMinorsButton->setEnabled(true);
    });
}

void DuplicatesManager::MergeToMajor() {
    if (QMessageBox::question(this, tr("Confirm"), tr("Merge to major?")) != QMessageBox::Yes)
        return;

    auto groups = m_duplicatesModel->collectGroups();

    QList<QFuture<bool>> futures;

    for (auto &g : groups) {
        futures.append(m_model->mergeRowsAsync(g.major, g.minors));
    }
    ui->mergeToMajorButton->setEnabled(false);
    QtFuture::whenAll(futures.begin(), futures.end())
        .then(this, [this](QList<QFuture<bool>> results) {
            // RefreshDuplicates();
            assert(results.size() == m_duplicatesModel->groupCount());
            int ok = 0, fail = 0;
            for (size_t i = 0; i < results.size(); ++i) {
                if (results[i].result()) {
                    m_duplicatesModel->setGroupStatus(i, DuplicateNode::GroupStatus::Success);
                    m_duplicatesModel->updateGroupData(i);
                    ++ok;
                } else {
                    m_duplicatesModel->setGroupStatus(i, DuplicateNode::GroupStatus::Failed);
                    ++fail;
                }
            }

            QMessageBox::information(this, tr("Done"),
                                     tr("Merged %1 success, %2 failed").arg(ok).arg(fail));
            ui->mergeToMajorButton->setEnabled(true);
        });
}

int DuplicatesManager::exec() {
    RefreshDuplicates();
    return QDialog::exec();
}
