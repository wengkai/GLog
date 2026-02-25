%{


#include "parser.h"

#include "driver.hpp"

#define YY_DECL int GLOG_PARSER::GLogParserDriver::lex(GLOG_PARSER::Parser::semantic_type *yylval)

%}
 
%option c++ interactive noyywrap noyylineno nodefault

EOH   [Ee][Oo][Hh]
EOR   [Ee][Oo][Rr]
L     [a-zA-Z_]
D     [0-9]
LD    {L}|{D}

%%

"<"{EOH}">"  {
  return Parser::token::EOH;
}

"<"{EOR}">"  {
  return Parser::token::EOR;
}

"<"{LD}+":"{D}+">" {
  int n;
  std::sscanf(YYText(), "<%[^:]:%d>", this->buf, &n);
  // std::cout << this->buf << " " << n << std::endl;
  std::string data;
  while(n--) {
    data.push_back(yyinput());
  }
  // std::cout << data << std::endl;
  yylval->emplace<std::pair<std::string, std::string>>( std::pair<std::string, std::string> {this->buf, data});
  return Parser::token::PAIR;
}

"<"{LD}+":"{D}+":"{LD}+">" {
  int n;
  std::sscanf(YYText(), "<%[^:]:%d:%*[^>]>", this->buf, &n);
  // std::cout << this->buf << " " << n << std::endl;
  std::string data;
  while(n--) {
    data.push_back(yyinput());
  }
  // std::cout << data << std::endl;
  yylval->emplace<std::pair<std::string, std::string>>( std::pair<std::string, std::string> {this->buf, data});
  return Parser::token::PAIR;
}

[ \t\v\n\f] {
  ;
}

. {
  ;
}

%%

int yyFlexLexer::yylex() {
    throw std::runtime_error("Bad call to yyFlexLexer::yylex()");
}
