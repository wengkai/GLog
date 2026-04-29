#ifndef ADIF_ENUM_TYPES
#define ADIF_ENUM_TYPES

#include "AdifDataBase.h"

template <typename T> struct AdifEnumValidator {
    static bool check(std::string_view data);
};

template <typename Derived> class AdifEnumBase : public AdifDataBase {
  public:
    template <typename S> static std::optional<Derived> create(S &&data) {
        if (AdifEnumValidator<Derived>::check(data)) {
            return Derived(std::forward<S>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override {
        if (AdifEnumValidator<Derived>::check(newValue)) {
            m_rawValue = std::move(newValue);
            AdifDataBase::normalizeDataToUpper(m_rawValue);
            return {true};
        }
        return {false, std::move(newValue)};
    }

  protected:
    explicit AdifEnumBase(std::string value) : AdifDataBase(std::move(value)) {}
    ADIF_DATA_TYPE_CLONE_BASE_CONS_DEC(AdifEnumBase)
};

#define ADIF_ENUM_DEC(classname)                                                                   \
    class classname : public AdifEnumBase<classname> {                                             \
        friend class AdifEnumBase<classname>;                                                      \
        explicit classname(std::string v) : AdifEnumBase(std::move(v)) {                           \
            AdifDataBase::normalizeDataToUpper(m_rawValue);                                        \
        }                                                                                          \
                                                                                                   \
      protected:                                                                                   \
        ADIF_DATA_TYPE_CLONE_DERIVED_DEC(classname, AdifEnumBase)                                  \
    };

ADIF_ENUM_DEC(AdifAntPath)
ADIF_ENUM_DEC(AdifArrlSection)
ADIF_ENUM_DEC(AdifAward)
ADIF_ENUM_DEC(AdifAwardSponsor)
ADIF_ENUM_DEC(AdifBandRx)
ADIF_ENUM_DEC(AdifContestId)
ADIF_ENUM_DEC(AdifContinent)
ADIF_ENUM_DEC(AdifCredit)
ADIF_ENUM_DEC(AdifDxcc)
ADIF_ENUM_DEC(AdifEqslAg)
ADIF_ENUM_DEC(AdifMorseKey)
ADIF_ENUM_DEC(AdifPropagation)
ADIF_ENUM_DEC(AdifQslMedium)
ADIF_ENUM_DEC(AdifQslRcvd)
ADIF_ENUM_DEC(AdifQslSent)
ADIF_ENUM_DEC(AdifQslVia)
ADIF_ENUM_DEC(AdifQsoComplete)
ADIF_ENUM_DEC(AdifQsoDownloadStatus)
ADIF_ENUM_DEC(AdifQsoUploadStatus)
ADIF_ENUM_DEC(AdifRegion)

#endif