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
  private:
    explicit AdifMultilineString(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
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

    static std::optional<AdifMultilineString> create(const std::string &data) {
        std::string cleanedData = sanitizeLineEndings(data);

        if (check(cleanedData)) {
            return AdifMultilineString(std::move(cleanedData));
        }

        return std::nullopt;
    }

    static std::string sanitizeLineEndings(const std::string &data) {
        std::string result = std::regex_replace(data, std::regex(R"(\r\n|\r|\n)"), "\n");
        return std::regex_replace(result, std::regex(R"(\n)"), "\r\n");
    }

    bool set(const std::string &newValue) override;
};

#endif