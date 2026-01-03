# Source Directory Structure

This directory contains the source code for the project. It is organized as follows:

- `lastexecuterecord`: Visual Studio 2022/2025 project for the `lastexecuterecord` console application.
- `lastexecuterecord.core`: Static library containing core functionality (TimeUtil, Json, Config, FileUtil, CommandRunner).
- `lastexecuterecord.mstest`: Native Unit Test Project using Microsoft Unit Testing Framework for C++.

The `lastexecuterecord` project includes:

- A Visual Studio 2022/2025 project file (`lastexecuterecord.vcxproj`) configured for Unicode.
- A filters file (`lastexecuterecord.vcxproj.filters`) for organizing the project files.
- A `main.cpp` entry point that links to the core library.

The application reads a JSON config and, on each invocation, executes registered commands sequentially.
It records the last execution time (seconds precision) back into the config and enforces a minimum interval to skip runs.

This structure is designed to facilitate easy navigation and understanding of the project's components and configuration.

