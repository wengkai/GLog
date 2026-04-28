#ifndef GLOG_PARSE_DRIVER_IMPL
#define GLOG_PARSE_DRIVER_IMPL

#include "driver.hpp"

#include "ilexer.hpp"

namespace GLOG_PARSER {

template <template <typename> class Wrapper> class ParserDriverImpl : public ParserDriver {
    std::unique_ptr<ILexer> m_plexer;

  public:
    using DataVecT = Wrapper<std::vector<std::vector<std::pair<std::string, std::string>>>>;
    using ErrorVecT = Wrapper<std::vector<std::string>>;

    DataVecT data;
    ErrorVecT errors;

    class LexerErrorHandle : public ILexerErrorHandle {
        ParserDriverImpl *This;

      public:
        LexerErrorHandle(ParserDriverImpl *_This) : This(_This){};
        void AddError(const std::string &msg) override { This->AddError(msg); }
    };

    explicit ParserDriverImpl(std::unique_ptr<ILexer> plexer) : m_plexer(std::move(plexer)) {
        m_plexer->m_error_handle = std::make_unique<LexerErrorHandle>(this);
    }
    ParserDriverImpl(const ParserDriverImpl &) = delete;
    ParserDriverImpl(ParserDriverImpl &&) = delete;
    ParserDriverImpl &operator=(const ParserDriverImpl &) = delete;
    ParserDriverImpl &operator=(ParserDriverImpl &&) = delete;

    int lex(Parser::semantic_type *yylval) override { return m_plexer->lex(yylval); }

    void switch_streams(std::istream &new_in, std::ostream &new_out) override {
        m_plexer->i_switch_streams(new_in, new_out);
    }

    void AddRec(std::vector<std::pair<std::string, std::string>> rec) override {
        // data.write([&](auto &vec) { vec.push_back(std::move(rec)); });
        data.requireWrite()->emplace_back(std::move(rec));
    }

  protected:
    void _AddError(const std::string &msg) override {
        // errors.write([&](auto &vec) { vec.push_back(msg); });
        errors.requireWrite()->push_back(msg);
    }

    void _ClearErrors() override {
        // errors.write([](auto &vec) { vec.clear(); });
        errors.requireWrite()->clear();
        m_plexer->reset();
    }
};

using DriverSynchronized = ParserDriverImpl<Synchronized>;
using DriverUnsynchronized = ParserDriverImpl<Unsynchronized>;

} // namespace GLOG_PARSER

#endif