#include "AdifDataTypes.h"

bool AdifMode::check(const std::string &data) {

    return ADIF::MODE_MAP.find(data) != ADIF::MODE_MAP.end();
}

bool AdifMode::set(const std::string &newValue) {

    if (!check(newValue)) {
        return false;
    }

    std::string targetMode = newValue;
    std::string targetSubMode = "";

    auto it = ADIF::MODE_MAP.find(newValue);
    if (it != ADIF::MODE_MAP.end() && it->second.import_only) {
        auto subIt = ADIF::SUBMODE_MAP.find(newValue);
        if (subIt != ADIF::SUBMODE_MAP.end()) {
            targetMode = subIt->second.parent_mode;
            targetSubMode = newValue;
        }
    }

    auto subPeer = m_subModePeer.lock();
    if (subPeer) {

        if (!targetSubMode.empty()) {
            this->m_rawValue = targetMode;
            subPeer->m_rawValue = targetSubMode;
            subPeer->m_pendingValue = "";
            return true;
        }

        const auto &validSubmodes = ADIF::MODE_MAP.at(targetMode).submodes;

        if (!subPeer->m_pendingValue.empty()) {
            auto foundPending =
                std::find(validSubmodes.begin(), validSubmodes.end(), subPeer->m_pendingValue);
            if (foundPending != validSubmodes.end()) {
                this->m_rawValue = targetMode;
                subPeer->m_rawValue = subPeer->m_pendingValue;
                subPeer->m_pendingValue = "";
                return true;
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
        return true;
    }

    this->m_rawValue = targetMode;
    return true;
}

bool AdifSubMode::check(const std::string & /*data*/) {

    throw std::runtime_error("AdifSubMode requires context. Use AdifModeFactory::check instead.");
    return false;
}

bool AdifSubMode::set(const std::string &newValue) {

    if (newValue.empty()) {
        m_rawValue = "";
        m_pendingValue = "";
        return true;
    }

    auto modePeer = m_modePeer.lock();
    if (!modePeer) {

        m_pendingValue = newValue;
        return false;
    }

    std::string currentMode = modePeer->get();

    auto it = ADIF::MODE_MAP.find(currentMode);
    if (it != ADIF::MODE_MAP.end()) {
        const auto &allowedSubmodes = it->second.submodes;
        auto found = std::find(allowedSubmodes.begin(), allowedSubmodes.end(), newValue);

        if (found != allowedSubmodes.end()) {

            m_rawValue = newValue;
            m_pendingValue = "";
            return true;
        }
    }

    m_pendingValue = newValue;
    return false;
}

bool AdifModeFactory::check(const std::string &mode, const std::string &submode) {

    auto it = ADIF::MODE_MAP.find(mode);
    if (it == ADIF::MODE_MAP.end()) return false;

    if (submode.empty()) return true;

    const auto &subList = it->second.submodes;
    return std::find(subList.begin(), subList.end(), submode) != subList.end();
}

std::pair<std::shared_ptr<AdifMode>, std::shared_ptr<AdifSubMode>>
AdifModeFactory::createModePair(const std::string &mode, const std::string &submode) {

    std::string finalMode = mode;
    std::string finalSubMode = submode;

    auto it = ADIF::MODE_MAP.find(mode);
    if (it != ADIF::MODE_MAP.end() && it->second.import_only) {
        auto subIt = ADIF::SUBMODE_MAP.find(mode);
        if (subIt != ADIF::SUBMODE_MAP.end()) {
            finalMode = subIt->second.parent_mode;
            finalSubMode = mode;
        }
    }

    auto subIt = ADIF::SUBMODE_MAP.find(submode);
    if (subIt != ADIF::SUBMODE_MAP.end()) {
        finalMode = subIt->second.parent_mode;
    }

    if (!check(finalMode, finalSubMode)) {
        finalMode.clear();
        finalSubMode.clear();
    }

    auto modePtr = std::shared_ptr<AdifMode>(new AdifMode(finalMode));
    auto subModePtr = std::shared_ptr<AdifSubMode>(new AdifSubMode(finalSubMode));

    modePtr->m_subModePeer = subModePtr;
    subModePtr->m_modePeer = modePtr;

    return {modePtr, subModePtr};
}
