#include "AdifIOTARefNo.h"

#include "constants/continent_map.h"

static const auto &CONTINENT_MAP = ADIF::CONTINENT_MAP;

/**
 * in format CC-XXX, where
 * CC is a member of the Continent enumeration
 * XXX is the island group designator, where 1 <= XXX <= 999  [use leading zeroes]
 */
auto AdifIOTARefNo::check(std::string_view data) -> bool {
    // 1. Find the hyphen
    const auto hyphen_pos = data.find('-');
    if (hyphen_pos == std::string_view::npos || hyphen_pos != 2) {
        return false; // no hyphen, or continent part not exactly 2 chars
    }

    // 2. Extract continent code
    const std::string_view continent = data.substr(0, 2);

    // 3. Check if it's a valid continent (lookup in the map)
    if (CONTINENT_MAP.find(continent) == CONTINENT_MAP.end()) {
        return false;
    }

    // 4. Extract the numeric part (must be exactly 3 digits)
    const std::string_view num_part = data.substr(hyphen_pos + 1);
    if (num_part.size() != 3) {
        return false;
    }

    // 5. Convert to integer and check range (001..999)
    int number = 0;
    const auto result = std::from_chars(num_part.data(), num_part.data() + num_part.size(), number);
    if (result.ec != std::errc{} || result.ptr != num_part.data() + num_part.size()) {
        return false; // non‑numeric or trailing characters
    }

    return number >= 1 && number <= 999; // leading zeroes are accepted
}

auto AdifIOTARefNo::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        AdifDataBase::normalizeDataToUpper(m_rawValue);
        return {true};
    }
    return {false, std::move(newValue)};
}

ADIF_DATA_TYPE_CLONE_IMP(AdifIOTARefNo)
