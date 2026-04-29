#include "record.h"

#include "AdifDataTypes.h"

#include <unordered_set>

using AdifFactoryFunc = std::function<std::shared_ptr<AdifDataBase>(std::string &&)>;

#ifndef NEW_FIELD_ENTRY
#define NEW_FIELD_ENTRY(field, classname)                                                          \
    {                                                                                              \
        #field, [](std::string &&v) {                                                              \
            return make_shared_from_optional(classname::create(std::move(v)));                     \
        }                                                                                          \
    }
#endif

#include "adif_field_map.h"

struct SpecialRules {
    using wrapper_type = GRecord::wrapper_type;
    using MyMapT = std::map<std::string, wrapper_type, std::less<>>;
    using iterator = MyMapT::iterator;
    using const_iterator = MyMapT::const_iterator;
    using FunctionT = std::function<void(MyMapT &, const_iterator)>;

    template <typename TSelf, typename TPeer>
    static void linkPeers(MyMapT &map, const_iterator iter, const std::string &peerName) {
        auto me = iter->second.getPtr<TSelf>();
        if (!me || me->hasPeer()) return;

        auto peer_iter = map.find(peerName);
        if (peer_iter != map.end()) {
            auto peer = peer_iter->second.getPtr<TPeer>();
            if (peer) {
                me->setPeer(peer);
                peer->setPeer(me);
            }
        }
    }

    struct Functions {
        FunctionT beforeSet;
    };

    const std::unordered_map<std::string, Functions> function_map{
        // TODO: more special_fields

        {"mode",
         {
             [](MyMapT &map, const_iterator iter) { // beforeSet
                 linkPeers<AdifMode, AdifSubMode>(map, iter, "submode");
             },
         }},
        {"submode",
         {
             [](MyMapT &map, const_iterator iter) { // beforeSet
                 linkPeers<AdifSubMode, AdifMode>(map, iter, "mode");
             },
         }},
        {"freq",
         {
             [](MyMapT &map, const_iterator iter) { // beforeSet
                 linkPeers<AdifFreq, AdifBand>(map, iter, "band");
             },
         }},
        {"band",
         {
             [](MyMapT &map, const_iterator iter) { // beforeSet
                 linkPeers<AdifBand, AdifFreq>(map, iter, "freq");
             },
         }},
    };
} special_rules;

static const auto &special_fields = special_rules.function_map;

auto GRecord::_addOrSetPair(const std::string &key_normalized, std::string value) -> bool {
    if (!valid && !emplaceEssentialFields()) {
        return false;
    }

    // strengthened:
    // only one find operation
    // avoid null entry to be added
    // !important: **remove existed null entry**
    // return false: if value can not be parsed as field value

    struct MyTrans {
        std::pair<iterator, bool> operation;
        bool commit; // set to true <==> add or set to an entry not null
        decltype(m_map) &map;
        ~MyTrans() noexcept {
            if (!commit) {
                map.erase(operation.first);
            }
        }
    } emplace_trans{m_map.try_emplace(key_normalized, nullptr), false, m_map};

    const auto &it = emplace_trans.operation.first;
    const auto &inserted = emplace_trans.operation.second;

    if (!inserted && it->second) {
        emplace_trans.commit = true; // avoid this entry from being removed
        _beforeSet(it);
        // Element already existed, just update value
        return it->second->set(std::move(value));
    }

    // New element or it->second is nullptr: create it
    auto field = _createField(key_normalized, std::move(value));
    if (field) {
        it->second = std::move(field);
        emplace_trans.commit = true;
        return true;
    }

    // it->second is really nullptr now, we DO NOT need it
    return false;
}

void GRecord::_beforeSet(const_iterator iter) {
    auto func_iter = special_fields.find(iter->first);
    if (func_iter != special_fields.end()) {
        func_iter->second.beforeSet(m_map, iter);
    }
}

auto GRecord::_createField(const std::string &key_normalized, std::string value) -> wrapper_type {

    // not allowed
    if (special_fields.count(key_normalized) != 0) {
        return wrapper_type(nullptr);
    }

    // general rules
    auto it = ADIF::ADIF_FIELD_FACTORY.find(key_normalized);
    if (it != ADIF::ADIF_FIELD_FACTORY.end()) {
        return wrapper_type(it->second(std::move(value))); // return nullptr when invalid
    }

    // fallback
    return wrapper_type(make_shared_from_optional(AdifGeneral::create(std::move(value))));
}

