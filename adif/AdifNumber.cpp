#include "AdifNumber.h"

bool AdifNumber::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}