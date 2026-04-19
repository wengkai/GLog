#ifndef GLOG_PARSE_DRIVER
#define GLOG_PARSE_DRIVER

#include <cstring>
#include "parserdriver.h"

#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

namespace GLOG_PARSER {

#ifndef CLASS_SCANNER
#define CLASS_SCANNER
class GLogParserDriver : public yyFlexLexer, public ParserDriver {
  public:
    GLogParserDriver(std::istream &arg_yyin, std::ostream &arg_yyout)
        : yyFlexLexer(arg_yyin, arg_yyout) {}
    explicit GLogParserDriver(std::istream *arg_yyin = nullptr, std::ostream *arg_yyout = nullptr)
        : yyFlexLexer(arg_yyin, arg_yyout) {}
    explicit GLogParserDriver(std::vector<std::string> &errors, std::istream *arg_yyin = nullptr,
                              std::ostream *arg_yyout = nullptr)
        : yyFlexLexer(arg_yyin, arg_yyout), ParserDriver(errors) {}
    int lex(Parser::semantic_type *yylval);

  private:
    // char buf[YY_BUF_SIZE];
};
#else
class GLogParserDriver;
#endif

} // namespace GLOG_PARSER

#endif