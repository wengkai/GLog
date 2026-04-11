#include "AdifGeneral.h"

auto AdifGeneral::set(const std::string &newValue) -> bool {
    m_rawValue = newValue;
    return true;
}