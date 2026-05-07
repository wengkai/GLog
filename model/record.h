#ifndef RECORD_H
#define RECORD_H

#include <algorithm>
#include <atomic>
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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "AdifDataBase.h"
#include "AdifGeneral.h"

#include "GLogKit/IGRecord.h"

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

class AdifDataWrap {

    using Mybase = std::shared_ptr<AdifDataBase>;

    Mybase m_ptr;

  public:
    explicit AdifDataWrap() : m_ptr(make_shared_from_optional(AdifGeneral::create(""))) {}
    explicit AdifDataWrap(Mybase other) : m_ptr(std::move(other)) {}
    explicit operator std::string() const { return (get() != nullptr) ? (*this)->get() : ""; }
    explicit operator std::string_view() const {
        assert(get() != nullptr);
        return get()->asView();
    }

    AdifDataWrap cloneAsPlainText() const {
        if (get() == nullptr) {
            return AdifDataWrap();
        }
        return AdifDataWrap(make_shared_from_optional(AdifGeneral::create((*this)->get())));
    }

    auto operator->() const -> AdifDataBase * { return m_ptr.operator->(); }
    auto get() const -> AdifDataBase * { return m_ptr.get(); }
    auto operator*() const -> AdifDataBase & { return m_ptr.operator*(); }
    operator bool() const { return static_cast<bool>(m_ptr); }

    template <typename AdifDataType = AdifDataBase> std::shared_ptr<AdifDataType> getPtr() const {
        if constexpr (std::is_same_v<AdifDataType, AdifDataBase>) {
            return m_ptr;
        }
        return std::dynamic_pointer_cast<AdifDataType>(m_ptr);
    }
};

/**
 * @class GRecord
 * @brief A high-level ADIF record container providing type-safe field access and persistence-aware
 * cloning.
 *
 * GRecord acts as a specialized associative container (based on std::map) that maps
 * normalized ADIF field names to polymorphic AdifDataWrap objects.
 *
 * @section Architecture
 * - **Identity Persistence:** Uses a shared atomic database ID to track the record's
 * origin across clones, facilitating "Dirty/Update" logic in the database layer.
 * - **Type Inflation:** Distinguishes between "Live" records (with rich behavioral logic,
 * e.g., Mode/Submode peer linking) and "Persistent" records (flattened for storage).
 * - **Transactional Integrity:** Methods like _addOrSetPair utilize RAII guards to
 * ensure that failed field validations do not leave the record in a corrupted state.
 *
 * @section ThreadSafety
 * - **Not Thread-Safe for Mutation:** This class is not designed for concurrent
 * multi-threaded access to a single instance.
 * - **Concurrency Strategy:** For multi-threaded processing, utilize the provided
 * clone() or copy constructor. The internal database ID is stored in a
 * std::shared_ptr<std::atomic<int64_t>>, ensuring that while data is unique per
 * thread, the database identity remains synchronized.
 *
 * @note Essential fields (like mode/submode) are enforced upon construction to
 * maintain ADIF protocol compliance.
 */
class GRecord : public IGRecord {

  public:
    using wrapper_type = AdifDataWrap;
    using MapT = std::map<std::string, wrapper_type, std::less<>>;
    using iterator = MapT::iterator;
    using const_iterator = MapT::const_iterator;

  private:
    MapT m_map;

    /**
     * @brief Atomically adds a new key-value pair or updates an existing one.
     * * This method ensures strong exception safety and map integrity using an RAII transaction
     * guard. It guarantees that the internal map is never left with a null or invalid
     * entry if field creation fails.
     *
     * @param key_normalized The pre-normalized lookup key.
     * @param value The raw string value to be parsed and stored.
     * @return true if the value was successfully added or updated; false if validation
     * failed or the value could not be parsed.
     * * @note Performance: Performs exactly one map lookup (O(log N)).
     * @note Consistency: Automatically rolls back (erases) map insertions if the
     * subsequent field construction fails.
     */
    bool _addOrSetPair(const std::string &key_normalized, std::string value);

    // core mapping
    static wrapper_type _createField(const std::string &key_normalized, std::string value);

    void _beforeSet(const_iterator iter);

    static bool _emplaceEssentialFields(MapT &map);

