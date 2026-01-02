# Config JSON schema (v1)

このプロジェクトは **設定 JSON を読み込み**、呼び出しごとに **コマンドを順次1回実行**し、
**前回実行時刻（秒精度）** と **最低実行間隔** に基づいてスキップします。

> 注意: JSON コメントは使えません。

## Root

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `version` | number | no | 1 | 予約 |
| `defaults.minIntervalSeconds` | number | no | 0 | コマンドの既定最短間隔 |
| `defaults.timeoutSeconds` | number | no | 0 | コマンドの既定タイムアウト（0は無制限） |
| `commands` | array | yes | - | 実行するコマンドを上から順に処理 |

## Command

| key | type | required | default | note |
| --- | --- | --- | --- | --- |
| `name` | string | yes | - | 表示名/識別子（`id` も name の代替として許容） |
| `enabled` | bool | no | true | falseなら常にスキップ |
| `exe` | string | yes | - | 実行ファイルパス（shellではない） |
| `args` | array of string | no | [] | 引数 |
| `workingDirectory` | string | no | "" | 作業ディレクトリ |
| `minIntervalSeconds` | number | no | `defaults.minIntervalSeconds` | スキップ判定に使用 |
| `timeoutSeconds` | number | no | `defaults.timeoutSeconds` | 0なら無制限 |
| `lastRunUtc` | string | no | - | `YYYY-MM-DDTHH:MM:SSZ`（UTC、秒精度） |
| `lastExitCode` | number | no | - | 前回の終了コード |

## Time format

- `lastRunUtc` は `YYYY-MM-DDTHH:MM:SSZ` のみ対応
  - 例: `2026-01-02T12:34:56Z`

## Skip logic

- `lastRunUtc` が存在し、かつ parse に成功した場合
  - `now - lastRun < minIntervalSeconds` ならスキップ
- `lastRunUtc` が壊れている場合
  - warning を出して「未実行扱い」にする（＝実行対象）
