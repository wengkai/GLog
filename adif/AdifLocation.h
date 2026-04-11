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
  private:
    explicit AdifLocation(std::string value) : AdifDataBase(std::move(value)) {

        if (!m_rawValue.empty()) {
            m_rawValue[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[0])));
        }
    }

  public:
    static bool check(const std::string &data) {
        if (data.length() != 11) return false;

        char x = static_cast<char>(std::toupper(static_cast<unsigned char>(data[0])));
        if (x != 'E' && x != 'W' && x != 'N' && x != 'S') return false;

        for (int i = 1; i <= 3; ++i) {
            if (!std::isdigit(static_cast<unsigned char>(data[i]))) return false;
        }
        int ddd = std::stoi(data.substr(1, 3));
        if (ddd < 0 || ddd > 180) return false;

        if (data[4] != ' ') return false;

        if (!std::isdigit(static_cast<unsigned char>(data[5]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[6]))) return false;
        if (data[7] != '.') return false;
        if (!std::isdigit(static_cast<unsigned char>(data[8]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[9]))) return false;
        if (!std::isdigit(static_cast<unsigned char>(data[10]))) return false;

        double mm = std::stod(data.substr(5, 6));
        if (mm < 0.0 || mm > 59.999) return false;

        return true;
    }

    static std::optional<AdifLocation> create(const std::string &data) {
        if (check(data)) {
            return AdifLocation(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    double asDecimal() const {
        double d = std::stod(m_rawValue.substr(1, 3));
        double m = std::stod(m_rawValue.substr(5, 6));
        double res = d + (m / 60.0);

        char x = m_rawValue[0];
        if (x == 'S' || x == 'W') res = -res;

        return res;
    }
};

#endif