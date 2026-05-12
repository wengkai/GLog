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
  protected:
    explicit AdifNumber(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifNumber)

  public:
    static bool check(std::string_view data) {
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

    template <typename STD_String> static std::optional<AdifNumber> create(STD_String &&data) {
        if (check(data)) {
            return AdifNumber(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    virtual double asDouble() const;

    CompareRes compare(const AdifDataBase &right) const override;

    static CompareRes compare_rational(std::string_view a_str, std::string_view b_str);

    static bool in_range(std::string_view lower, std::string_view val, std::string_view upper) {
        auto compare_to_lower = AdifNumber::compare_rational(val, lower);
        auto compare_to_upper = AdifNumber::compare_rational(val, upper);
        return (compare_to_lower != less && compare_to_upper != greater);
    }
};

#endif