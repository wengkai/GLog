#include "AwardPluginManager.h"
#include <QFileDialog>
#include <QMessageBox>
#include "Concurrent.h"
#include "ui_AwardPluginManager.h"

AwardPluginManager::AwardPluginManager(QWidget *parent)
    : QDialog(parent), Invalid_Index(tr("Invalid index")),
      Invalid_Plugin(tr("Plugin is not valid")), Busy_Plugin(tr("Plugin is busy")),
      Plugin_Installed(tr("Successfully installed: %1")),
      Plugin_Uninstalled(tr("Successfully uninstalled: %1")) {
    ui = new Ui::AwardPluginManagerClass;
    ui->setupUi(this);
    m_model = new AwardPluginModel(this);
    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    connect(ui->loadButton, &QPushButton::clicked, this, &AwardPluginManager::loadAwardPlugin);
    connect(ui->removeButton, &QPushButton::clicked, this, &AwardPluginManager::removeAwardPlugin);
    connect(ui->installButton, &QPushButton::clicked, this,
            &AwardPluginManager::installAwardPlugin);
    connect(ui->uninstallButton, &QPushButton::clicked, this,
            &AwardPluginManager::uninstallAwardPlugin);
    connect(ui->closeButton, &QPushButton::clicked, this, &AwardPluginManager::reject);
}

AwardPluginManager::~AwardPluginManager() { delete ui; }

void AwardPluginManager::loadAwardPlugin() {
    auto pluginFile = QFileDialog::getOpenFileName(
        this, tr("Load Plugin"), "..", tr("Libraries (*.dll *.so* *.dylib);;All Files (*)"));

    if (pluginFile.isEmpty()) {
        return;
    }

    QFileInfo fi(pluginFile);
    if (!fi.exists()) {
        emit userCritical(tr("Error"), tr("File does not exist."));
        return;
    }

    QString canonicalPath = fi.canonicalFilePath();
    if (canonicalPath.isEmpty()) {
        emit userCritical(tr("Error"), tr("Cannot resolve canonical file path."));
        return;
    }
    bool duplicate = m_award_plugins.read([this, canonicalPath](const auto &m_award_plugins) {
        auto it = std::find_if(
            m_award_plugins.begin(), m_award_plugins.end(), [&](const LoadedAwardPlugin &p) {
                return QString::compare(p.filename, canonicalPath, Qt::CaseInsensitive) == 0;
            });

        return it != m_award_plugins.end();
    });

    if (duplicate) {
        emit userInformation(tr("Info"), tr("Plugin '%1' is already loaded.").arg(fi.baseName()));
        return;
    }

    auto myLib = std::make_unique<QLibrary>(canonicalPath);
    if (!myLib->load()) {
        emit userCritical(tr("Error"), tr("Could not load library: %1").arg(myLib->errorString()));
        return;
    }

    loadAwardPluginAfterConfirmed(LoadedAwardPlugin{canonicalPath, std::move(myLib)});
}

void AwardPluginManager::loadAwardPluginAfterConfirmed(LoadedAwardPlugin &&plugin) {
    m_model->loadAwardPlugin(std::move(plugin));
}

void AwardPluginManager::removeAwardPlugin() {
    const QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    if (QMessageBox::question(
            this, tr("Remove"),
            tr("Are you sure you want to remove this plugin? This will disable the plugin but keep "
               "the plugin data in the system (if already installed). You can reinstall the plugin "
               "later."),
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No) ==
        QMessageBox::StandardButton::No) {
        return;
    }

    removeAwardPluginAfterConfirmed(index);
}

void AwardPluginManager::removeAwardPluginAfterConfirmed(QModelIndex index) {
    if (!index.isValid()) {
        return;
    }
    m_model->removeAwardPlugin(index);
}

void AwardPluginManager::installAwardPlugin() {
    const QPersistentModelIndex index(ui->tableView->currentIndex());
    if (!index.isValid()) {
        return;
    }

    if (QMessageBox::question(
            this, tr("Install"),
            tr("Are you sure you want to install this plugin? This will enable the plugin and "
               "install the plugin data to the system. You can uninstall the plugin to remove the "
               "plugin data later."),
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No) ==
        QMessageBox::StandardButton::No) {
        return;
    }

    installAwardPluginAfterConfirmed(index);
}

