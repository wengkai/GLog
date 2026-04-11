#include "AdifLocation.h"

auto AdifLocation::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = newValue;
        m_rawValue[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[0])));
        return true;
    }
    return false;
}