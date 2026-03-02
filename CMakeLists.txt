cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project ("GLog")

find_package(Qt6
    COMPONENTS
        Core
        Gui
        Widgets 
REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

get_target_property(MY_QT_TARGET_TYPE Qt6::Core TYPE)
if (MY_QT_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
    set(QT_LINK_STATIC ON)
    message(STATUS "Qt was built as a static library!")
    else()
    set(QT_LINK_STATIC OFF)
endif()
if (QT_LINK_STATIC)
    if(MSVC)
        message(STATUS "Static linking with MSVC, changing /MD to /MT")
	    foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif()
endif()

if (WIN32)
	message(STATUS "OS:Windows")
	add_definitions(-D YY_NO_UNISTD_H)
elseif (UNIX)
	message(STATUS "OS:UNIX-like OS's")
else ()
    message(WARNING "OS:others")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(STATUS "CC:MSVC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "CC:GNU")
else ()
    message(WARNING "CC:others")
endif()

if (UNIX)
    find_package(Threads)
    list (APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

if (WIN32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(STATUS "Adding ' -lwsock32 -lws2_32' for Windows GNU")
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lwsock32 -lws2_32")
    endif()
endif()

set(ROOT_DIR ${PROJECT_SOURCE_DIR})

# list (APPEND INCLUDES_DIR "${ROOT_DIR}/adif")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/parser")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/gui")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/model")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/tools")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/thirdparty")

include_directories(
	${INCLUDES_DIR}
	${CMAKE_BINARY_DIR}
)

# add_subdirectory(${ROOT_DIR}/adif)
# list (APPEND EXTRA_LIBS adif)

add_subdirectory(${ROOT_DIR}/parser)
list (APPEND EXTRA_LIBS parser)
add_subdirectory(${ROOT_DIR}/model)
list (APPEND EXTRA_LIBS model)
add_subdirectory(${ROOT_DIR}/tools)
list (APPEND EXTRA_LIBS tools)

qt_add_executable(${PROJECT_NAME} 
		${ROOT_DIR}/main.cpp
		${ROOT_DIR}/gui/GLogApplication.cpp
		${ROOT_DIR}/gui/GLogApplication.h
		${ROOT_DIR}/gui/GLogApplication.ui
)

if (WIN32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()

if (APPLE)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            MACOSX_BUNDLE TRUE
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${EXTRA_LIBS}
        Qt6::Widgets
)

# TODO
enable_testing()



