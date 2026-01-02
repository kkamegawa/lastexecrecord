# 引き継ぎメモ（仕様・設計・実装の経緯）

このドキュメントは、lastexecrecord（LastExecuteRecord）の仕様決定〜実装の過程を、他ツール/他エージェントへ引き継ぐためにまとめたものです。

- 対象リポジトリ: `kkamegawa/lastexecrecord`
- 対象プロジェクト: `src/lastexecuterecord/lastexecuterecord.vcxproj`
- 方針: 外部ライブラリ極小（Win32 API + STL）、高速、セキュリティ注意

## 1. 元の状態（調査結果）

### 元の意図

README には「NTFS data stream (ADS) に最終実行時刻を記録」とだけ記載。

### 実装の実態と問題

- `src/lastexecuterecord/main.cpp` の引数チェックが `if (argc != 2 || argc != 1)` になっており、論理的に常に true。
  - そのため常に Usage を表示して終了し、実質動作していなかった。
- ADS 書き込みも成立していなかった（例）:
  - `filename:streamname` の形式で CreateFile を開いておらず、ADS になっていない。
  - `WriteFile` のサイズ計算が `std::string` なのに `sizeof(WCHAR)` を掛けており破壊的。
  - 引数参照の範囲外アクセスの潜在バグ（`argc==2` で `argv[2]` を参照）。

結論: 要件（JSON定義でコマンドを順次実行し、秒精度の前回実行時刻と最短間隔でスキップ）を満たすには、既存コードの部分修正ではなく、**エントリポイントとデータモデルの再設計**が必要。

## 2. 要件（ユーザー提示）

- 呼び出されたときに一度だけ、JSONに登録されたコマンドを順次実行する Windows コンソールアプリ
- 設定用 JSON に含める情報
  - 実行するコマンド定義
  - 前回実行時間（秒単位まで記録）
  - 最低実行間隔（指定時間内は呼び出されても実行しない）
  - ローカルPCのみ実行対象とする
- 余計な外部ライブラリに極力依存せず、高速で、セキュリティに注意

## 3. 設計判断（なぜその仕様にしたか）

### 3.1 記録先: state.json 分離ではなく config へ書き戻し

- 初期案では `state.json` に最終実行記録を分離する案もあった。
- ただし要件は「設定用 JSON に前回実行時間が入る」ことが中心のため、運用の簡単さを優先。
- **今回の実装では config JSON に `lastRunUtc` と `lastExitCode` を追記/更新する方式**を採用。

備考:

- 将来、設定ファイルを読み取り専用で運用したい場合や改ざん耐性を上げたい場合は、`state.json` 分離方式への移行を推奨。

### 3.2 外部依存なし JSON

- 要件により JSON ライブラリ（例: nlohmann/json）を採用せず、最小 JSON パーサ/ライタを同梱。
- 制約:
  - JSON コメント不可
  - 文字列のエスケープや Unicode は必要最低限

### 3.3 セキュリティ: shell 文字列を避ける

- 文字列コマンドを shell に渡す方式はインジェクション面で不利。
- `exe` と `args[]` を分離し、`CreateProcessW` で起動。
- Windows のコマンドライン規則に沿って引数をクォート。

### 3.4 同時起動対策と原子的更新

- `<configPath>.lock` を排他オープンして多重起動を抑止。
- 更新は `.tmp` に書いて `MoveFileEx(REPLACE_EXISTING|WRITE_THROUGH)` で原子的に置換。

### 3.5 ローカルPCのみ（pinning）

- `localOnly=true` の場合、設定を PC 名にピン留めし、他PCでの実行を抑止。
- `runOnlyOnComputerName` が未指定なら初回起動で自動記入。

## 4. 現在の仕様（v1）

### 4.1 CLI

- `--config PATH` : config JSON を指定（未指定なら exe と同名の `.json`）
- `--dry-run` : 実行せず、実行/スキップの判定だけ表示
- `--verbose` : 詳細ログ（スキップ理由など）

### 4.2 スキップ条件

- `lastRunUtc` が parse できる場合に `now - lastRun < minIntervalSeconds` ならスキップ
- `lastRunUtc` が壊れている場合は warning を出して「未実行扱い」

### 4.3 記録更新のタイミング

- 実行開始時刻（秒精度）を `lastRunUtc` に保存
- `lastExitCode` も保存

## 5. 実装マップ（どこを見れば良いか）

- Entry point: `src/lastexecuterecord/main.cpp`
- JSON: `src/lastexecuterecord/Json.h/.cpp`
- Config: `src/lastexecuterecord/Config.h/.cpp`
- File I/O: `src/lastexecuterecord/FileUtil.h/.cpp`
- Time: `src/lastexecuterecord/TimeUtil.h/.cpp`
- Process: `src/lastexecuterecord/CommandRunner.h/.cpp`

## 6. 既知の制約 / 今後の改善候補

- この環境では msbuild が見つからず、こちら側での実ビルド検証は未実施（VS/Build Tools 環境で確認が必要）。
- exe パスの絶対パス強制や正規化は現状最小限（必要なら Win32 API で強化）。
- `cmd.exe` / `powershell.exe` を禁止しているわけではない（exe に指定すれば動く）。
  - セキュリティ優先なら禁止ポリシーや allow-list の導入を検討。
- config 更新を「成功時のみ」にする/「開始時に予約する」などのポリシー確定が必要。
- ADS（NTFS data stream）へ記録する要件が復活した場合は、state専用ファイルの ADS に JSON を保存する方式を検討。

## 7. 実行サンプル

- `lastexecuterecord.sample.json` を `lastexecuterecord.json` として exe と同じフォルダに置く
- 1回目実行で `lastRunUtc` / `lastExitCode` が追記される
- `minIntervalSeconds` 内の2回目はスキップされる（`--verbose` 推奨）
