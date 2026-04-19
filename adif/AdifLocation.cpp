#include "AdifLocation.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifLocation)

auto AdifLocation::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        m_rawValue[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(m_rawValue[0])));
        return {true};
    }
    return {false, std::move(newValue)};
}