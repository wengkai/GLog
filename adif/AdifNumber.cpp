#include "AdifNumber.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifNumber)

auto AdifNumber::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}

auto AdifNumber::asDouble() const -> double { return std::stod(m_rawValue); }

auto AdifNumber::compare(const AdifDataBase &right) const -> CompareRes {
    const auto *check_right = dynamic_cast<decltype(this)>(&right);

    if (check_right == nullptr) {
        throw std::invalid_argument("Cannot compare Number with different DataBase type");
    }

    return compare_rational(m_rawValue, check_right->m_rawValue);
}

struct RationalView {
    bool is_neg;
    bool is_zero;
    std::string_view integer;
    std::string_view fraction;
};

// Helper to parse and normalize the string view
static inline auto parse_rational(std::string_view s) -> RationalView {
    bool neg = (!s.empty() && s[0] == '-');
    size_t start = neg ? 1 : 0;
    size_t dot = s.find('.', start);

    std::string_view i_part =
        s.substr(start, (dot == std::string_view::npos) ? s.size() - start : dot - start);
    std::string_view f_part =
        (dot != std::string_view::npos) ? s.substr(dot + 1) : std::string_view();

    // Trim leading zeros from integer
    size_t first_digit = i_part.find_first_not_of('0');
    i_part = (first_digit == std::string_view::npos) ? "" : i_part.substr(first_digit);

    // Check if effectively zero
    bool is_zero = i_part.empty() &&
                   (f_part.empty() || f_part.find_first_not_of('0') == std::string_view::npos);

    return {neg && !is_zero, is_zero, i_part, f_part};
}

static inline auto compare_magnitudes(const RationalView &a, const RationalView &b)
    -> AdifNumber::CompareRes {

    using CompareRes = AdifNumber::CompareRes;

    // 1. Integer length check
    if (a.integer.size() != b.integer.size()) {
        return a.integer.size() > b.integer.size() ? CompareRes::greater : CompareRes::less;
    }

    // 2. Integer value check
    if (int int_cmp = a.integer.compare(b.integer); int_cmp != 0) {
        return int_cmp > 0 ? CompareRes::greater : CompareRes::less;
    }

    // 3. Fractional check
    const size_t max_len = std::max(a.fraction.size(), b.fraction.size());
    for (size_t i = 0; i < max_len; ++i) {
        char ca = (i < a.fraction.size()) ? a.fraction[i] : '0';
        char cb = (i < b.fraction.size()) ? b.fraction[i] : '0';
        if (ca != cb) {
            return ca > cb ? CompareRes::greater : CompareRes::less;
        }
    }

    return CompareRes::equal_to;
}

auto AdifNumber::compare_rational(std::string_view a_str, std::string_view b_str) -> CompareRes {
    auto a = parse_rational(a_str);
    auto b = parse_rational(b_str);

    // 1. Zero handling
    if (a.is_zero && b.is_zero) {
        return equal_to;
    }
    if (a.is_zero) {
        return b.is_neg ? greater : less;
    }
    if (b.is_zero) {
        return a.is_neg ? less : greater;
    }

    // 2. Sign handling
    if (a.is_neg != b.is_neg) {
        return a.is_neg ? less : greater;
    }

    // 3. Magnitude comparison (Absolute Value)
    CompareRes res = compare_magnitudes(a, b);

    // 4. Flip result if both are negative
    if (a.is_neg && res != equal_to) {
        return (res == greater) ? less : greater;
    }
    return res;
}
