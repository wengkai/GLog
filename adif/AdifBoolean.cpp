#include "AdifBoolean.h"

auto AdifBoolean::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = static_cast<char>(std::toupper(static_cast<unsigned char>(newValue[0])));
        return true;
    }
    return false;
}