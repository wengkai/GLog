#ifndef ADIF_BOOLEAN
#define ADIF_BOOLEAN
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifBoolean: Boolean (B)
 *
 */
class AdifBoolean : public AdifDataBase {
  private:
    explicit AdifBoolean(std::string value) : AdifDataBase(std::move(value)) {
        if (!m_rawValue.empty()) {
            m_rawValue[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[0])));
        }
    }

  public:
    static bool check(const std::string &data) {
        if (data.length() != 1) return false;
        char c = data[0];
        return (c == 'Y' || c == 'y' || c == 'N' || c == 'n');
    }

    static std::optional<AdifBoolean> create(const std::string &data) {
        if (check(data)) {
            return AdifBoolean(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    bool asBool() const { return (m_rawValue == "Y"); }
};

#endif