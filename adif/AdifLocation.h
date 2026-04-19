#ifndef ADIF_LOCATION
#define ADIF_LOCATION
#include <cctype>
#include <cmath>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifLocation: Location (L)
 *
 * XDDD MM.MMM
 *    - X: {E, W, N, S}
 *    - DDD: (000-180)
 *    - (ASCII 32)
 *    - MM.MMM: (00.000-59.999)
 */
class AdifLocation : public AdifDataBase {
  protected:
    explicit AdifLocation(std::string value) : AdifDataBase(std::move(value)) {
        m_rawValue[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[0])));
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifLocation)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 11) return false;

        char x = static_cast<char>(std::toupper(static_cast<unsigned char>(data[0])));
        if (x != 'E' && x != 'W' && x != 'N' && x != 'S') return false;

        for (int i = 1; i <= 3; ++i) {
            if (!std::isdigit(static_cast<unsigned char>(data[i]))) return false;
        }

        auto [ddd, mm] = asDecimal<int, double>(data);
        if (ddd < 0 || ddd > 180) return false;

        if (data[4] != ' ') return false;

        if (!std::isdigit(static_cast<unsigned char>(data[5]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[6]))) return false;
        if (data[7] != '.') return false;
        if (!std::isdigit(static_cast<unsigned char>(data[8]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[9]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[10]))) return false;

        if (mm < 0.0 || mm > 59.999) return false;

        return true;
    }

    template <typename STD_String> static std::optional<AdifLocation> create(STD_String &&data) {
        if (check(data)) {
            return AdifLocation(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

  private:
    template <typename Num1, typename Num2>
    static auto asDecimal(std::string_view data) -> std::pair<Num1, Num2> {
        Num1 ddd;
        Num2 mm;
        std::from_chars(data.data() + 1, data.data() + 4, ddd);
        std::from_chars(data.data() + 5, data.data() + 11, mm);
        return std::pair<Num1, Num2>{ddd, mm};
    }

  public:
    double asDecimal() const {
        auto [d, m] = asDecimal<double, double>(m_rawValue);
        double res = d + (m / 60.0);

        char x = m_rawValue[0];
        if (x == 'S' || x == 'W') res = -res;

        return res;
    }
};

#endif