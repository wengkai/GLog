#include "AdifGridSquareList.h"

bool AdifGridSquareList::set(const std::string &newValue) {
    if (check(newValue)) {

        *this = AdifGridSquareList(newValue);
        return true;
    }
    return false;
}