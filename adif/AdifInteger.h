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
class GLOGKIT_API AdifInteger : public AdifDataBase {
  protected:
    explicit AdifInteger(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifInteger)

  public:
    static bool check(std::string_view data) {
        if (data.empty()) return false;

        size_t startPos = 0;
        if (data[0] == '-') {
            if (data.length() == 1) return false;
            startPos = 1;
        }

        return std::all_of(data.begin() + startPos, data.end(),
                           [](unsigned char c) { return std::isdigit(c); });
    }

    template <typename STD_String> static std::optional<AdifInteger> create(STD_String &&data) {
        if (check(data)) {
            return AdifInteger(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    long long asInt64() const { return std::stoll(m_rawValue); }
};

#endif