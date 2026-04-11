#include "record.h"

using AdifFactoryFunc = std::function<std::shared_ptr<AdifDataBase>(const std::string &)>;

#ifndef NEW_FIELD_ENTRY
#define NEW_FIELD_ENTRY(field, classname)                                                          \
    {                                                                                              \
        #field,                                                                                    \
            [](const std::string &v) { return make_shared_from_optional(classname::create(v)); }   \
    }
#endif

#include "adif_field_map.h"

auto GRecord::_addOrSetPair(const std::string &key_normalized, const std::string &value) -> bool {
    // general set
    if (auto iter = m_map.find(key_normalized); iter != m_map.end()) {
        return iter->second->set(value);
    }

    // general add
    auto field = _createField(key_normalized, value);
    if (field) {
        m_map[key_normalized] = std::move(field);
        return true;
    }
    return false;
}

auto GRecord::_createField(const std::string &key_normalized, const std::string &value)
    -> AdifDataWrap {
    // TODO: special rules
    if (key_normalized == "mode" || key_normalized == "submode") {
        return AdifDataWrap(nullptr);
    }

    // general rules
    auto it = ADIF_FIELD_FACTORY.find(key_normalized);
    if (it != ADIF_FIELD_FACTORY.end()) {
        return AdifDataWrap(it->second(value)); // return nullptr when invaild
    }

    // fallback
    return AdifDataWrap(make_shared_from_optional(AdifGeneral::create(value)));
}

GRecord::GRecord() {
    // special classes
    auto [mode, submode] = AdifModeFactory::createModePair("", "");
    m_map.emplace("mode", mode);
    m_map.emplace("submode", submode);
}
