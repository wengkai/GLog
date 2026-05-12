#ifndef ADIFPARSE_SERVICE_H
#define ADIFPARSE_SERVICE_H

#include <istream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "record.h"

#include "app_export.h"

/** Stateless ADIF text parsing; no Qt model dependency. */
class GLOGKIT_API AdifParseService {
  public:
    using Record = GRecord;

    struct ParseRes {
        bool parse_res = false;
        std::vector<Record> parse_records;
        std::unordered_set<std::string> parse_headers;
    };

    enum class ParseMode { Batch, Interactive };

    static ParseRes parse(std::istream &in, std::vector<std::string> &errors, ParseMode mode);

    static ParseRes
    fromRawData(bool success,
                std::vector<std::vector<std::pair<std::string, std::string>>> raw_data);
};

#endif
