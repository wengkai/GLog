#ifndef ADIF_NUMBER
#define ADIF_NUMBER
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifNumber: Number (N)
 *
 */
class AdifNumber : public AdifDataBase {
  private:
    explicit AdifNumber(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.empty()) return false;

        bool hasDigit = false;
        bool hasDecimalPoint = false;
        size_t startPos = 0;

        if (data[0] == '-') {
            if (data.length() == 1) return false;
            startPos = 1;
        }

        for (size_t i = startPos; i < data.length(); ++i) {
            char c = data[i];
            if (c == '.') {
                if (hasDecimalPoint) return false;
                hasDecimalPoint = true;
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                hasDigit = true;
            } else {
                return false;
            }
        }

        return hasDigit;
    }

    static std::optional<AdifNumber> create(const std::string &data) {
        if (check(data)) {
            return AdifNumber(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    double asDouble() const { return std::stod(m_rawValue); }
};

#endif