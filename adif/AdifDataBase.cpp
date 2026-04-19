#include "AdifDataBase.h"

#include <algorithm>

AdifDataBase::AdifDataBase(std::string value) : m_rawValue(std::move(value)) {}

AdifDataBase::AdifDataBase(std::string value, std::string pendingValue, std::any externed)
    : m_rawValue(std::move(value)), m_pendingValue(std::move(pendingValue)),
      externed(std::move(externed)) {}

void AdifDataBase::normalizeDataToUpper() { normalizeDataToUpper(m_rawValue); }

void AdifDataBase::normalizeDataToUpper(std::string &data) {
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c) { return std::toupper(c); });
}

auto AdifDataBase::normalizeDataToUpper(const std::string &data) -> std::string {
    auto ret = data;
    normalizeDataToUpper(ret);
    return ret;
}

AdifDataBase::~AdifDataBase() = default;

auto AdifDataBase::get() const -> std::string { return m_rawValue; }

auto AdifDataBase::compare(const AdifDataBase &right) const -> CompareRes {
    if (typeid(*this) != typeid(right)) {
        throw std::invalid_argument("Cannot compare with different DataBase type");
    }

    const auto &leftVal = m_rawValue;
    auto result = leftVal.compare(right.get());

    if (result < 0) {
        return CompareRes::less;
    }
    if (result > 0) {
        return CompareRes::greater;
    }
    return CompareRes::equal_to;
}
