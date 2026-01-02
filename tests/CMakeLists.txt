cmake_minimum_required(VERSION 3.15)

# Find doctest
find_package(doctest CONFIG REQUIRED)

# Test sources
set(TEST_SOURCES
    test_main.cpp
    TimeUtil.tests.cpp
    Json.tests.cpp
    CommandRunner.tests.cpp
    Config.tests.cpp
    FileUtil.tests.cpp
)

# Create test executable
add_executable(lastexecuterecord.tests ${TEST_SOURCES})
target_link_libraries(lastexecuterecord.tests 
    PRIVATE 
    lastexecuterecord_lib
    doctest::doctest
)

# Register tests with CTest
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake OPTIONAL)
if(COMMAND doctest_discover_tests)
    doctest_discover_tests(lastexecuterecord.tests)
else()
    add_test(NAME AllTests COMMAND lastexecuterecord.tests)
endif()
