#ifndef EQSL_AG_MAP_H_GENERATED_H
#define EQSL_AG_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"

#include <string>
#include <map>

namespace ADIF {

using EQSL_AG_Container = std::map<std::string, std::string, CaseInsensitiveLess>;

inline const EQSL_AG_Container EQSL_AG_MAP = {
        {"N", "the QSO is confirmed but not \"Authenticity Guaranteed\" byeQSL"},
        {"U", "unknown"},
        {"Y", "the QSO is confirmed and \"Authenticity Guaranteed\" byeQSL"},
};
} // namespace ADIF

// clang-format on

#endif // EQSL_AG_MAP_H_GENERATED_H
