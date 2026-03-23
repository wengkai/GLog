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

if(${FLEX_INCLUDE_DIRS} MATCHES "NOTFOUND$")
        get_filename_component(FLEX_BIN_DIR "${FLEX_EXECUTABLE}" DIRECTORY)
        find_path(FLEX_INCLUDE_DIRS 
            NAMES FlexLexer.h
            HINTS "${FLEX_BIN_DIR}"
        )
        if(${NOT FLEX_INCLUDE_DIRS)
            message(WARNING "FLEX_INCLUDE_DIRS:NOTFOUND")
        else()
            list (APPEND PARSER_INCLUDE_FILES ${FLEX_INCLUDE_DIRS})
        endif()
else()
        list (APPEND PARSER_INCLUDE_FILES ${FLEX_INCLUDE_DIRS})
endif()

target_include_directories(${LIB_NAME} PUBLIC
        ${PARSER_INCLUDE_FILES}
)
