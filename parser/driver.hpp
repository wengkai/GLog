#ifndef GLOG_PARSE_DRIVER
#define GLOG_PARSE_DRIVER

#include <cstring>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "Synchronize.h"

namespace GLOG_PARSER {

#ifndef CLASS_SCANNER
#define CLASS_SCANNER

class ParserDriver {

  public:
    explicit ParserDriver() = default;
    ~ParserDriver() = default;

    virtual int lex(Parser::semantic_type *yylval) = 0;
    virtual void switch_streams(std::istream &new_in, std::ostream &new_out) = 0;

    virtual void AddRec(std::vector<std::pair<std::string, std::string>> rec) = 0;
    void AddError(const std::string &msg) {
        _AddError(msg);

        if (++m_errors >= m_max_error_count) {
            throw std::runtime_error("Parser halted: reached limit of " +
                                     std::to_string(m_max_error_count) + " errors. " +
                                     "Last error: " + msg);
        }
    }

    static constexpr size_t MAX_ERROR_COUNT = 100;

    void SetMaxErrorCount(size_t count) { m_max_error_count = count; }

  private:
    size_t m_errors = 0;
    size_t m_max_error_count = MAX_ERROR_COUNT;

  protected:
    virtual void _AddError(const std::string &msg) = 0;
    void ClearErrors() {
        _ClearErrors();
        m_errors = 0;
    }
    virtual void _ClearErrors() = 0;
};

#else
class ParserDriver;
#endif

} // namespace GLOG_PARSER

#endif