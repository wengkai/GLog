cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_AUTOGEN_COMPLIANCE_CHECK ON)

project ("GLogKit"
    VERSION 0.1.0 
    LANGUAGES CXX
)

set(SDK_EXPORT_NAME "${PROJECT_NAME}Targets")

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

set(EXTRA_LIBS "")

if(LINUX OR UNIX AND NOT APPLE AND NOT ANDROID)
    message(STATUS "Detecting Wayland support for Qt6...")
    find_package(Qt6 COMPONENTS WaylandClient QUIET)
    if(Qt6WaylandClient_FOUND)
        message(STATUS "Qt6 Wayland support found. Adding to EXTRA_LIBS.")
        list(APPEND EXTRA_LIBS Qt6::WaylandClient)
        add_definitions(-DHAS_WAYLAND_SUPPORT)
    else()
        message(WARNING "Qt6 WaylandClient NOT found. Application will rely on X11/XCB.")
    endif()
endif()

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
    message(STATUS "CC:others")
    if (WIN32)
        add_compile_options("-Wno-deprecated-declarations")
    endif ()
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

list (APPEND INCLUDES_DIR "${ROOT_DIR}/include")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/include/GLogKit")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/adif")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/adif/constants")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/parser")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/gui")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/model")
list (APPEND INCLUDES_DIR "${ROOT_DIR}/tools")

include_directories(
    ${INCLUDES_DIR}
)

add_subdirectory(${ROOT_DIR}/adif)
list (APPEND EXTRA_LIBS adif)
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
list (APPEND UI_FILES ${ROOT_DIR}/gui/DuplicatesManager.ui)
list (APPEND UI_FILES ${ROOT_DIR}/gui/AwardPluginManager.ui)

list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/GLogApplication.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/SearchBar.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/MapWidget.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/ConfigureCtyDialog.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/AddQSODialog.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/DuplicatesManager.h)
list (APPEND UI_INCLUDES ${ROOT_DIR}/gui/AwardPluginManager.h)

set(PROJECT_SOURCES
#        ${ROOT_DIR}/main.cpp
        ${ROOT_DIR}/gui/GLogApplication.cpp
        ${ROOT_DIR}/gui/SearchBar.cpp
        ${ROOT_DIR}/gui/MapWidget.cpp
        ${ROOT_DIR}/gui/ConfigureCtyDialog.cpp
        ${ROOT_DIR}/gui/AddQSODialog.cpp
        ${ROOT_DIR}/gui/DuplicatesManager.cpp
        ${ROOT_DIR}/gui/AwardPluginManager.cpp
        ${UI_FILES}
        ${UI_INCLUDES}
)

qt_add_resources(PROJECT_SOURCES assets.qrc)

set(PROJECT_SHARED "${PROJECT_NAME}Core")

add_library(${PROJECT_SHARED} SHARED ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_SHARED} PRIVATE GLOG_LIB)
target_compile_definitions(${PROJECT_SHARED} PRIVATE GLOGKIT_LIB)

list(APPEND EXTRA_LIBS Qt6::Widgets)
list(APPEND EXTRA_LIBS Qt6::Network)
list(APPEND EXTRA_LIBS Qt6::SvgWidgets)
list(APPEND EXTRA_LIBS Qt6::OpenGLWidgets)
list(APPEND EXTRA_LIBS Qt6::Sql)

# Static libs merged into ${PROJECT_SHARED}; do not link them again into exe/tests (LNK2005).
set(GLOGKIT_CORE_MERGED_LIBS adif parser model tools)
set(GLOGKIT_APP_LINK_LIBS ${EXTRA_LIBS})
list(REMOVE_ITEM GLOGKIT_APP_LINK_LIBS ${GLOGKIT_CORE_MERGED_LIBS})

target_link_libraries(${PROJECT_SHARED}
    PRIVATE
        ${EXTRA_LIBS}
)

set_target_properties(${PROJECT_SHARED} PROPERTIES ENABLE_EXPORTS TRUE)

target_include_directories(${PROJECT_SHARED} PUBLIC 
    ${CMAKE_INCLUDE_CURRENT_DIR}
	$<INSTALL_INTERFACE:include>
)

