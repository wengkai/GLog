#ifndef RECORD_H
#define RECORD_H

#include <algorithm>
#include <charconv>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <cstdlib>
#include <cstring>

#include "AdifDataTypes.h"

template <typename AdifDataType>
std::shared_ptr<AdifDataBase> make_shared_from_optional(const std::optional<AdifDataType> &opt) {
    if (opt) {
        return std::make_shared<AdifDataType>(*opt);
    }
    return nullptr;
}

template <typename AdifDataType>
std::shared_ptr<AdifDataBase> make_shared_from_optional(std::optional<AdifDataType> &&opt) {
    if (opt) {
        return std::make_shared<AdifDataType>(std::move(*opt));
    }
    return nullptr;
}

class AdifDataWrap : public std::shared_ptr<AdifDataBase> {

    using Mybase = std::shared_ptr<AdifDataBase>;

  public:
    explicit AdifDataWrap() : Mybase(make_shared_from_optional(AdifGeneral::create(""))) {}
    explicit AdifDataWrap(const Mybase &other) : Mybase(other) {}
    explicit AdifDataWrap(Mybase &&other) : Mybase(std::move(other)) {}
    explicit operator std::string() { return (get() != nullptr) ? (*this)->get() : ""; }
};

class GRecord {

    std::map<std::string, AdifDataWrap> m_map;
    using iterator = std::map<std::string, AdifDataWrap>::iterator;
    using const_iterator = std::map<std::string, AdifDataWrap>::const_iterator;

    std::string _sub_mode_setter;

    bool _addOrSetPair(const std::string &key_normalized, const std::string &value);

    // core mapping
    static AdifDataWrap _createField(const std::string &key_normalized, const std::string &value);

  public:
    static constexpr const char *RESOLVE_HEADERS[] = {
        "qso_date", "time_on", "call", "freq", "mode", "submode", "rst_rcvd", "rst_sent"};
    static constexpr std::size_t RESOLVE_HEADERS_COUNT = sizeof(RESOLVE_HEADERS) / sizeof(char *);

    explicit GRecord();

    template <typename Compare>
    static inline bool compare(const std::string &l, const std::string &r, const std::string &field,
                               Compare comp) {
        if (field == "freq") {
            return comp(std::stod(l), std::stod(r));
        }
        if (field == "rst_rcvd" || field == "rst_sent") {
            return comp(std::stoi(l), std::stoi(r));
        }
        return comp(l, r);
    };

    static inline bool less(const GRecord &a, const GRecord &b,
                            const std::vector<std::string> &fields) {
        for (auto &field : fields) {
            auto pa = a.find(field);
            auto pb = b.find(field);
            if (pa == a.end() && pb == b.end()) {
                return false;
            }
            if (pb == b.end()) {
                return true;
            }
            if (pa == a.end()) {
                return false;
            }
            if (compare(pa->second->get(), pb->second->get(), field, std::equal_to<>{})) {
                continue;
            }
            return compare(pa->second->get(), pb->second->get(), field, std::less<>{});
        }
        return false;
    };

    static auto normalizeKey(const std::string &key) {
        std::string lowerKey = key;
        normalizeKey(lowerKey);
        return lowerKey;
    }
    static void normalizeKey(std::string &key) {
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    }

    static auto createField(const std::string &key, const std::string &value) {
        return _createField(normalizeKey(key), value);
    }
    static auto createFieldAndNormalizeKey(std::string &key, const std::string &value) {
        normalizeKey(key);
        return _createField(key, value);
    }

    auto addOrSetPair(const std::string &key, const std::string &value) {
        return _addOrSetPair(normalizeKey(key), value);
    }
    auto addOrSetPairAndNormalizeKey(std::string &key, const std::string &value) {
        normalizeKey(key);
        return _addOrSetPair(key, value);
    }

    auto findItem(const std::string &key) const {
        auto iter = find(normalizeKey(key));
        if (iter != end()) {
            return iter->second;
        }
        return AdifDataWrap{};
    }

    auto at(const std::string &key) { return findItem(key); }
    auto at(const std::string &key) const { return findItem(key); }
    auto operator[](const std::string &key) { return findItem(key); }
    auto operator[](const std::string &key) const { return findItem(key); }

    iterator begin() { return m_map.begin(); }
    iterator end() { return m_map.end(); }
    const_iterator begin() const { return m_map.begin(); }
    const_iterator end() const { return m_map.end(); }

    iterator find(const std::string &key) { return m_map.find(key); }
    const_iterator find(const std::string &key) const { return m_map.find(key); }

    auto erase(const_iterator iter) { return m_map.erase(iter); }
    auto erase(iterator iter) { return m_map.erase(iter); }
};

using GRecordFilter = std::function<void(GRecord &)>;

static const std::vector<GRecordFilter> GRecordOutputFilters = {};

template <typename Ostream> inline Ostream &operator<<(Ostream &stream, const GRecord &record) {
    auto r = record;
    // for (auto & func : GRecordOutputFilters) {
    //     func(r);
    // }
    for (auto &pair : r) {
        if (pair.second->get().empty()) {
            continue;
        }
        stream << '<';
        stream << pair.first;
        stream << ':';
        stream << pair.second->get().size();
        stream << '>';
        stream << pair.second->get();
    }
    stream << "<eor>";
    return stream;
};

#endif