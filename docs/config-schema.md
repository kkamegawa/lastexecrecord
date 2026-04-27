# Config JSON schema (v1)

This project **reads a configuration JSON**, **executes commands sequentially once per invocation**,
and skips execution based on **last execution time (seconds precision)** and **minimum execution interval**.

> Note: JSON comments are not supported. Duplicate object keys, unescaped control
> characters in strings, non-JSON whitespace, and fractional values for integer
> fields are rejected.

## Root

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `version` | integer | no | 1 | Reserved for future use |
| `networkOption` | integer | no | 2 | Network-based execution control (0: connected only, 1: metered OK, 2: always execute) |
| `defaults.minIntervalSeconds` | integer | no | 0 | Default minimum interval for commands |
| `defaults.timeoutSeconds` | integer | no | 0 | Default timeout for commands (0 means unlimited) |
| `defaults.installerWaitBehavior` | string or number | no | "wait" | Default behavior when installer is running ("wait" or "skip", or 0/1) |
| `defaults.installerWaitSeconds` | integer | no | 30 | Default wait time in seconds between installer checks |
| `defaults.installerMaxRetries` | integer | no | 10 | Default maximum number of retries when waiting for installer |
| `commands` | array | yes | - | Commands to execute in order from top to bottom |

## Command

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `name` | string | yes | - | Display name/identifier (`id` is also accepted as an alternative to name) |
| `enabled` | bool | no | true | Always skip if false |
| `exe` | string | yes | - | Absolute path to an existing executable file (not a shell command) |
| `args` | array of string | no | [] | Arguments |
| `workingDirectory` | string | no | "" | Absolute path to an existing directory when set |
| `minIntervalSeconds` | integer | no | `defaults.minIntervalSeconds` | Used for skip decision |
| `timeoutSeconds` | integer | no | `defaults.timeoutSeconds` | 0 means unlimited |
| `installerWaitBehavior` | string or number | no | `defaults.installerWaitBehavior` | Behavior when installer is running ("wait" or "skip", or 0/1) |
| `installerWaitSeconds` | integer | no | `defaults.installerWaitSeconds` | Wait time in seconds between installer checks |
| `installerMaxRetries` | integer | no | `defaults.installerMaxRetries` | Maximum number of retries when waiting for installer |
| `lastRunUtc` | string | no | - | `YYYY-MM-DDTHH:MM:SSZ` (UTC, seconds precision) |
| `lastExitCode` | integer | no | - | Previous exit code |

## Time format

- `lastRunUtc` only supports `YYYY-MM-DDTHH:MM:SSZ` format
  - Example: `2026-01-02T12:34:56Z`

## Network option

- `networkOption` controls execution based on network status
  - `0`: Execute only when internet is connected (not on metered connections)
  - `1`: Execute even on metered connections (internet connection required)
  - `2`: Always execute (regardless of network status) - Default
- Network check is performed once at startup, and all commands are skipped if the condition is not met

## Skip logic

- If `lastRunUtc` exists and parses successfully
  - Skip if `now - lastRun < minIntervalSeconds`
- If `lastRunUtc` is corrupted
  - Issue a warning and treat as "not executed" (= eligible for execution)

## Installer detection and wait behavior

The application can detect when Windows installer processes (such as `msiexec.exe`) are running and either wait for them to complete or skip execution:

- `installerWaitBehavior` controls the behavior:
  - `"wait"` (or `0`): Wait for the installer to finish before executing the command (default)
  - `"skip"` (or `1`): Skip the command if an installer is running
- When waiting (`installerWaitBehavior` is `"wait"`):
  - The application checks if an installer is running before executing each command
  - If an installer is detected, the application waits for `installerWaitSeconds` seconds and then checks again
  - This process repeats up to `installerMaxRetries` times
  - If the installer is still running after all retries, the command is skipped
  - Default: wait 30 seconds, retry up to 10 times (total wait: up to 300 seconds)
- When skipping (`installerWaitBehavior` is `"skip"`):
  - The command is immediately skipped if an installer is detected

**Detected installer processes:**
- `msiexec.exe` (Windows Installer)
- `setup.exe`
- `install.exe`
- `setupapi.exe`
- `msiinstall.exe`
- `windows installer.exe`

These settings can be configured globally in the `defaults` section or per-command.
