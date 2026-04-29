#ifndef QSO_UPLOAD_STATUS_MAP_H_GENERATED_H
#define QSO_UPLOAD_STATUS_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
/**
 * ADIF QSO Upload Status Enumeration
 */
using QsoUploadStatusMap = std::map<std::string, std::string, CaseInsensitiveLess>;

inline const QsoUploadStatusMap QSO_UPLOAD_STATUS_MAP = {
    {"Y", "the QSO has been uploaded to, and accepted by, the online service"},
    {"N", "do not upload the QSO to the online service"},
    {"M", "the QSO has been modified since being uploaded to the online service"},
};
} // namespace ADIF

// clang-format on

#endif // QSO_UPLOAD_STATUS_MAP_H_GENERATED_H
