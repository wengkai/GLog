cmake_minimum_required(VERSION 3.16)
project(GLogKitWASAwardPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

find_package(GLogKit REQUIRED)

setup_bundled_qt_tools()
import_bundled_qt_module(Core)
import_bundled_qt_module(Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(WASPlugin MODULE was_plugin.cpp)

target_include_directories(WASPlugin PRIVATE
    ${SDK_INCLUDE_DIR}
)

target_link_libraries(WASPlugin PRIVATE
    GLogKit::GLogKitCore
    Qt6::Core
	Qt6::Sql
)

if(UNIX AND NOT APPLE)
    set_target_properties(WASPlugin PROPERTIES 
        INSTALL_RPATH "${SDK_LIB_DIR}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()