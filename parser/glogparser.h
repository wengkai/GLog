#ifndef GLOG_PARSER_H
#define GLOG_PARSER_H

// clang-format off
#include "parser.h"
#define yyFlexLexer xxFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#define yyFlexLexer yyFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#include "driver.hpp"
#include "driver_inter.hpp"
#include "driver_batch.hpp"
#include "driver_impl.hpp"
// clang-format on

#endif