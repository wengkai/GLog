#include "AdifGridSquareExt.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifGridSquareExt)

auto AdifGridSquareExt::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        normalizeDataToUpper();
        return {true};
    }
    return {false, std::move(newValue)};
}