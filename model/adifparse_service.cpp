#include "adifparse_service.h"

#include <cassert>
#include <memory>

#include "glogparser.h"

auto AdifParseService::fromRawData(
    bool success, std::vector<std::vector<std::pair<std::string, std::string>>> raw_data)
    -> ParseRes {
    ParseRes ret{success, {}, {}};
    if (!success) {
        return ret;
    }
    auto &m_records = ret.parse_records;
    auto &m_headers = ret.parse_headers;
    m_records.reserve(raw_data.size());
    for (auto &row : raw_data) {
        if (row.empty()) {
            continue;
        }
        Record m_record;
        bool valid = false;
        for (auto &[key, value] : row) {
            bool inserted = m_record.addOrSetPairAndNormalizeKey(key, std::move(value));
            if (inserted) {
                valid = true;
                m_headers.insert(std::move(key));
                continue;
            }
        }
        if (!valid) {
            continue;
        }
        m_records.push_back(std::move(m_record));
    }
    return ret;
}

auto AdifParseService::parse(std::istream &in, std::vector<std::string> &errors, ParseMode mode)
    -> ParseRes {

    std::unique_ptr<GLOG_PARSER::DriverUnsynchronized> driver;

    switch (mode) {
    case ParseMode::Batch:
        driver = std::make_unique<GLOG_PARSER::DriverUnsynchronized>(
            std::make_unique<GLOG_PARSER::LexerBatch>());
        break;

    case ParseMode::Interactive:
        driver = std::make_unique<GLOG_PARSER::DriverUnsynchronized>(
            std::make_unique<GLOG_PARSER::LexerInteractive>());
        break;

    default:
        assert(false && "Invalid mode");
        break;
    }

    GLOG_PARSER::Parser parser{driver.get()};
    driver->switch_streams(in, std::cerr);
    auto success = parser.parse() == 0;

    ParseRes ret{success, {}, {}};
    driver->errors.write([&](auto &d_errors) { std::swap(errors, d_errors); });
    if (!success) {
        return ret;
    }

    ret = driver->data.write([&](auto &data) { return fromRawData(success, std::move(data)); });

    return ret;
}
