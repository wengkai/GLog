#include "AdifGeneral.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifGeneral)

auto AdifGeneral::take(std::string &&newValue) -> TakeRes {
    m_rawValue = std::move(newValue);
    return {true};
}