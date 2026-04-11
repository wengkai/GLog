#include "AdifIntlCharacter.h"

bool AdifIntlCharacter::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = newValue;
        return true;
    }
    return false;
}