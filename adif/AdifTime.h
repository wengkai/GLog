#ifndef ADIF_TIME
#define ADIF_TIME
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifTime: Time (T)
 *
 * 1. (HHMM) or (HHMMSS)
 * 2. 00 <= HH <= 23
 * 3. 00 <= MM <= 59
 * 4. 00 <= SS <= 59
 */
class AdifTime : public AdifDataBase {
  private:
    explicit AdifTime(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.length() != 4 && data.length() != 6) return false;

        if (!std::all_of(data.begin(), data.end(),
                         [](unsigned char c) { return std::isdigit(c); })) {
            return false;
        }

        int hh = std::stoi(data.substr(0, 2));
        int mm = std::stoi(data.substr(2, 2));

        if (hh < 0 || hh > 23) return false;
        if (mm < 0 || mm > 59) return false;

        if (data.length() == 6) {
            int ss = std::stoi(data.substr(4, 2));
            if (ss < 0 || ss > 59) return false;
        }

        return true;
    }

    static std::optional<AdifTime> create(const std::string &data) {
        if (check(data)) {
            return AdifTime(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    struct TimePart {
        int hour;
        int minute;
        int second;
        bool hasSeconds;
    };

    TimePart asParts() const {
        TimePart tp{0, 0, 0, false};
        tp.hour = std::stoi(m_rawValue.substr(0, 2));
        tp.minute = std::stoi(m_rawValue.substr(2, 2));
        if (m_rawValue.length() == 6) {
            tp.second = std::stoi(m_rawValue.substr(4, 2));
            tp.hasSeconds = true;
        }
        return tp;
    }
};

#endif