# lastexecrecord

Lightweight Windows console application.
Every time it is invoked, it **runs the commands registered in the JSON config once, in order**, and skips commands when they should not run based on:

- **Last run time** (seconds precision)
- **Minimum interval** (seconds)

It avoids external libraries (Win32 API + STL only) and focuses on safe operation via config file locking and atomic updates.

## Quick start

1. Run `lastexecuterecord.exe` (no arguments needed)
2. The app creates a sample config at `%USERPROFILE%\.lastexecrecord\config.json`
3. Edit the config file: set `enabled: true` and customize `exe` / `args` for your commands
4. Run `lastexecuterecord.exe` again to execute your commands

By default, the app reads **`%USERPROFILE%\.lastexecrecord\config.json`**.
If this file doesn't exist, a minimal sample config is created automatically (with commands disabled by default for safety).

## Usage

- `lastexecuterecord.exe`: Run with default config (auto-creates sample if missing)
- `lastexecuterecord.exe --config <path>`: Specify a custom config JSON path
- `lastexecuterecord.exe --dry-run`: Do not execute; only show decisions
- `lastexecuterecord.exe --verbose`: Verbose logs (including skip reasons)

All options can be combined, for example: `lastexecuterecord.exe --config myconfig.json --dry-run --verbose`

### Windows Terminal Profile Setup

To register `lastexecuterecord.exe` as a Windows Terminal profile and continue running PowerShell or Command Prompt after execution, add the following profile to your Windows Terminal `settings.json`:

```json
{
  "profiles": {
    "list": [
      {
        "name": "lastexecrecord + PowerShell",
        "commandline": "cmd.exe /c lastexecuterecord.exe && pwsh.exe",
        "icon": "ms-appx:///ProfileIcons/PowerShell.png"
      },
      {
        "name": "lastexecrecord + Command Prompt",
        "commandline": "cmd.exe /c lastexecuterecord.exe && cmd.exe",
        "icon": "ms-appx:///ProfileIcons/CommandPrompt.png"
      }
    ]
  }
}
```

- `&&` executes the next shell only if `lastexecuterecord.exe` exits successfully
- Use `;` instead of `&&` if you want to continue regardless of exit code: `cmd.exe /c lastexecuterecord.exe ; pwsh.exe`

To edit `settings.json`, open Windows Terminal and press `Ctrl + ,` or click the Settings icon.

## Config schema (version 1)

Example: `lastexecuterecord.sample.json`

### Root fields

- `version` (number, optional): Default is 1
- `networkOption` (number, optional): Control execution based on network status. Default is 2
  - `0`: Execute only when internet is connected (not on metered connections)
  - `1`: Execute even on metered connections (internet connection required)
  - `2`: Always execute (ignore network status)
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

### sample(winget)

```json
{
  "version": 1,
  "defaults": {
    "minIntervalSeconds": 64800,
    "timeoutSeconds": 0
  },
  "commands": [
    {
      "name": "winget",
      "enabled": true,
      "exe": "c:\\windows\\system32\\sudo.exe",
      "args": [
        "winget",
        "upgrade",
        "--all",
        "--accept-package-agreements",
        "--silent"
      ]
    }
  ]
}
```

## Notes (security)

- The config uses `exe` + `args[]` and does not assume shell execution like `cmd.exe /c` (helps reduce injection risk).
- To prevent concurrent runs, the program acquires an exclusive `<config>.lock` file.
- **DO NOT** use environment variables or user input to construct `exe` or `args` in the config file, as this may lead to command injection vulnerabilities.

## Development

### Building

This project uses Visual Studio 2022/2025 or MSBuild.

**Visual Studio:**
```cmd
Open src\lastexecrecord.sln in Visual Studio
Build -> Build Solution
```

**MSBuild (command line):**
```cmd
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=x64
```

For other platforms:
```cmd
REM Debug build for x64
msbuild src\lastexecrecord.sln /p:Configuration=Debug /p:Platform=x64

REM Release build for ARM64
msbuild src\lastexecrecord.sln /p:Configuration=Release /p:Platform=ARM64
```

### Testing

Tests have been migrated to **Microsoft Unit Testing Framework for C++ (MSTest)**.

**Run tests in Visual Studio:**
1. Open `src\lastexecrecord.sln` in Visual Studio
2. Build the solution (Ctrl+Shift+B)
3. Open Test Explorer (Test → Test Explorer)
4. Click "Run All" to execute all tests

See `src/lastexecuterecord.mstest/` for the test project and [src/docs/MSTEST-MIGRATION.md](src/docs/MSTEST-MIGRATION.md) for migration details.

### Documentation

- [Unit Test Design (t_wada style)](docs/unit-test-design-twada.md)
- [Handover Document](docs/handover.md)
- [Implementation Map](docs/implementation-map.md)
- [Config Schema](docs/config-schema.md)
- [Security Notes](docs/security-notes.md)
