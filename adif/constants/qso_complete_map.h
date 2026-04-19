#ifndef QSO_COMPLETE_MAP_H_GENERATED_H
#define QSO_COMPLETE_MAP_H_GENERATED_H

// clang-format off


namespace ADIF {
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
} // namespace ADIF

// clang-format on

#endif // QSO_COMPLETE_MAP_H_GENERATED_H