    std::shared_ptr<std::atomic<int64_t>> m_dbInternalId;

    bool valid = false;

    struct construct_nop {};

    explicit GRecord(construct_nop){};

  public:
    explicit GRecord();
    // shared with one ptr:m_dbInternalId
    // All the wrapper_type(ptr) are shared
    // if need a real copy for different db row, use clone
    GRecord(const GRecord &) = default;
    GRecord(GRecord &&rhs) noexcept
        : m_map(std::move(rhs.m_map)), m_dbInternalId(std::exchange(rhs.m_dbInternalId, nullptr)),
          valid(std::exchange(rhs.valid, false)){};

    GRecord &operator=(const GRecord &) = default;
    GRecord &operator=(GRecord &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        m_map = std::move(rhs.m_map);
        m_dbInternalId = std::exchange(rhs.m_dbInternalId, nullptr);
        valid = std::exchange(rhs.valid, false);
        return *this;
    };

    friend void swap(GRecord &lhs, GRecord &rhs) noexcept {
        using std::swap;
        swap(lhs.m_map, rhs.m_map);
        swap(lhs.m_dbInternalId, rhs.m_dbInternalId);
        swap(lhs.valid, rhs.valid);
    }

    // remake essential fields
    // set valid = true
    bool emplaceEssentialFields();

    int64_t getDbId() const { return m_dbInternalId ? m_dbInternalId->load() : INVALID_DB_ID; }
    void setDbId(int64_t id) { // only called by db manager
        if (!m_dbInternalId) {
            m_dbInternalId = std::make_shared<std::atomic<int64_t>>(id);
        } else {
            m_dbInternalId->store(id); // Update the shared value, don't replace the pointer
        }
    }

    GRecord remap() const;

    GRecord clone() const;

    /**
     * @brief Creates a lightweight, storage-optimized snapshot of the record.
     * * This method performs a specialized deep copy intended for persistence layers.
     * It differs from a standard clone in two key ways:
     * 1. Identity: The internal database ID is shared (via smart pointer) to maintain
     * the link to the existing persistent entity.
     * 2. Data Flattening: Member data is cloned as plain text/general types,
     * bypassing the expensive factory mapping used for live, "inflated" objects.
     * * @throw std::runtime_error If the record is marked invalid or lacks a
     * database identity.
     * @return GRecord A decoupled snapshot suitable for serialization or DB writing.
     */
    GRecord cloneForPersistence() const;

    /**
     * @brief Creates a skeletal identity-only snapshot for synchronization ordering.
     * * This specialized clone captures the object's database identity (m_dbInternalId)
     * without copying its data payload (m_map). It is designed for scenarios where
     * record sequence or membership in a collection (like std::vector<GRecord>)
     * needs to be managed without the memory overhead of full data duplication.
     * * @details
     * - Identity: Shares the internal database ID (shared pointer).
     * - Data: The resulting clone's data map will be empty.
     * - Purpose: Mapping memory-based collection order to database persistence order.
     * * @throw std::runtime_error If the source record is invalid or lacks a database ID.
     * @return GRecord A shell instance representing the same database entity.
     */
    GRecord cloneForSyncOrder() const;

    bool merge(const std::vector<GRecord> &others);

    static inline bool less(const GRecord &a, const GRecord &b,
                            const std::vector<std::string> &fields) {
        for (auto &field : fields) {
            auto pa = a.find(field);
            auto pb = b.find(field);
            if (pa == a.end() && pb == b.end()) {
                continue;
            }
            if (pb == b.end()) {
                return true;
            }
            if (pa == a.end()) {
                return false;
            }
            // throws std::invalid_argument if typeid(*pa->second) != typeid(*pb->second)
            auto result = pa->second->compare(*(pb->second));
            if (result == decltype(result)::equal_to) {
                continue;
            }
            return result == decltype(result)::less;
        }
        return false;
    };

    static inline bool equal_to(const GRecord &a, const GRecord &b,
                                const std::vector<std::string> &fields) {
        for (auto &field : fields) {
            auto pa = a.find(field);
            auto pb = b.find(field);

            if (pa == a.end() && pb == b.end()) {
                continue;
            }

            if (pa == a.end() || pb == b.end()) {
                return false;
            }

            auto result = pa->second->compare(*(pb->second));
            if (result != decltype(result)::equal_to) {
                return false;
            }
        }

        return true;
    }

