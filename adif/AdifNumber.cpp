#include "AdifNumber.h"

auto AdifNumber::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}