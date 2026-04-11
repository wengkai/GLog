#include "AdifWWFFRef.h"

bool AdifWWFFRef::set(const std::string &newValue) {
    if (check(newValue)) {
        m_rawValue = newValue;
        for (char &c : m_rawValue) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return true;
    }
    return false;
}