# Unit Tests for lastexecuterecord

**注意: このディレクトリのCMakeベースのテストは非推奨となりました。**

## 現在のテストフレームワーク

テストは **Microsoft Unit Testing Framework for C++ (MSTest)** に移行されました。

Visual Studioのテストプロジェクトを使用してください:
- **場所**: `src/lastexecuterecord.mstest/`
- **フレームワーク**: MSTest (Visual Studio 標準)
- **依存関係**: なし（vcpkg不要）

## テストの実行方法

1. Visual Studio で `src/lastexecrecord.sln` を開く
2. ソリューションをビルド (Ctrl+Shift+B)
3. Test Explorer を開く (Test → Test Explorer)
4. "Run All" をクリックしてテストを実行

## このディレクトリについて

このディレクトリのテストファイルは参考用に残されていますが、CMakeビルドからは除外されています。
新しいテストは `src/lastexecuterecord.mstest/` に追加してください。

### 参考ファイル

- `TimeUtil.tests.cpp` - 時刻関連ユーティリティのテスト
- `Json.tests.cpp` - JSONパーサー/ライターのテスト  
- `CommandRunner.tests.cpp` - プロセス実行機能のテスト
- `Config.tests.cpp` - 設定ファイル読み込みのテスト
- `FileUtil.tests.cpp` - ファイルI/Oユーティリティのテスト

### CMakeを使用したビルド（Windows）

```cmd
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

### Visual Studioでのビルド

1. `src/lastexecrecord.sln` を開く
2. テストプロジェクトをソリューションに追加（必要に応じて）
3. ビルド → ソリューションのビルド

## テスト実行

### CMakeを使用

```cmd
cd build
ctest --output-on-failure
```

または直接実行:

```cmd
build\tests\Debug\lastexecuterecord.tests.exe
```

### Visual Studioのテストエクスプローラー

1. テスト → テストエクスプローラー
2. すべてのテストを実行

## テスト結果の確認

doctestは失敗したテストのみを詳細に表示します。すべてのテストがパスすると、簡潔なサマリーが表示されます。

```
[doctest] doctest version is "2.4.12"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 45 | 45 passed | 0 failed | 0 skipped
[doctest] assertions: 123 | 123 passed | 0 failed |
[doctest] Status: SUCCESS!
```

## テストオプション

```cmd
# 特定のテストケースのみ実行
lastexecuterecord.tests.exe -tc="TimeUtil*"

# 詳細な出力
lastexecuterecord.tests.exe -s

# ヘルプ
lastexecuterecord.tests.exe --help
```

## 注意事項

### Windows専用機能

このプロジェクトはWindows専用です。Windows API（`Windows.h`）を使用しているため、Linux/macOS上でのビルドには制限があります。

テストコードはクロスプラットフォームを考慮して書かれていますが、以下の機能はWindows上でのみテストされます：

- ファイルロック（`FileLock`）
- プロセス実行（`runProcess`）
- ファイルI/O（原子的書き込み）

### テスト用一時ファイル

テストは一時ファイルを作成します（`test_*.json`, `test_*.lock`など）。これらは`.gitignore`で除外されており、テスト終了時にクリーンアップされます。

## トラブルシューティング

### vcpkg install が失敗する

- `vcpkg-configuration.json`のbaselineが古い場合、最新のコミットハッシュに更新してください
- `vcpkg update`を実行してvcpkg自体を更新してください

### ビルドエラー

- Visual Studio 2022以降を使用していることを確認
- Windows 10 SDK がインストールされていることを確認
- `vcpkg install`が正常に完了していることを確認

### テストが失敗する

- Windowsの日時設定が正確であることを確認（TimeUtilのテスト）
- ファイルシステムに書き込み権限があることを確認
- ウイルス対策ソフトがファイル操作をブロックしていないか確認

## 参考資料

- [doctest Documentation](https://github.com/doctest/doctest)
- [vcpkg Documentation](https://vcpkg.io/)
- [t_wada style testing](../docs/unit-test-design-twada.md)
