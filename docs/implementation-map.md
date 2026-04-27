# Implementation map

どこに何があるか、メンテ/追加実装時の入口をまとめます。

## Entry point

- `src/lastexecuterecord/main.cpp`
  - `wmain` 実装
  - `--config`, `--dry-run`, `--verbose`
  - コマンドの順次実行、スキップ判定、config の更新

## Config

- `src/lastexecuterecord/Config.h/.cpp`
  - `loadAndValidateConfig(path)`
  - `applyCommandsToJson(cfg)`

## JSON

- `src/lastexecuterecord/Json.h/.cpp`
  - `parseJson(text)` / `writeJson(value)`
  - 型: null/bool/int/double/string/array/object

## File I/O and locking

- `src/lastexecuterecord/FileUtil.h/.cpp`
  - `readUtf8FileToWString(path)`
  - `writeWStringToUtf8FileAtomic(path, content)`
  - `acquireLockFile(path)`
  - path helpers: `getFullPath(path)`, `pathIsAbsolute(path)`, `fileExists(path)`, `directoryExists(path)`

## Time

- `src/lastexecuterecord/TimeUtil.h/.cpp`
  - `nowEpochSecondsUtc()`
  - `tryParseIsoUtcToEpochSeconds(iso, out)`
  - `formatEpochSecondsAsIsoUtc(epoch)`

## Process execution

- `src/lastexecuterecord/CommandRunner.h/.cpp`
  - `runProcess(exe, args, workingDirectory, timeoutSeconds)`
  - `quoteArgForWindowsCommandLine(arg)`

## Network checking

- `src/lastexecuterecord/NetworkUtil.h/.cpp`
  - `hasInternetConnection()`
  - `isConnectionMetered()`
  - `shouldExecuteBasedOnNetwork(option)`
  - `NetworkOption` enum: ExecuteWhenConnected(0), ExecuteOnMetered(1), AlwaysExecute(2)

## Installer detection

  - `src/lastexecuterecord/InstallerUtil.h/.cpp`
  - `checkInstallerRunning()` - Returns Running/NotRunning/CheckFailed status
  - `isInstallerRunning()` - Checks if installer processes (msiexec.exe, setup.exe, etc.) are running
  - `waitForInstallerToFinish(waitSeconds, maxRetries)` - Waits for installer to finish with retries
  - Uses `CreateToolhelp32Snapshot` and process enumeration APIs
  - `InstallerWaitBehavior` enum: Wait(0), Skip(1)
  - Integrated into main.cpp before command execution

## Project files

- `src/lastexecuterecord/lastexecuterecord.vcxproj`
  - 新規ソース/ヘッダーを追加済み
