#ifndef ADIF_GRID_SQUARE
#define ADIF_GRID_SQUARE
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"

/**
 * @brief AdifGridSquare
 *
 */
class GLOGKIT_API AdifGridSquare : public AdifDataBase {
  protected:
    explicit AdifGridSquare(std::string value) : AdifDataBase(std::move(value)) {
        normalizeDataToUpper();
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifGridSquare)

  public:
    static bool check(std::string_view data) {
        size_t len = data.length();

        if (len != 2 && len != 4 && len != 6 && len != 8) return false;

        // Pair 1: Characters 1-2 (Fields) -> A-R
        for (size_t i = 0; i < 2; ++i) {
            char c = static_cast<char>(std::toupper(static_cast<unsigned char>(data[i])));
            if (c < 'A' || c > 'R') return false;
        }

        // Pair 2: Characters 3-4 (Squares) -> 0-9
        if (len >= 4) {
            for (size_t i = 2; i < 4; ++i) {
                if (!std::isdigit(static_cast<unsigned char>(data[i]))) return false;
            }
        }

        // Pair 3: Characters 5-6 (Subsquares) -> A-X
        if (len >= 6) {
            for (size_t i = 4; i < 6; ++i) {
                char c = static_cast<char>(std::toupper(static_cast<unsigned char>(data[i])));
                if (c < 'A' || c > 'X') return false;
            }
        }

        // Pair 4: Characters 7-8 (Extended Squares) -> 0-9
        if (len >= 8) {
            for (size_t i = 6; i < 8; ++i) {
                if (!std::isdigit(static_cast<unsigned char>(data[i]))) return false;
            }
        }

        return true;
    }

    template <typename STD_String> static std::optional<AdifGridSquare> create(STD_String &&data) {
        if (check(data)) {
            return AdifGridSquare(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    int precisionLevel() const { return static_cast<int>(m_rawValue.length() / 2); }
};

#endif