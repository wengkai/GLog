#ifndef ADIF_DIGIT
#define ADIF_DIGIT
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifDigit
 *
 */
class GLOGKIT_API AdifDigit : public AdifDataBase {
  protected:
    explicit AdifDigit(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifDigit)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 1) return false;
        char c = data[0];
        return (c >= '0' && c <= '9');
    }

    template <typename STD_String> static std::optional<AdifDigit> create(STD_String &&data) {
        if (check(data)) {
            return AdifDigit(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    int asInt() const { return m_rawValue[0] - '0'; }
};

#endif