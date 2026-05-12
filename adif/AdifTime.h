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
class GLOGKIT_API AdifTime : public AdifDataBase {
  protected:
    explicit AdifTime(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifTime)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 4 && data.length() != 6) return false;

        if (!std::all_of(data.begin(), data.end(),
                         [](unsigned char c) { return std::isdigit(c); })) {
            return false;
        }

        auto [hh, mm, ss, has_ss] = asParts(data);

        if (hh < 0 || hh > 23) return false;
        if (mm < 0 || mm > 59) return false;

        if (has_ss) {
            if (ss < 0 || ss > 59) return false;
        }

        return true;
    }

    template <typename STD_String> static std::optional<AdifTime> create(STD_String &&data) {
        if (check(data)) {
            return AdifTime(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    struct TimePart {
        int hour;
        int minute;
        int second;
        bool hasSeconds;
    };

  private:
    static TimePart asParts(std::string_view data) {
        TimePart tp{0, 0, 0, false};
        std::from_chars(data.data(), data.data() + 2, tp.hour);
        std::from_chars(data.data() + 2, data.data() + 4, tp.minute);
        if (data.size() == 6) {
            std::from_chars(data.data() + 4, data.data() + 6, tp.second);
            tp.hasSeconds = true;
        }
        return tp;
    }

  public:
    TimePart asParts() const { return asParts(m_rawValue); }
};

#endif