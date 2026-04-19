#include "AdifPOTARefList.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifPOTARefList)

auto AdifPOTARefList::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(normalize(newValue));
        return {true};
    }
    return {false, std::move(newValue)};
}