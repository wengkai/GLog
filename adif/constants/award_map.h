#ifndef AWARD_MAP_H_GENERATED_H
#define AWARD_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
/**
 * @brief ADIF Award Enumeration (Import-only markers included)
 * Key: Award Name, Value: true if the award is import-only.
 */

using AwardMap = std::map<std::string, bool, CaseInsensitiveLess>;

inline const AwardMap AWARD_MAP = {
    {"AJA", true},
    {"CQDX", true},
    {"CQDXFIELD", true},
    {"CQWAZ_MIXED", true},
    {"CQWAZ_CW", true},
    {"CQWAZ_PHONE", true},
    {"CQWAZ_RTTY", true},
    {"CQWAZ_160m", true},
    {"CQWPX", true},
    {"DARC_DOK", true},
    {"DXCC", true},
    {"DXCC_MIXED", true},
    {"DXCC_CW", true},
    {"DXCC_PHONE", true},
    {"DXCC_RTTY", true},
    {"IOTA", true},
    {"JCC", true},
    {"JCG", true},
    {"MARATHON", true},
    {"RDA", true},
    {"WAB", true},
    {"WAC", true},
    {"WAE", true},
    {"WAIP", true},
    {"WAJA", true},
    {"WAS", true},
    {"WAZ", true},
    {"USACA", true},
    {"VUCC", true},
};
} // namespace ADIF

// clang-format on

#endif // AWARD_MAP_H_GENERATED_H
