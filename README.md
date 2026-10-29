# 授業の課題・学習メモ

このフォルダは、授業資料と演習コードを置く作業場所です。会話で確認した内容を、次回以降の作業でも参照できるように記録します。

最終更新：2026-10-29（日本時間）

## 参照先

- 授業資料：[Linuxで学ぶx86-64アセンブリ言語 — 前書き](https://gondow.github.io/linux-x86-64-programming/1-pre.html)
- シラバス：[2026年度 アセンブリ言語（CSC.T262）](https://syllabus.s.isct.ac.jp/courses/-/-/-/202602427)
- 以前の参照先：[LMSへのリンク（本文未確認）](https://lms.s.isct.ac.jp/2025/mod/url/view.php?id=82478)
- VS Code：[Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers)
- VS Code：[起動済みコンテナへの接続](https://code.visualstudio.com/docs/devcontainers/attach-container)

授業資料のWebページは確認済み。Linux・x86-64・GNUアセンブラを前提とした資料です。公開シラバスも2026-10-16にログインなしで本文を取得できました。以前のLMSリンクの本文は未確認です。

### 公開シラバスで確認したこと

- 科目：アセンブリ言語（CSC.T262）、担当：権藤克彦。
- 開講：2026年度3Q、火・金3–4時限、W9-324、対面講義。
- 授業形式：座学とプログラム演習。
- 内容：アセンブリの文法、バイナリ表現、CPU・レジスタ・メモリ、機械語命令、関数呼び出しとスタック、Cとのリンクと呼び出し規約、インラインアセンブラ、ABI、リンクと再配置、特権命令・I/O・割り込み。
- 成績評価：プログラミング課題レポート100%。
- シラバス更新日：2026-03-05。

公開シラバスには発展課題の具体的な要件や、自作言語・コンパイラが対象として認められるかの記載は見当たりません。個別の課題文を確認する必要があります。

## フォルダ構成

```text
kadai/
├── README.md
├── notes/
│   └── mougen.md                 # 本人の思いつき。正式な課題方針とは別扱い
└── 20260928/
    └── 20260928/
        ├── 1-intro.pdf           # 導入・Dockerなど
        ├── 2-calc.pdf
        ├── 10-intr.pdf
        ├── x86-64.pdf
        ├── fact.c               # 今回作成した階乗のサンプル
        ├── calc/                # 配布されたCの例、テスト用ファイルなど
        └── src/                 # ブート処理などのアセンブリ教材
```

この図は主要ファイルの抜粋です。配布資料のフォルダ名が二重になっていますが、現状の配置を維持しています。

## 今回確認したこと

ユーザーは `1-intro.pdf` に出てくる `fact.c` の目的とサンプル、DockerとVS Codeの連携方法を知りたいと相談しました。

- PDFの47ページには、ホスト側フォルダを `/workspace` に共有する例があります。
- 48ページでは、配布済みイメージ `gondow/linux-prog:latest` が紹介されています。
- 49〜50ページでは `gcc -g fact.c` でコンパイルし、`fact (5) = 120` を表示して、GDBで実行を確認しています。
- 確認したPDFには `fact.c` のソース本体が載っていないため、元の実装は不明です。
- 52ページには、VS Codeから手動でコンテナへ接続する方法と、`.devcontainer/devcontainer.json` に設定を書く方法が挙げられています。

### 作成した fact.c

[fact.c](20260928/20260928/fact.c) は、PDFの出力に合わせて今回作成したサンプルです。配布された原本の復元ではありません。

- `fact(n)` は再帰で階乗を計算します。
- `main()` では `n = 5` を渡します。
- 想定する入力範囲は `0〜12`。負数の検査や大きい整数への対応はしていません。
- `gcc -std=c11 -Wall -Wextra -pedantic -g` で警告なくコンパイルでき、`fact (5) = 120` の出力を確認済みです。

作業フォルダのルートから実行する例：

```bash
cd 20260928/20260928
gcc -Wall -Wextra -g fact.c -o fact
./fact
```

GCCが生成するアセンブリを読む場合：

```bash
gcc -O0 -S fact.c -o fact.s
```

## Code Runner（WSL上でCを実行）

ワークスペースのルート `kadai` をVS CodeのWSLウィンドウで開きます。拡張機能は [Code Runner](https://marketplace.visualstudio.com/items?itemName=formulahendry.code-runner)（`formulahendry.code-runner`）を使います。

設定は [.vscode/settings.json](.vscode/settings.json) に保存しています。Cファイルを開き、コマンドパレットの `Run Code`、エディタ右上の `Run Code` ボタン、または `Ctrl+Alt+N` で実行します。

Code Runnerは開いているCファイルを保存し、そのファイルのフォルダでGCCを呼びます。例えば `calc0.c` なら次の処理です。

```bash
gcc -Wall -Wextra -g -O0 calc0.c -o calc0 && ./calc0
```

- `-Wall -Wextra` は警告を表示します。
- `-g -O0` はデバッグ情報を付け、最適化を無効にします。
- `-o calc0` は実行ファイル名を指定します。ソースと同じフォルダに生成します。
- `&&` により、コンパイルが成功した場合だけ実行します。
- 入出力とエラーはVS Codeのターミナルに表示します。入力待ちや実行中のプログラムは、そのターミナルで `Ctrl+C` を押して停止します。
- 選択範囲にかかわらず、ファイル全体を対象にします。

この設定は単一のCファイル向けです。コマンドライン引数は自動で渡していません。引数が必要なプログラムは、生成後にターミナルから例えば `./calc0 '1+2='` と実行してください。`scanf` などの標準入力と、`argv` で受け取るコマンドライン引数は別です。

2026-10-27時点で `25B31026/calc0.c` は空です。実行可能なプログラムには `main` 関数が必要なので、最初の動作確認には既存の `20260928/20260928/fact.c` を使えます。

## GDBでデバッグ（WSL）

MicrosoftのC/C++拡張（`ms-vscode.cpptools`）と `/usr/bin/gdb` を使用します。設定は [.vscode/tasks.json](.vscode/tasks.json) と [.vscode/launch.json](.vscode/launch.json) です。

- `tasks.json`：開いている単一のCファイルを `gcc -g -O0 -Wall -Wextra` でビルドします。実行ファイルはソースと同じフォルダに、拡張子を除いた名前で生成します。
- `launch.json`：上記タスクを実行した後、生成した実行ファイルをGDBで起動します。`preLaunchTask` はタスクの `label` に対応します。
- `stopAtEntry: true`：起動時に `main` の入口で停止します。不要になったら `false` にできます。

使い方：

1. `fact.c` など、デバッグしたいCファイルをエディタで開きます。
2. 「実行とデバッグ」で `C: 開いているファイルをGDBでデバッグ (WSL)` を選び、F5を押します。
3. 必要なら行番号の左をクリックしてブレークポイントを設定します。
4. F10で関数内に入らず次の行へ、F11で関数内へ、Shift+F11で現在の関数から戻り、F5で次の停止位置まで実行します。Shift+F5で停止します。

`fact.c` では `if (n <= 1)` の行にブレークポイントを置くと、再帰呼び出しのたびに `n` と「コールスタック」の変化を確認できます。実行位置を示す行は、通常は次に実行される行です。

「変数」「ウォッチ」「コールスタック」は停止中に確認します。実行中のプログラムの標準入出力はターミナル、デバッガへの問い合わせはデバッグコンソールで扱います。例えばデバッグコンソールで `-exec info registers` や `-exec bt` を実行できます。

GDBは実際にプログラムを制御するデバッガです。CLIでは人がGDBにコマンドを入力し、VS CodeではC/C++拡張のデバッグアダプターがGDB/MI経由でGDBを操作し、結果を画面に表示します。CLIとGUIでGDB本体は共通です。

この設定は開いている単一のCファイル向けです。起動前にはJSONやREADMEではなくCファイルを選んでください。`argv` に引数が必要なら `launch.json` の `args` を、例えば `["1+2="]` に変更します。`scanf` 等の標準入力とは別です。複数ファイルのリンクや、生成した `.s` を別の実行ファイルにする場合は、対応したビルド・起動設定が必要です。

## DockerとVS Code

現在の方針（2026-10-27）：普段の開発にはx86-64のWSL Ubuntuを使い、Dockerは必要になったら導入します。既存の `.devcontainer` 設定は残しています。以下のDocker手順は参照用です。

Dev Containersを使うと、PC上のVS Codeの画面から、コンテナ内のファイル・ターミナル・コンパイラ・デバッガを扱えます。Ubuntuのデスクトップ画面を表示する機能ではありません。Dockerコンテナと仮想マシンも別の仕組みです。

以下は案内した手順です。Dockerでのコンテナ起動・VS Code接続の成功は、この作業では未確認です。ユーザーのホストOSも未確認です。

### 起動済みコンテナに手動接続

Dockerを動作させ、VS CodeにMicrosoft提供の **Dev Containers** 拡張機能（`ms-vscode-remote.remote-containers`）を入れます。Windows＋WSLの場合はDocker Desktopと対象WSLディストリビューションの連携も確認します。

以下のコマンドはWSL・Linux・macOSのbash等を想定しています。`fact.c` のあるフォルダで実行します。

```bash
docker pull --platform linux/amd64 gondow/linux-prog:latest
docker run --name kadai-dev \
  --platform linux/amd64 \
  -it \
  -v "$(pwd):/workspace" \
  -w /workspace \
  gondow/linux-prog:latest \
  bash
```

このシェルは起動したまま、VS Codeで次を操作します。

1. コマンドパレットを開く（Windows/Linux：`Ctrl+Shift+P`、macOS：`Cmd+Shift+P`）。
2. `Dev Containers: Attach to Running Container...` を実行する。
3. `kadai-dev` を選ぶ。
4. 接続後、`/workspace` を開く。
5. VS Codeの新しいターミナルで `gcc -g fact.c -o fact`、`./fact` を実行する。

`-v` はPC側のフォルダを共有する指定なので、編集したソースや生成物はPC側にも残ります。同名のコンテナを再利用するときは `docker run` を繰り返さず、停止していれば `docker start kadai-dev` で起動してから接続します。

### devcontainer.jsonを使う方法（提案のみ）

以下は会話で紹介した設定例です。`.devcontainer/devcontainer.json` はまだ作成していません。

```json
{
  "name": "授業用 C 開発環境",
  "image": "gondow/linux-prog:latest",
  "runArgs": ["--platform=linux/amd64"],
  "customizations": {
    "vscode": {
      "extensions": ["ms-vscode.cpptools"]
    }
  }
}
```

設定を置いたプロジェクトフォルダをVS Codeで開き、`Dev Containers: Reopen in Container` を実行します。VS Codeがコンテナの作成・起動とフォルダ共有を行います。

### Apple SiliconでのGDB

PDFの49〜51ページでは、Apple Silicon上のx86-64コンテナでGDBを使うために、QEMUユーザモード経由で接続する方法が説明されています。

```bash
gcc -g fact.c
qemu-x86_64-static -g 1234 ./a.out &
gdb -q -ex "target remote :1234" ./a.out
```

この手順では、GDBの `run` ではなく `continue` で実行を開始します。利用環境に合わせて適用してください。

## 次回以降の作業で参照すること

- このREADMEを、これまでの確認事項の引き継ぎとして参照する。
- 配布資料の記述・今回作ったサンプル・未検証の提案を区別する。
- 課題要件の判断には、該当する課題文とシラバスの確認が必要。
- 本人の自由なアイデアは [妄言メモ](notes/mougen.md) に分けて記録する。そこにある発言だけを根拠に、実装や課題方針の確定を進めない。
