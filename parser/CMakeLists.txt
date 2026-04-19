set(LIB_NAME parser)

aux_source_directory(. PARSER_FILES)

find_package(BISON)
BISON_TARGET(MyParser glog.y ${CMAKE_BINARY_DIR}/parser.cpp
             DEFINES_FILE ${CMAKE_BINARY_DIR}/parser.h)
list (APPEND PARSER_FILES ${CMAKE_BINARY_DIR}/parser.cpp)

find_package(FLEX)
FLEX_TARGET(MyScanner glog.l  ${CMAKE_BINARY_DIR}/driver_lex.cpp)
list (APPEND PARSER_FILES ${CMAKE_BINARY_DIR}/driver_lex.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

qt_add_library(${LIB_NAME} STATIC
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
endif()

target_include_directories(${LIB_NAME} PUBLIC
    BEFORE
        ${PARSER_INCLUDE_FILES}
)

target_compile_definitions(${LIB_NAME} PRIVATE GLOG_LIB)
