#ifndef AWARDPLUGINMANAGER_H
#define AWARDPLUGINMANAGER_H

#include <QAbstractTableModel>
#include <QDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QPointer>
#include <functional>
#include <memory>
#include <vector>

#include "LoadedAwardPlugin.h"
#include "Synchronize.h"
#include "app_export.h"

namespace Ui {
class AwardPluginManagerClass;
};

class AwardPluginModel;

class GLOGKIT_API AwardPluginManager : public QDialog, public IAwardPluginsManager {
    Q_OBJECT

    friend class AwardPluginModel;

    Ui::AwardPluginManagerClass *ui;
    std::atomic<size_t> lockListCount = 0;
    std::atomic<size_t> lockInstallCount = 0;
    AwardPluginModel *m_model = nullptr;
    Synchronized<std::vector<LoadedAwardPlugin>> m_award_plugins;
    std::atomic<size_t> m_plugin_count = 0;

  public:
    AwardPluginManager(QWidget *parent = nullptr);
    virtual ~AwardPluginManager();

    class getAwardPluginsRet : public IAwardPluginsProxy {
        decltype(m_award_plugins.requireRead()) m_proxy;

      public:
        getAwardPluginsRet(decltype(m_proxy) proxy, std::function<void()> &&initializer,
                           std::function<void()> &&finalizer)
            : m_proxy(std::move(proxy)),
              IAwardPluginsProxy(std::move(initializer), std::move(finalizer)){};
        virtual ~getAwardPluginsRet() = default;
        virtual const std::vector<LoadedAwardPlugin> *operator->() const override {
            return m_proxy.operator->();
        }
        virtual const std::vector<LoadedAwardPlugin> &operator*() const override {
            return m_proxy.operator*();
        }
    };

    virtual std::unique_ptr<IAwardPluginsProxy> getPAwardPlugins() override;

    // ==== no question box api ====

    void loadAwardPluginAfterConfirmed(LoadedAwardPlugin &&plugin);
    void removeAwardPluginAfterConfirmed(QModelIndex index);
    void installAwardPluginAfterConfirmed(QPersistentModelIndex index);
    void uninstallAwardPluginAfterConfirmed(QPersistentModelIndex index);

    // ==== api for testing ====

    QModelIndex index(int row) const;
    size_t rowCount() const { return m_plugin_count; }

  public slots:
    // ==== Modify List ====

    void loadAwardPlugin();
    void removeAwardPlugin();

    // ==== Read List ====

    void installAwardPlugin();
    void uninstallAwardPlugin();

  signals:
    void userInformation(const QString &title, const QString &text,
                         QMessageBox::StandardButton button0 = QMessageBox::Ok,
                         QMessageBox::StandardButton button1 = QMessageBox::NoButton);
    void userCritical(const QString &title, const QString &text,
                      QMessageBox::StandardButton button0 = QMessageBox::Ok,
                      QMessageBox::StandardButton button1 = QMessageBox::NoButton);

  private:
    void lockListUi();
    void lockInstallUi();
    void unlockListUi();
    void unlockInstallUi();

    const QString Invalid_Index;
    const QString Invalid_Plugin;
    const QString Busy_Plugin;
    const QString Plugin_Installed;
    const QString Plugin_Uninstalled;

    [[nodiscard]] getAwardPluginsRet internalGetAwardPlugins();
};

class AwardPluginModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    AwardPluginModel(AwardPluginManager *parent = nullptr);
    ~AwardPluginModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // 1: Plugin Name, 2: File Name, 3: Status
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // functional methods
    // void loadAwardPlugin(QString i_filename, std::unique_ptr<QLibrary> i_plugin_lib);
    void loadAwardPlugin(LoadedAwardPlugin &&plugin);
    void removeAwardPlugin(const QModelIndex &index);

    void updateData(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void updateData(const QModelIndex &rowIndex);

  private:
    AwardPluginManager *m_manager = nullptr;
    static constexpr int COLUMN_COUNT = 3;
    static constexpr int COLUMN_NAME = 0;
    static constexpr int COLUMN_FILENAME = 1;
    static constexpr int COLUMN_STATUS = 2;
};

#endif