#include "AdifIntlMultilineString.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifIntlMultilineString)

auto AdifIntlMultilineString::take(std::string &&newValue) -> TakeRes {
    std::string cleanedData = AdifMultilineString::sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return {true};
    }
    return {false, std::move(newValue)};
}