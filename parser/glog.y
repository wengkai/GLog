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
} // %code requires
 
%code
{
    #include "driver.hpp"
    
    #define yylex(x) driver->lex(x)
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
  ;

RECORDS:
  RECORD { 
    ($$).push_back($1); 
  }
  | RECORD RECORDS { 
    ($2).push_back($1);
    $$ = std::move($2);
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
  | PAIR PAIRS { 
    ($2).push_back($1); 
    $$ = std::move($2);
  }
  ;


%%

void GLOG_PARSER::Parser::error(const std::string& msg) {
    driver->GLogParserSetError(const_cast<char *>(msg.c_str()));
}