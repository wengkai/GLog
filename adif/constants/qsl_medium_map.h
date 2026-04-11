#include <map>
#include <string>

/**
 * ADIF QSL Medium Enumeration
 */
using QslMediumMap = std::map<std::string, std::string>;

static const QslMediumMap QSL_MEDIUM_MAP = {
    {"CARD", "QSO confirmation via paper QSL card"},
    {"EQSL", "QSO confirmation viaeQSL.cc"},
    {"LOTW", "QSO confirmation viaARRL Logbook of the World"},
};