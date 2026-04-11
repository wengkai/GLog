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
class AdifString : public AdifDataBase {
  private:
    explicit AdifString(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        return std::all_of(data.begin(), data.end(),
                           [](unsigned char c) { return (c >= 32 && c <= 126); });
    }

    static std::optional<AdifString> create(const std::string &data) {
        if (check(data)) {
            return AdifString(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;
};

#endif