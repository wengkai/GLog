#ifndef ADIF_INTL_STRING
#define ADIF_INTL_STRING
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifIntlString: IntlString (I)
 *
 */
class GLOGKIT_API AdifIntlString : public AdifDataBase {
  protected:
    explicit AdifIntlString(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifIntlString)

  public:
    static bool check(std::string_view data) {
        size_t i = 0;
        size_t len = data.length();

        while (i < len) {
            uint8_t c = static_cast<uint8_t>(data[i]);

            if (c == 0x0A || c == 0x0D) {
                return false;
            }

            int bytes_to_follow = 0;

            if ((c & 0x80) == 0) {
                bytes_to_follow = 0;
            } else if ((c & 0xE0) == 0xC0) {
                bytes_to_follow = 1;
                if (c < 0xC2) return false;
            } else if ((c & 0xF0) == 0xE0) {
                bytes_to_follow = 2;
            } else if ((c & 0xF8) == 0xF0) {
                bytes_to_follow = 3;
                if (c > 0xF4) return false;
            } else {
                return false;
            }

            i++;

            for (int j = 0; j < bytes_to_follow; ++j) {
                if (i >= len) return false;

                uint8_t next_c = static_cast<uint8_t>(data[i]);
                if ((next_c & 0xC0) != 0x80) return false;

                if (j == 0) {
                    if (bytes_to_follow == 2) {
                        if (c == 0xE0 && next_c < 0xA0) return false;  // Overlong
                        if (c == 0xED && next_c >= 0xA0) return false; // Surrogate
                    } else if (bytes_to_follow == 3) {
                        if (c == 0xF0 && next_c < 0x90) return false;  // Overlong
                        if (c == 0xF4 && next_c >= 0x90) return false; // Out of range
                    }
                }

                i++;
            }
        }
        return true;
    }

    template <typename STD_String> static std::optional<AdifIntlString> create(STD_String &&data) {
        if (check(data)) {
            return AdifIntlString(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif