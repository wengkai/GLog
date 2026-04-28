set(LIB_NAME parser)

aux_source_directory(. PARSER_FILES)

find_package(BISON)
# 1. Generate Parser
BISON_TARGET(MyParser glog.y ${CMAKE_BINARY_DIR}/parser.cpp
             DEFINES_FILE ${CMAKE_BINARY_DIR}/parser.h)
list (APPEND PARSER_FILES ${CMAKE_BINARY_DIR}/parser.cpp)

find_package(FLEX)

set(FLEX_FLAGS "--case-insensitive")

set(FLEX_COMPAT "")
if(WIN32)
set(FLEX_COMPAT "--wincompat --nounistd")
endif()

# 2. Generate Batch Lexer
set(BATCH_LEXER_FLAGS "${FLEX_COMPAT} ${FLEX_FLAGS} --batch")
message(STATUS "BATCH_LEXER_FLAGS: ${BATCH_LEXER_FLAGS}")
FLEX_TARGET(ScannerBatch glog.l ${CMAKE_BINARY_DIR}/driver_lex.cpp
            COMPILE_FLAGS ${BATCH_LEXER_FLAGS})
list (APPEND PARSER_FILES ${CMAKE_BINARY_DIR}/driver_lex.cpp)

# 3. Generate Interactive Lexer (Add prefix to avoid collisions)
set(INTER_LEXER_FLAGS "${FLEX_COMPAT} ${FLEX_FLAGS} --interactive --prefix=xx --yyclass=xxFlexLexer")
message(STATUS "INTER_LEXER_FLAGS: ${INTER_LEXER_FLAGS}")
FLEX_TARGET(ScannerInter glog.l ${CMAKE_BINARY_DIR}/driver_lex_interactive.cpp
            COMPILE_FLAGS ${INTER_LEXER_FLAGS})
list (APPEND PARSER_FILES ${CMAKE_BINARY_DIR}/driver_lex_interactive.cpp)

ADD_FLEX_BISON_DEPENDENCY(ScannerBatch MyParser)
ADD_FLEX_BISON_DEPENDENCY(ScannerInter MyParser)

add_library(${LIB_NAME} STATIC
        ${PARSER_FILES}
)

list (APPEND PARSER_INCLUDE_FILES ${CMAKE_BINARY_DIR})

if(NOT FLEX_INCLUDE_DIRS OR NOT EXISTS "${FLEX_INCLUDE_DIRS}/FlexLexer.h")
    if(WIN32)
        unset(FLEX_INCLUDE_DIRS CACHE)
        find_path(FLEX_INCLUDE_DIRS 
            NAMES FlexLexer.h
            PATHS "C:/ProgramData/chocolatey/lib/winflexbison3/tools"
        )
    endif()
    if(APPLE)
        unset(FLEX_INCLUDE_DIRS CACHE)
        execute_process(
            COMMAND brew --prefix flex 
            OUTPUT_VARIABLE 
            FLEX_PREFIX 
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(FLEX_PREFIX AND EXISTS "${FLEX_PREFIX}/include/FlexLexer.h")
            set(FLEX_INCLUDE_DIRS "${FLEX_PREFIX}/include")
        endif()
    endif()
endif()

if(NOT FLEX_INCLUDE_DIRS OR NOT EXISTS "${FLEX_INCLUDE_DIRS}/FlexLexer.h")
    message(WARNING "FLEX_INCLUDE_DIRS:NOTFOUND")
else()
    list(APPEND PARSER_INCLUDE_FILES ${FLEX_INCLUDE_DIRS})
    message(STATUS "FLEX_INCLUDE_DIRS: ${FLEX_INCLUDE_DIRS}")
endif()

target_include_directories(${LIB_NAME} PUBLIC
    BEFORE
        ${PARSER_INCLUDE_FILES}
)

target_compile_definitions(${LIB_NAME} PRIVATE GLOG_LIB)
