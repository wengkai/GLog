#ifndef ADIF_POTA_REF
#define ADIF_POTA_REF
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include "AdifDataBase.h"
#include "AdifString.h"

/**
 * @brief AdifPOTARef: Parks on the Air
 *
 */
class GLOGKIT_API AdifPOTARef : public AdifDataBase {
  protected:
    explicit AdifPOTARef(std::string value) : AdifDataBase(std::move(value)) {
        normalizeDataToUpper();
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifPOTARef)

  public:
    static bool check(std::string_view data) {
        size_t len = data.length();
        if (len < 6 || len > 17) return false;

        size_t dashPos = data.find('-');
        size_t atPos = data.find('@');

        if (dashPos == std::string_view::npos || dashPos == 0) return false;

        std::string_view xxxx = data.substr(0, dashPos);
        if (xxxx.length() > 4) return false;
        if (!AdifString::check(xxxx)) return false;

        size_t numEnd = (atPos == std::string_view::npos) ? len : atPos;
        if (numEnd <= dashPos + 1) return false;

        std::string_view nnnnn = data.substr(dashPos + 1, numEnd - (dashPos + 1));
        if (nnnnn.length() != 4 && nnnnn.length() != 5) return false;
        if (!std::all_of(nnnnn.begin(), nnnnn.end(), ::isdigit)) return false;

        if (atPos != std::string_view::npos) {
            if (atPos < dashPos + 5) return false;

            std::string_view yyyyyy = data.substr(atPos + 1);
            if (yyyyyy.length() < 4 || yyyyyy.length() > 6) return false;
            if (!AdifString::check(yyyyyy)) return false;
        }

        return true;
    }

    static std::optional<AdifPOTARef> create(const std::string &data) {
        if (check(data)) {
            return AdifPOTARef(data);
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    struct PotaParts {
        std::string program;  // xxxx
        std::string number;   // nnnnn
        std::string location; // yyyyyy (empty if none)
    };

    PotaParts asParts() const {
        size_t dash = m_rawValue.find('-');
        size_t at = m_rawValue.find('@');

        PotaParts p;
        p.program = m_rawValue.substr(0, dash);
        if (at == std::string::npos) {
            p.number = m_rawValue.substr(dash + 1);
        } else {
            p.number = m_rawValue.substr(dash + 1, at - (dash + 1));
            p.location = m_rawValue.substr(at + 1);
        }
        return p;
    }
};

#endif