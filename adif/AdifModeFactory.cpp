#include "AdifDataTypes.h"

#include "mode_map.h"
#include "submode_map.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifMode)
ADIF_DATA_TYPE_CLONE_IMP(AdifSubMode)

auto AdifMode::check(const std::string &data) -> bool {
    return _m_check(normalizeDataToUpper(data));
}

auto AdifMode::_m_check(const std::string &data) -> bool {
    return ADIF::MODE_MAP.find(data) != ADIF::MODE_MAP.end();
}

auto AdifMode::take(std::string &&newValue) -> TakeRes {

    auto n_newValue = std::move(newValue);

    normalizeDataToUpper(n_newValue);

    if (!_m_check(n_newValue)) {
        return {false, std::nullopt};
    }

    std::string targetMode = n_newValue;
    std::string targetSubMode;

    auto it = ADIF::MODE_MAP.find(n_newValue);
    if (it != ADIF::MODE_MAP.end() && it->second.import_only) {
        auto subIt = ADIF::SUBMODE_MAP.find(n_newValue);
        if (subIt != ADIF::SUBMODE_MAP.end()) {
            targetMode = subIt->second.parent_mode;
            targetSubMode = n_newValue;
        }
    }

    auto subPeer = m_subModePeer.lock();
    if (subPeer) {

        if (!targetSubMode.empty()) {
            this->m_rawValue = targetMode;
            subPeer->m_rawValue = targetSubMode;
            subPeer->m_pendingValue = "";
            return {true};
        }

        const auto &validSubmodes = ADIF::MODE_MAP.at(targetMode).submodes;

        if (!subPeer->m_pendingValue.empty()) {
            auto foundPending =
                std::find(validSubmodes.begin(), validSubmodes.end(), subPeer->m_pendingValue);
            if (foundPending != validSubmodes.end()) {
                this->m_rawValue = targetMode;
                subPeer->m_rawValue = subPeer->m_pendingValue;
                subPeer->m_pendingValue = "";
                return {true};
            }
        }

        std::string currentSub = subPeer->get();
        bool isCompatible = currentSub.empty() ||
                            (std::find(validSubmodes.begin(), validSubmodes.end(), currentSub) !=
                             validSubmodes.end());

        if (isCompatible) {
            this->m_rawValue = targetMode;

            subPeer->m_pendingValue = "";
        } else {

            this->m_rawValue = targetMode;
            subPeer->m_rawValue = "";
            subPeer->m_pendingValue = "";
        }
        return {true};
    }

    this->m_rawValue = targetMode;
    return {true};
}

auto AdifSubMode::check(const std::string & /*data*/) -> bool {

    throw std::runtime_error("AdifSubMode requires context. Use AdifModeFactory::check instead.");
    return false;
}

auto AdifSubMode::take(std::string &&newValue) -> TakeRes {

    if (newValue.empty()) {
        m_rawValue = "";
        m_pendingValue = "";
        return {true};
    }

    auto n_newValue = std::move(newValue);

    normalizeDataToUpper(n_newValue);

    auto modePeer = m_modePeer.lock();
    if (!modePeer) {

        m_pendingValue = std::move(n_newValue);
        return {false, std::nullopt};
    }

    std::string currentMode = modePeer->get();

    auto it = ADIF::MODE_MAP.find(currentMode);
    if (it != ADIF::MODE_MAP.end()) {
        const auto &allowedSubmodes = it->second.submodes;
        auto found = std::find(allowedSubmodes.begin(), allowedSubmodes.end(), n_newValue);

        if (found != allowedSubmodes.end()) {

            m_rawValue = n_newValue;
            m_pendingValue = "";
            return {true};
        }
    }

    m_pendingValue = std::move(n_newValue);
    return {false, std::nullopt};
}

auto AdifModeFactory::_m_check(const std::string &mode, const std::string &submode) -> bool {

    auto it = ADIF::MODE_MAP.find(mode);
    if (it == ADIF::MODE_MAP.end()) {
        return false;
    }

    if (submode.empty()) {
        return true;
    }

    const auto &subList = it->second.submodes;
    return std::find(subList.begin(), subList.end(), submode) != subList.end();
}

auto AdifModeFactory::check(const std::string &mode, const std::string &submode) -> bool {
    return _m_check(AdifMode::normalizeDataToUpper(mode),
                    AdifSubMode::normalizeDataToUpper(submode));
}

auto AdifModeFactory::getModeMap() -> const ModeMap & {
    static ModeMap map{[]() -> ModeMap {
        ModeMap m_map;
        for (const auto &[key, value] : ADIF::MODE_MAP) {
            const auto &import_only = value.import_only;
            const auto &submodes = value.submodes;
            m_map.emplace(key, ModeInfo{import_only, submodes});
        }
        return m_map;
    }()};
    return map;
}

auto AdifModeFactory::_createModePair(std::string &&mode, std::string &&submode)
    -> std::pair<std::shared_ptr<AdifMode>, std::shared_ptr<AdifSubMode>> {

    std::string finalMode = std::move(mode);
    std::string finalSubMode = std::move(submode);

    AdifMode::normalizeDataToUpper(finalMode);
    AdifSubMode::normalizeDataToUpper(finalSubMode);

    auto it = ADIF::MODE_MAP.find(finalMode);
    if (it != ADIF::MODE_MAP.end() && it->second.import_only) {
        auto subIt = ADIF::SUBMODE_MAP.find(finalMode);
        if (subIt != ADIF::SUBMODE_MAP.end()) {
            finalSubMode = std::move(finalMode);
            finalMode = subIt->second.parent_mode;
        }
    }

    auto subIt = ADIF::SUBMODE_MAP.find(finalSubMode);
    if (subIt != ADIF::SUBMODE_MAP.end()) {
        finalMode = subIt->second.parent_mode;
    }

    bool valid_pair = _m_check(finalMode, finalSubMode);

    auto modePtr = std::shared_ptr<AdifMode>(new AdifMode(finalMode));
    auto subModePtr = std::shared_ptr<AdifSubMode>(new AdifSubMode(finalSubMode));

    if (!valid_pair) {
        modePtr->switchPending();
        subModePtr->switchPending();
    }

    modePtr->m_subModePeer = subModePtr;
    subModePtr->m_modePeer = modePtr;

    return {modePtr, subModePtr};
}
