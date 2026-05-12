#include "AdifFreqBandFactory.h"
#include "band_map.h"

ADIF_DATA_TYPE_CLONE_IMP(AdifFreq)
ADIF_DATA_TYPE_CLONE_IMP(AdifBand)

static const auto &BAND_MAP = ADIF::BAND_MAP;

// ================================================

static auto safeStod(std::string_view sv) -> std::optional<double> {
    double val = 0.0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec == std::errc() && ptr == sv.data() + sv.size()) {
        return val;
    }
    return std::nullopt;
}

// ======================== AdifFreq ========================

AdifFreq::AdifFreq(std::string value) : AdifDataBase(std::move(value)) {}

auto AdifFreq::parseFrequency(std::string_view sv) -> std::optional<double> { return safeStod(sv); }

auto AdifFreq::deriveBand(std::string_view freqMHz) -> std::optional<std::string> {
    for (const auto &[bandName, range] : BAND_MAP) {
        if (AdifNumber::in_range(range.lower_mhz, freqMHz, range.upper_mhz)) {
            return bandName;
        }
    }
    return std::nullopt;
}

auto AdifFreq::isFreqInBand(std::string_view freqMHz, std::string_view band) -> bool {
    return AdifBand::isFreqInBand(freqMHz, band);
}

auto AdifFreq::take(std::string &&newValue) -> TakeRes {
    if (!AdifNumber::check(newValue)) {
        m_pendingValue = std::move(newValue);
        return {false, std::nullopt};
    }

    auto optFreq = parseFrequency(newValue);
    if (!optFreq) {
        m_pendingValue = std::move(newValue);
        return {false, std::nullopt};
    }

    auto bandPeer = m_bandPeer.lock();

    if (!bandPeer) {
        m_rawValue = std::move(newValue);
        m_pendingValue.clear();
        return {true, std::nullopt};
    }

    if (bandPeer->m_rawValue.empty()) {
        auto derived = deriveBand(newValue);
        if (derived) {
            bandPeer->m_rawValue = *derived;
        }
        m_rawValue = std::move(newValue);
        m_pendingValue.clear();
        return {true, std::nullopt};
    }

    const bool compatCurrent = isFreqInBand(newValue, bandPeer->m_rawValue);

    if (compatCurrent) {
        m_rawValue = std::move(newValue);
        m_pendingValue.clear();
        return {true, std::nullopt};
    }

    const bool compatPending = isFreqInBand(newValue, bandPeer->m_pendingValue);

    if (compatPending) {
        m_rawValue = std::move(newValue);
        m_pendingValue.clear();
        bandPeer->m_rawValue = std::move(bandPeer->m_pendingValue);
        return {true, std::nullopt};
    }

    return {false, std::nullopt};
}

auto AdifFreq::asDouble() const -> double { return std::stod(m_rawValue); }

auto AdifFreq::compare(const AdifDataBase &right) const -> CompareRes {
    const auto *check_right = dynamic_cast<decltype(this)>(&right);

    if (check_right == nullptr) {
        throw std::invalid_argument("Cannot compare with different DataBase type");
    }

    return AdifNumber::compare_rational(m_rawValue, check_right->m_rawValue);
}

// ======================== AdifBand ========================

AdifBand::AdifBand(std::string value) : AdifDataBase(std::move(value)) {}

auto AdifBand::isValidBand(std::string_view band) -> bool {
    return BAND_MAP.find(band) != BAND_MAP.end();
}

auto AdifBand::isFreqInBand(std::string_view freqMHz, std::string_view band) -> bool {
    auto it = BAND_MAP.find(band);
    if (it == BAND_MAP.end()) {
        return false;
    }
    const auto &range = it->second;
    return AdifNumber::in_range(range.lower_mhz, freqMHz, range.upper_mhz);
}

auto AdifBand::take(std::string &&newValue) -> TakeRes {
    if (newValue.empty()) {
        m_rawValue.clear();
        m_pendingValue.clear();
        return {true, std::nullopt};
    }

    std::string normValue = std::move(newValue);
    normalizeDataToUpper(normValue);
    if (!isValidBand(normValue)) {
        m_pendingValue = std::move(normValue);
        return {false, std::nullopt};
    }

    auto freqPeer = m_freqPeer.lock();

    if (!freqPeer) {
        m_pendingValue = std::move(normValue);
        return {false, std::nullopt};
    }

    if (isFreqInBand(freqPeer->m_rawValue, normValue)) {
        m_rawValue = std::move(normValue);
        m_pendingValue.clear();
        return {true, std::nullopt};
    }

    if (isFreqInBand(freqPeer->m_pendingValue, normValue)) {
        m_rawValue = std::move(normValue);
        m_pendingValue.clear();
        freqPeer->m_rawValue = std::move(freqPeer->m_pendingValue);
        return {true, std::nullopt};
    }

    m_pendingValue = std::move(normValue);
    return {false, std::nullopt};
}

// ======================== AdifFreqBandFactory ========================

auto AdifFreqBandFactory::checkInternal(const std::string &freq, const std::string &band) -> bool {
    if (!AdifFreq::check(freq)) {
        return false;
    }

    if (!AdifBand::isValidBand(band)) {
        return false;
    }

    auto optFreq = safeStod(freq);
    if (!optFreq) {
        return false;
    }

    return AdifBand::isFreqInBand(freq, band);
}

auto AdifFreqBandFactory::check(const std::string &freq, const std::string &band) -> bool {
    std::string normBand = AdifBand::normalizeDataToUpper(band);
    return checkInternal(freq, normBand);
}

auto AdifFreqBandFactory::createFreqBandPair(std::string freq, std::string band)
    -> std::pair<std::shared_ptr<AdifFreq>, std::shared_ptr<AdifBand>> {
    std::string normBand = std::move(band);
    AdifBand::normalizeDataToUpper(normBand);

    bool valid_number = AdifNumber::check(freq);
    bool valid_band = AdifBand::isValidBand(normBand);

    std::string finalFreq = valid_number ? std::move(freq) : std::string{};
    std::string finalBand = valid_band ? std::move(normBand) : std::string{};

    auto freqPtr = std::shared_ptr<AdifFreq>(new AdifFreq(std::move(finalFreq)));
    auto bandPtr = std::shared_ptr<AdifBand>(new AdifBand(std::move(finalBand)));

    if (!AdifBand::isFreqInBand(freqPtr->m_rawValue, bandPtr->m_rawValue)) {
        bandPtr->switchPending();
    }

    freqPtr->m_bandPeer = bandPtr;
    bandPtr->m_freqPeer = freqPtr;

    return {freqPtr, bandPtr};
}

auto AdifFreqBandFactory::getBandMap() -> const BandMap & {
    static BandMap band_map{[]() {
        BandMap bm;
        for (const auto &[band, range] : ADIF::BAND_MAP) {
            bm.emplace(band, BandRange{range.lower_mhz, range.upper_mhz});
        }
        return bm;
    }()};
    return band_map;
}