if(WIN32 AND MSVC)
    set_target_properties(${PROJECT_SHARED} PROPERTIES
        LINK_FLAGS "/ignore:4197"
    )
endif()

qt_add_executable(${PROJECT_NAME} ${ROOT_DIR}/main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS TRUE)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${PROJECT_SHARED}
    ${GLOGKIT_APP_LINK_LIBS}
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

if(APPLE)
    set_target_properties(${PROJECT_SHARED} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
        FRAMEWORK TRUE
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        MACOSX_FRAMEWORK_IDENTIFIER "com.${PROJECT_NAME}.${PROJECT_SHARED}"
    )
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

set(deploy_extra_options "")

if(FORCE_OPENSSL)
    list(APPEND deploy_extra_options "-force-openssl")
    message(STATUS "Pass -force-openssl for deployment")
endif()

option(DISTRIBUTE_SDK "Distribute SDK" ON)
option(BUNDLED_QT "BUNDLED_QT" OFF)
if(BUNDLED_QT)

message(STATUS "BUNDLED_QT:ON")

if (TARGET Qt6::Platform)
set(PROPS_TO_EXTRACT
    INTERFACE_COMPILE_DEFINITIONS
    INTERFACE_COMPILE_OPTIONS
)

foreach(prop ${PROPS_TO_EXTRACT})
    get_target_property(VAL Qt6::Platform ${prop})
    if(VAL)
        set(EXPORTED_PLATFORM_${prop} "${VAL}")
        message(STATUS "Detected Qt6::Platform:EXPORTED_PLATFORM_${prop}: ${VAL}")
        set_property(TARGET ${PROJECT_SHARED} APPEND PROPERTY ${prop} "${VAL}")
    endif()
endforeach()
endif()
endif() # BUNDLED_QT

if(APPLE)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
    )
    install(TARGETS ${PROJECT_SHARED}
        EXPORT ${SDK_EXPORT_NAME}
        LIBRARY DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
        FRAMEWORK DESTINATION  ${PROJECT_NAME}.app/Contents/Frameworks
        RUNTIME DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
    )
else()
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(TARGETS ${PROJECT_SHARED}
        EXPORT ${SDK_EXPORT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(WIN32)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
    list(APPEND deploy_extra_options "${CMAKE_INSTALL_BINDIR}/${PROJECT_SHARED}.dll")
endif()

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_extra_options}
)
install(SCRIPT ${deploy_script})



if(DISTRIBUTE_SDK)
message(STATUS "DISTRIBUTE_SDK:ON")
set(APPLE_SDK_PREFIX "SDK")

if (NOT APPLE)
install(DIRECTORY include/ DESTINATION include)
else()
install(TARGETS ${PROJECT_SHARED}
    FRAMEWORK DESTINATION "${APPLE_SDK_PREFIX}/"
    PUBLIC_HEADER DESTINATION "${PROJECT_SHARED}.framework/Headers"
)
install(DIRECTORY include/
    DESTINATION "${APPLE_SDK_PREFIX}/${PROJECT_SHARED}.framework/Headers"
)
endif()


function(get_all_qt6_targets target out_var)
    set(all_targets)
    set(targets_to_process ${target})
    set(processed_targets)

    while(targets_to_process)
        list(POP_FRONT targets_to_process current)
        list(FIND processed_targets "${current}" _found)
        if(_found EQUAL -1)
            list(APPEND processed_targets "${current}")
            get_target_property(link_libs ${current} LINK_LIBRARIES)
            if(link_libs)
                foreach(lib ${link_libs})
                    _extract_qt6_target("${lib}" lib_clean)
                    if(lib_clean AND TARGET ${lib_clean})
                        list(APPEND all_targets ${lib_clean})
                        list(APPEND targets_to_process ${lib_clean})
                    endif()
                endforeach()
            endif()
            get_target_property(iface_libs ${current} INTERFACE_LINK_LIBRARIES)
            if(iface_libs)
                foreach(lib ${iface_libs})
                    _extract_qt6_target("${lib}" lib_clean)
                    if(lib_clean AND TARGET ${lib_clean})
                        list(APPEND all_targets ${lib_clean})
                        list(APPEND targets_to_process ${lib_clean})
                    endif()
                endforeach()
            endif()
        endif()
    endwhile()

    list(REMOVE_DUPLICATES all_targets)
    set(${out_var} ${all_targets} PARENT_SCOPE)
