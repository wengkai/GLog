#ifndef ADIF_CREDIT_LIST
#define ADIF_CREDIT_LIST
#include <cctype>
#include <optional>
#include <string>
#include "AdifCommaList.h"
#include "AdifDataBase.h"

class AdifCreditList : public AdifDataBase {
    class AdifCreditItem {
      public:
        static bool check(std::string_view data);
        static void normalize(std::string &data);
    };

    using ListUtil = AdifCommaList<AdifCreditItem>;

    static std::string normalize(const std::string &data) {
        return ListUtil::normalize(data, [](std::string &d) { AdifCreditItem::normalize(d); });
    }

  protected:
    explicit AdifCreditList(std::string value) : AdifDataBase(std::move(value)) {
        m_rawValue = normalize(m_rawValue);
    }

    ADIF_DATA_TYPE_CLONE_DEC(AdifCreditList)

  public:
    static bool check(std::string_view data) { return ListUtil::check(data); }

    template <typename STD_String> static std::optional<AdifCreditList> create(STD_String &&data) {
        if (check(data)) {
            return AdifCreditList(std::forward<STD_String>(data));
        }
        return std::nullopt;
    }

    TakeRes take(std::string &&newValue) override;
};

#endif