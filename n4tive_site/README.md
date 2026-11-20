# n4tive Playground

ブラウザで `.n4` を編集し、標準入力付きコードテストを実行し、AtCoder提出用の GCC C++23 コードをコピーできる小さなサイトです。NeoShowcase（旧称や誤記の NewShowcase ではありません）の Runtime / Dockerfile ビルドを想定しています。`25B31026/calc4.c` と `25B31026/codes/gas_to_cpp.py` を使います。LSPやユーザーアカウント、データベースはありません。エディタのソースと入力はブラウザの localStorage に保存します。

## ローカルで動かす

リポジトリのルートで実行します。`gcc`、`g++`、`bwrap` が必要です。

```bash
python3 n4tive_site/server.py
```

`http://127.0.0.1:8000/` を開きます。起動時に `calc4.c` をビルドするので、コンパイラを変更したらサーバーを再起動します。`GET /healthz` が `{"status":"ok"}` を返せばコードテストの隔離環境も使えます。環境変数 `PORT` でポートを変更できます。

`/guide` には C++ 経験者向けの文法ガイドがあります。レポートに掲載した A・B・C の実例を読み、サンプルをエディタへ読み込めます。

デフォルトの例は三角数を求めるプログラムです。標準入力 `5`、期待する出力 `15` で「コードテスト」を押すと AC になります。期待する出力を空欄にすると比較せず実行します。「C++ を生成」は実行せずに GAS と C++ の生成・ビルドだけ行います。生成した C++ は「コピー」または「Main.cpp を保存」で取得できます。

## NeoShowcase に配置する準備

この作業ディレクトリの GitHub リモートは `https://github.com/n-and-n3/kadai_assembly.git` です。GitHub を公開するだけでは NeoShowcase への配置は始まりません。手元の NeoShowcase 利用資料では、管理画面で traP Gitea のリポジトリと追跡ブランチを選ぶ方式です。管理画面で GitHub URL を直接選べない場合は、GitHub 公開後に同じ内容を traP Gitea に置き、そのリポジトリを選んでください。管理画面では次のように設定します。

| 項目 | 値 |
|---|---|
| Deploy Type | Runtime |
| Build Type | Dockerfile |
| Context | `.`（リポジトリルート） |
| Dockerfile Name | `n4tive_site/Dockerfile` |
| Entrypoint / Command | 空欄 |
| HTTP Port | `8000` |
| HTTPS | 有効 |
| Authentication | 最初は Hard（traP ID必須） |

アプリは `0.0.0.0:8000` で待ち受けます。データベースと永続ボリュームは不要です。URLを追加した後、`/healthz` とサイト画面を確認してください。NeoShowcaseの設定手順は `Philosophy_of_Science_B/ref/neoshowcase-usage.md` を参照しました。

コードテストは **Bubblewrap の user / network / PID namespace** を使います。NeoShowcase 側で非特権 user namespace が許可されない場合、画面は表示できますが `/healthz` は503になり、コンパイルと実行は停止します。その場合は公開前に実行環境の隔離方法を調整する必要があります。生成されたネイティブプログラムを実行するため、隔離が動作するまで認証を外して公開しないでください。

## API

`POST /api/compile` と `POST /api/run` に JSON `{"source":"main(){print 7;}","stdin":"","expected":"7\n"}` を送ります。`compile` は生成された C++ と GAS を返し、`run` は `AC` / `WA` / `OK` / `CE` / `RE` / `TLE` / `OLE`（出力上限超過）と標準出力・標準エラー・実行時間を返します。1回の実行は3秒まで、同時処理は2件までです。
