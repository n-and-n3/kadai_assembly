# MyLang を AtCoder の画面で使う

AtCoder の言語欄に `MyLang (GCC C++23)` を追加するユーザースクリプトと、手元で動かすコンパイラサーバーです。言語 ID は本物の GCC C++23 を使用します。エディタには MyLang を書き、Easy Test は手元で実行します。AtCoder の「コードテスト」と「提出」を押すと、送信直前に MyLang → GAS → インラインアセンブリ入り C++ へ変換します。AtCoder に届くのは C++ です。

## 初回設定

1. WSL のこのリポジトリで次を実行し、ターミナルを開いたままにします。

   ```bash
   python3 playground/mylang_atcoder_server.py
   ```

   `http://127.0.0.1:8765/` で待ち受けます。`gcc` と `g++` が必要です。`calc4.c` を変更したらサーバーを再起動してください。

2. Windows のブラウザに Tampermonkey と AtCoder Easy Test v2 を入れます。Tampermonkey の「新規スクリプト」に [`mylang_atcoder.user.js`](mylang_atcoder.user.js) の全文を貼り付けて保存します。

3. AtCoder の問題ページを開き、Easy Test の「Setting」で `codeRunner.localRunnerURL` を `http://127.0.0.1:8765/` にします。ページを更新します。Easy Test の実行環境に `MyLang [Local]` が現れれば接続できています。

   ブラウザが「このデバイス上のアプリへのアクセス」を確認した場合は AtCoder からのアクセスを許可します。接続できない場合は、Windows のブラウザで `http://127.0.0.1:8765/` を開き、`{"status": "ready"}` が見えるか確認してください。

4. AtCoder の「言語」欄で `MyLang (C++23 (GCC ...))` を選びます。ほかの拡張機能が言語選択の表示を書き換える場合は、欄の横の `MyLang を使う` ボタンで `MyLang ON` にしてください。このモードは同じタブ内で記憶されます。コード欄に MyLang を書いてください。Easy Test の実行環境は `MyLang [Local]` に自動で切り替わります。

## 普段の使い方

- Easy Test の `Run` / `Test All Samples`: MyLang をローカルで C++ に変換し、`g++` でビルドして実行します。出力比較は Easy Test が行います。最初に実行環境が `MyLang [Local]` になっていることを確認してください。
- `生成 GAS を表示`: 現在のソースから生成されたアセンブリを別タブで表示します。
- AtCoder の「コードテスト」: クリック時に提出用 C++ に変換し、元のコードテスト操作に渡します。テスト後、編集欄には MyLang を戻します。
- AtCoder の「提出」および Easy Test の `Test & Submit`: 提出時に C++ へ変換します。MyLang のソースはブラウザの同じタブ内に保存されます。

コンパイルエラーやサーバー未起動時は送信を止め、エラーを表示します。MyLang のソースは C コンパイラのコマンドライン引数として渡しているため、現状の上限は約 100 KB です。ローカルの実行は 3 秒で打ち切ります。

AtCoder の画面構造や Easy Test の実装が変わると、ユーザースクリプトのセレクタを調整する必要があります。コードテストと提出の画面操作はログインしたブラウザで確認してください。
