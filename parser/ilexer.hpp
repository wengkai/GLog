#ifndef GLOG_PARSE_DRIVER_ILEXER
#define GLOG_PARSE_DRIVER_ILEXER

#include <memory>

namespace GLOG_PARSER {

class ILexerErrorHandle {
  public:
    virtual ~ILexerErrorHandle() = default;
    virtual void AddError(const std::string &msg) = 0;
};

class ILexer {
  public:
    virtual ~ILexer() = default;
    virtual int lex(Parser::semantic_type *yylval) = 0;
    virtual void i_switch_streams(std::istream &new_in, std::ostream &new_out) = 0;
    void reset() {
        look_for_eor_ = false;
        _rest();
    }
    std::unique_ptr<ILexerErrorHandle> m_error_handle;

    void AddError(const std::string &msg) {
        if (m_error_handle) {
            m_error_handle->AddError(msg);
        }
    }

  protected:
    bool look_for_eor_ = false;
    virtual void _rest(){};
};

} // namespace GLOG_PARSER

#endif