#include "AdifGridSquareList.h"

auto AdifGridSquareList::set(const std::string &newValue) -> bool {
    if (check(newValue)) {

        *this = AdifGridSquareList(newValue);
        return true;
    }
    return false;
}