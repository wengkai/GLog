#ifndef CASE_INSEN_LESS_H
#define CASE_INSEN_LESS_H

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

struct CaseInsensitiveLess {
    using is_transparent = void;

    static inline char to_lower(char c) { return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c; }

    bool operator()(std::string_view lhs, std::string_view rhs) const {
        return std::lexicographical_compare(
            lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
            [](char a, char b) { return to_lower(a) < to_lower(b); });
    }
};

#endif
