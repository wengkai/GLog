#include "AdifCreditList.h"

#include "constants/credit_map.h"
#include "constants/qsl_medium_map.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifCreditList)

auto AdifCreditList::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = normalize(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}

static const auto &CREDIT_MAP = ADIF::CREDIT_MAP;
static const auto &QSL_MEDIUM_MAP = ADIF::QSL_MEDIUM_MAP;

/**
 * A member of the Credit enumeration. or
 * A member of the Credit enumeration followed by a colon and an ampersand-delimited list of members
 * of the QSL_Medium enumeration.
 */
bool AdifCreditList::AdifCreditItem::check(std::string_view data) {
    // Case 1: a pure Credit value
    if (CREDIT_MAP.count(data)) {
        return true;
    }

    // Case 2: "Credit:Medium1&Medium2&..."
    const auto colon_pos = data.find(':');
    if (colon_pos == std::string_view::npos) {
        return false; // no colon, and we already know it's not a plain Credit
    }

    const std::string_view credit_part = data.substr(0, colon_pos);
    if (!CREDIT_MAP.count(credit_part)) {
        return false; // unknown Credit
    }

    const std::string_view media_part = data.substr(colon_pos + 1);
    if (media_part.empty()) {
        return false; // colon present but no medium list
    }

    // Split media_part by '&' and validate each token
    size_t start = 0;
    size_t end;
    while ((end = media_part.find('&', start)) != std::string_view::npos) {
        const auto medium = media_part.substr(start, end - start);
        if (medium.empty() || !QSL_MEDIUM_MAP.count(medium)) {
            return false;
        }
        start = end + 1;
    }
    // Handle the last medium (or the only one if no '&' was found)
    const auto last_medium = media_part.substr(start);
    if (last_medium.empty() || !QSL_MEDIUM_MAP.count(last_medium)) {
        return false;
    }

    return true;
}

void AdifCreditList::AdifCreditItem::normalize(std::string &data) {
    AdifDataBase::normalizeDataToUpper(data);
}
