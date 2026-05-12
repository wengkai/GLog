#ifndef ADIF_MODE_FACTORY_H
#define ADIF_MODE_FACTORY_H

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "AdifDataBase.h"

class AdifSubMode;
class AdifMode;

struct ModeInfo {
    bool import_only;
    std::vector<std::string> submodes;
};

using ModeMap = std::map<std::string, ModeInfo>;

class GLOGKIT_API AdifModeFactory {

    static bool _m_check(const std::string &mode, const std::string &submode);

    static std::pair<std::shared_ptr<AdifMode>, std::shared_ptr<AdifSubMode>>
    _createModePair(std::string &&mode, std::string &&submode);

  public:
    static bool check(const std::string &mode, const std::string &submode);

    static std::pair<std::shared_ptr<AdifMode>, std::shared_ptr<AdifSubMode>>
    createModePair(std::string mode, std::string submode) {
        return _createModePair(std::move(mode), std::move(submode));
    }

    static const ModeMap &getModeMap();
};

/**
 * @brief AdifMode
 */
class GLOGKIT_API AdifMode : public AdifDataBase {
    friend class AdifSubMode;
    friend class AdifModeFactory;

  private:
    std::weak_ptr<AdifSubMode> m_subModePeer;

    static bool _m_check(const std::string &data);

    explicit AdifMode(std::string value) : AdifDataBase(std::move(value)) {}

  protected:
    ADIF_DATA_TYPE_CLONE_DEC(AdifMode)

  public:
    static bool check(const std::string &data);

    TakeRes take(std::string &&newValue) override;

    static std::optional<AdifMode> create(const std::string &data) = delete;

    bool hasPeer() const { return !m_subModePeer.expired(); }

    void setPeer(std::shared_ptr<AdifSubMode> peer) { m_subModePeer = peer; }

    explicit AdifMode(const AdifMode &) = delete;
    AdifMode &operator=(const AdifMode &) = delete;
};

/**
 * @brief AdifSubMode
 */
class GLOGKIT_API AdifSubMode : public AdifDataBase {
    friend class AdifMode;
    friend class AdifModeFactory;

  private:
    std::weak_ptr<AdifMode> m_modePeer;

    explicit AdifSubMode(std::string value) : AdifDataBase(std::move(value)) {}

  protected:
    ADIF_DATA_TYPE_CLONE_DEC(AdifSubMode)

  public:
    static bool check(const std::string &data);

    TakeRes take(std::string &&newValue) override;

    static std::optional<AdifSubMode> create(const std::string &data) = delete;

    bool hasPeer() const { return !m_modePeer.expired(); }

    void setPeer(std::shared_ptr<AdifMode> peer) { m_modePeer = peer; }

    explicit AdifSubMode(const AdifSubMode &) = delete;
    AdifSubMode &operator=(const AdifSubMode &) = delete;
};

#endif