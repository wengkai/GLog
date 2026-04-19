#ifndef ADIF_MULTILINE_STRING
#define ADIF_MULTILINE_STRING
#include <optional>
#include <regex>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifMultilineString: MultilineString (M)
 *
 */
class AdifMultilineString : public AdifDataBase {
  protected:
    explicit AdifMultilineString(std::string &&value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifMultilineString)

  public:
    static bool check(std::string_view data) {
        for (size_t i = 0; i < data.length(); ++i) {
            unsigned char c = static_cast<unsigned char>(data[i]);

            if (c >= 32 && c <= 126) {
                continue;
            }

            if (c == 13) {
                if (i + 1 < data.length() && static_cast<unsigned char>(data[i + 1]) == 10) {
                    i++;
                    continue;
                }
                return false;
            }

            return false;
        }
        return true;
    }

    template <typename STD_String>
    static std::optional<AdifMultilineString> create(STD_String &&data) {
        std::string cleanedData = sanitizeLineEndings(std::forward<STD_String>(data));

        if (check(cleanedData)) {
            return AdifMultilineString(std::move(cleanedData));
        }

        return std::nullopt;
    }

    static std::string sanitizeLineEndings(const std::string &data) {
        std::string result;
        result.reserve(data.size() + (data.size() / 10));

        for (size_t i = 0; i < data.size(); ++i) {
            char c = data[i];
            if (c == '\r') {
                if (i + 1 < data.size() && data[i + 1] == '\n') {
                    result.push_back('\r');
                    result.push_back('\n');
                    ++i;
                } else {
                    result.push_back('\r');
                    result.push_back('\n');
                }
            } else if (c == '\n') {
                result.push_back('\r');
                result.push_back('\n');
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif