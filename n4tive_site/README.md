# n4tive Playground

ブラウザで `.n4` を編集し、標準入力付きコードテストを実行し、AtCoder提出用の GCC C++23 コードをコピーできる小さなサイトです。NeoShowcase の Runtime / Dockerfile ビルドを想定しています。`25B31026/calc4.c` で GAS を生成し、`25B31026/codes/gas_to_cpp.py` で C++ に埋め込みます。コードテスト時は生成した C++ と標準入力を外部サービス [Wandbox](https://wandbox.org/) に送ってコンパイル・実行します。NeoShowcase 内で利用者の生成したネイティブプログラムは実行しません。LSPやユーザーアカウント、データベースはありません。エディタのソースと入力はブラウザの localStorage に保存します。

## ローカルで動かす

リポジトリのルートで実行します。`gcc` と `curl` が必要です。

```bash
python3 n4tive_site/server.py
```

`http://127.0.0.1:8000/` を開きます。起動時に `calc4.c` をビルドするので、コンパイラを変更したらサーバーを再起動します。`GET /healthz` はサーバーの起動状態を返します。Wandbox への通信可否はコードテスト時に確認されます。環境変数 `PORT` でポートを変更できます。

`/guide` には C++ 経験者向けの文法ガイドがあります。レポートに掲載した A・B・C の実例を読み、サンプルをエディタへ読み込めます。

デフォルトの例は三角数を求めるプログラムです。標準入力 `5`、期待する出力 `15` で「コードテスト」を押すと AC になります。期待する出力を空欄にすると比較せず実行します。「C++ を生成」は GAS と C++ の生成だけを行い、Wandbox には送信しません。生成した C++ は「コピー」または「Main.cpp を保存」で取得できます。

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

NeoShowcase 上のコンテナでは Bubblewrap のマウント隔離が拒否されたため、コードテストの実行先を Wandbox に変更しました。Wandbox は AtCoder と異なる GCC バージョンと実行制限を持つため、最終的な提出結果は AtCoder で確認してください。Wandbox に送られるのは生成した C++ とその回の標準入力です。外部サービスが停止したり、NeoShowcase からの外向き HTTPS 接続が制限された場合、コードテストはエラーになりますが C++ の生成は引き続き利用できます。

## API

`POST /api/compile` と `POST /api/run` に JSON `{"source":"main(){print 7;}","stdin":"","expected":"7\n"}` を送ります。`compile` は生成された C++ と GAS を返し、`run` は `AC` / `WA` / `OK` / `CE` / `RE` / `OLE`（出力上限超過）と標準出力・標準エラー・所要時間を返します。同時処理は2件まで、Wandbox への接続待ちは最大28秒です。
