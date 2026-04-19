#ifndef ADIF_SOTA_REF
#define ADIF_SOTA_REF
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"
#include "AdifString.h"

/**
 * @brief AdifSOTARef: SOTA
 *
 */
class AdifSOTARef : public AdifDataBase {
  protected:
    explicit AdifSOTARef(std::string value) : AdifDataBase(std::move(value)) {
        normalizeDataToUpper();
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifSOTARef)

  public:
    static bool check(const std::string &data) {
        if (data.empty()) return false;

        size_t slashPos = data.find('/');
        if (slashPos == std::string::npos || slashPos == 0 || slashPos == data.length() - 1) {
            return false;
        }

        if (!AdifString::check(data)) return false;

        std::string suffix = data.substr(slashPos + 1);
        if (suffix.find('-') == std::string::npos) {
            return false;
        }

        if (data.length() < 5 || data.length() > 15) return false;

        return true;
    }

    static std::optional<AdifSOTARef> create(const std::string &data) {
        if (check(data)) {
            return AdifSOTARef(data);
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    struct SotaParts {
        std::string association; // (G, W2)
        std::string region;      // (LD, WE)
        std::string reference;   // (003)
    };

    SotaParts asParts() const {
        size_t slash = m_rawValue.find('/');
        size_t dash = m_rawValue.find('-', slash);

        SotaParts p;
        p.association = m_rawValue.substr(0, slash);
        if (dash != std::string::npos) {
            p.region = m_rawValue.substr(slash + 1, dash - (slash + 1));
            p.reference = m_rawValue.substr(dash + 1);
        }
        return p;
    }
};

#endif