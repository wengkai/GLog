#include "AdifMultilineString.h"

bool AdifMultilineString::set(const std::string &newValue) {
    std::string cleanedData = sanitizeLineEndings(newValue);
    if (check(cleanedData)) {
        m_rawValue = std::move(cleanedData);
        return true;
    }
    return false;
}