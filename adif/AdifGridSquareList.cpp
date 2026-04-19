#include "AdifGridSquareList.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifGridSquareList)

auto AdifGridSquareList::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        *this = AdifGridSquareList(std::move(newValue));
        return {true};
    }
    return {false, std::move(newValue)};
}