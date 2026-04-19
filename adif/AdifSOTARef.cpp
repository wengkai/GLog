#include "AdifSOTARef.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifSOTARef)

auto AdifSOTARef::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        normalizeDataToUpper();
        return {true};
    }
    return {false, std::move(newValue)};
}