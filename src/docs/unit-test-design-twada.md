# Unit Test design (t_wada style)

対象: `lastexecuterecord`（Windows コンソール）

このドキュメントは、t_wada スタイル（テスト駆動開発のプラクティスに沿った「仕様を例で固定する」「観測可能な振る舞いを小さく分けてテストする」「境界は Fake/Stub で置き換える」）で、ユニットテストを設計するための方針とテスト一覧をまとめたものです。

> 注意: ここでは「設計」を行います。テスト実装・テスト用プロジェクト追加は別タスクとします。

---

## 1. ゴール

- 重要な仕様（スキップ判定、JSON 取扱い、時刻フォーマット、コマンドライン引用、原子的書き込み等）を **テストで固定**する。
- Win32 の副作用（`CreateProcessW`, ファイル I/O, ロック）を **境界として分離**し、ドメインロジックを **純粋関数/小さな関数**としてテスト可能にする。
- 失敗時のメッセージや戻り値など、ユーザーにとって意味のある **観測可能な振る舞い**を重視する。

## 2. テスト方針（t_wada スタイル）

### 2.1 テスト分類

- **Small / Unit**
  - 依存をすべて Fake/Stub に置き換えて、メモリ上で完結する
  - 例: JSON パーサ、時刻の parse/format、コマンドライン quoting
- **Medium / Component**
  - ファイル操作など一部の実リソースを使う（テンポラリディレクトリ）
  - 例: `writeWStringToUtf8FileAtomic` の原子性、ロック取得
- **Large / E2E**（このプロジェクトでは最低限）
  - 実行ファイル起動まで含めた統合（CI で時間が掛かる）

本ドキュメントは主に **Small/Unit** を設計対象とし、必要最小限の Medium を追加します。

### 2.2 命名規約

- `Class_Method_Scenario_Expected`
- あるいは関数テストで `Function_Scenario_Expected`

例:
- `TimeUtil_TryParseIsoUtcToEpochSeconds_ValidZ_ReturnsTrueAndEpoch`
- `CommandRunner_QuoteArg_Space_AddsQuotes`

### 2.3 AAA（Arrange-Act-Assert）

- Arrange: 入力/前提を作る
- Act: 対象を1回呼ぶ
- Assert: 期待結果を1つの観点で確認（複数観点はテストを分ける）

### 2.4 1テスト1理由

- 1テストで落ちる理由を1つに絞る
- 複数の仕様を混ぜない

### 2.5 境界の扱い（Fake/Stub/Spy）

将来の実装作業で、以下の境界をインターフェイス化/関数ポインタ化して差し替え可能にする。

- 時計: `nowEpochSecondsUtc()`
- プロセス実行: `runProcess()`
- ファイル I/O: `readUtf8FileToWString`, `writeWStringToUtf8FileAtomic`
- ロック: `acquireLockFile`

ただし現時点は設計のみ。

---

## 3. テスト対象の分割（テストしやすい単位）

### 3.1 Pure-ish なユーティリティ（最優先で Unit テスト）

- `TimeUtil`（純粋に近い）
- `Json`（パース/シリアライズ）
- `CommandRunner::quoteArgForWindowsCommandLine`（純粋）
- `Config::loadAndValidateConfig`（I/O を含むので、将来的に `parseAndValidateConfig(JsonValue root)` に分割すると Unit 化しやすい）

### 3.2 `main` のロジック

`wmain` の中にある「スキップ判定＋実行結果の記録」の核は、将来的に

- `EvaluateAndRunCommands(cfg, now, runner, dryRun, verbose)`

のように関数へ抽出すると Unit テスト可能。

---

## 4. テストケース一覧

### 4.1 `TimeUtil` テスト

#### `tryParseIsoUtcToEpochSeconds`

1. `TimeUtil_TryParseIso_ValidZ_ReturnsTrue`
   - Arrange: `2026-01-02T12:34:56Z`
   - Act: parse
   - Assert: `true`

2. `TimeUtil_TryParseIso_ValidNoZ_TreatedAsUtc_ReturnsTrue`
   - Arrange: `2026-01-02T12:34:56`
   - Assert: `true`

3. `TimeUtil_TryParseIso_InvalidLength_ReturnsFalse`
   - Arrange: `2026-01-02T12:34`
   - Assert: `false`

4. `TimeUtil_TryParseIso_InvalidSeparator_ReturnsFalse`
   - Arrange: `2026/01/02T12:34:56Z`
   - Assert: `false`

5. `TimeUtil_TryParseIso_FractionalSeconds_ReturnsFalse`
   - Arrange: `2026-01-02T12:34:56.123Z`
   - Assert: `false`

