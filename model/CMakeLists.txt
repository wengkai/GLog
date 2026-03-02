cmake_minimum_required(VERSION 3.10)

set(LIB_NAME model)

aux_source_directory(. MODEL_FILES)

qt_add_library(${LIB_NAME} STATIC
        ${MODEL_FILES}
)

target_link_libraries(${LIB_NAME} PUBLIC
        Qt6::Widgets
        parser
)

