cmake_minimum_required(VERSION 3.15)

# Find GoogleTest
find_package(GTest CONFIG REQUIRED)

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
    GTest::gtest
    GTest::gmock
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(lastexecuterecord.tests)
