#ifndef ADIF_IOTA_REF_NO
#define ADIF_IOTA_REF_NO
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"

class AdifIOTARefNo : public AdifDataBase {
  protected:
    explicit AdifIOTARefNo(std::string value) : AdifDataBase(std::move(value)) {
        AdifDataBase::normalizeDataToUpper(m_rawValue);
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifIOTARefNo)

  public:
    static bool check(std::string_view data);

    template <typename STD_String> static std::optional<AdifIOTARefNo> create(STD_String &&data) {
        if (check(data)) {
            return AdifIOTARefNo(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif