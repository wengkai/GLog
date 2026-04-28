#ifndef ADIF_DATA_BASE
#define ADIF_DATA_BASE
#include <any>
#include <charconv>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#define ADIF_DATA_CLONE
#ifdef ADIF_DATA_CLONE
#define ADIF_DATA_BASE_CLONE_DEC virtual AdifDataBase *_clone() const = 0;
#define ADIF_DATA_TYPE_CLONE_DEC(classname)                                                        \
    virtual AdifDataBase *_clone() const override;                                                 \
    explicit classname(std::string value, std::string pendingValue, std::any externed)             \
        : AdifDataBase(std::move(value), std::move(pendingValue), std::move(externed)) {}
#define ADIF_DATA_TYPE_CLONE_IMP(classname)                                                        \
    auto classname ::_clone() const->AdifDataBase * {                                              \
        return new classname(m_rawValue, m_pendingValue, externed);                                \
    }
#else
#define ADIF_DATA_BASE_CLONE_DEC
#define ADIF_DATA_TYPE_CLONE_DEC(classname)
#define ADIF_DATA_TYPE_CLONE_IMP(classname)
#endif

/**
 * @brief ADIF Meta Base
 */
class AdifDataBase {
  protected:
    std::string m_rawValue;
    std::string m_pendingValue;
    std::any externed;

    explicit AdifDataBase(std::string value);

    explicit AdifDataBase(std::string value, std::string m_pendingValue, std::any externed);

    void normalizeDataToUpper();

    static void normalizeDataToUpper(std::string &data);

    static std::string normalizeDataToUpper(const std::string &data);

    ADIF_DATA_BASE_CLONE_DEC

  public:
    virtual ~AdifDataBase();

    std::string get() const;

    std::string_view asView() const noexcept { return std::string_view(m_rawValue); }

    void switchPending() { std::swap(m_rawValue, m_pendingValue); }

    struct TakeRes {
        bool flag = false;
        std::optional<std::string> failed_store = std::nullopt;
    };

    virtual TakeRes take(std::string &&newValue) = 0;

    bool set(std::string newValue) {
        auto [flag, str_opt] = take(std::move(newValue));
        if (!flag && str_opt.has_value()) {
            m_pendingValue = std::move(*str_opt);
        }
        return flag;
    }

    std::shared_ptr<AdifDataBase> clone() const { return std::shared_ptr<AdifDataBase>(_clone()); }

    enum CompareRes {
        equal_to = 0,
        less = -1,
        greater = 1,
    };

    virtual CompareRes compare(const AdifDataBase &right) const;
};

#endif