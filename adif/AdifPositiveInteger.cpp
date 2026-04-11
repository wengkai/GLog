#include "AdifPositiveInteger.h"

auto AdifPositiveInteger::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}