#ifndef AWARD_PLUGIN_H
#define AWARD_PLUGIN_H

#include <GLogKit/app_export.h>

#include <cstdint>

/**
 * @class AwardPlugin
 *
 * @note ThreadSafety:
 * This class is not designed for concurrent
 * multi-threaded access to a single instance.
 */
class APP_EXPORT AwardPlugin {
  public:
    virtual ~AwardPlugin();
    virtual const char *pluginName() const noexcept = 0;
    // Prepare persistent data (init database). Return
    // true if the data is already prepared
    virtual bool install() noexcept = 0;
    // Clean up all the persistent data
    virtual bool uninstall() noexcept = 0;
    // Prepare for batch evaluation
    virtual bool beforeEvaluate() noexcept = 0;
    // Clean up after batch evaluation
    virtual bool afterEvaluate() noexcept = 0;
    virtual bool evaluate(const char *QSO, unsigned long long len) noexcept = 0;

    virtual const char *getResult() const noexcept = 0;
    virtual const char *getLastError() const noexcept = 0;
    virtual void deleteString(const char *) noexcept = 0;
};

#ifndef GLOG_LIB
extern "C" {
AwardPlugin *create_glog_award_plugin();
void destroy_glog_award_plugin(AwardPlugin *);
}
#endif

#endif // AWARD_PLUGIN_H