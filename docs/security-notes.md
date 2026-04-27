# Security notes

This tool "controls process execution via configuration JSON", so protecting the configuration file directly impacts security.

## 1. Command injection prevention

- Uses `exe` and `args[]` separation with `CreateProcessW`.
- Requires `exe` to be an absolute path to an existing file before execution.
- Requires `workingDirectory`, when set, to be an absolute path to an existing directory.
- Does not use shell execution methods like `cmd.exe /c` (however, if `cmd.exe` is specified in `exe`, it can be executed, so additional policies are needed to prohibit it).

## 2. Concurrent execution and corruption prevention

- Prevents multiple simultaneous runs by exclusively opening `<config>.lock`.
- The config path is normalized before deriving `<config>.lock`, reducing lock bypass via path aliases.
- Config updates are atomic: write to a unique same-directory temporary file → replace with `MoveFileEx(REPLACE_EXISTING|WRITE_THROUGH)`.
- Lock acquisition is bounded by `--lock-timeout` (default: 300 seconds).

## 3. Recommended practices

- Place config in a trusted location (e.g., user-only directory).
- Be especially careful when running with administrator privileges (config tampering = arbitrary code execution).
- If possible, restrict config ACL to the user only.

## 4. Future enhancements

- Allow-list to prohibit/restrict `cmd.exe`, `powershell.exe`, etc.
- Signature/hash verification for `exe` (depending on requirements).

## 5. Network status check

- Execution control via `networkOption` uses Win32 API (INetworkListManager).
- Network status check is performed once at startup and uses COM.
- COM initialization is done within functions and does not re-initialize if already initialized.
- No administrator privileges required to access network information (works with standard user).
- Network-gated modes fail closed: if network status cannot be verified for `ExecuteWhenConnected` or `ExecuteOnMetered`, commands are skipped and verbose mode reports the error code.

## 6. Installer detection

- Uses `CreateToolhelp32Snapshot` and process enumeration APIs to detect running installer processes.
- No administrator privileges required to enumerate processes (works with standard user).
- Process name comparison is case-insensitive for reliability.
- Detection is limited to common installer process names (`msiexec.exe`, `setup.exe`, etc.).
- Failed snapshot creation is represented as `CheckFailed`; verbose mode logs a warning and execution proceeds to avoid blocking on an unverifiable installer state.
- The handle to the snapshot is explicitly closed using `CloseHandle` to avoid resource leaks.
