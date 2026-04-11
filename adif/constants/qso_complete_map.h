#include <map>
#include <string>

/**
 * ADIF QSO Complete Enumeration
 */
using QsoCompleteMap = std::map<std::string, std::string>;

static const QsoCompleteMap QSO_COMPLETE_MAP = {
    {"Y", "yes"},
    {"N", "no"},
    {"NIL", "not heard"},
    {"?", "uncertain"},
};