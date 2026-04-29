#ifndef ANT_PATH_MAP_H_GENERATED_H
#define ANT_PATH_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
using AntPathMap = std::map<std::string, std::string, CaseInsensitiveLess>;

inline const AntPathMap ANT_PATH_MAP = {
    {"G", "grayline"},
    {"O", "other"},
    {"S", "short path"},
    {"L", "long path"},
};
} // namespace ADIF

// clang-format on

#endif // ANT_PATH_MAP_H_GENERATED_H
