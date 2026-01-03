# Copilot Instructions for lastexecrecord

## Repository Overview

**lastexecrecord** is a lightweight Windows console application that executes commands from a JSON configuration file, tracking execution history and respecting minimum intervals between runs. The application focuses on:
- Minimal external dependencies (Win32 API + STL only)
- High performance and efficiency
- Security-conscious design
- Safe operation through file locking and atomic updates

## Technology Stack

- **Language**: C++17
- **Platform**: Windows (primary target)
- **Build Systems**: Visual Studio 2026, MSBuild
- **Package Manager**: vcpkg
- **Testing Framework**: Microsoft Unit Testing Framework for C++
- **Standard Library**: STL (prefer over third-party alternatives)
- **API**: Win32 API (primary for Windows-specific operations)

## Security Best Practices

### Command Injection Prevention
- **ALWAYS** use `exe` + `args[]` separation pattern, never shell string concatenation
- Use `CreateProcessW` directly instead of shell execution (`cmd.exe /c`)
- Validate and sanitize all file paths before use
- Consider path normalization and absolute path requirements

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
- Use Unicode APIs (`CreateProcessW`, `WriteFile`, etc.)
- Check return values from all Win32 API calls
- Use `GetLastError()` for detailed error information
- Close handles properly (RAII wrappers recommended)

## C++ Modern Practices (C++17)

### Language Features
- Use structured bindings for multiple return values
- Prefer `std::optional` over special return values
- Use `constexpr` for compile-time constants
- Use `inline` variables for header-only constants
- Prefer `if constexpr` for compile-time branching
- Use `std::filesystem` for path operations (when available)

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
1. **First Choice**: Win32 API + STL
   - Use standard library containers and algorithms
   - Use Win32 API for Windows-specific operations
   
2. **Second Choice**: vcpkg libraries with MIT license
   - Only add if functionality doesn't exist in STL/Win32
   - Must have MIT license or compatible permissive license
   - Document justification for the dependency

3. **Avoid**: Third-party libraries unless absolutely necessary
   - No external JSON libraries (we have custom implementation)
   - No boost unless critical feature needed
   - No GPL-licensed dependencies

## Testing Practices

### Test Organization
- Tests located in `tests/` directory
- One test file per source module: `Json.tests.cpp`, `Config.tests.cpp`
- Use doctest macros: `TEST_CASE`, `REQUIRE`, `CHECK`
- Follow t_wada style testing principles (see `docs/unit-test-design-twada.md`)

### Test Requirements
- Write tests for all new functionality
- Include edge cases and error conditions
- Test Windows-specific behavior when applicable
- Mock external dependencies where appropriate
- Run tests before committing: `ctest --output-on-failure`

### Test Structure
```cpp
TEST_CASE("ComponentName: descriptive test name") {
    // Arrange
    auto config = setupTestConfig();
    
    // Act
    auto result = performOperation(config);
    
    // Assert
    REQUIRE(result.isValid());
    CHECK(result.value() == expectedValue);
}
```

## Build and Development Workflow

### Building the Project

**Visual Studio 2026:**
```cmd
Open src\lastexecrecord.sln
Build -> Build Solution
```

**msbuild with vcpkg:**
```cmd
msbuild src\lastexecrecord.sln /p:Configuration=Debug /p:Platform=x64 /p:VcpkgTriplet=x64-windows
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=x64 /p:VcpkgTriplet=x64-windows
msbuild tests\lastexecrecord.tests.sln /p:Configuration=Debug /p:Platform=Arm64 /p:VcpkgTriplet=arm64-windows
msbuild tests\lastexecrecord.tests.sln /p:Configuration=Release /p:Platform=Arm64 /p:VcpkgTriplet=arm64-windows
```


### Before Committing
1. Build successfully with no warnings
2. Run all tests and verify they pass
3. Check that no memory leaks are introduced
4. Verify security implications of changes
5. Update documentation if needed

## Code Conventions

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
- Introduce unnecessary external dependencies
- Add GPL or restrictive licensed libraries
- Commit secrets or sensitive data
- Break backward compatibility without version bump
- Use platform-specific code without #ifdef guards (for future portability)

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

**Remember**: This is a security-conscious, performance-oriented Windows utility. Always think about security implications, minimize dependencies, and maintain high code quality standards.
