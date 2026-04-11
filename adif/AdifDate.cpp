#include "AdifDate.h"

auto AdifDate::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}