endfunction()

macro(_extract_qt6_target input output)
    set(${output})
    if("${input}" MATCHES "^Qt6::")
        set(${output} "${input}")
    elseif("${input}" MATCHES "\\$<LINK_ONLY:([^>]+)>")
        string(REGEX REPLACE "\\$<LINK_ONLY:([^>]+)>" "\\1" tmp "${input}")
        if(tmp MATCHES "^Qt6::")
            set(${output} "${tmp}")
        endif()
    endif()
endmacro()

get_all_qt6_targets(${PROJECT_SHARED} ALL_QT6_TARGETS)

set(ALL_QT6_MODULES)
foreach(tgt ${ALL_QT6_TARGETS})
    if(NOT TARGET ${tgt})
        continue()
    endif()
    get_target_property(tgt_type ${tgt} TYPE)
    if(NOT tgt_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_imported ${tgt} IMPORTED)
        if(NOT is_imported)
            continue()
        endif()
    endif()
    get_target_property(inc_dirs ${tgt} INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT inc_dirs)
        continue()
    endif()
    string(REGEX REPLACE "^Qt6::" "" module ${tgt})
    set(has_valid_header FALSE)
    foreach(dir ${inc_dirs})
        if(EXISTS "${dir}/Qt${module}")
            set(has_valid_header TRUE)
            break()
        endif()
    endforeach()
    if(has_valid_header)
        list(APPEND ALL_QT6_MODULES ${module})
    endif()
endforeach()

list(REMOVE_DUPLICATES ALL_QT6_MODULES)
message(STATUS "Detected Qt6 modules for installation: ${ALL_QT6_MODULES}")

if(BUNDLED_QT)
if(NOT APPLE)
foreach(module ${ALL_QT6_MODULES})
    set(tgt_name Qt6::${module})
    if(TARGET ${tgt_name})
        get_target_property(inc_dirs ${tgt_name} INTERFACE_INCLUDE_DIRECTORIES)
        set(module_installed FALSE)
        foreach(dir ${inc_dirs})
            if(module_installed)
                break()
            endif()
            get_filename_component(dir_name ${dir} NAME)
            if(dir_name STREQUAL "Qt${module}")
                if(EXISTS ${dir} AND IS_DIRECTORY ${dir})
                    install(DIRECTORY ${dir} DESTINATION include)
                    set(module_installed TRUE)
                    break()
                endif()
            endif()
            set(sub_dir "${dir}/Qt${module}")
            if(EXISTS ${sub_dir} AND IS_DIRECTORY ${sub_dir})
                install(DIRECTORY ${sub_dir} DESTINATION include)
                set(module_installed TRUE)
                break()
            endif()
        endforeach()
        if(NOT module_installed)
            message(WARNING "Could not find header directory for module ${module}")
        endif()
    endif()
endforeach()
else()
endif()
endif()

if(BUNDLED_QT)
if(WIN32)
    foreach(module ${ALL_QT6_MODULES})
        set(tgt_name Qt6::${module})
        if(TARGET ${tgt_name})
            install(FILES $<TARGET_LINKER_FILE:${tgt_name}> DESTINATION lib)
        endif()
    endforeach()
endif()
if(APPLE)
    foreach(module ${ALL_QT6_MODULES})
        set(tgt_name Qt6::${module})
        if(TARGET ${tgt_name})
            get_target_property(_path ${tgt_name} IMPORTED_LOCATION)
            
            # Start climbing up from the binary location
            set(_root "${_path}")
            set(_is_fw FALSE)
            
            # Loop up to 4 levels (usually enough to hit .framework from the binary)
            foreach(i RANGE 4)
                get_filename_component(_root "${_root}" DIRECTORY)
                if(_root MATCHES "\\.framework$")
                    set(_is_fw TRUE)
                    break()
                endif()
            endforeach()

            if(_is_fw)
                # We found the .framework folder! 
                # Install the whole folder into the SDK directory
                install(DIRECTORY "${_root}" 
                        DESTINATION "${APPLE_SDK_PREFIX}/"
                        USE_SOURCE_PERMISSIONS)
            else()
                message(WARNING "Could not find .framework root for ${tgt_name} at ${_path}")
            endif()
        endif()
    endforeach()
