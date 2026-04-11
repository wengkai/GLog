#include "AdifIntlString.h"

bool AdifIntlString::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}