# Running tests (Visual Studio with MSTest)

## Prerequisites

- Visual Studio 2022/2025 with C++ workload
- Microsoft Unit Testing Framework for C++ (included with Visual Studio)

## Build and run

- Open `src/lastexecrecord.sln`.
- Build the solution (includes `lastexecuterecord`, `lastexecuterecord.core`, and `lastexecuterecord.mstest`).
- Open **Test Explorer** (Test → Test Explorer).
- Tests will automatically appear and can be run directly from Test Explorer.

No external dependencies (vcpkg, GoogleTest, etc.) are required.

## Project structure

- `lastexecuterecord`: Main executable (contains only `main.cpp`)
- `lastexecuterecord.core`: Static library with core functionality (TimeUtil, Json, Config, FileUtil, CommandRunner)
- `lastexecuterecord.mstest`: Native Unit Test Project using Microsoft Unit Testing Framework for C++

