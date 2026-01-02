# lastexecrecord

Lightweight Windows console application.
Every time it is invoked, it **runs the commands registered in the JSON config once, in order**, and skips commands when they should not run based on:

- **Last run time** (seconds precision)
- **Minimum interval** (seconds)

It avoids external libraries (Win32 API + STL only) and focuses on safe operation via config file locking and atomic updates.

## Quick start

1. Copy `lastexecuterecord.sample.json` to `lastexecuterecord.json`
2. Edit `exe` / `args` to match your commands
3. Run `lastexecuterecord.exe`

By default, the app reads **`<exe>.json`** (for example, `lastexecuterecord.json`).

## Usage

- `lastexecuterecord.exe --config <path>`: Specify the config JSON
- `lastexecuterecord.exe --dry-run`: Do not execute; only show decisions
- `lastexecuterecord.exe --verbose`: Verbose logs (including skip reasons)

## Config schema (version 1)

Example: `lastexecuterecord.sample.json`

### Root fields

- `version` (number, optional): Default is 1
- `defaults.minIntervalSeconds` (number, optional): Default minimum interval for commands
- `defaults.timeoutSeconds` (number, optional): Default timeout for commands
- `commands` (array, required): List of commands to run (processed from top to bottom)

### Command fields

- `name` (string, required): Display name / identifier
- `enabled` (bool, optional): Default is true
- `exe` (string, required): Executable path (**not a shell string; use exe + args**)
- `args` (array of string, optional): Arguments
- `workingDirectory` (string, optional)
- `minIntervalSeconds` (number, optional): Defaults to `defaults.minIntervalSeconds`
- `timeoutSeconds` (number, optional): Defaults to `defaults.timeoutSeconds`
- `lastRunUtc` (string, optional): Example `2026-01-02T12:34:56Z` (seconds precision)
- `lastExitCode` (number, optional): Previous exit code

## Notes (security)

- The config uses `exe` + `args[]` and does not assume shell execution like `cmd.exe /c` (helps reduce injection risk).
- To prevent concurrent runs, the program acquires an exclusive `<config>.lock` file.

## Development

### Building

This project uses Visual Studio 2022 and can also be built with CMake.

**Visual Studio:**
```cmd
Open src\lastexecrecord.sln in Visual Studio
Build -> Build Solution
```

**CMake (with vcpkg):**
```cmd
vcpkg install
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

### Testing

Unit tests are located in the `tests/` directory and use the doctest framework.

**Run tests with CMake:**
```cmd
cd build
ctest --output-on-failure
```

**Run tests directly:**
```cmd
build\tests\Debug\lastexecuterecord.tests.exe
```

See [tests/README.md](tests/README.md) for more details.

### Documentation

- [Unit Test Design (t_wada style)](docs/unit-test-design-twada.md)
- [Handover Document](docs/handover.md)
- [Implementation Map](docs/implementation-map.md)
- [Config Schema](docs/config-schema.md)
- [Security Notes](docs/security-notes.md)
