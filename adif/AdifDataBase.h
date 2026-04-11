#ifndef ADIF_DATA_BASE
#define ADIF_DATA_BASE
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

/**
 * @brief ADIF Meta Base
 */
class AdifDataBase {
  protected:
    std::string m_rawValue;

    explicit AdifDataBase(std::string value);

  public:
    virtual ~AdifDataBase();

    virtual std::string get() const;

    virtual bool set(const std::string &newValue) = 0;
};

#endif