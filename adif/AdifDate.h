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
class AdifDate : public AdifDataBase {
  private:
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

  public:
    static bool check(const std::string &data) {
        if (data.length() != 8) return false;

        for (char c : data) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        }

        int y = std::stoi(data.substr(0, 4));
        int m = std::stoi(data.substr(4, 2));
        int d = std::stoi(data.substr(6, 2));

        if (y < 1930) return false;

        if (m < 1 || m > 12) return false;

        if (d < 1 || d > daysInMonth(m, y)) return false;

        return true;
    }

    static std::optional<AdifDate> create(const std::string &data) {
        if (check(data)) {
            return AdifDate(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    struct DatePart {
        int year;
        int month;
        int day;
    };

    DatePart asParts() const {
        return {std::stoi(m_rawValue.substr(0, 4)), std::stoi(m_rawValue.substr(4, 2)),
                std::stoi(m_rawValue.substr(6, 2))};
    }
};

#endif