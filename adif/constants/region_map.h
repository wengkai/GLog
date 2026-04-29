#ifndef REGION_MAP_H_GENERATED_H
#define REGION_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
struct RegionEntry {
    int dxcc_entity_code;
    std::string name;
    std::string prefix;
    std::string applicability; // CQ,WAE
    std::string start_date;    // YYYY-MM-DD
    std::string end_date;      // YYYY-MM-DD
};

/**
 * @brief ADIF Region Multimap
 */
using RegionMap = std::multimap<std::string, RegionEntry, CaseInsensitiveLess>;

inline const RegionMap REGION_MAP = {
    {"NONE", { 0, "Not within a WAE or CQ region that is within a DXCC entity", "", "", "", "" } },
    {"IV", { 206, "ITU Vienna", "4U1V", "CQ,WAE", "", "" } },
    {"AI", { 248, "African Italy", "IG9", "CQ", "", "" } },
    {"SY", { 248, "Sicily", "IT9", "CQ,WAE", "", "" } },
    {"BI", { 259, "Bear Island", "JW/B", "CQ,WAE", "", "" } },
    {"SI", { 279, "Shetland Islands", "GM/S", "CQ,WAE", "", "" } },
    {"KO", { 296, "Kosovo", "YU8", "CQ,WAE", "", "2012-09-11" } },
    {"KO", { 0, "Kosovo", "Z6", "CQ,WAE", "2012-09-12", "2018-01-20" } },
    {"KO", { 522, "Kosovo", "Z6", "CQ,WAE", "2018-01-21", "" } },
    {"ET", { 390, "European Turkey", "TA1", "CQ", "", "" } },
};
} // namespace ADIF

// clang-format on

#endif // REGION_MAP_H_GENERATED_H
