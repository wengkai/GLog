#ifndef ADIF_INTL_MULTILINE_STRING
#define ADIF_INTL_MULTILINE_STRING
#include <optional>
#include <string>
#include "AdifDataBase.h"
#include "AdifMultilineString.h"

/**
 * @brief AdifIntlMultilineString: IntlMultilineString (G)
 *
 */
class GLOGKIT_API AdifIntlMultilineString : public AdifDataBase {
  protected:
    explicit AdifIntlMultilineString(std::string &&value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifIntlMultilineString)

  public:
    static bool check(std::string_view data) {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data.data());
        size_t len = data.length();
        int remaining_bytes = 0;

        for (size_t i = 0; i < len; ++i) {
            uint8_t current = bytes[i];

            if (current == 0x0A) {
                if (i == 0 || bytes[i - 1] != 0x0D) {
                    return false;
                }
            } else if (current == 0x0D) {
                if (i + 1 >= len || bytes[i + 1] != 0x0A) {
                    return false;
                }
            }

            if (remaining_bytes == 0) {
                if ((current & 0x80) == 0) {
                    remaining_bytes = 0;
                } else if ((current & 0xE0) == 0xC0) {
                    if (current < 0xC2) return false;
                    remaining_bytes = 1;
                } else if ((current & 0xF0) == 0xE0) {
                    remaining_bytes = 2;
                } else if ((current & 0xF8) == 0xF0) {
                    if (current > 0xF4) return false;
                    remaining_bytes = 3;
                } else {
                    return false;
                }
            } else {
                if ((current & 0xC0) != 0x80) {
                    return false;
                }

                if (remaining_bytes == 2) {
                    uint8_t prev = bytes[i - 1];
                    if (prev == 0xE0 && current < 0xA0) return false;
                    if (prev == 0xED && current > 0x9F) return false;
                } else if (remaining_bytes == 3) {
                    uint8_t prev = bytes[i - 1];
                    if (prev == 0xF0 && current < 0x90) return false;
                    if (prev == 0xF4 && current > 0x8F) return false;
                }

                remaining_bytes--;
            }
        }

        return remaining_bytes == 0;
    }

    template <typename STD_String>
    static std::optional<AdifIntlMultilineString> create(STD_String &&data) {
        std::string cleanedData =
            AdifMultilineString::sanitizeLineEndings(std::forward<STD_String>(data));
        if (check(cleanedData)) {
            return AdifIntlMultilineString(std::move(cleanedData));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif