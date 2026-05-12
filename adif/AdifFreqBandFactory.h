#ifndef ADIF_FREQ_BAND_FACTORY_H
#define ADIF_FREQ_BAND_FACTORY_H

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "AdifNumber.h"

struct BandRange {
    std::string lower_mhz;
    std::string upper_mhz;
};

using BandMap = std::map<std::string, BandRange>;

class AdifFreq;
class AdifBand;

class AdifFreq : public AdifDataBase {
    friend class AdifBand;
    friend class AdifFreqBandFactory;

  private:
    std::weak_ptr<AdifBand> m_bandPeer;

    explicit AdifFreq(std::string value);

    static std::optional<double> parseFrequency(std::string_view sv);

    static std::optional<std::string> deriveBand(std::string_view freqMHz);

    static bool isFreqInBand(std::string_view freqMHz, std::string_view band);

  protected:
    ADIF_DATA_TYPE_CLONE_DEC(AdifFreq)

  public:
    static bool check(std::string_view data) { return AdifNumber::check(data); }

    TakeRes take(std::string &&newValue) override;

    bool hasPeer() const { return !m_bandPeer.expired(); }
    void setPeer(std::shared_ptr<AdifBand> peer) { m_bandPeer = peer; }

    double asDouble() const;

    const std::string &getPendingValue() const { return m_pendingValue; }

    CompareRes compare(const AdifDataBase &right) const override;

    AdifFreq(const AdifFreq &) = delete;
    AdifFreq &operator=(const AdifFreq &) = delete;
};

class AdifBand : public AdifDataBase {
    friend class AdifFreq;
    friend class AdifFreqBandFactory;

  private:
    std::weak_ptr<AdifFreq> m_freqPeer;

    explicit AdifBand(std::string value);

    static bool isValidBand(std::string_view band);

    static bool isFreqInBand(std::string_view freqMHz, std::string_view band);

  protected:
    ADIF_DATA_TYPE_CLONE_DEC(AdifBand)

  public:
    static bool check(std::string_view data) {
        return isValidBand(normalizeDataToUpper(std::string(data)));
    }

    TakeRes take(std::string &&newValue) override;

    bool hasPeer() const { return !m_freqPeer.expired(); }
    void setPeer(std::shared_ptr<AdifFreq> peer) { m_freqPeer = peer; }

    const std::string &getPendingValue() const { return m_pendingValue; }

    AdifBand(const AdifBand &) = delete;
    AdifBand &operator=(const AdifBand &) = delete;
};

class AdifFreqBandFactory {
  public:
    static bool check(const std::string &freq, const std::string &band);

    static std::pair<std::shared_ptr<AdifFreq>, std::shared_ptr<AdifBand>>
    createFreqBandPair(std::string freq, std::string band);

    static const BandMap &getBandMap();

  private:
    static bool checkInternal(const std::string &freq, const std::string &band);
};

#endif
