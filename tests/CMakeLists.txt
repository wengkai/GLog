find_package(Qt6 6.5
    COMPONENTS
        Core
        Test
REQUIRED)

set(HELLO_CTEST "HelloCTest")

qt_add_executable(${HELLO_CTEST} hello_ctest.cpp)

target_link_libraries(${HELLO_CTEST} PRIVATE 
    ${PROJECT_SHARED}
    Qt6::Core
    Qt6::Test
)

add_test(NAME ${HELLO_CTEST} COMMAND ${HELLO_CTEST})

if(WIN32)
    get_target_property(_qt_core_path Qt6::Core LOCATION)
    
    get_filename_component(_qt_bin_dir "${_qt_core_path}" DIRECTORY)

    set_tests_properties(
        HelloCTest 
            PROPERTIES ENVIRONMENT 
            "PATH=$<TARGET_FILE_DIR:${PROJECT_SHARED}>;${_qt_bin_dir};$ENV{PATH}"
    )

    message(STATUS "Test Runtime Qt Dir: ${_qt_bin_dir}")
endif()
