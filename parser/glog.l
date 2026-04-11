%top{
// NOLINTBEGIN
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4005)
#endif
// STL first
#include <optional>
#include <regex>
#include <algorithm>
#include <string_view>
#include <charconv>
#include <exception>
#include <memory>

}

%{


#include "parser.h"

#include "driver.hpp"

#define YY_DECL int GLOG_PARSER::GLogParserDriver::lex(GLOG_PARSER::Parser::semantic_type *yylval)

// standard classes using stl
#include "AdifDataTypes.h"

#define YY_FATAL_ERROR(msg) throw std::runtime_error( msg )

%}
 
%option c++ never-interactive noyywrap noyylineno nodefault

EOH   [Ee][Oo][Hh]
EOR   [Ee][Oo][Rr]
L     [a-zA-Z_]
D     [0-9]
LD    {L}|{D}

%%

"<"{EOH}">"  {
  look_for_eor_ = false;
  return Parser::token::EOH;
}

"<"{EOR}">"  {
  look_for_eor_ = false;
  return Parser::token::EOR;
}

"<"{LD}+":"{D}+">" { // <[a-zA-Z_0-9]+:[0-9]+>
  unsigned long long n;
  std::string_view sv(YYText());
  auto pos1 = sv.find(':');
  auto part1 = std::string(sv.substr(1, pos1 - 1));
  auto pos2 = sv.find('>', pos1 + 1);
  std::string_view num_part = sv.substr(pos1 + 1, pos2 - pos1 - 1);
  auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), n);
  // FATAL: Can not continue
  if (ec != std::errc{}) {
    YY_FATAL_ERROR("Invalid length in protocol"); // #define YY_FATAL_ERROR(msg) throw std::runtime_error( msg )
  }
  // std::cout << part1 << " " << n << std::endl;
  // Data part: UTF-8 string
  std::string data;
  data.reserve(n);
  while(n--) {
    int c = yyinput();
    // Case noyywrap
    // The lexer confirms it has reached the ultimate end of the input stream and returns 0
    // This break will cause a syntax error reported by parser, since it will look for a <EOR> token and find nothing
    if (c <= 0) {
      break;
    }
    data.push_back(static_cast<char>(c));
  }
  // std::cout << data << std::endl;
  yylval->emplace<std::pair<std::string, std::string>>( std::pair<std::string, std::string> {std::move(part1), std::move(data)});
  look_for_eor_ = true;
  return Parser::token::PAIR;
}

"<"{LD}+":"{D}+":"{LD}+">" { // <[a-zA-Z_0-9]+:[0-9]+:[a-zA-Z_0-9]+>
  unsigned long long n;
  std::string_view sv(YYText());
  auto pos1 = sv.find(':');
  auto part1 = std::string(sv.substr(1, pos1 - 1));
  auto pos2 = sv.find(':', pos1 + 1);
  std::string_view num_part = sv.substr(pos1 + 1, pos2 - pos1 - 1);
  auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), n);
  // FATAL: Can not continue
  if (ec != std::errc{}) {
    YY_FATAL_ERROR("Invalid length in protocol"); // #define YY_FATAL_ERROR(msg) throw std::runtime_error( msg )
  }
  // std::cout << part1 << " " << n << std::endl;
  // Data part: UTF-8 string
  std::string data;
  data.reserve(n);
  while(n--) {
    int c = yyinput();
    // Case noyywrap
    // The lexer confirms it has reached the ultimate end of the input stream and returns 0
    // This break will cause a syntax error reported by parser, since it will look for a <EOR> token and find nothing
    if (c <= 0) {
      break;
    }
    data.push_back(static_cast<char>(c));
  }
  // std::cout << data << std::endl;
  yylval->emplace<std::pair<std::string, std::string>>( std::pair<std::string, std::string> {std::move(part1), std::move(data)});
  look_for_eor_ = true;
  return Parser::token::PAIR;
}

<<EOF>> {
    if (look_for_eor_) {
        SetError("Unexpected End of File.");
        look_for_eor_ = false;
        return Parser::token::EOR;
    }
    yyterminate();
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

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
// NOLINTEND
