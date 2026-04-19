#ifndef QSL_MEDIUM_MAP_H_GENERATED_H
#define QSL_MEDIUM_MAP_H_GENERATED_H

// clang-format off


namespace ADIF {
/**
 * ADIF QSL Medium Enumeration
 */
using QslMediumMap = std::map<std::string, std::string>;

static const QslMediumMap QSL_MEDIUM_MAP = {
    {"CARD", "QSO confirmation via paper QSL card"},
    {"EQSL", "QSO confirmation viaeQSL.cc"},
    {"LOTW", "QSO confirmation viaARRL Logbook of the World"},
};
} // namespace ADIF

// clang-format on

#endif // QSL_MEDIUM_MAP_H_GENERATED_H