void AwardPluginManager::installAwardPluginAfterConfirmed(QPersistentModelIndex index) {
    if (!index.isValid()) {
        return;
    }
    GLogConcurrent::makeFuture([weakThis = QPointer<AwardPluginManager>(this), index]() mutable {
        if (!weakThis) {
            return std::make_pair(false, QString{});
        }
        auto plugins = weakThis->internalGetAwardPlugins();
        if (index.row() < 0 || static_cast<size_t>(index.row()) >= plugins->size()) {
            return std::make_pair(false, weakThis->Invalid_Index);
        }
        const auto &plugin = plugins->at(index.row());
        if (!plugin.valid()) {
            return std::make_pair(false, weakThis->Invalid_Plugin);
        }
        if (plugin.status->exchange(LoadedAwardPlugin::Status::Pending) ==
            LoadedAwardPlugin::Status::Pending) {
            return std::make_pair(false, weakThis->Busy_Plugin);
        }
        weakThis->m_model->updateData(index);
        struct Finalizer {
            const LoadedAwardPlugin &plugin;
            LoadedAwardPlugin::Status status = LoadedAwardPlugin::Status::Disabled;
            ~Finalizer() { plugin.status->store(status); }
        } finalizer{plugin};
        auto result = plugin.install();
        uint64_t error_len = 0;
        QString hint;
        if (!result) {
            plugin.getLastError(nullptr, &error_len, 0);
            auto buf = std::make_unique<char[]>(error_len + 1);
            plugin.getLastError(buf.get(), &error_len, error_len + 1);
            hint = QString::fromUtf8(buf.get(), error_len);
        } else {
            if (weakThis) {
                hint = weakThis->Plugin_Installed.arg(plugin.filename);
            }
            finalizer.status = LoadedAwardPlugin::Status::Enabled;
        }
        return std::make_pair(result, hint);
    })
        .then(this,
              [this, index](const std::pair<bool, QString> &result) mutable {
                  m_model->updateData(index);
                  const auto &[flag, hint] = result;
                  if (!flag) {
                      emit userCritical(tr("Error"), hint);
                      return;
                  }
                  emit userInformation(tr("Installed"), hint);
              })
        .onFailed(this, [this, index](const std::exception &e) {
            m_model->updateData(index);
            emit userCritical(tr("Error"), QString::fromUtf8(e.what()));
        });
}

void AwardPluginManager::uninstallAwardPlugin() {
    const QPersistentModelIndex index(ui->tableView->currentIndex());
    if (!index.isValid()) {
        return;
    }

    if (QMessageBox::question(
            this, tr("Uninstall"),
            tr("Are you sure you want to uninstall this plugin? This will disable the plugin and "
               "remove the plugin data from the system. You can reinstall the plugin later. Note: "
               "if you just want to disable the plugin, you can simply remove the plugin from the "
               "list (maintains the plugin data)."),
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No) ==
        QMessageBox::StandardButton::No) {
        return;
    }

    uninstallAwardPluginAfterConfirmed(index);
}

void AwardPluginManager::uninstallAwardPluginAfterConfirmed(QPersistentModelIndex index) {
    if (!index.isValid()) {
        return;
    }
    GLogConcurrent::makeFuture([weakThis = QPointer<AwardPluginManager>(this), index]() mutable {
        if (!weakThis) {
            return std::make_pair(false, QString{});
        }
        auto plugins = weakThis->internalGetAwardPlugins();
        if (index.row() < 0 || static_cast<size_t>(index.row()) >= plugins->size()) {
            return std::make_pair(false, weakThis->Invalid_Index);
        }
        const auto &plugin = plugins->at(index.row());
        if (!plugin.valid()) {
            return std::make_pair(false, weakThis->Invalid_Plugin);
        }
        if (plugin.status->exchange(LoadedAwardPlugin::Status::Pending) ==
            LoadedAwardPlugin::Status::Pending) {
            return std::make_pair(false, weakThis->Busy_Plugin);
        }
        weakThis->m_model->updateData(index);
        struct Finalizer {
            const LoadedAwardPlugin &plugin;
            LoadedAwardPlugin::Status status = LoadedAwardPlugin::Status::Disabled;
            ~Finalizer() { plugin.status->store(status); }
        } finalizer{plugin};
        auto result = plugin.uninstall();
        uint64_t error_len = 0;
        QString hint;
        if (!result) {
            plugin.getLastError(nullptr, &error_len, 0);
            auto buf = std::make_unique<char[]>(error_len + 1);
            plugin.getLastError(buf.get(), &error_len, error_len + 1);
            hint = QString::fromUtf8(buf.get(), error_len);
        } else {
            if (weakThis) {
                hint = weakThis->Plugin_Uninstalled.arg(plugin.filename);
            }
        }
        return std::make_pair(result, hint);
    })
        .then(this,
              [this, index](const std::pair<bool, QString> &result) mutable {
                  m_model->updateData(index);
                  const auto &[flag, hint] = result;
                  if (!flag) {
                      emit userCritical(tr("Error"), hint);
                      return;
                  }
                  emit userInformation(tr("Uninstalled"), hint);
              })
        .onFailed(this, [this, index](const std::exception &e) {
            m_model->updateData(index);
            emit userCritical(tr("Error"), QString::fromUtf8(e.what()));
        });
}

