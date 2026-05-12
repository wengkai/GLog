#ifndef ADIF_WWFF_REF
#define ADIF_WWFF_REF
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include "AdifDataBase.h"
#include "AdifString.h"

/**
 * @brief AdifWWFFRef: (World Wildlife Flora & Fauna)
 *
 */
class GLOGKIT_API AdifWWFFRef : public AdifDataBase {
  protected:
    explicit AdifWWFFRef(std::string value) : AdifDataBase(std::move(value)) {
        normalizeDataToUpper();
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifWWFFRef)

  public:
    static bool check(const std::string &data) {
        size_t len = data.length();
        if (len < 8 || len > 11) return false;

        std::string upperData = data;
        for (char &c : upperData)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        size_t ffDashPos = upperData.find("FF-");

        if (ffDashPos == std::string::npos || ffDashPos < 1 || ffDashPos > 4) {
            return false;
        }

        std::string xx = data.substr(0, ffDashPos);
        if (!AdifString::check(xx)) return false;

        std::string nnnn = data.substr(ffDashPos + 3);
        if (nnnn.length() != 4) return false;
        if (!std::all_of(nnnn.begin(), nnnn.end(), ::isdigit)) return false;

        return true;
    }

    static std::optional<AdifWWFFRef> create(const std::string &data) {
        if (check(data)) {
            return AdifWWFFRef(data);
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;

    struct WwffParts {
        std::string program; //  (K, 3DA)
        std::string number;  //  (4655, 0002)
    };

    WwffParts asParts() const {
        size_t ffDash = m_rawValue.find("FF-");
        WwffParts p;
        p.program = m_rawValue.substr(0, ffDash);
        p.number = m_rawValue.substr(ffDash + 3);
        return p;
    }
};

#endif