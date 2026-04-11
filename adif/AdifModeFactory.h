#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "AdifDataBase.h"

class AdifSubMode;
class AdifMode;

class AdifModeFactory {
  public:
    static bool check(const std::string &mode, const std::string &submode);

    static std::pair<std::shared_ptr<AdifMode>, std::shared_ptr<AdifSubMode>>
    createModePair(const std::string &mode, const std::string &submode);
};

/**
 * @brief AdifMode
 */
class AdifMode : public AdifDataBase {
    friend class AdifSubMode;
    friend class AdifModeFactory;

  private:
    std::weak_ptr<AdifSubMode> m_subModePeer;

    explicit AdifMode(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data);

    bool set(const std::string &newValue) override;

    static std::optional<AdifMode> create(const std::string &data) = delete;
};

/**
 * @brief AdifSubMode
 */
class AdifSubMode : public AdifDataBase {
    friend class AdifMode;
    friend class AdifModeFactory;

  private:
    std::weak_ptr<AdifMode> m_modePeer;
    std::string m_pendingValue;

    explicit AdifSubMode(std::string value) : AdifDataBase(std::move(value)) {}

  public:
    static bool check(const std::string &data);

    bool set(const std::string &newValue) override;

    static std::optional<AdifSubMode> create(const std::string &data) = delete;
};