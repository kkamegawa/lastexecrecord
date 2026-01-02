# Security notes

このツールは「設定 JSON によってプロセス起動を制御する」ため、設定ファイルの保護がそのまま安全性に直結します。

## 1. コマンドインジェクション回避

- `exe` と `args[]` を分離し、`CreateProcessW` を使用。
- 文字列を `cmd.exe /c` などの shell に渡す方式は採用しない（ただし `exe` に cmd.exe を指定すれば実行できるため、禁止するなら追加ポリシーが必要）。

## 2. ローカル実行の制限（localOnly）

- `localOnly=true` の場合、config を PC 名にピン留めして他PCでの実行を抑止。
- `exe` が UNC パス（`\\server\\share...`）の場合、localOnly 有効時は拒否。

## 3. 同時起動・破損対策

- `<config>.lock` を排他オープンして多重起動を抑止。
- config 更新は `.tmp` 書き込み→ `MoveFileEx(REPLACE_EXISTING|WRITE_THROUGH)` で原子的に置換。

## 4. 推奨運用

- config を信頼できる場所に置く（ユーザー専用ディレクトリ等）。
- 管理者権限で実行する場合は特に注意（config 改ざん＝任意コード実行になりうる）。
- 可能なら config の ACL をユーザー限定にする。

## 5. 将来の強化案

- `exe` の絶対パス強制と正規化。
- `cmd.exe`, `powershell.exe` 等を禁止/制限する allow-list。
- `exe` の署名/ハッシュ検証（要件次第）。
