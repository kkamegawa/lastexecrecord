# Copilot Instructions for lastexecrecord

## Repository Overview

**lastexecrecord** is a lightweight Windows console application that executes commands from a JSON configuration file, tracking execution history and respecting minimum intervals between runs. The application focuses on:
- Minimal external dependencies (Win32 API + STL only)
- High performance and efficiency
- Security-conscious design
- Safe operation through file locking and atomic updates

## Technology Stack

- **Language**: C++17 or later
- **Platform**: Windows (Windows 11 24H2+, Windows Server 2019+)
- **Build Systems**: Visual Studio 2022 or 2026, MSBuild
- **Package Manager**: vcpkg (for build tooling only)
- **Testing Framework**: Microsoft Unit Testing Framework for C++
- **Standard Library**: STL (prefer over third-party alternatives)
- **API**: Win32 API (primary for Windows-specific operations)
- **Output**: Single executable file (.exe) only

## Security Best Practices

**Security is the highest priority.** All design and implementation decisions must consider security implications first.

### Command Injection Prevention
- **ALWAYS** use `exe` + `args[]` separation pattern, never shell string concatenation
- Use `CreateProcessW` directly instead of shell execution (`cmd.exe /c`)
- Validate and sanitize all file paths before use
- Consider path normalization and absolute path requirements
- Never trust external input - validate everything

### File Operations Security
- Use exclusive file locking (`<config>.lock`) to prevent concurrent execution
- Implement atomic file updates: write to `.tmp` → `MoveFileEx(REPLACE_EXISTING|WRITE_THROUGH)`
- Protect configuration files with appropriate ACLs in production
- Never store secrets or credentials in configuration files

### Input Validation
- Validate JSON configuration thoroughly before execution
- Check all required fields and enforce schema constraints
- Sanitize command-line arguments and paths
- Implement bounds checking for all array/vector access

### Memory Safety
- Use RAII patterns consistently for resource management
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
- Avoid manual memory management when possible
- Use `std::vector` and `std::string` for dynamic allocations

## Performance Guidelines

### Efficiency Principles
- Minimize file I/O operations
- Use move semantics where appropriate (`std::move`)
- Prefer `std::wstring_view` for read-only string operations
- Avoid unnecessary copies (use references and const references)
- Reserve capacity for vectors when size is known (`reserve()`)

### Windows API Best Practices
- Use Unicode APIs (`CreateProcessW`, `WriteFile`, etc.) - **never use ANSI variants**
- Check return values from **all** Win32 API calls
- Use `GetLastError()` for detailed error information
- Close handles properly (RAII wrappers recommended)
- Follow Microsoft Learn best practices for Windows C++ development
- Target Windows 11 24H2+ and Windows Server 2019+ APIs
- Reference Microsoft Learn documentation for Windows C++ best practices:
  - Modern C++ practices for Windows
  - Security best practices for Win32 applications
  - Memory management and RAII patterns
  - Error handling strategies

## C++ Modern Practices (C++17 and Later)

### Language Features
- Use C++17 as minimum standard, prefer newer features when available
- Use structured bindings for multiple return values
- Prefer `std::optional` over special return values
- Use `constexpr` for compile-time constants
- Use `inline` variables for header-only constants
- Prefer `if constexpr` for compile-time branching
- Use `std::filesystem` for path operations (when appropriate)

### Code Style
- Use `#pragma once` for header guards
- Namespace: `ler` (short for lastexecrecord)
- Use `camelCase` for function names: `loadAndValidateConfig`
- Use `camelCase` for variable names: `configPath`, `dryRun`
- Use `PascalCase` for types: `CommandConfig`, `JsonValue`
- Use descriptive names; avoid abbreviations unless standard
- Keep line length reasonable (prefer under 100 characters)

### Type Safety
- Use `std::int64_t`, `std::uint32_t` for fixed-width integers
- Prefer `enum class` over plain `enum`
- Use `static_cast` for explicit conversions
- Avoid C-style casts
- Use `nullptr` instead of `NULL`

## Dependency Management

### Library Preferences
1. **First Choice**: Win32 API + STL only
   - Use standard library containers and algorithms
   - Use Win32 API for Windows-specific operations
   - **NO third-party libraries** - implement functionality using only STL and Win32 API
   
2. **Strictly Avoid**: Any third-party libraries
   - No external JSON libraries (we have custom implementation)
   - No boost
   - No vcpkg dependencies beyond build tooling
   - The project must produce a single .exe file with no external dependencies

## Testing Practices

### Test Organization
- Tests located in `src/lastexecuterecord.mstest/` directory
- One test file per source module: `JsonTests.cpp`, `ConfigTests.cpp`
- Use Microsoft Unit Testing Framework for C++ macros: `TEST_CLASS`, `TEST_METHOD`, `Assert::*`
- Follow t_wada style testing principles (see `docs/unit-test-design-twada.md`)

### Test Requirements
- **Write tests for all new functionality** - unit tests are mandatory for new features
- Include edge cases and error conditions
- Test Windows-specific behavior when applicable
- Mock external dependencies where appropriate
- Run tests before committing using Visual Studio Test Explorer or `vstest.console.exe`

### Test Structure
```cpp
TEST_CLASS(ComponentNameTests)
{
public:
    TEST_METHOD(DescriptiveTestName)
    {
        // Arrange
        auto config = setupTestConfig();
        
        // Act
        auto result = performOperation(config);
        
        // Assert
        Assert::IsTrue(result.isValid());
        Assert::AreEqual(expectedValue, result.value());
    }
};
```

## Build and Development Workflow

### Building the Project

**The project must produce a single .exe file** with no external runtime dependencies.

**Visual Studio 2022 or 2026:**
```cmd
Open src\lastexecrecord.sln
Build -> Build Solution
```

