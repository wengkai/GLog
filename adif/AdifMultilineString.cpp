#include "AdifMultilineString.h"

auto AdifMultilineString::set(const std::string &newValue) -> bool {
    std::string cleanedData = sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return true;
    }
    return false;
}