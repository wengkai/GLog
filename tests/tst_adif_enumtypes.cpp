#include <QtTest>

#include <cctype>
#include <optional>
#include <string>

#include "AdifEnumTypes.h"
#include "constants/adif_constants.h"

namespace {

std::string toUpperAscii(std::string s) {
    for (char &c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string makeMixedCase(std::string s) {
    // Flip the first alphabetic character to lower-case to ensure validation + normalize path is
    // exercised.
    for (char &c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc)) {
            c = static_cast<char>(std::tolower(uc));
            break;
        }
    }
    return s;
}

const std::string invalidKey = "INVALID_ENUM_VALUE";

template <typename EnumType, typename MapType>
void testEnumType(const char *enumTypeName, const MapType &enumMap) {
    auto it = enumMap.begin();
    QVERIFY2(it != enumMap.end(), "Enum map must not be empty");

    const std::string validKey = it->first;
    const std::string mixedValidKey = makeMixedCase(validKey);
    const std::string expectedUpper = toUpperAscii(mixedValidKey);

    // AdifEnumValidator specializations (AdifEnumTypes.cpp) - check success/failure.
    QVERIFY2(AdifEnumValidator<EnumType>::check(validKey), enumTypeName);
    QVERIFY2(!AdifEnumValidator<EnumType>::check(invalidKey), enumTypeName);

    // check/create: valid input should create and normalize to upper-case.
    {
        auto objOpt = EnumType::create(mixedValidKey);
        QVERIFY2(objOpt.has_value(), enumTypeName);
        EnumType obj = *objOpt;
        QCOMPARE(QString::fromStdString(obj.get()), QString::fromStdString(expectedUpper));

        // set (success): should return true and keep normalized value.
        QVERIFY2(obj.set(mixedValidKey), enumTypeName);
        QCOMPARE(QString::fromStdString(obj.get()), QString::fromStdString(expectedUpper));

        // set (failed): invalid input should return false and keep previous value.
        QVERIFY2(!obj.set(invalidKey), enumTypeName);
        QCOMPARE(QString::fromStdString(obj.get()), QString::fromStdString(expectedUpper));
    }

    // create (failed): invalid input should return nullopt.
    {
        auto objOpt = EnumType::create(invalidKey);
        QVERIFY2(!objOpt.has_value(), enumTypeName);
    }
}

} // namespace

class TestAdifEnumTypes : public QObject {
    Q_OBJECT

  private slots:
    void test_check_create_set_multiple_enum_types();
};

void TestAdifEnumTypes::test_check_create_set_multiple_enum_types() {
    testEnumType<AdifAntPath>("AdifAntPath", ADIF::ANT_PATH_MAP);
    testEnumType<AdifArrlSection>("AdifArrlSection", ADIF::ARRL_SECTION_MAP);
    testEnumType<AdifAward>("AdifAward", ADIF::AWARD_MAP);
    testEnumType<AdifAwardSponsor>("AdifAwardSponsor", ADIF::AWARD_SPONSOR_MAP);
    testEnumType<AdifBandRx>("AdifBandRx", ADIF::BAND_MAP);
    testEnumType<AdifContestId>("AdifContestId", ADIF::CONTEST_ID_MAP);
    testEnumType<AdifContinent>("AdifContinent", ADIF::CONTINENT_MAP);
    testEnumType<AdifCredit>("AdifCredit", ADIF::CREDIT_MAP);
    testEnumType<AdifDxcc>("AdifDxcc", ADIF::DXCC_MAP);
    testEnumType<AdifEqslAg>("AdifEqslAg", ADIF::EQSL_AG_MAP);
    testEnumType<AdifMorseKey>("AdifMorseKey", ADIF::MORSE_KEY_MAP);
    testEnumType<AdifPropagation>("AdifPropagation", ADIF::PROPAGATION_MAP);
    testEnumType<AdifQslMedium>("AdifQslMedium", ADIF::QSL_MEDIUM_MAP);
    testEnumType<AdifQslRcvd>("AdifQslRcvd", ADIF::QSL_RCVD_MAP);
    testEnumType<AdifQslSent>("AdifQslSent", ADIF::QSL_SENT_MAP);
    testEnumType<AdifQslVia>("AdifQslVia", ADIF::QSL_VIA_MAP);
    testEnumType<AdifQsoComplete>("AdifQsoComplete", ADIF::QSO_COMPLETE_MAP);
    testEnumType<AdifQsoDownloadStatus>("AdifQsoDownloadStatus", ADIF::QSO_DOWNLOAD_STATUS_MAP);
    testEnumType<AdifQsoUploadStatus>("AdifQsoUploadStatus", ADIF::QSO_UPLOAD_STATUS_MAP);
    testEnumType<AdifRegion>("AdifRegion", ADIF::REGION_MAP);
}

QTEST_MAIN(TestAdifEnumTypes)
#include "tst_adif_enumtypes.moc"
