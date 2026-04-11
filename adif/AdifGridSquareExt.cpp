#include "AdifGridSquareExt.h"

auto AdifGridSquareExt::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = newValue;

        for (size_t i = 0; i < 2 && i < m_rawValue.length(); ++i) {
            m_rawValue[i] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[i])));
        }
        return true;
    }
    return false;
}