auto GRecord::_emplaceEssentialFields(MapT &map) -> bool {
    // special classes
    auto [mode, submode] = AdifModeFactory::createModePair("", "");
    if (!mode) {
        return false;
    }
    if (!submode) {
        return false;
    }

    map["mode"] = wrapper_type(mode);
    map["submode"] = wrapper_type(submode);

    auto [freq, band] = AdifFreqBandFactory::createFreqBandPair("", "");
    if (!freq) {
        return false;
    }
    if (!band) {
        return false;
    }
    map["freq"] = wrapper_type(freq);
    map["band"] = wrapper_type(band);

    return true;
}

GRecord::GRecord() : m_dbInternalId(std::make_shared<std::atomic<int64_t>>(INVALID_DB_ID)) {
    valid = _emplaceEssentialFields(m_map);
}

GRecord GRecord::remap() const {
    GRecord snapshot;
    for (const auto &[key, value] : m_map) {
        if (value) {
            snapshot._addOrSetPair(key,
                                   std::string{value->asView()}); // use FIELD_FACTORY mapping again
        }
    }
    return snapshot;
}

GRecord GRecord::clone() const {
#ifdef ADIF_DATA_CLONE
    GRecord snapshot;
    for (const auto &[key, value] : m_map) {
        if (value) {
            snapshot.m_map[key] = wrapper_type(value->clone());
        }
    }
    return snapshot;
#else
    return remap();
#endif
}

GRecord GRecord::cloneForPersistence() const {
    GRecord snapshot(construct_nop{});
    if (!valid || !m_dbInternalId) {
        throw std::runtime_error(
            "GRecord: persistent clone failed - object is in an invalid state.");
    }
    snapshot.m_dbInternalId = this->m_dbInternalId; // share ptr:dbInternalId
    snapshot.valid = valid;

    for (const auto &[key, wrap] : m_map) {
        snapshot.m_map.emplace(
            key,
            wrap.cloneAsPlainText()); // as General type, cheap clone without factory mapping
    }
    return snapshot;
}

GRecord GRecord::cloneForSyncOrder() const {
    GRecord snapshot(construct_nop{});
    if (!valid || !m_dbInternalId) {
        throw std::runtime_error(
            "GRecord: persistent clone failed - object is in an invalid state.");
    }
    // for mapping: memory order <==> db order : I don't have my memory
    // order. It is managed by containers like std::vector<GRecord>
    snapshot.m_dbInternalId = this->m_dbInternalId;
    return snapshot;
}

auto GRecord::tryRemoveField(const std::string &key) -> bool {
    auto key_normalized = normalizeKey(key);
    if (special_fields.count(key_normalized) != 0) {
        return false;
    }
    if (auto iter = m_map.find(key_normalized); iter != m_map.end()) {
        m_map.erase(iter);
        return true;
    }
    return false;
}

auto GRecord::emplaceEssentialFields() -> bool {
    struct Transaction {
        decltype(m_map) backup;
        decltype(m_map) &original;
        bool commit = false;

        Transaction(decltype(m_map) &ref) : backup(ref), original(ref) {}

        ~Transaction() {
            if (!commit) {
                original = std::move(backup);
            }
        }
    } trans{m_map};

    if (!_emplaceEssentialFields(m_map)) {
        return false;
    }

    return trans.commit = valid = true;
}

auto /* static callback for plugins */
GRecord::getValueByField(const IGRecord *rec, const char *field, uint64_t field_len,
                         char *result_buf, uint64_t *result_len, uint64_t max_result_len) noexcept
    -> Result {
    if (!result_len) {
        return Result::InvalidResultLenOutput;
    }
    *result_len = 0;
    if (!rec || !field || field_len == 0) {
        return Result::InvalidInput;
    }
    const auto *This = static_cast<const GRecord *>(rec);
    auto iter = This->m_map.find(std::string_view(field, field_len));
    if (iter == This->m_map.end()) {
        return Result::NotFound;
    }
    if (!iter->second) {
        return Result::InternalError;
    }
    auto ret_sv =
        iter->second->asView(); // std::string_view asView() const noexcept { return (m_rawValue); }
    auto ret_len = ret_sv.size();
    if (ret_len == 0) {
        return Result::NoError;
    }
    *result_len = ret_len;
    if (!result_buf || max_result_len < ret_len) {
        return Result::Overflow;
    }
    std::copy_n(ret_sv.data(), ret_len, result_buf);
    return Result::NoError;
}
