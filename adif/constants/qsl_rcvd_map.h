#include <map>
#include <string>

#ifndef QSL_STATUS_INFO_DEFINED
#define QSL_STATUS_INFO_DEFINED
struct QslStatusInfo {
    bool import_only;
    std::string meaning;
    std::string description;
};
#endif

using QslRcvdMap = std::map<std::string, QslStatusInfo>;

static const QslRcvdMap QSL_RCVD_MAP = {
    {"Y",
     {false, "yes (confirmed)",
      "an incoming QSL card has been received ; the QSO has been confirmed by the online service"}},
    {"N",
     {false, "no",
      "an incoming QSL card has not been received ; the QSO has not been confirmed by the online "
      "service"}},
    {"R",
     {false, "requested",
      "thelogging stationhas requested a QSL card ; thelogging stationhas requested the QSO be "
      "uploaded to the online service"}},
    {"I", {false, "ignore or invalid", ""}},
    {"V  ( import-only )",
     {true, "verified",
      "DXCC award credit granted for the QSL card - instead "
      "use<CREDIT_GRANTED:39>DXCC:card,DXCC_BAND:card,DXCC_MODE:card ; DXCC credit granted for the "
      "LoTW confirmation - instead use<CREDIT_GRANTED:39>DXCC:lotw,DXCC_BAND:lotw,DXCC_MODE:lotw"}},
};