QModelIndex AwardPluginManager::index(int row) const {
    return m_model->index(row, 0, QModelIndex());
}

void AwardPluginManager::lockListUi() {
    if (lockListCount.fetch_add(1) == 0) {
        GLogConcurrent::makeFuture(
            [this]() {
                ui->loadButton->setEnabled(false);
                ui->removeButton->setEnabled(false);
            },
            GLogConcurrent::MainThreadExecutor{});
    }
}

void AwardPluginManager::lockInstallUi() {
    if (lockInstallCount.fetch_add(1) == 0) {
        GLogConcurrent::makeFuture(
            [this]() {
                ui->installButton->setEnabled(false);
                ui->uninstallButton->setEnabled(false);
            },
            GLogConcurrent::MainThreadExecutor{});
    }
}

void AwardPluginManager::unlockListUi() {
    assert(lockListCount > 0);
    if (lockListCount.fetch_sub(1) == 1) {
        GLogConcurrent::makeFuture(
            [this]() {
                ui->loadButton->setEnabled(true);
                ui->removeButton->setEnabled(true);
            },
            GLogConcurrent::MainThreadExecutor{});
    }
}

void AwardPluginManager::unlockInstallUi() {
    assert(lockInstallCount > 0);
    if (lockInstallCount.fetch_sub(1) == 1) {
        GLogConcurrent::makeFuture(
            [this]() {
                ui->installButton->setEnabled(true);
                ui->uninstallButton->setEnabled(true);
            },
            GLogConcurrent::MainThreadExecutor{});
    }
}

auto AwardPluginManager::getPAwardPlugins() -> std::unique_ptr<IAwardPluginsProxy> {
    return std::make_unique<getAwardPluginsRet>(
        std::move(m_award_plugins.requireRead()),
        [manager = QPointer<AwardPluginManager>(this)]() {
            if (manager) {
                manager->lockInstallUi();
                manager->lockListUi();
            }
        },
        [manager = QPointer<AwardPluginManager>(this)]() {
            if (manager) {
                manager->unlockInstallUi();
                manager->unlockListUi();
            }
        });
}

auto AwardPluginManager::internalGetAwardPlugins() -> getAwardPluginsRet {
    return {std::move(m_award_plugins.requireRead()),
            [manager = QPointer<AwardPluginManager>(this)]() {
                if (manager) {
                    manager->lockListUi();
                }
            },
            [manager = QPointer<AwardPluginManager>(this)]() {
                if (manager) {
                    manager->unlockListUi();
                }
            }};
}

// ======== AwardPluginModel ========

AwardPluginModel::AwardPluginModel(AwardPluginManager *parent)
    : QAbstractTableModel(parent), m_manager(parent) {}

AwardPluginModel::~AwardPluginModel() = default;

auto AwardPluginModel::rowCount(const QModelIndex &parent) const -> int {
    return static_cast<int>(m_manager->m_plugin_count.load());
}

auto AwardPluginModel::columnCount(const QModelIndex &parent) const -> int { return COLUMN_COUNT; }

