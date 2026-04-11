#include <map>
#include <string>

using AwardSponsorMap = std::map<std::string, std::string>;

/**
 * @brief ADIF Award Sponsor Prefixes
 * Used for validating SPONSOR_PROGRAM_AWARD format.
 */
static const AwardSponsorMap AWARD_SPONSOR_MAP = {
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