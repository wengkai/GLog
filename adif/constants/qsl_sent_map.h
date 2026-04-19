#ifndef QSL_SENT_MAP_H_GENERATED_H
#define QSL_SENT_MAP_H_GENERATED_H

// clang-format off


namespace ADIF {
#ifndef QSL_STATUS_INFO_DEFINED
#define QSL_STATUS_INFO_DEFINED
struct QslStatusInfo {
    bool import_only;
    std::string meaning;
    std::string description;
};
#endif

using QslSentMap = std::map<std::string, QslStatusInfo>;

static const QslSentMap QSL_SENT_MAP = {
    {"Y", {false, "yes", "an outgoing QSL card has been sent ; the QSO has been uploaded to, and accepted by, the online service"}},
    {"N", {false, "no", "do not send an outgoing QSL card ; do not upload the QSO to the online service"}},
    {"R", {false, "requested", "the contacted station has requested a QSL card ; the contacted station has requested the QSO be uploaded to the online service"}},
    {"Q", {false, "queued", "an outgoing QSL card has been selected to be sent ; a QSO has been selected to be uploaded to the online service"}},
    {"I", {false, "ignore or invalid", ""}},
};
} // namespace ADIF

// clang-format on

#endif // QSL_SENT_MAP_H_GENERATED_H