auto AwardPluginModel::data(const QModelIndex &index, int role) const -> QVariant {
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (!index.isValid()) {
        return {};
    }

    return m_manager->m_award_plugins.read([index](const auto &plugins) -> QVariant {
        int row = index.row();
        int column = index.column();
        if (row < 0 || row >= plugins.size()) {
            return {};
        }
        const auto &plugin = static_cast<const LoadedAwardPlugin &>(plugins[row]);
        switch (column) {
        case COLUMN_NAME:
            if (!plugin.valid()) {
                return tr("Invalid Plugin");
            }
            return QString::fromLatin1(static_cast<const char *>(plugin.pluginName()));
        case COLUMN_FILENAME:
            return plugin.filename;
        case COLUMN_STATUS:
            if (!plugin.valid()) {
                return tr("Invalid");
            }
            switch (plugin.status->load()) {
            case LoadedAwardPlugin::Status::Enabled:
                return tr("Enabled");
                break;

            case LoadedAwardPlugin::Status::Pending:
                return tr("Pending");
                break;

            default:
                break;
            }
            return tr("Disabled");
        default:
            return {};
        }
    });
}

auto AwardPluginModel::headerData(int section, Qt::Orientation orientation, int role) const
    -> QVariant {
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal) {
        switch (section) {
        case COLUMN_NAME:
            return tr("Plugin Name");
        case COLUMN_FILENAME:
            return tr("File Name");
        case COLUMN_STATUS:
            return tr("Status");
        default:
            return {};
        }
    }
    return section + 1;
}

void AwardPluginModel::loadAwardPlugin(LoadedAwardPlugin &&plugin) {
    LoadedAwardPlugin loadedAwardPlugin{std::move(plugin)};
    if (!loadedAwardPlugin.valid()) {
        auto error_msg = tr("Library is missing required export symbols.\n");
        for (const auto &msg : loadedAwardPlugin.errors) {
            error_msg += (msg + '\n');
        }
        emit m_manager->userCritical(tr("Error"), error_msg);
        return;
    }
    QString name = QString::fromLatin1(static_cast<const char *>(loadedAwardPlugin.pluginName()));

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    {
        auto m_award_plugins = m_manager->m_award_plugins.requireWrite();
        m_award_plugins->emplace_back(std::move(loadedAwardPlugin));
        m_manager->m_plugin_count++;
    }
    endInsertRows();

    emit m_manager->userInformation(tr("Loaded"), tr("Successfully loaded: %1").arg(name));
}

// void AwardPluginModel::loadAwardPlugin(QString i_filename, std::unique_ptr<QLibrary>
// i_plugin_lib) {
//     LoadedAwardPlugin loadedAwardPlugin{i_filename, std::move(i_plugin_lib)};
//     if (!loadedAwardPlugin.valid()) {
//         auto error_msg = tr("Library is missing required export symbols.\n");
//         for (const auto &msg : loadedAwardPlugin.errors) {
//             error_msg += (msg + '\n');
//         }
//         emit m_manager->userCritical(tr("Error"), error_msg);
//         return;
//     }
//     QString name = QString::fromLatin1(static_cast<const char
//     *>(loadedAwardPlugin.pluginName()));

//     beginInsertRows(QModelIndex(), rowCount(), rowCount());
//     {
//         auto m_award_plugins = m_manager->m_award_plugins.requireWrite();
//         m_award_plugins->emplace_back(std::move(loadedAwardPlugin));
//         m_manager->m_plugin_count++;
//     }
//     endInsertRows();

//     emit m_manager->userInformation(tr("Loaded"), tr("Successfully loaded: %1").arg(name));
// }

void AwardPluginModel::removeAwardPlugin(const QModelIndex &index) {
    if (rowCount() == 0) {
        return;
    }
    if (index.row() < 0 || index.row() >= rowCount()) {
        return;
    }

    auto plugin = LoadedAwardPlugin{};

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    {
        auto m_award_plugins = m_manager->m_award_plugins.requireWrite();
        auto where = m_award_plugins->begin() + static_cast<ptrdiff_t>(index.row());
        plugin = std::move(*where);
        m_award_plugins->erase(where);
        m_manager->m_plugin_count--;
    }
    endRemoveRows();

    emit m_manager->userInformation(tr("Removed"),
                                    tr("Successfully removed: %1").arg(plugin.filename));
}

void AwardPluginModel::updateData(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
    GLogConcurrent::makeFuture(
        [this, topLeft = topLeft, bottomRight = bottomRight]() {
            emit dataChanged(topLeft, bottomRight);
        },
        GLogConcurrent::MainThreadExecutor{});
}

void AwardPluginModel::updateData(const QModelIndex &rowIndex) {
    updateData(index(rowIndex.row(), COLUMN_NAME), index(rowIndex.row(), COLUMN_STATUS));
}
