#include "AdifIntlMultilineString.h"

auto AdifIntlMultilineString::set(const std::string &newValue) -> bool {
    std::string cleanedData = AdifMultilineString::sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return true;
    }
    return false;
}