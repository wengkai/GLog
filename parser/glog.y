%{
#include <iostream>
#include <string>
#define yyFlexLexer xxFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#define yyFlexLexer yyFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
%}
 
%require "3.7.0"
%language "C++"

%define parse.error detailed
%define api.parser.class {Parser}
%define api.namespace {GLOG_PARSER}
%define api.value.type variant
%parse-param {ParserDriver* driver}
 
%code requires
{
    namespace GLOG_PARSER {
        class ParserDriver;
    } // namespace GLOG_PARSER
    // NOLINTBEGIN
    // STL first
    #include <optional>
    #include <regex>
    #include <algorithm>
    #include <string_view>
    #include <charconv>
    #include <exception>
    #include <memory>
} // %code requires

%code provides {
    // NOLINTEND
}
 
%code
{
    #include "driver.hpp"
    
    #define yylex(x) driver->lex(x)
    #include "AdifDataTypes.h"
    // NOLINTBEGIN
}

%token EOR EOH
%token <std::pair<std::string, std::string>> PAIR

%type <std::vector<std::pair<std::string, std::string>>> PAIRS


%%

ADIF:
  PAIRS EOH RECORDS { 
    // driver->data = std::move($3); 
  }
  | RECORDS { 
    // driver->data = std::move($1); 
  }
  | EOH RECORDS {
    // driver->data = std::move($2);
  }
  ;

RECORDS:
  RECORD { 
    // ($$).push_back($1); 
  }
  | RECORDS RECORD { 
    // ($1).push_back($2);
    // $$ = std::move($1);
  }
  | RECORDS error {
    error("parse record failed");
    yyclearin;
    yyerrok;
    // $$ = std::move($1);
  }
  ;

RECORD:
  PAIRS EOR {
    // driver->data.write([rec = std::move($1)](decltype(driver->data)::MyType & vec){
    //   vec.emplace_back(std::move(rec));
    // });
    driver->AddRec(std::move($1));
    // $$ = std::move($1); 
  }
  ;

PAIRS:
  PAIR { 
    ($$).emplace_back(std::move($1)); 
  }
  | PAIRS PAIR { 
    ($1).emplace_back(std::move($2)); 
    $$ = std::move($1);
  }
  | PAIRS error {
    error("parse pair failed");
    yyclearin;
    yyerrok;
    $$ = std::move($1);
  }
  ;


%%

void GLOG_PARSER::Parser::error(const std::string& msg) {
    driver->AddError(msg);
}
// NOLINTEND