endif()
endif()

if(BUNDLED_QT)
if(NOT APPLE)
install(PROGRAMS $<TARGET_FILE:Qt6::moc> DESTINATION bin)
install(PROGRAMS $<TARGET_FILE:Qt6::rcc> DESTINATION bin)
install(PROGRAMS $<TARGET_FILE:Qt6::uic> DESTINATION bin)
else()
get_target_property(MOC_LOC Qt6::moc IMPORTED_LOCATION)
get_target_property(RCC_LOC Qt6::rcc IMPORTED_LOCATION)
get_target_property(UIC_LOC Qt6::uic IMPORTED_LOCATION)
install(PROGRAMS ${MOC_LOC} ${RCC_LOC} ${UIC_LOC} 
    DESTINATION "${APPLE_SDK_PREFIX}/bin")
endif()
endif()

set(QT_METADATA_INJECTION "")

set(_ALLOWED_MODULES "")
foreach(_MOD ${ALL_QT6_MODULES})
    string(REPLACE "Qt6::" "" _PURE_MOD ${_MOD})
    list(APPEND _ALLOWED_MODULES ${_PURE_MOD})
endforeach()

foreach(_MOD_FULL ${ALL_QT6_MODULES})
    string(REPLACE "Qt6::" "" _PURE_MOD ${_MOD_FULL})
    
    get_target_property(_RAW_DEPS Qt6::${_PURE_MOD} INTERFACE_LINK_LIBRARIES)
    
    set(_FILTERED_DEPS "")
    foreach(_DEP ${_RAW_DEPS})
        if(_DEP MATCHES "^Qt6::([A-Za-z0-9]+)")
            set(_DEP_NAME ${CMAKE_MATCH_1})
            list(APPEND _FILTERED_DEPS ${_DEP_NAME})
            if(_DEP_NAME IN_LIST _ALLOWED_MODULES)
            elseif(_DEP_NAME STREQUAL "Platform")
                continue()
            else()
                message(AUTHOR_WARNING "Module ${_PURE_MOD} depends on ${_DEP_NAME}, but ${_DEP_NAME} is NOT in the distribution list. Skipping dependency.")
            endif()
        endif()
    endforeach()

    if(_FILTERED_DEPS)
        list(REMOVE_DUPLICATES _FILTERED_DEPS)
        string(REPLACE ";" " " _DEPS_STR "${_FILTERED_DEPS}")
        string(APPEND QT_METADATA_INJECTION "        set(_QT_METADATA_${_PURE_MOD}_DEPS ${_DEPS_STR})\n")
    endif()
endforeach()

message(STATUS "Detected Qt6 metadata: \n${QT_METADATA_INJECTION}")

include(CMakePackageConfigHelpers)

if(NOT APPLE)
install(EXPORT ${SDK_EXPORT_NAME}
    FILE ${SDK_EXPORT_NAME}.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)
else()
install(EXPORT ${SDK_EXPORT_NAME}
    FILE ${SDK_EXPORT_NAME}.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION "${APPLE_SDK_PREFIX}/${PROJECT_SHARED}.framework/Resources/CMake"
)
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

set(QT_VERSION ${Qt6_VERSION})
set(QT_VERSION_MAJOR ${Qt6_VERSION_MAJOR})
set(QT_VERSION_MINOR ${Qt6_VERSION_MINOR})

if(NOT APPLE)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
else()
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${APPLE_SDK_PREFIX}/${PROJECT_SHARED}.framework/Resources/CMake"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
endif()

if(NOT APPLE)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
)
else()
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${APPLE_SDK_PREFIX}/${PROJECT_SHARED}.framework/Resources/CMake"
)
endif()