**MSBuild (command line):**
```cmd
msbuild src\lastexecrecord.sln /p:Configuration=Debug /p:Platform=x64
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=x64
```

**MSBuild with vcpkg (for build tooling):**
```cmd
msbuild src\lastexecrecord.sln /p:Configuration=Debug /p:Platform=x64 /p:VcpkgTriplet=x64-windows
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=x64 /p:VcpkgTriplet=x64-windows
msbuild src\lastexecrecord.sln /p:Configuration=Debug /p:Platform=ARM64 /p:VcpkgTriplet=arm64-windows
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=ARM64 /p:VcpkgTriplet=arm64-windows
```

### Testing

Run tests in Visual Studio Test Explorer:
1. Open `src\lastexecrecord.sln` in Visual Studio
2. Build the solution (Ctrl+Shift+B)
3. Open Test Explorer (Test → Test Explorer)
4. Click "Run All" to execute all tests


### Before Committing
1. Build successfully with **no warnings** (treat warnings as errors)
2. Run all tests and verify they pass
3. Check that no memory leaks are introduced
4. **Verify security implications** of all changes
5. Update documentation if needed
6. Ensure the output is still a single .exe file

## Code Conventions

### Code Maintenance Priority
- Write maintainable, readable code - clarity over cleverness
- Follow consistent naming conventions throughout the codebase
- Keep functions focused and single-purpose
- Document complex logic and non-obvious decisions
- Refactor duplication when safe to do so

### Error Handling
- Use exceptions for exceptional conditions
- Use return values for expected failures
- Provide descriptive error messages
- Log errors with context information

### Comments
- Write self-documenting code; minimize comments
- Comment "why" not "what" when needed
- Use `//` for single-line comments
- Use `/* */` for multi-line explanations
- Document complex algorithms or non-obvious logic
- Add English comments for international collaboration

### File Organization
- Header files (`.h`): declarations, inline functions
- Source files (`.cpp`): implementations
- One class/major component per file pair
- Group related functions together
- Separate platform-specific code clearly

## Documentation

### Required Documentation
- Update `README.md` for user-facing changes
- Update relevant docs in `docs/` folder:
  - `config-schema.md`: JSON schema changes
  - `security-notes.md`: Security implications
  - `implementation-map.md`: Architecture changes
  - `handover.md`: Design decision rationale

### Code Documentation
- Document public APIs with clear descriptions
- Explain parameter meanings and return values
- Note any preconditions or side effects
- Document thread-safety considerations

## Configuration File (JSON)

### Schema Principles
- Maintain backward compatibility when possible
- Use version field for schema evolution
- Validate all fields before use
- Provide clear error messages for invalid configs
- Preserve order for arrays and objects
- Use ISO 8601 format for timestamps (UTC)

### Field Naming
- Use `camelCase` for JSON fields
- Be descriptive: `minIntervalSeconds`, `workingDirectory`
- Boolean fields: use positive names (`enabled` not `disabled`)
- Time fields: include unit in name (`timeoutSeconds`)

## Windows-Specific Considerations

### Unicode Handling
- Use wide characters (`wchar_t`, `std::wstring`)
- Define `UNICODE` and `_UNICODE`
- Use Unicode Win32 API variants (`CreateProcessW`, not `CreateProcessA`)
- Use `wmain` instead of `main` for entry point

### Process Creation
- Use `CreateProcessW` for process execution
- Pass command line as separate executable and arguments
- Set appropriate process creation flags
- Handle process termination and exit codes properly
- Implement timeout mechanism for long-running processes

### File System
- Use exclusive locks to prevent concurrent access
- Implement atomic file updates for safety
- Handle file system errors gracefully
- Use appropriate sharing modes for file access

## Common Patterns in This Codebase

### JSON Handling
```cpp
// Parse JSON
auto root = ler::parseJson(jsonString);

// Access fields
auto& obj = root.o;
for (const auto& [key, value] : obj) {
    // Process key-value pairs
}
```

### Config Loading
```cpp
// Load and validate config
auto config = ler::loadAndValidateConfig(configPath);

// Update and save
ler::applyCommandsToJson(config);
ler::writeConfigAtomic(configPath, config);
```

### Error Reporting
```cpp
if (!isValid) {
    std::wcerr << L"Error: " << detailedMessage << L"\n";
    return errorCode;
}
```

## Prohibited Practices

### Do NOT:
- Use shell execution for commands (`cmd.exe /c`, `system()`)
- Ignore Win32 API return values
- Use raw pointers for ownership
- **Introduce ANY external dependencies** - the project must remain dependency-free (STL + Win32 API only)
- Add GPL or restrictive licensed libraries
- Commit secrets or sensitive data
- Break backward compatibility without version bump
- Use platform-specific code without #ifdef guards (for future portability)
- Produce anything other than a single .exe file
- Compromise security for convenience

## Future Considerations

### Potential Enhancements
- Support for state.json separation (read-only config)
- Enhanced security: signature/hash verification for executables
- Allow-list for permitted executables
- Cross-platform support (Linux/macOS with stubs)
- Enhanced logging and monitoring
- Config file encryption support

### Maintaining Compatibility
- Version field in JSON for schema evolution
- Deprecation warnings before removing features
- Migration tools for breaking changes
- Clear changelog documentation

---

**Remember**: 
1. **Security is the highest priority** - always consider security implications first
2. This is a **single .exe** Windows utility with **zero external dependencies**
3. Use only **STL and Win32 API** - no third-party libraries allowed
4. Write **unit tests for all new features** using Microsoft Unit Testing Framework for C++
5. Build with **MSBuild** for Visual Studio 2022 or 2026
6. Target **Windows 11 24H2+** and **Windows Server 2019+**
7. **Code maintenance** is critical - write clear, maintainable code
