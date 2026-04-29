#include "AdifEnumTypes.h"

#include "constants/adif_constants.h"

#define ADIF_ENUM_IMPL(classname, static_map)                                                      \
    template <> bool AdifEnumValidator<classname>::check(std::string_view data) {                  \
        return static_map.count(data);                                                             \
    }                                                                                              \
    ADIF_DATA_TYPE_CLONE_IMP(classname)

ADIF_ENUM_IMPL(AdifAntPath, ADIF::ANT_PATH_MAP)
ADIF_ENUM_IMPL(AdifArrlSection, ADIF::ARRL_SECTION_MAP)
ADIF_ENUM_IMPL(AdifAward, ADIF::AWARD_MAP)
ADIF_ENUM_IMPL(AdifAwardSponsor, ADIF::AWARD_SPONSOR_MAP)
ADIF_ENUM_IMPL(AdifBandRx, ADIF::BAND_MAP)
ADIF_ENUM_IMPL(AdifContestId, ADIF::CONTEST_ID_MAP)
ADIF_ENUM_IMPL(AdifContinent, ADIF::CONTINENT_MAP)
ADIF_ENUM_IMPL(AdifCredit, ADIF::CREDIT_MAP)
ADIF_ENUM_IMPL(AdifDxcc, ADIF::DXCC_MAP)
ADIF_ENUM_IMPL(AdifEqslAg, ADIF::EQSL_AG_MAP)
ADIF_ENUM_IMPL(AdifMorseKey, ADIF::MORSE_KEY_MAP)
ADIF_ENUM_IMPL(AdifPropagation, ADIF::PROPAGATION_MAP)
ADIF_ENUM_IMPL(AdifQslMedium, ADIF::QSL_MEDIUM_MAP)
ADIF_ENUM_IMPL(AdifQslRcvd, ADIF::QSL_RCVD_MAP)
ADIF_ENUM_IMPL(AdifQslSent, ADIF::QSL_SENT_MAP)
ADIF_ENUM_IMPL(AdifQslVia, ADIF::QSL_VIA_MAP)
ADIF_ENUM_IMPL(AdifQsoComplete, ADIF::QSO_COMPLETE_MAP)
ADIF_ENUM_IMPL(AdifQsoDownloadStatus, ADIF::QSO_DOWNLOAD_STATUS_MAP)
ADIF_ENUM_IMPL(AdifQsoUploadStatus, ADIF::QSO_UPLOAD_STATUS_MAP)
ADIF_ENUM_IMPL(AdifRegion, ADIF::REGION_MAP)
