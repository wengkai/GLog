#ifndef ADIF_CHARACTER
#define ADIF_CHARACTER
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifCharacter
 *
 */
class AdifCharacter : public AdifDataBase {
  private:
    explicit AdifCharacter(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data) {
        if (data.length() != 1) return false;

        unsigned char c = static_cast<unsigned char>(data[0]);
        return (c >= 32 && c <= 126);
    }

    static std::optional<AdifCharacter> create(const std::string &data) {
        if (check(data)) {
            return AdifCharacter(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    char asChar() const { return m_rawValue[0]; }
};

#endif