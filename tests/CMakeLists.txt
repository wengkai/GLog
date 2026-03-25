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
        Qt6::Core
        Qt6::Test
        Qt6::Widgets
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

    list(APPEND TEST_UNITS ${test_name})
    
    message(STATUS "Added test unit: ${test_name}")
endmacro()

add_test_unit(hello_ctest)

add_test_unit(ctydb_test)

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
