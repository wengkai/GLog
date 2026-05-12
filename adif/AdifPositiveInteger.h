#ifndef ADIF_POSITIVE_INTEGER
#define ADIF_POSITIVE_INTEGER
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifPositiveInteger
 *
 */
class GLOGKIT_API AdifPositiveInteger : public AdifDataBase {
  protected:
    explicit AdifPositiveInteger(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifPositiveInteger)

  public:
    static bool check(std::string_view data) {
        if (data.empty()) return false;

        bool hasNonZero = false;
        for (char c : data) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
            if (c != '0') {
                hasNonZero = true;
            }
        }

        return hasNonZero;
    }

    template <typename STD_String>
    static std::optional<AdifPositiveInteger> create(STD_String &&data) {
        if (check(data)) {
            return AdifPositiveInteger(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    unsigned long long asUInt64() const { return std::stoull(m_rawValue); }
};

#endif