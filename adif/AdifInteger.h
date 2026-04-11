#ifndef ADIF_INTEGER
#define ADIF_INTEGER
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifInteger
 *
 */
class AdifInteger : public AdifDataBase {
  private:
    explicit AdifInteger(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.empty()) return false;

        size_t startPos = 0;
        if (data[0] == '-') {
            if (data.length() == 1) return false;
            startPos = 1;
        }

        return std::all_of(data.begin() + startPos, data.end(),
                           [](unsigned char c) { return std::isdigit(c); });
    }

    static std::optional<AdifInteger> create(const std::string &data) {
        if (check(data)) {
            return AdifInteger(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    long long asInt64() const { return std::stoll(m_rawValue); }
};

#endif