endif() #DISTRIBUTE_SDK

set(GLOGKIT_INTERNAL_BUILD ON)
add_subdirectory(${ROOT_DIR}/plugins/was_plugin)

option(BUILD_TESTING "Build the testing suite" ON)

if(BUILD_TESTING)
enable_testing()
add_subdirectory(tests)

if(WIN32)
    set(GLOG_COVERAGE_CTEST_CONFIG "$<CONFIG>" CACHE STRING
        "CTest -C value for the coverage custom target (match your MSVC configuration, e.g. Debug or RelWithDebInfo)")
    # Note: $ENV{ProgramFiles(x86)} is invalid CMake syntax; use bracket form if needed.
    find_program(OpenCppCoverage NAMES OpenCppCoverage.exe
        HINTS
            "$ENV{ProgramFiles}/OpenCppCoverage"
            "$ENV{ProgramW6432}/OpenCppCoverage"
            "$ENV{ProgramFiles\(x86\)}/OpenCppCoverage"
    )
    if(OpenCppCoverage)
        add_custom_target(coverage
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
                "${CMAKE_SOURCE_DIR}/scripts/Generate-TestCoverage.ps1"
                -OpenCppCoverage "${OpenCppCoverage}"
                -BuildDir "${CMAKE_BINARY_DIR}"
                -SourceRoot "${CMAKE_SOURCE_DIR}"
                -Config "${GLOG_COVERAGE_CTEST_CONFIG}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "OpenCppCoverage + ctest (build ${GLOG_COVERAGE_CTEST_CONFIG} with PDBs first)"
            VERBATIM
        )
        if(GLOG_TEST_EXECUTABLE_TARGETS)
            add_dependencies(coverage ${GLOG_TEST_EXECUTABLE_TARGETS})
        endif()
        message(STATUS "OpenCppCoverage: ${GLOG_TEST_EXECUTABLE_TARGETS}")
    else()
        message(STATUS "OpenCppCoverage.exe not found; coverage target disabled")
    endif()

    # LLVM source-based coverage target. Only meaningful with the clang/LLVM toolchain
    # built with -fprofile-instr-generate -fcoverage-mapping. The target invokes
    # scripts/Generate-LlvmCoverage.ps1 which runs ctest with LLVM_PROFILE_FILE set,
    # merges the resulting .profraw files via llvm-profdata and renders HTML + lcov
    # via llvm-cov.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        find_program(LLVM_COV_EXECUTABLE NAMES llvm-cov)
        find_program(LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata)
        if(LLVM_COV_EXECUTABLE AND LLVM_PROFDATA_EXECUTABLE)
            add_custom_target(llvm-coverage
                COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
                    "${CMAKE_SOURCE_DIR}/scripts/Generate-LlvmCoverage.ps1"
                    -LlvmCov "${LLVM_COV_EXECUTABLE}"
                    -LlvmProfdata "${LLVM_PROFDATA_EXECUTABLE}"
                    -BuildDir "${CMAKE_BINARY_DIR}"
                    -SourceRoot "${CMAKE_SOURCE_DIR}"
                    -Config "${GLOG_COVERAGE_CTEST_CONFIG}"
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                COMMENT "LLVM source-based coverage (clang + ctest + llvm-cov)"
                VERBATIM
            )
            if(GLOG_TEST_EXECUTABLE_TARGETS)
                add_dependencies(llvm-coverage ${GLOG_TEST_EXECUTABLE_TARGETS})
            endif()
            if(NOT CMAKE_CXX_FLAGS MATCHES "fcoverage-mapping")
                message(WARNING "llvm-coverage target enabled, but CMAKE_CXX_FLAGS does not contain"
                                " -fcoverage-mapping. Reconfigure with"
                                " -DCMAKE_CXX_FLAGS=\"-fprofile-instr-generate -fcoverage-mapping ...\""
                                " for meaningful results.")
            else()
                message(STATUS "llvm-coverage: ${LLVM_COV_EXECUTABLE}")
            endif()
        else()
            message(STATUS "llvm-cov / llvm-profdata not found; llvm-coverage target disabled")
        endif()
    endif()
endif()

endif()



