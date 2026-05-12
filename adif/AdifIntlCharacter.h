#ifndef ADIF_INTL_CHARACTER
#define ADIF_INTL_CHARACTER
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifIntlCharacter
 *
 */
class GLOGKIT_API AdifIntlCharacter : public AdifDataBase {
  protected:
    explicit AdifIntlCharacter(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifIntlCharacter)

  public:
    static bool check(std::string_view data) {
        if (data.empty()) return false;

        if (data.length() == 1) {
            unsigned char c = static_cast<unsigned char>(data[0]);
            if (c == 10 || c == 13) return false;
            return c < 128;
        }

        unsigned char first = static_cast<unsigned char>(data[0]);
        size_t expectedLen = 0;

        if (first < 0x80)
            expectedLen = 1;
        else if ((first & 0xE0) == 0xC0)
            expectedLen = 2;
        else if ((first & 0xF0) == 0xE0)
            expectedLen = 3;
        else if ((first & 0xF8) == 0xF0)
            expectedLen = 4;
        else
            return false;

        return data.length() == expectedLen;
    }

    template <typename STD_String>
    static std::optional<AdifIntlCharacter> create(STD_String &&data) {
        if (check(data)) {
            return AdifIntlCharacter(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif