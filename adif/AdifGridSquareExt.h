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
class AdifGridSquareExt : public AdifDataBase {
  private:
    explicit AdifGridSquareExt(std::string value) : AdifDataBase(std::move(value)) {
        for (size_t i = 0; i < 2 && i < m_rawValue.length(); ++i) {
            m_rawValue[i] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[i])));
        }
    }

  public:
    static bool check(const std::string &data) {
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

    static std::optional<AdifGridSquareExt> create(const std::string &data) {
        if (check(data)) {
            return AdifGridSquareExt(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    size_t extensionLength() const { return m_rawValue.length(); }
};

#endif