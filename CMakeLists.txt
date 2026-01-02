cmake_minimum_required(VERSION 3.15)
project(lastexecuterecord VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/lastexecuterecord/CommandRunner.cpp
    src/lastexecuterecord/Config.cpp
    src/lastexecuterecord/FileUtil.cpp
    src/lastexecuterecord/Json.cpp
    src/lastexecuterecord/TimeUtil.cpp
)

set(HEADERS
    src/lastexecuterecord/CommandRunner.h
    src/lastexecuterecord/Config.h
    src/lastexecuterecord/FileUtil.h
    src/lastexecuterecord/Json.h
    src/lastexecuterecord/TimeUtil.h
)

# Create a library for shared code between main executable and tests
add_library(lastexecuterecord_lib ${SOURCES} ${HEADERS})
target_include_directories(lastexecuterecord_lib PUBLIC src/lastexecuterecord)

# Main executable
add_executable(lastexecuterecord src/lastexecuterecord/main.cpp)
target_link_libraries(lastexecuterecord PRIVATE lastexecuterecord_lib)

# Platform-specific settings
if(WIN32)
    # Windows-specific libraries
    target_compile_definitions(lastexecuterecord_lib PUBLIC UNICODE _UNICODE)
else()
    # For non-Windows platforms (testing/development on Linux/Mac)
    # We'll need to stub out Windows-specific functionality
    target_compile_definitions(lastexecuterecord_lib PUBLIC LASTEXEC_STUB_WINDOWS)
endif()

# Enable testing
enable_testing()
add_subdirectory(tests)
