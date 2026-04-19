#include "AdifPositiveInteger.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifPositiveInteger)

auto AdifPositiveInteger::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}