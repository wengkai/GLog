#include "AdifBoolean.h"

bool AdifBoolean::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = static_cast<char>(std::toupper(static_cast<unsigned char>(newValue[0])));
        return true;
    }
    return false;
}