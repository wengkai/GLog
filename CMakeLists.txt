cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project ("GLog")

if (WIN32)
	message(STATUS "OS:Windows")
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

list (APPEND INCLUDES_DIR "${ROOT_DIR}/adif")

include_directories(
	${INCLUDES_DIR}
)

add_subdirectory(${ROOT_DIR}/adif)
list (APPEND EXTRA_LIBS adif)

add_executable(${PROJECT_NAME} ${ROOT_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME}
		${EXTRA_LIBS}
)

# TODO
enable_testing()