#### `formatEpochSecondsAsIsoUtc`

6. `TimeUtil_FormatEpoch_Zero_Returns1970...Z`
   - epoch=0 の期待文字列（UTC）を固定

7. `TimeUtil_FormatAndParse_RoundTrip_ReturnsSameEpoch`
   - epoch を複数サンプル（0/1/任意時刻）で round trip

### 4.2 `Json` テスト

#### parse

1. `Json_Parse_Null_ReturnsNull`
2. `Json_Parse_Bool_ReturnsBool`
3. `Json_Parse_Int_ReturnsInt`
4. `Json_Parse_Double_ReturnsDouble`
5. `Json_Parse_String_EscapesHandled`
   - `\n`, `\t`, `\"`, `\\`, `\uXXXX`, surrogate pair を含む

6. `Json_Parse_Array_ReturnsArray`
7. `Json_Parse_Object_PreservesInsertionOrder`
   - `o` が挿入順を保持する仕様の固定

8. `Json_Parse_TrailingChars_Throws`

#### write

9. `Json_Write_String_Escapes`
10. `Json_Write_Object_IndentationStable`

### 4.3 `CommandRunner::quoteArgForWindowsCommandLine` テスト

1. `QuoteArg_NoSpaces_NoQuotes_ReturnsSame`
2. `QuoteArg_WithSpace_AddsQuotes`
3. `QuoteArg_WithQuote_EscapesQuote`
4. `QuoteArg_TrailingBackslashes_EscapesCorrectly`
   - 例: `C:\path\` の末尾 `\` の倍化

5. `QuoteArg_Combo_BackslashesBeforeQuote_EscapesCorrectly`

### 4.4 `Config` テスト（現状は Component 寄り）

`loadAndValidateConfig` はファイル読み込み込みのため、テンポラリファイルを用いる。
将来的に pure 部分を抽出したら Small に移行。

1. `Config_Load_MinimalValidConfig_ReturnsCommands`
   - commands 配列、name、exe 必須

2. `Config_Load_CommandNameMissing_Throws`
   - name も id もない

3. `Config_Load_CommandExeMissing_Throws`

4. `Config_Load_DefaultsApplied_MinIntervalTimeout`
   - root defaults が command に反映

5. `Config_Load_LastRunUtcEmpty_HasLastRunUtcFalse`

6. `Config_Load_LastExitCodeNull_HasLastExitCodeFalse`

### 4.5 `FileUtil` テスト（Medium）

1. `FileUtil_WriteUtf8Atomic_WritesUtf8`
   - BOM なし UTF-8 で書かれること

2. `FileUtil_WriteUtf8Atomic_IsAtomicLike_ReplacesExisting`
   - 既存ファイルが置換される（少なくとも同一パスで内容が更新）

3. `FileUtil_AcquireLockFile_SecondAcquireFailsOrBlocks`
   - 同一プロセス内で2回目の排他取得ができないことを確認（※実装依存で設計要確認）

---

## 5. `wmain` のロジックに対するユニットテスト設計（将来の抽出前提）

現状のままだと Win32 依存が強いので、次の抽出を前提にテストを設計する。

### 5.1 抽出したい関数（案）

- `ShouldSkip(now, lastRunUtc, minIntervalSeconds) -> (skip:bool, reason:string?)`
- `UpdateExecutionRecord(command, startEpoch, exitCode)`

### 5.2 テストケース

1. `ShouldSkip_NoLastRun_ReturnsFalse`
2. `ShouldSkip_IntervalNotReached_ReturnsTrue`
3. `ShouldSkip_IntervalReached_ReturnsFalse`
4. `ShouldSkip_LastRunInFuture_NotSkipped`（現実には clock skew、仕様として delta<0 をどう扱うか固定）
5. `UpdateExecutionRecord_SetsLastRunUtcAndExitCode`

---

## 6. テストデータ（推奨）

- JSON: 最小構成 / 異常系 / エスケープ強め
- 時刻: 1970-01-01, 閏年境界, 月末境界

---

## 7. テスト実装の技術選定メモ（設計時点）

- 言語: C++
- フレームワーク候補: `doctest`, `Catch2`, `GoogleTest`
  - t_wada スタイル的にはフレームワークは何でもよいが、導入コストと VS 連携を優先。
- テストは `src/lastexecuterecord.tests/` のような別プロジェクトに分離。

---

## 8. 受け入れ条件（この設計の完了条件）

- 主要な仕様（JSON/時刻/コマンドライン quoting/スキップ判定）がテストケースとして列挙されている
- 副作用境界が明確で、Fake/Stub に置き換える前提が記載されている
- 1テスト1理由・AAA を満たす粒度でケースが分割されている
