#include "AdifPOTARefList.h"

bool AdifPOTARefList::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = std::move(normalize(newValue));
        return true;
    }
    return false;
}