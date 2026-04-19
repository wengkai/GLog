#include "AdifIntlCharacter.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifIntlCharacter)

auto AdifIntlCharacter::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}