    static auto normalizeKey(const std::string &key) {
        std::string lowerKey = key;
        normalizeKey(lowerKey);
        return lowerKey;
    }
    static void normalizeKey(std::string &key) {
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    template <typename STD_String>
    static auto createField(const std::string &key, STD_String &&value) {
        return _createField(normalizeKey(key), std::forward<STD_String>(value));
    }
    template <typename STD_String>
    static auto createFieldAndNormalizeKey(std::string &key, STD_String &&value) {
        normalizeKey(key);
        return _createField(key, std::forward<STD_String>(value));
    }
    /**
     * @brief Upserts a key-value pair with on-the-fly key normalization.
     * * This is the standard entry point for adding data. It normalizes the provided
     * key without modifying the original input string and forwards the value efficiently.
     * * @tparam STD_String Support for any string-like type (std::string, const char*, etc.)
     * @param key The lookup key to be normalized internally.
     * @param value The value to store, perfectly forwarded to minimize copies.
     * @return true if the pair was successfully integrated into the record.
     */
    template <typename STD_String> auto addOrSetPair(const std::string &key, STD_String &&value) {
        return _addOrSetPair(normalizeKey(key), std::forward<STD_String>(value));
    }
    /**
     * @brief Normalizes the input key in-place and upserts the pair.
     * * Performance-oriented variant used when the caller wants to retain and reuse
     * the normalized version of the key. Note that the 'key' argument is modified.
     * * @param key Reference to a string that will be transformed to its normalized form.
     * @param value The value to store, perfectly forwarded.
     * @return true if the pair was successfully integrated.
     */
    template <typename STD_String>
    auto addOrSetPairAndNormalizeKey(std::string &key, STD_String &&value) {
        normalizeKey(key);
        return _addOrSetPair(key, std::forward<STD_String>(value));
    }

    // return ptr: always editable via ->set
    auto findItem(const std::string &key) const {
        auto iter = find(normalizeKey(key));
        if (iter != end()) {
            return iter->second;
        }
        return wrapper_type(nullptr);
    }

    auto at(const std::string &key) { return findItem(key); }
    auto at(const std::string &key) const { return findItem(key); }
    // auto operator[](const std::string &key) { return findItem(key); }
    // auto operator[](const std::string &key) const { return findItem(key); }

    iterator begin() { return m_map.begin(); }
    iterator end() { return m_map.end(); }
    const_iterator begin() const { return m_map.begin(); }
    const_iterator end() const { return m_map.end(); }

    iterator find(const std::string &key) { return m_map.find(key); }
    const_iterator find(const std::string &key) const { return m_map.find(key); }

    // auto erase(const_iterator iter) { return m_map.erase(iter); }
    // auto erase(iterator iter) { return m_map.erase(iter); }

    bool tryRemoveField(const std::string &key);

    static constexpr const char *RESOLVE_HEADERS[] = {
        "qso_date", "time_on", "call", "freq", "band", "mode", "submode", "rst_rcvd", "rst_sent"};
    static constexpr std::size_t RESOLVE_HEADERS_COUNT = sizeof(RESOLVE_HEADERS) / sizeof(char *);

    static constexpr int64_t INVALID_DB_ID = -1;

    template <typename Ostream> void writeTo(Ostream &stream) const {
        for (const auto &[key, value] : m_map) {
            if (!value) {
                continue;
            }
            auto value_string = static_cast<std::string_view>(value);
            if (value_string.empty()) {
                continue;
            }
            stream << '<';
            stream << key;
            stream << ':';
            stream << value_string.size();
            stream << '>';
            stream << value_string;
        }
        stream << "<eor>";
    }

    static Result getValueByField(const IGRecord *rec, const char *field, uint64_t field_len,
                                  char *result_buf, uint64_t *result_len,
                                  uint64_t max_result_len) noexcept;
};

template <typename Ostream> inline Ostream &operator<<(Ostream &stream, const GRecord &record) {
    record.writeTo(stream);
    return stream;
};

#endif