#ifndef ADIF_POTA_REF_LIST
#define ADIF_POTA_REF_LIST
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "AdifCommaList.h"
#include "AdifDataBase.h"
#include "AdifPOTARef.h"

/**
 * @brief AdifPOTARefList: POTA
 *
 */
class AdifPOTARefList : public AdifDataBase {
    using ListUtil = AdifCommaList<AdifPOTARef>;

  protected:
    explicit AdifPOTARefList(std::string value) : AdifDataBase(std::move(value)) {
        m_rawValue = std::move(normalize(m_rawValue));
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifPOTARefList)

    static std::string normalize(const std::string &data) {
        return ListUtil::normalize(data,
                                   [](std::string &d) { AdifDataBase::normalizeDataToUpper(d); });
    }

  public:
    static bool check(std::string_view data) { return ListUtil::check(data); }

    static std::optional<AdifPOTARefList> create(const std::string &data) {
        if (check(data)) {
            return AdifPOTARefList(data);
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

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