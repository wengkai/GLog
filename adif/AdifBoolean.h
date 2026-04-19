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
  protected:
    explicit AdifBoolean(std::string value) : AdifDataBase(std::move(value)) { m_normalize(); }

    void m_normalize() {
        auto &ch0 = m_rawValue[0];
        if (ch0 >= 'a' && ch0 <= 'z') {
            ch0 += ('A' - 'a');
        }
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifBoolean)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 1) return false;
        char c = data[0];
        return (c == 'Y' || c == 'y' || c == 'N' || c == 'n');
    }

    template <typename STD_String> static std::optional<AdifBoolean> create(STD_String &&data) {
        if (check(data)) {
            return AdifBoolean(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    bool asBool() const { return (m_rawValue == "Y"); }
};

#endif