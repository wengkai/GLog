#ifndef ADIF_GENERAL
#define ADIF_GENERAL
#include <optional>
#include <string>
#include <utility>
#include "AdifDataBase.h"

/**
 * @brief AdifGeneral
 */
class AdifGeneral : public AdifDataBase {
  protected:
    explicit AdifGeneral(std::string value) : AdifDataBase(std::move(value)) {}

    ADIF_DATA_TYPE_CLONE_DEC(AdifGeneral)

  public:
    static bool check(std::string_view /*data*/) { return true; }

    template <typename STD_String> static std::optional<AdifGeneral> create(STD_String &&data) {
        return AdifGeneral(std::forward<STD_String>(data));
    }

    TakeRes take(std::string &&newValue) override;
};

#endif