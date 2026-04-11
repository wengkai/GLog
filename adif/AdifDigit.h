#ifndef ADIF_DIGIT
#define ADIF_DIGIT
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifDigit
 *
 */
class AdifDigit : public AdifDataBase {
  private:
    explicit AdifDigit(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.length() != 1) return false;
        char c = data[0];
        return (c >= '0' && c <= '9');
    }

    static std::optional<AdifDigit> create(const std::string &data) {
        if (check(data)) {
            return AdifDigit(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    int asInt() const { return m_rawValue[0] - '0'; }
};

#endif