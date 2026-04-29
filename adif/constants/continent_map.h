#ifndef CONTINENT_MAP_H_GENERATED_H
#define CONTINENT_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
// ADIF Continent Enumeration Mapping
using ContinentMap = std::map<std::string, std::string, CaseInsensitiveLess>;

inline const ContinentMap CONTINENT_MAP = {
    {"NA", "North America"},
    {"SA", "South America"},
    {"EU", "Europe"},
    {"AF", "Africa"},
    {"OC", "Oceania"},
    {"AS", "Asia"},
    {"AN", "Antarctica"},
};
} // namespace ADIF

// clang-format on

#endif // CONTINENT_MAP_H_GENERATED_H
