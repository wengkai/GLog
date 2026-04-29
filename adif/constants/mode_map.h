#ifndef MODE_MAP_H_GENERATED_H
#define MODE_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
#ifndef MODE_MAP_T
#define MODE_MAP_T
struct ModeInfo {
    bool import_only;               
    std::vector<std::string> submodes; 
};

using ModeMap = std::map<std::string, ModeInfo, CaseInsensitiveLess>;

#endif
inline const ModeMap MODE_MAP = {
    {"AM", {false, {}}},
    {"ARDOP", {false, {}}},
    {"ATV", {false, {}}},
    {"CHIP", {false, {"CHIP64", "CHIP128"}}},
    {"CLO", {false, {}}},
    {"CONTESTI", {false, {}}},
    {"CW", {false, {"PCW"}}},
    {"DIGITALVOICE", {false, {"C4FM", "DMR", "DSTAR", "FREEDV", "M17"}}},
    {"DOMINO", {false, {"DOM-M", "DOM4", "DOM5", "DOM8", "DOM11", "DOM16", "DOM22", "DOM44", "DOM88", "DOMINOEX", "DOMINOF"}}},
    {"DYNAMIC", {false, {"VARA HF", "VARA SATELLITE", "VARA FM 1200", "VARA FM 9600"}}},
    {"FAX", {false, {}}},
    {"FM", {false, {}}},
    {"FSK441", {false, {}}},
    {"FSK", {false, {"SCAMP_FAST", "SCAMP_SLOW", "SCAMP_VSLOW"}}},
    {"FT8", {false, {}}},
    {"HELL", {false, {"FMHELL", "FSKH105", "FSKH245", "FSKHELL", "HELL80", "HELLX5", "HELLX9", "HFSK", "PSKHELL", "SLOWHELL"}}},
    {"ISCAT", {false, {"ISCAT-A", "ISCAT-B"}}},
    {"JT4", {false, {"JT4A", "JT4B", "JT4C", "JT4D", "JT4E", "JT4F", "JT4G"}}},
    {"JT6M", {false, {}}},
    {"JT9", {false, {"JT9-1", "JT9-2", "JT9-5", "JT9-10", "JT9-30", "JT9A", "JT9B", "JT9C", "JT9D", "JT9E", "JT9E FAST", "JT9F", "JT9F FAST", "JT9G", "JT9G FAST", "JT9H", "JT9H FAST"}}},
    {"JT44", {false, {}}},
    {"JT65", {false, {"JT65A", "JT65B", "JT65B2", "JT65C", "JT65C2"}}},
    {"MFSK", {false, {"FSQCALL", "FST4", "FST4W", "FT4", "JS8", "JTMS", "MFSK4", "MFSK8", "MFSK11", "MFSK16", "MFSK22", "MFSK31", "MFSK32", "MFSK64", "MFSK64L", "MFSK128", "MFSK128L", "Q65"}}},
    {"MSK144", {false, {}}},
    {"MTONE", {false, {"SCAMP_OO", "SCAMP_OO_SLW"}}},
    {"MT63", {false, {}}},
    {"OLIVIA", {false, {"OLIVIA 4/125", "OLIVIA 4/250", "OLIVIA 8/250", "OLIVIA 8/500", "OLIVIA 16/500", "OLIVIA 16/1000", "OLIVIA 32/1000"}}},
    {"OPERA", {false, {"OPERA-BEACON", "OPERA-QSO"}}},
    {"PAC", {false, {"PAC2", "PAC3", "PAC4"}}},
    {"PAX", {false, {"PAX2"}}},
    {"PKT", {false, {}}},
    {"PSK", {false, {"8PSK125", "8PSK125F", "8PSK125FL", "8PSK250", "8PSK250F", "8PSK250FL", "8PSK500", "8PSK500F", "8PSK1000", "8PSK1000F", "8PSK1200F", "FSK31", "PSK10", "PSK31", "PSK63", "PSK63F", "PSK63RC4", "PSK63RC5", "PSK63RC10", "PSK63RC20", "PSK63RC32", "PSK125", "PSK125C12", "PSK125R", "PSK125RC10", "PSK125RC12", "PSK125RC16", "PSK125RC4", "PSK125RC5", "PSK250", "PSK250C6", "PSK250R", "PSK250RC2", "PSK250RC3", "PSK250RC5", "PSK250RC6", "PSK250RC7", "PSK500", "PSK500C2", "PSK500C4", "PSK500R", "PSK500RC2", "PSK500RC3", "PSK500RC4", "PSK800C2", "PSK800RC2", "PSK1000", "PSK1000C2", "PSK1000R", "PSK1000RC2", "PSKAM10", "PSKAM31", "PSKAM50", "PSKFEC31", "QPSK31", "QPSK63", "QPSK125", "QPSK250", "QPSK500", "SIM31"}}},
    {"PSK2K", {false, {}}},
    {"Q15", {false, {}}},
    {"QRA64", {false, {"QRA64A", "QRA64B", "QRA64C", "QRA64D", "QRA64E"}}},
    {"ROS", {false, {"ROS-EME", "ROS-HF", "ROS-MF"}}},
    {"RTTY", {false, {"ASCI"}}},
    {"RTTYM", {false, {}}},
    {"SSB", {false, {"LSB", "USB"}}},
    {"SSTV", {false, {}}},
    {"T10", {false, {}}},
    {"THOR", {false, {"THOR-M", "THOR4", "THOR5", "THOR8", "THOR11", "THOR16", "THOR22", "THOR25X4", "THOR50X1", "THOR50X2", "THOR100"}}},
    {"THRB", {false, {"THRBX", "THRBX1", "THRBX2", "THRBX4", "THROB1", "THROB2", "THROB4"}}},
    {"TOR", {false, {"AMTORFEC", "GTOR", "NAVTEX", "SITORB"}}},
    {"V4", {false, {}}},
    {"VOI", {false, {}}},
    {"WINMOR", {false, {}}},
    {"WSPR", {false, {}}},
    {"AMTORFEC", {true, {}}},
    {"ASCI", {true, {}}},
    {"C4FM", {true, {}}},
    {"CHIP64", {true, {}}},
    {"CHIP128", {true, {}}},
    {"DOMINOF", {true, {}}},
    {"DSTAR", {true, {}}},
    {"FMHELL", {true, {}}},
    {"FSK31", {true, {}}},
    {"GTOR", {true, {}}},
    {"HELL80", {true, {}}},
    {"HFSK", {true, {}}},
    {"JT4A", {true, {}}},
    {"JT4B", {true, {}}},
    {"JT4C", {true, {}}},
    {"JT4D", {true, {}}},
    {"JT4E", {true, {}}},
    {"JT4F", {true, {}}},
    {"JT4G", {true, {}}},
    {"JT65A", {true, {}}},
    {"JT65B", {true, {}}},
    {"JT65C", {true, {}}},
    {"MFSK8", {true, {}}},
    {"MFSK16", {true, {}}},
    {"PAC2", {true, {}}},
    {"PAC3", {true, {}}},
    {"PAX2", {true, {}}},
    {"PCW", {true, {}}},
    {"PSK10", {true, {}}},
    {"PSK31", {true, {}}},
    {"PSK63", {true, {}}},
    {"PSK63F", {true, {}}},
    {"PSK125", {true, {}}},
    {"PSKAM10", {true, {}}},
    {"PSKAM31", {true, {}}},
    {"PSKAM50", {true, {}}},
    {"PSKFEC31", {true, {}}},
    {"PSKHELL", {true, {}}},
    {"QPSK31", {true, {}}},
    {"QPSK63", {true, {}}},
    {"QPSK125", {true, {}}},
    {"THRBX", {true, {}}},
};
} // namespace ADIF

// clang-format on

#endif // MODE_MAP_H_GENERATED_H
