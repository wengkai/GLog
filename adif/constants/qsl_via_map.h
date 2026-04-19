#ifndef QSL_VIA_MAP_H_GENERATED_H
#define QSL_VIA_MAP_H_GENERATED_H

// clang-format off


namespace ADIF {
struct QslViaInfo {
    bool import_only;
    std::string description;
};

using QslViaMap = std::map<std::string, QslViaInfo>;

static const QslViaMap QSL_VIA_MAP = {
    {"B", {false, "bureau"}},
    {"D", {false, "direct"}},
    {"E", {false, "electronic"}},
    {"M  ( import-only )", {true, "manager  ( import-only )"}},
};
} // namespace ADIF

// clang-format on

#endif // QSL_VIA_MAP_H_GENERATED_H
