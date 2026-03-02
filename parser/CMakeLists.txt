cmake_minimum_required(VERSION 3.10)

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

add_library(${LIB_NAME} STATIC
        ${PARSER_FILES}
)

target_include_directories(${LIB_NAME} PUBLIC
        ${FLEX_INCLUDE_DIRS}
)
