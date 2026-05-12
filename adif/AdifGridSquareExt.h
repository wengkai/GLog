#ifndef ADIF_GRID_SQUARE_EXT
#define ADIF_GRID_SQUARE_EXT
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifGridSquareExt
 *
 */
class GLOGKIT_API AdifGridSquareExt : public AdifDataBase {
  protected:
    explicit AdifGridSquareExt(std::string value) : AdifDataBase(std::move(value)) {
        normalizeDataToUpper();
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifGridSquareExt)

  public:
    static bool check(std::string_view data) {
        size_t len = data.length();

        if (len != 2 && len != 4) return false;

        for (size_t i = 0; i < 2; ++i) {
            char c = static_cast<char>(std::toupper(static_cast<unsigned char>(data[i])));
            if (c < 'A' || c > 'X') return false;
        }

        if (len == 4) {
            for (size_t i = 2; i < 4; ++i) {
                if (!std::isdigit(static_cast<unsigned char>(data[i]))) return false;
            }
        }

        return true;
    }

    template <typename STD_String>
    static std::optional<AdifGridSquareExt> create(STD_String &&data) {
        if (check(data)) {
            return AdifGridSquareExt(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    size_t extensionLength() const { return m_rawValue.length(); }
};

#endif