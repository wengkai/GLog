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
  private:
    explicit AdifGeneral(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string & /*data*/) { return true; }

    static std::optional<AdifGeneral> create(const std::string &data) { return AdifGeneral(data); }

    bool set(const std::string &newValue) override;
};

#endif