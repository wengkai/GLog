#include "AdifIntlMultilineString.h"

bool AdifIntlMultilineString::set(const std::string &newValue) {
    std::string cleanedData = AdifMultilineString::sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return true;
    }
    return false;
}