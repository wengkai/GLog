cmake_minimum_required(VERSION 3.10)

set(LIB_NAME tools)

aux_source_directory(. TOOLS_FILES)

qt_add_library(${LIB_NAME} STATIC
        ${TOOLS_FILES}
)

target_link_libraries(${LIB_NAME} PUBLIC
        Qt6::Widgets
)

