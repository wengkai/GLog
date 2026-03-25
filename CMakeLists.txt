cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("GLog")

find_package(Qt6 6.7
    COMPONENTS
        Core
        Gui
        Widgets 
        Network
        SvgWidgets
        OpenGLWidgets
        Sql
        Test
REQUIRED)
qt_standard_project_setup()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
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

include_directories(
    ${INCLUDES_DIR}
)

# add_subdirectory(${ROOT_DIR}/adif)
# list (APPEND EXTRA_LIBS adif)

add_subdirectory(${ROOT_DIR}/parser)
list (APPEND EXTRA_LIBS parser)
add_subdirectory(${ROOT_DIR}/model)
list (APPEND EXTRA_LIBS model)
add_subdirectory(${ROOT_DIR}/tools)
list (APPEND EXTRA_LIBS tools)

list (APPEND UI_FILES ${ROOT_DIR}/gui/GLogApplication.ui)
list (APPEND UI_FILES ${ROOT_DIR}/gui/SearchBar.ui)
list (APPEND UI_FILES ${ROOT_DIR}/gui/MapWidget.ui)
list (APPEND UI_FILES ${ROOT_DIR}/gui/ConfigureCtyDialog.ui)
list (APPEND UI_FILES ${ROOT_DIR}/gui/AddQSODialog.ui)

list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/GLogApplication.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/SearchBar.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/MapWidget.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/ConfigureCtyDialog.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/AddQSODialog.h)

set(PROJECT_SOURCES
#        ${ROOT_DIR}/main.cpp
        ${ROOT_DIR}/gui/GLogApplication.cpp
        ${ROOT_DIR}/gui/SearchBar.cpp
        ${ROOT_DIR}/gui/MapWidget.cpp
        ${ROOT_DIR}/gui/ConfigureCtyDialog.cpp
        ${ROOT_DIR}/gui/AddQSODialog.cpp
        ${UI_FILES}
        ${UI_INCLUDES}
)

qt_add_resources(PROJECT_SOURCES assets.qrc)

set(PROJECT_SHARED "${PROJECT_NAME}Core")

add_library(${PROJECT_SHARED} SHARED ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_SHARED} PRIVATE GLOG_LIB)

target_link_libraries(${PROJECT_SHARED}
    PUBLIC
        ${EXTRA_LIBS}
        Qt6::Widgets
        Qt6::Network
        Qt6::SvgWidgets
        Qt6::OpenGLWidgets
        Qt6::Sql
)

target_include_directories(${PROJECT_SHARED} PUBLIC 
    ${CMAKE_INCLUDE_CURRENT_DIR}
)

if(WIN32)
    set_target_properties(${PROJECT_SHARED} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        set_target_properties(${PROJECT_SHARED} PROPERTIES 
            LINK_FLAGS "/ignore:4197"
        )
    endif()
endif()

qt_add_executable(${PROJECT_NAME} ${ROOT_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_SHARED})

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

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

set(deploy_extra_options "")

if(FORCE_OPENSSL)
    list(APPEND deploy_extra_options "-force-openssl")
    message(STATUS "Pass -force-openssl for deployment")
endif()

if(WIN32)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
endif()

if(APPLE)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
    )
    install(TARGETS ${PROJECT_SHARED}
        LIBRARY DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
    )
else()
    install(TARGETS ${PROJECT_NAME} ${PROJECT_SHARED}
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_extra_options}
)
install(SCRIPT ${deploy_script})

option(BUILD_TESTING "Build the testing suite" ON)

if(BUILD_TESTING)
enable_testing()
add_subdirectory(tests)
endif()



