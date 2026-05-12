find_package(Qt6 6.5
    COMPONENTS
        Core
        Test
        Widgets
REQUIRED)

macro(add_test_unit test_name)
    qt_add_executable(${test_name} "${test_name}.cpp")

    target_link_libraries(${test_name} PRIVATE
        ${PROJECT_SHARED}
        ${GLOGKIT_APP_LINK_LIBS}
        Qt6::Test
    )
    # GLogKitCore is linked against static ${GLOGKIT_CORE_MERGED_LIBS}; 
    # the linker only pulls .o members
    # referenced from the shared lib's own TUs. Types used only in unit tests (e.g.
    # AdifCharacter) may be absent from the .so on ELF, causing undefined vtable at
    # link time. MSVC: do not link ${GLOGKIT_CORE_MERGED_LIBS} again here 
    # (LNK2005 duplicate symbols vs DLL).
    if(NOT MSVC)
        target_link_libraries(${test_name} PRIVATE ${GLOGKIT_CORE_MERGED_LIBS})
    endif()

    # Multi-config generators (Visual Studio): resolve exe at test time.
    add_test(NAME ${test_name} COMMAND "$<TARGET_FILE:${test_name}>")

    list(APPEND TEST_UNITS ${test_name})
    
    # message(STATUS "Added test unit: ${test_name}")
endmacro()

file(GLOB TEST_SOURCES "*.cpp")

foreach(test_file_path ${TEST_SOURCES})
    get_filename_component(test_name_with_ext ${test_file_path} NAME)
    
    string(REPLACE ".cpp" "" test_name ${test_name_with_ext})
    
    add_test_unit(${test_name})
endforeach()

if(WIN32)
    get_target_property(_qt_core_path Qt6::Core LOCATION)
    
    get_filename_component(_qt_bin_dir "${_qt_core_path}" DIRECTORY)

    set(PATH_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:${PROJECT_SHARED}>"
        "PATH=path_list_prepend:${_qt_bin_dir}"
    )

    set_tests_properties(
        ${TEST_UNITS}
            PROPERTIES ENVIRONMENT_MODIFICATION 
            "${PATH_MODIFICATION}"
    )

    message(STATUS "Tests Qt Runtime Dir: ${_qt_bin_dir}")
endif()

set(GLOG_TEST_EXECUTABLE_TARGETS "${TEST_UNITS}" PARENT_SCOPE)
