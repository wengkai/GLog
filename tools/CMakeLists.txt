set(LIB_NAME tools)

aux_source_directory(. TOOLS_FILES)

qt_add_library(${LIB_NAME} STATIC
        ${TOOLS_FILES}
)

target_link_libraries(${LIB_NAME} PUBLIC
        Qt6::Widgets
        Qt6::Network
        Qt6::SvgWidgets
        Qt6::OpenGLWidgets
)

