%{
#include <iostream>
#include <string>
#include "FlexLexer.h"
%}
 
%require "3.7.0"
%language "C++"

%define parse.error detailed
%define api.parser.class {Parser}
%define api.namespace {GLOG_PARSER}
%define api.value.type variant
%parse-param {GLogParserDriver* driver}
 
%code requires
{
    namespace GLOG_PARSER {
        class GLogParserDriver;
    } // namespace GLOG_PARSER
    #include "parserdriver.h"
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

%type <std::vector<std::pair<std::string, std::string>>> PAIRS RECORD
%type <std::vector<std::vector<std::pair<std::string, std::string>>>> RECORDS
%type <std::string> ADIF


%%

ADIF:
  PAIRS EOH RECORDS { 
    driver->data = std::move($3); 
  }
  | RECORDS { 
    driver->data = std::move($1); 
  }
  | error EOH RECORDS {
    yyerrok;
    driver->data = std::move($3);
  }
  ;

RECORDS:
  RECORD { 
    ($$).push_back($1); 
  }
  | RECORDS RECORD { 
    ($1).push_back($2);
    $$ = std::move($1);
  }
  | RECORDS error {
    error("parse record failed");
    yyerrok;
    $$ = std::move($1);
  }
  ;

RECORD:
  PAIRS EOR { 
    $$ = std::move($1); 
  }
  ;

PAIRS:
  PAIR { 
    ($$).push_back($1); 
  }
  | PAIRS PAIR { 
    ($1).push_back($2); 
    $$ = std::move($1);
  }
  | PAIRS error {
    error("parse pair failed");
    yyerrok;
    $$ = std::move($1);
  }
  ;


%%

void GLOG_PARSER::Parser::error(const std::string& msg) {
    driver->SetError(const_cast<char *>(msg.c_str()));
}
// NOLINTEND