#ifndef ADIF_STRING
#define ADIF_STRING
#include <algorithm>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifString: String (S)
 *
 *  (ASCII 32-126)
 */
class GLOGKIT_API AdifString : public AdifDataBase {
  protected:
    explicit AdifString(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifString)

  public:
    static bool check(std::string_view data) {
        return std::all_of(data.begin(), data.end(),
                           [](unsigned char c) { return (c >= 32 && c <= 126); });
    }

    template <typename STD_String> static std::optional<AdifString> create(STD_String &&data) {
        if (check(data)) {
            return AdifString(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif