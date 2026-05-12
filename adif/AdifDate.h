#ifndef ADIF_DATE
#define ADIF_DATE
#include <optional>
#include <string>
#include <vector>
#include "AdifDataBase.h"

/**
 * @brief AdifDate: Date (D)
 *
 * 1. YYYYMMDD
 * 2. YYYY >= 1930
 * 3. 01 <= MM <= 12
 * 4. 01 <= DD <= DaysInMonth(MM, YYYY)
 */
class GLOGKIT_API AdifDate : public AdifDataBase {
  protected:
    explicit AdifDate(std::string value) : AdifDataBase(std::move(value)) {}

    static bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static int daysInMonth(int m, int y) {
        static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && isLeapYear(y)) return 29;
        if (m >= 1 && m <= 12) return days[m - 1];
        return 0;
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifDate)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 8) return false;

        for (char c : data) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        }

        auto [y, m, d] = asParts(data);

        if (y < 1930) return false;

        if (m < 1 || m > 12) return false;

        if (d < 1 || d > daysInMonth(m, y)) return false;

        return true;
    }

    template <typename STD_String> static std::optional<AdifDate> create(STD_String &&data) {
        if (check(data)) {
            return AdifDate(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    struct DatePart {
        int year;
        int month;
        int day;
    };

  private:
    static DatePart asParts(std::string_view data) {
        int y, m, d;
        std::from_chars(data.data(), data.data() + 4, y);
        std::from_chars(data.data() + 4, data.data() + 6, m);
        std::from_chars(data.data() + 6, data.data() + 8, d);
        return {y, m, d};
    }

  public:
    DatePart asParts() const { return asParts(m_rawValue); }
};

#endif