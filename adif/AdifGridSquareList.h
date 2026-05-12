#ifndef ADIF_GRID_SQUARE_LIST
#define ADIF_GRID_SQUARE_LIST
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "AdifCommaList.h"
#include "AdifDataBase.h"
#include "AdifGridSquare.h"

/**
 * @brief AdifGridSquareList
 */
class GLOGKIT_API AdifGridSquareList : public AdifDataBase {
    using ListUtil = AdifCommaList<AdifGridSquare>;

  protected:
    explicit AdifGridSquareList(std::string value) : AdifDataBase(std::move(value)) {
        m_rawValue = std::move(ListUtil::normalize(m_rawValue, [this](std::string &data) {
            AdifGridSquareList::normalizeDataToUpper(data);
        }));
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifGridSquareList)

  public:
    static bool check(std::string_view data) { return ListUtil::check(data); }

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