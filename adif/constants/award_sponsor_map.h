#ifndef AWARD_SPONSOR_MAP_H_GENERATED_H
#define AWARD_SPONSOR_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
using AwardSponsorMap = std::map<std::string, std::string, CaseInsensitiveLess>;

/**
 * @brief ADIF Award Sponsor Prefixes
 * Used for validating SPONSOR_PROGRAM_AWARD format.
 */
inline const AwardSponsorMap AWARD_SPONSOR_MAP = {
    {"ADIF_", "ADIF Development Group"},
    {"ARI_", "ARI - l'Associazione Radioamatori Italiani"},
    {"ARRL_", "ARRL - American Radio Relay League"},
    {"CQ_", "CQ Magazine"},
    {"DARC_", "DARC - Deutscher Amateur-Radio-Club e.V."},
    {"EQSL_", "eQSL"},
    {"IARU_", "IARU - International Amateur Radio Union"},
    {"JARL_", "JARL - Japan Amateur Radio League"},
    {"RSGB_", "RSGB - Radio Society of Great Britain"},
    {"TAG_", "TAG - Tambov award group"},
    {"WABAG_", "WAB - Worked all Britain"},
};
} // namespace ADIF

// clang-format on

#endif // AWARD_SPONSOR_MAP_H_GENERATED_H
