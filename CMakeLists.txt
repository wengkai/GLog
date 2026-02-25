cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project ("GLog")

find_package(Qt6
    COMPONENTS
        Core
        Gui
        Widgets 
REQUIRED)
qt_standard_project_setup()

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

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${EXTRA_LIBS}
        Qt6::Widgets
)

# TODO
enable_testing()



