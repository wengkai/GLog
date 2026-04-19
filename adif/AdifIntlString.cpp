#include "AdifIntlString.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifIntlString)

auto AdifIntlString::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}