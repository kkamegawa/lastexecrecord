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
  - localOnly の pinning（PC名の自動埋め込み）

## JSON

- `src/lastexecuterecord/Json.h/.cpp`
  - `parseJson(text)` / `writeJson(value)`
  - 型: null/bool/int/double/string/array/object

## File I/O and locking

- `src/lastexecuterecord/FileUtil.h/.cpp`
  - `readUtf8FileToWString(path)`
  - `writeWStringToUtf8FileAtomic(path, content)`
  - `acquireLockFile(path)`
  - `getComputerNameString()`

## Time

- `src/lastexecuterecord/TimeUtil.h/.cpp`
  - `nowEpochSecondsUtc()`
  - `tryParseIsoUtcToEpochSeconds(iso, out)`
  - `formatEpochSecondsAsIsoUtc(epoch)`

## Process execution

- `src/lastexecuterecord/CommandRunner.h/.cpp`
  - `runProcess(exe, args, workingDirectory, timeoutSeconds)`
  - `quoteArgForWindowsCommandLine(arg)`

## Project files

- `src/lastexecuterecord/lastexecuterecord.vcxproj`
  - 新規ソース/ヘッダーを追加済み
