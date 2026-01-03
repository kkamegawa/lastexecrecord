# MSTest for C++ 移行タスクリスト（static lib 方式）

目的: GoogleTest/vcpkg 依存のテスト実行を破棄し、Visual Studio 標準の **Microsoft Unit Testing Framework for C++ (MSTest)** に移行する。

前提:
- 対象ソリューション: `src/lastexecrecord.sln`
- 本体プロジェクト: `src/lastexecuterecord/lastexecuterecord.vcxproj`
- 既存テスト: `src/lastexecuterecord.tests/`（GoogleTest）および `tests/`（CMake 経由）

---

## 1. 事前整理

- [ ] `tests/`（CMake 側）を今後どう扱うか決める（削除/凍結/別運用）
- [ ] `vcpkg.json` の `gtest` 依存を削除してよいか決める（テスト用途のみなら削除）

---

## 2. 本体コードを static lib 化

### 2.1 新規 static lib プロジェクト作成

- [ ] `src/lastexecuterecord.core/lastexecuterecord.core.vcxproj` を作成（ConfigurationType=StaticLibrary）
- [ ] `lastexecuterecord.core` に本体の core ソースを移す/追加する
  - [ ] `CommandRunner.cpp/.h`
  - [ ] `Config.cpp/.h`
  - [ ] `FileUtil.cpp/.h`
  - [ ] `Json.cpp/.h`
  - [ ] `TimeUtil.cpp/.h`
- [ ] include path と `UNICODE/_UNICODE` 定義を core 側へ移す

### 2.2 既存 exe プロジェクトを core にリンク

- [ ] `src/lastexecuterecord/lastexecuterecord.vcxproj` から core ソースを除外する
- [ ] `lastexecuterecord.vcxproj` に `main.cpp` のみ残す
- [ ] `lastexecuterecord.vcxproj` で `lastexecuterecord.core` を project reference し、リンクする

### 2.3 ビルド確認

- [ ] `Debug|x64` で `lastexecuterecord.exe` がビルドできる
- [ ] 既存の動作（実行・config 読込）が壊れていない

---

## 3. MSTest (C++) テストプロジェクト追加

### 3.1 MSTest プロジェクト作成

- [ ] `src/lastexecuterecord.mstest/lastexecuterecord.mstest.vcxproj` を作成（Native Unit Test Project）
- [ ] `src/lastexecrecord.sln` にテストプロジェクトを追加

### 3.2 参照設定

- [ ] `lastexecuterecord.mstest` から `lastexecuterecord.core` を参照
  - [ ] Project Reference を追加
  - [ ] include directories を設定

### 3.3 Test Explorer 確認

- [ ] Visual Studio の Test Explorer に `lastexecuterecord.mstest` のテストが表示される

---

## 4. テスト実装（docs/unit-test-design-twada.md 準拠）

> 方針: 1テスト1理由 / AAA / 例で仕様を固定

### 4.1 共通ユーティリティ

- [ ] `TestUtil.h/.cpp` を追加
  - [ ] テンポラリファイルパス生成（衝突回避）
  - [ ] 片付け用 RAII（TempFile など）

### 4.2 TimeUtil

- [ ] `TimeUtilTests.cpp`
  - [ ] `tryParseIsoUtcToEpochSeconds` の正常系/異常系
  - [ ] `formatEpochSecondsAsIsoUtc` の固定値と round-trip

### 4.3 Json

- [ ] `JsonTests.cpp`
  - [ ] parse（null/bool/int/double/string/array/object/trailing）
  - [ ] write（escape、整形の基本形）

### 4.4 CommandRunner

- [ ] `CommandRunnerTests.cpp`
  - [ ] `quoteArgForWindowsCommandLine` の代表ケース

### 4.5 Config（テンポラリファイル）

- [ ] `ConfigTests.cpp`
  - [ ] minimal valid
  - [ ] required 欠落で例外
  - [ ] defaults 適用
  - [ ] lastRunUtc/lastExitCode optional
  - [ ] `applyCommandsToJson` 更新

### 4.6 FileUtil（テンポラリファイル）

- [ ] `FileUtilTests.cpp`
  - [ ] UTF-8 BOMなし
  - [ ] 置換（atomic-like）
  - [ ] lock（二重取得不可）

---

## 5. GoogleTest / vcpkg 依存の撤去

- [ ] `src/lastexecuterecord.tests/`（GoogleTest）をソリューションから除外 or 削除
- [ ] `README.vcpkg-tests.md` を更新・削除（vcpkg が不要なら削除、必要なら用途を明確化）
- [ ] `vcpkg.json` から `gtest` を削除（テスト用途のみなら）
- [ ] `tests/`（CMake テスト）を凍結する場合は README に明記

---

## 6. CI/運用（任意）

- [ ] `msbuild` + `vstest.console` でテスト実行できるようにする
- [ ] GitHub Actions/Azure Pipelines などに test を組み込む

---

## 7. 完了条件

- [ ] `src/lastexecrecord.sln` を開くだけで Test Explorer にテストが表示される
- [ ] vcpkg/gtest なしでテストがビルド・実行できる
- [ ] 設計書（`docs/unit-test-design-twada.md`）の主要ケースがテストでカバーされている
