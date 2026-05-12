cmake_minimum_required(VERSION 3.16)
project(GLogKitWASAwardPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Sql Network REQUIRED)

set(CMAKE_AUTOMOC ON)

add_library(WASPlugin MODULE was_plugin.cpp)

target_link_libraries(WASPlugin PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)

if(GLOGKIT_INTERNAL_BUILD)
if(NOT APPLE)
install(TARGETS WASPlugin LIBRARY DESTINATION plugins)
else()
install(TARGETS WASPlugin LIBRARY DESTINATION GLogKit.app/Contents/PlugIns)
endif()
endif()
