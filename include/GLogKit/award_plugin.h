#ifndef AWARD_PLUGIN_H
#define AWARD_PLUGIN_H

#include <GLogKit/app_export.h>

#include <GLogKit/IGRecord.h>

#include <cstdint>

#ifndef GLOGKIT_LIB
extern "C" {
DECL_EXPORT const char *pluginName();
DECL_EXPORT bool install();
DECL_EXPORT bool uninstall();
DECL_EXPORT bool beforeEvaluate();
DECL_EXPORT bool afterEvaluate();
DECL_EXPORT bool evaluate(const IGRecord *QSO, IGRecordGetValueByField callback);
DECL_EXPORT void getResult(char *result_buf, uint64_t *result_len, uint64_t max_result_len);
DECL_EXPORT void getLastError(char *result_buf, uint64_t *result_len, uint64_t max_result_len);
}
#else
#error "Unexpected include in building GLogKitCore"
#endif

#endif // AWARD_PLUGIN_H
