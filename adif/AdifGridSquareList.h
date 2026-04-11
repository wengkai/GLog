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
 *
 */
class AdifGridSquareList : public AdifDataBase {
  private:
    explicit AdifGridSquareList(std::string value) : AdifDataBase(std::move(value)) {
        std::string normalized;
        std::stringstream ss(m_rawValue);
        std::string item;
        bool first = true;

        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" "));
            item.erase(item.find_last_not_of(" ") + 1);

            if (!item.empty()) {
                if (!first) normalized += ",";
                for (char &c : item)
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                normalized += item;
                first = false;
            }
        }
        m_rawValue = normalized;
    }

  public:
    static bool check(const std::string &data) {
        if (data.empty()) return false;

        size_t lastPos = data.find_last_not_of(" ");
        if (lastPos == std::string::npos || data[lastPos] == ',') {
            return false;
        }

        std::stringstream ss(data);
        std::string item;
        bool hasAtLeastOne = false;

        while (std::getline(ss, item, ',')) {
            size_t first = item.find_first_not_of(" ");
            if (first == std::string::npos) return false;

            size_t last = item.find_last_not_of(" ");
            std::string trimmedItem = item.substr(first, (last - first + 1));

            if (!AdifGridSquare::check(trimmedItem)) {
                return false;
            }
            hasAtLeastOne = true;
        }

        return hasAtLeastOne;
    }

    static std::optional<AdifGridSquareList> create(const std::string &data) {
        if (check(data)) {
            return AdifGridSquareList(data);
        }
        return std::nullopt;
    }

    bool set(const std::string &newValue) override;

    std::vector<std::string> asVector() const {
        std::vector<std::string> result;
        std::stringstream ss(m_rawValue);
        std::string item;
        while (std::getline(ss, item, ',')) {
            result.push_back(item);
        }
        return result;
    }

    size_t count() const {
        if (m_rawValue.empty()) return 0;
        return std::count(m_rawValue.begin(), m_rawValue.end(), ',') + 1;
    }
};

#endif