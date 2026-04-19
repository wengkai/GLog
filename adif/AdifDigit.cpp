#include "AdifDigit.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifDigit)

auto AdifDigit::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}