#ifndef ADIF_COMMA_LIST
#define ADIF_COMMA_LIST
#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

template <typename ItemT> class AdifCommaList {
  public:
    template <typename ItemNorm>
    [[nodiscard]] static std::string normalize(std::string_view input, ItemNorm &&item_norm) {
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
                std::invoke(std::forward<ItemNorm>(item_norm), itemStr);
                normalized += itemStr;
                first = false;
            }

            start = commaPos + 1;
        }

        return normalized;
    }

    template <typename ItemCheck> static bool check(std::string_view data, ItemCheck &&item_check) {
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

            if (!std::invoke(std::forward<ItemCheck>(item_check), trimmed)) {
                return false;
            }
            hasAtLeastOne = true;

            start = commaPos + 1;
        }

        return hasAtLeastOne;
    }

    static bool check(std::string_view data) {
        return check(data, [](std::string_view d) { return ItemT::check(d); });
    }
};

#endif