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
  protected:
    explicit AdifCharacter(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifCharacter)

  public:
    static bool check(std::string_view data) {
        if (data.length() != 1) return false;

        unsigned char c = static_cast<unsigned char>(data[0]);
        return (c >= 32 && c <= 126);
    }

    template <typename STD_String> static std::optional<AdifCharacter> create(STD_String &&data) {
        if (check(data)) {
            return AdifCharacter(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    char asChar() const { return m_rawValue[0]; }
};

#endif