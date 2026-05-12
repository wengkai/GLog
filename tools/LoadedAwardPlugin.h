#ifndef LOADED_AWARD_PLUGIN_H
#define LOADED_AWARD_PLUGIN_H

#include <QLibrary>
#include <QList>
#include <QString>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "IGRecord.h"

struct LoadedAwardPlugin {
    using AwardPluginPluginName = const char *(*)();
    using AwardPluginInstallF = bool (*)();
    using AwardPluginUninstallF = bool (*)();
    using AwardPluginBeforeEvaluateF = bool (*)();
    using AwardPluginAfterEvaluateF = bool (*)();
    using AwardPluginEvaluateF = bool (*)(const IGRecord *QSO, IGRecordGetValueByField callback);
    using AwardPluginGetResultF = void (*)(char *result_buf, uint64_t *result_len,
                                           uint64_t max_result_len);
    using AwardPluginGetLastErrorF = void (*)(char *result_buf, uint64_t *result_len,
                                              uint64_t max_result_len);

    QString filename;
    QList<QString> errors;
    std::unique_ptr<QLibrary> plugin_lib;
    AwardPluginPluginName pluginName = nullptr;
    AwardPluginInstallF install = nullptr;
    AwardPluginUninstallF uninstall = nullptr;
    AwardPluginBeforeEvaluateF beforeEvaluate = nullptr;
    AwardPluginAfterEvaluateF afterEvaluate = nullptr;
    AwardPluginEvaluateF evaluate = nullptr;
    AwardPluginGetResultF getResult = nullptr;
    AwardPluginGetLastErrorF getLastError = nullptr;

    enum class Status {
        Enabled,
        Pending,
        Disabled,
    };
    std::unique_ptr<std::atomic<Status>> status =
        std::make_unique<std::atomic<Status>>(Status::Disabled);

    bool valid() const {
        return pluginName && install && uninstall && beforeEvaluate && afterEvaluate && evaluate &&
               getResult && getLastError;
    }
    LoadedAwardPlugin() = default;
    LoadedAwardPlugin(QString i_filename, std::unique_ptr<QLibrary> i_plugin_lib)
        : filename(std::move(i_filename)), plugin_lib(std::move(i_plugin_lib)), pluginName(nullptr),
          install(nullptr), uninstall(nullptr), beforeEvaluate(nullptr), afterEvaluate(nullptr),
          evaluate(nullptr), getResult(nullptr), getLastError(nullptr) {
        tryResolve();
    }
    LoadedAwardPlugin(LoadedAwardPlugin &&) = default;
    LoadedAwardPlugin &operator=(LoadedAwardPlugin &&) = default;

#define ResolvePluginFunctionByName(function)                                                      \
    do {                                                                                           \
        if (!function) {                                                                           \
            function = reinterpret_cast<decltype(function)>(plugin_lib->resolve(#function));       \
            if (!function) {                                                                       \
                errors.append(QStringLiteral("Failed to resolve symbol: %1").arg(#function));      \
            }                                                                                      \
        }                                                                                          \
    } while (false);

    bool tryResolve() {
        if (!plugin_lib || !plugin_lib->isLoaded()) {
            return false;
        }
        errors.clear();
        ResolvePluginFunctionByName(pluginName);
        ResolvePluginFunctionByName(install);
        ResolvePluginFunctionByName(uninstall);
        ResolvePluginFunctionByName(beforeEvaluate);
        ResolvePluginFunctionByName(afterEvaluate);
        ResolvePluginFunctionByName(evaluate);
        ResolvePluginFunctionByName(getResult);
        ResolvePluginFunctionByName(getLastError);
        return valid();
    }
#undef ResolvePluginFunctionByName
};

class IAwardPluginsProxy {
    std::function<void()> m_initializer;
    std::function<void()> m_finalizer;

  public:
    IAwardPluginsProxy(
        std::function<void()> &&initializer = []() {}, std::function<void()> &&finalizer = []() {})
        : m_initializer(initializer), m_finalizer(finalizer) {
        m_initializer();
    };
    virtual ~IAwardPluginsProxy() { m_finalizer(); };
    virtual const std::vector<LoadedAwardPlugin> *operator->() const = 0;
    virtual const std::vector<LoadedAwardPlugin> &operator*() const = 0;
};

class IAwardPluginsManager {
  public:
    virtual std::unique_ptr<IAwardPluginsProxy> getPAwardPlugins() = 0;
    virtual ~IAwardPluginsManager() = default;
};

#endif
