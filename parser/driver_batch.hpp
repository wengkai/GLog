#ifndef GLOG_PARSE_DRIVER_BATCH
#define GLOG_PARSE_DRIVER_BATCH

#include "ilexer.hpp"

namespace GLOG_PARSER {

class LexerBatch : public yyFlexLexer, public ILexer {
  public:
    using yyFlexLexer::yyFlexLexer;

    int lex(Parser::semantic_type *yylval) override;
    void i_switch_streams(std::istream &new_in, std::ostream &new_out) override {
        yyFlexLexer::switch_streams(new_in, new_out);
    }
};

} // namespace GLOG_PARSER

#endif