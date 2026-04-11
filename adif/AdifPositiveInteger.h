#ifndef ADIF_POSITIVE_INTEGER
#define ADIF_POSITIVE_INTEGER
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifPositiveInteger
 *
 */
class AdifPositiveInteger : public AdifDataBase {
  private:
    explicit AdifPositiveInteger(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.empty()) return false;

        bool hasNonZero = false;
        for (char c : data) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
            if (c != '0') {
                hasNonZero = true;
            }
        }

        return hasNonZero;
    }

    static std::optional<AdifPositiveInteger> create(const std::string &data) {
        if (check(data)) {
            return AdifPositiveInteger(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    unsigned long long asUInt64() const { return std::stoull(m_rawValue); }
};

#endif