# Security notes

This tool "controls process execution via configuration JSON", so protecting the configuration file directly impacts security.

## 1. Command injection prevention

- Uses `exe` and `args[]` separation with `CreateProcessW`.
- Does not use shell execution methods like `cmd.exe /c` (however, if `cmd.exe` is specified in `exe`, it can be executed, so additional policies are needed to prohibit it).

## 2. Concurrent execution and corruption prevention

- Prevents multiple simultaneous runs by exclusively opening `<config>.lock`.
- Config updates are atomic: write to `.tmp` → replace with `MoveFileEx(REPLACE_EXISTING|WRITE_THROUGH)`.

## 3. Recommended practices

- Place config in a trusted location (e.g., user-only directory).
- Be especially careful when running with administrator privileges (config tampering = arbitrary code execution).
- If possible, restrict config ACL to the user only.

## 4. Future enhancements

- Enforce absolute path and normalization for `exe`.
- Allow-list to prohibit/restrict `cmd.exe`, `powershell.exe`, etc.
- Signature/hash verification for `exe` (depending on requirements).

## 5. Network status check

- Execution control via `networkOption` uses Win32 API (INetworkListManager).
- Network status check is performed once at startup and uses COM.
- COM initialization is done within functions and does not re-initialize if already initialized.
- No administrator privileges required to access network information (works with standard user).
