#include <map>
#include <string>

/**
 * ADIF QSO Download Status Enumeration
 */
using QsoDownloadStatusMap = std::map<std::string, std::string>;

static const QsoDownloadStatusMap QSO_DOWNLOAD_STATUS_MAP = {
    {"Y", "the QSO has been downloaded from the online service"},
    {"N", "the QSO has not been downloaded from the online service"},
    {"I", "ignore or invalid"},
};