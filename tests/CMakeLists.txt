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
    set_tests_properties(
        HelloCTest 
            PROPERTIES ENVIRONMENT 
            "PATH=$<TARGET_FILE_DIR:${PROJECT_SHARED}>;$ENV{PATH}"
    )
endif()
