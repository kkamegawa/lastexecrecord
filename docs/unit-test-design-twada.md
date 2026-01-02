# ユニットテスト設計（t_wada style）

このドキュメントは `lastexecuterecord` のユニットテスト設計方針をまとめたものです。

## テスト方針

### t_wada style とは

- テストケースをシンプルかつ明確に記述する
- テストファイル名は `<ModuleName>.tests.cpp` の形式
- 各モジュールの機能を独立してテストする
- テストは読みやすく、メンテナンスしやすい構造にする

### テストフレームワーク

- **doctest** を使用
- ヘッダーオンリーで軽量
- 高速なテスト実行
- わかりやすいアサーション構文

## テスト対象モジュール

### 1. TimeUtil (TimeUtil.h/cpp)

**目的**: 時刻関連のユーティリティ機能

**テスト項目**:
- `nowEpochSecondsUtc()`: 現在時刻のエポック秒取得
- `formatEpochSecondsAsIsoUtc()`: エポック秒からISO 8601形式への変換
- `tryParseIsoUtcToEpochSeconds()`: ISO 8601形式からエポック秒への変換
  - 正常なフォーマット
  - 不正なフォーマット
  - エッジケース（境界値）

### 2. Json (Json.h/cpp)

**目的**: 最小限のJSONパーサー/ライター

**テスト項目**:
- JSONパース機能
  - 正常なJSON
  - ネストされたオブジェクト
  - 配列
  - 文字列エスケープ
  - 数値
  - ブール値
  - null
- JSON生成機能
  - オブジェクトの構築
  - 配列の構築
  - 各種データ型の出力
- エラーハンドリング
  - 不正なJSON
  - 予期しない終端

### 3. Config (Config.h/cpp)

**目的**: 設定ファイルの読み込みと検証

**テスト項目**:
- `loadAndValidateConfig()`: 設定ファイル読み込み
  - 正常な設定
  - デフォルト値の適用
  - 必須フィールドの検証
  - 不正な設定の検出
- `applyCommandsToJson()`: 設定のJSON化
- `defaultConfigPath()`: デフォルト設定パス

### 4. FileUtil (FileUtil.h/cpp)

**目的**: ファイル操作ユーティリティ

**テスト項目**:
- ファイル読み込み
  - UTF-8ファイル
  - 空ファイル
  - 存在しないファイル
- 原子的ファイル書き込み
  - 正常な書き込み
  - 既存ファイルの上書き
- ファイルロック
  - ロック取得
  - 多重ロックの防止

### 5. CommandRunner (CommandRunner.h/cpp)

**目的**: 外部コマンド実行

**テスト項目**:
- プロセス実行
  - 正常終了
  - 異常終了
  - タイムアウト
- 引数のエスケープ
  - 通常の引数
  - スペースを含む引数
  - 特殊文字を含む引数

## テストコードの構造

各テストファイルは以下の構造を持つ:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "<TargetModule>.h"

TEST_CASE("モジュール名: テストケースの説明") {
    // Arrange - テストデータの準備
    
    // Act - テスト対象の実行
    
    // Assert - 結果の検証
    CHECK(actual == expected);
}
```

## ビルドとテスト実行

### vcpkg によるdoctestインストール

```bash
vcpkg install doctest
```

### CMakeによるビルド

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### テスト実行

```bash
./build/tests/lastexecuterecord.tests
```

## 今後の改善案

- CI/CDパイプラインでの自動テスト実行
- コードカバレッジの測定
- モックを使った依存関係の分離（Windows API呼び出しなど）
- 統合テストの追加
