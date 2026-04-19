#include "AdifMultilineString.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifMultilineString)

auto AdifMultilineString::take(std::string &&newValue) -> TakeRes {
    std::string cleanedData = sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return {true};
    }
    return {false, std::move(newValue)};
}