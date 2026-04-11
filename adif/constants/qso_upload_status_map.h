#include <map>
#include <string>

/**
 * ADIF QSO Upload Status Enumeration
 */
using QsoUploadStatusMap = std::map<std::string, std::string>;

static const QsoUploadStatusMap QSO_UPLOAD_STATUS_MAP = {
    {"Y", "the QSO has been uploaded to, and accepted by, the online service"},
    {"N", "do not upload the QSO to the online service"},
    {"M", "the QSO has been modified since being uploaded to the online service"},
};