#include "AdifNumber.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifNumber)

auto AdifNumber::take(std::string &&newValue) -> TakeRes {
    if (check(newValue)) {
        m_rawValue = std::move(newValue);
        return {true};
    }
    return {false, std::move(newValue)};
}

double AdifNumber::asDouble() const { return std::stod(m_rawValue); }

auto AdifNumber::compare(const AdifDataBase &right) const -> CompareRes {
    const auto *check_right = dynamic_cast<decltype(this)>(&right);

    if (check_right == nullptr) {
        throw std::invalid_argument("Cannot compare Number with different DataBase type");
    }

    return compare_rational(m_rawValue, check_right->m_rawValue);
}

auto AdifNumber::compare_rational(std::string_view a, std::string_view b) -> CompareRes {
    bool a_neg = (!a.empty() && a[0] == '-');
    size_t a_num_start = a_neg ? 1 : 0;
    size_t a_dot = a.find('.', a_num_start);

    std::string_view a_int =
        a.substr(a_num_start,
                 (a_dot == std::string_view::npos) ? a.size() - a_num_start : a_dot - a_num_start);
    std::string_view a_frac =
        (a_dot != std::string_view::npos) ? a.substr(a_dot + 1) : std::string_view();

    size_t a_int_nonzero = a_int.find_first_not_of('0');
    std::string_view a_int_trim = (a_int_nonzero == std::string_view::npos)
                                      ? std::string_view()
                                      : a_int.substr(a_int_nonzero);

    bool a_is_zero = a_int_trim.empty() &&
                     (a_frac.empty() || a_frac.find_first_not_of('0') == std::string_view::npos);

    bool b_neg = (!b.empty() && b[0] == '-');
    size_t b_num_start = b_neg ? 1 : 0;
    size_t b_dot = b.find('.', b_num_start);

    std::string_view b_int =
        b.substr(b_num_start,
                 (b_dot == std::string_view::npos) ? b.size() - b_num_start : b_dot - b_num_start);
    std::string_view b_frac =
        (b_dot != std::string_view::npos) ? b.substr(b_dot + 1) : std::string_view();

    size_t b_int_nonzero = b_int.find_first_not_of('0');
    std::string_view b_int_trim = (b_int_nonzero == std::string_view::npos)
                                      ? std::string_view()
                                      : b_int.substr(b_int_nonzero);

    bool b_is_zero = b_int_trim.empty() &&
                     (b_frac.empty() || b_frac.find_first_not_of('0') == std::string_view::npos);

    if (a_is_zero && b_is_zero) {
        return equal_to;
    }
    if (a_is_zero) {
        return b_neg ? greater : less;
    }
    if (b_is_zero) {
        return a_neg ? less : greater;
    }

    if (a_neg != b_neg) {
        return a_neg ? less : greater;
    }

    bool both_negative = a_neg;

    if (a_int_trim.size() != b_int_trim.size()) {
        bool a_longer = a_int_trim.size() > b_int_trim.size();
        if (both_negative) a_longer = !a_longer;
        return a_longer ? greater : less;
    }

    int int_cmp = a_int_trim.compare(b_int_trim);
    if (int_cmp != 0) {
        bool a_greater = int_cmp > 0;
        if (both_negative) a_greater = !a_greater;
        return a_greater ? greater : less;
    }

    size_t max_frac_len = std::max(a_frac.size(), b_frac.size());
    for (size_t i = 0; i < max_frac_len; ++i) {
        char ca = (i < a_frac.size()) ? a_frac[i] : '0';
        char cb = (i < b_frac.size()) ? b_frac[i] : '0';
        if (ca != cb) {
            bool a_greater = ca > cb;
            if (both_negative) a_greater = !a_greater;
            return a_greater ? greater : less;
        }
    }

    return equal_to;
}
