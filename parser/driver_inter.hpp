#ifndef GLOG_PARSE_DRIVER_INTER
#define GLOG_PARSE_DRIVER_INTER

#include "ilexer.hpp"

namespace GLOG_PARSER {

class LexerInteractive : public xxFlexLexer, public ILexer {
  public:
    using xxFlexLexer::xxFlexLexer;

    int lex(Parser::semantic_type *yylval) override;

    void i_switch_streams(std::istream &new_in, std::ostream &new_out) override {
        xxFlexLexer::switch_streams(new_in, new_out);
    }
};

} // namespace GLOG_PARSER

#endif