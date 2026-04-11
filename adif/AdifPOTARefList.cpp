#include "AdifPOTARefList.h"

auto AdifPOTARefList::set(const std::string &newValue) -> bool {
    if (check(newValue)) {
        m_rawValue = std::move(normalize(newValue));
        return true;
    }
    return false;
}