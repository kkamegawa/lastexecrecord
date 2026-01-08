# Config JSON schema (v1)

This project **reads a configuration JSON**, **executes commands sequentially once per invocation**,
and skips execution based on **last execution time (seconds precision)** and **minimum execution interval**.

> Note: JSON comments are not supported.

## Root

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `version` | number | no | 1 | Reserved for future use |
| `networkOption` | number | no | 2 | Network-based execution control (0: connected only, 1: metered OK, 2: always execute) |
| `defaults.minIntervalSeconds` | number | no | 0 | Default minimum interval for commands |
| `defaults.timeoutSeconds` | number | no | 0 | Default timeout for commands (0 means unlimited) |
| `commands` | array | yes | - | Commands to execute in order from top to bottom |

## Command

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `name` | string | yes | - | Display name/identifier (`id` is also accepted as an alternative to name) |
| `enabled` | bool | no | true | Always skip if false |
| `exe` | string | yes | - | Executable file path (not a shell command) |
| `args` | array of string | no | [] | Arguments |
| `workingDirectory` | string | no | "" | Working directory |
| `minIntervalSeconds` | number | no | `defaults.minIntervalSeconds` | Used for skip decision |
| `timeoutSeconds` | number | no | `defaults.timeoutSeconds` | 0 means unlimited |
| `lastRunUtc` | string | no | - | `YYYY-MM-DDTHH:MM:SSZ` (UTC, seconds precision) |
| `lastExitCode` | number | no | - | Previous exit code |

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
