#ifndef QSO_DOWNLOAD_STATUS_MAP_H_GENERATED_H
#define QSO_DOWNLOAD_STATUS_MAP_H_GENERATED_H

// clang-format off


namespace ADIF {
/**
 * ADIF QSO Download Status Enumeration
 */
using QsoDownloadStatusMap = std::map<std::string, std::string>;

static const QsoDownloadStatusMap QSO_DOWNLOAD_STATUS_MAP = {
    {"Y", "the QSO has been downloaded from the online service"},
    {"N", "the QSO has not been downloaded from the online service"},
    {"I", "ignore or invalid"},
};
} // namespace ADIF

// clang-format on

#endif // QSO_DOWNLOAD_STATUS_MAP_H_GENERATED_H
