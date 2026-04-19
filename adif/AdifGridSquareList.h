#ifndef ADIF_GRID_SQUARE_LIST
#define ADIF_GRID_SQUARE_LIST
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "AdifDataBase.h"
#include "AdifGridSquare.h"

/**
 * @brief AdifGridSquareList
 */
class AdifGridSquareList : public AdifDataBase {
  protected:
    explicit AdifGridSquareList(std::string value) : AdifDataBase(std::move(value)) {
        std::string_view input = m_rawValue;
        std::string normalized;
        normalized.reserve(input.size());

        size_t start = 0;
        bool first = true;
        while (start <= input.size()) {
            size_t commaPos = input.find(',', start);
            if (commaPos == std::string_view::npos) commaPos = input.size();

            std::string_view item = input.substr(start, commaPos - start);

            size_t firstNonSpace = item.find_first_not_of(' ');
            if (firstNonSpace != std::string_view::npos) {
                size_t lastNonSpace = item.find_last_not_of(' ');
                item = item.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
            } else {
                item = std::string_view();
            }

            if (!item.empty()) {
                if (!first) normalized += ',';
                std::string itemStr(item);
                normalizeDataToUpper(itemStr);
                normalized += itemStr;
                first = false;
            }

            start = commaPos + 1;
        }

        m_rawValue = std::move(normalized);
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifGridSquareList)

  public:
    static bool check(std::string_view data) {
        if (data.empty()) return false;

        size_t lastNonSpace = data.find_last_not_of(' ');
        if (lastNonSpace == std::string_view::npos || data[lastNonSpace] == ',') {
            return false;
        }

        size_t start = 0;
        bool hasAtLeastOne = false;
        while (start <= data.size()) {
            size_t commaPos = data.find(',', start);
            if (commaPos == std::string_view::npos) commaPos = data.size();

            std::string_view item = data.substr(start, commaPos - start);

            size_t first = item.find_first_not_of(' ');
            if (first == std::string_view::npos) {
                return false;
            }
            size_t last = item.find_last_not_of(' ');
            std::string_view trimmed = item.substr(first, last - first + 1);

            if (!AdifGridSquare::check(trimmed)) {
                return false;
            }
            hasAtLeastOne = true;

            start = commaPos + 1;
        }

        return hasAtLeastOne;
    }

    template <typename STD_String>
    static std::optional<AdifGridSquareList> create(STD_String &&data) {
        if (check(data)) {
            return AdifGridSquareList(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    std::vector<std::string> asVector() const {
        std::vector<std::string> result;
        std::string_view input = m_rawValue;

        size_t start = 0;
        while (start <= input.size()) {
            size_t commaPos = input.find(',', start);
            if (commaPos == std::string_view::npos) commaPos = input.size();

            result.emplace_back(input.substr(start, commaPos - start));
            start = commaPos + 1;
        }
        return result;
    }

    size_t count() const {
        if (m_rawValue.empty()) return 0;
        size_t cnt = 1;
        for (char c : m_rawValue) {
            if (c == ',') ++cnt;
        }
        return cnt;
    }
};

#endif