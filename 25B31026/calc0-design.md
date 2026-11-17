# `calc0.c` の状態遷移と関数呼び出し

この図は [`calc0.c`](calc0.c) の現行実装を表す。矢印のラベルは「入力 / 主な動作」。丸数字は下の関数呼び出し図と対応する。

```mermaid
flowchart LR
    start((開始)) -->|"states = 0; res = 0; cur = 0; last_op = '+'"| NUM["数字入力状態<br/>states == 0"]

    NUM -->|"数字 0–9 / ① cur = concat(cur, cmd)<br/>S / ① cur = concat(cur, 'S')"| NUM
    NUM -->|"演算子 + - * /<br/>op = cmd; states = 1"| OP["演算キー入力後<br/>states == 1"]
    OP -->|"演算子 + - * /<br/>op = cmd"| OP
    OP -->|"数字 0–9 または S<br/>② res = eval(last_op, res, cur); last_op = op; reset()<br/>① cur = concat(cur, cmd); states = 0"| NUM

    NUM -->|"= / ② res = eval(last_op, res, cur)<br/>last_op = op; reset()"| NUM
    NUM -->|"P・M / ③ op_memory(cmd)"| NUM
    OP -->|"= / ② eval → last_op 更新 → reset"| OP
    OP -->|"P・M / ③ op_memory(cmd)"| OP

    NUM -->|"入力終端 '\0'"| done["display()<br/>結果または E を出力"]
    OP -->|"入力終端 '\0'"| done

    classDef state fill:#eaf3ff,stroke:#3273c4,stroke-width:2px,color:#111;
    classDef endpoint fill:#e9f6eb,stroke:#438a52,stroke-width:2px,color:#111;
    class NUM,OP state;
    class start,done endpoint;
```

```mermaid
flowchart LR
    one["① 数字・S を処理"] -.-> concat["concat(x, cmd)"]
    two["② 保留中の演算を確定"] -.-> eval["eval(last_op, res, cur)"]
    three["③ P・M を処理"] -.-> op_memory["op_memory(cmd)"]

    concat -->|"S: 符号反転 / 数字: x × 10"| mul["mul"]
    concat -->|"数字かつ x ≥ 0"| add["add"]
    concat -->|"数字かつ x < 0"| sub["sub"]

    eval -->|"+"| add
    eval -->|"−"| sub
    eval -->|"×"| mul
    eval -->|"÷"| div["div"]

    op_memory -->|"P・M: 現在値を確定"| eval
    op_memory -->|"P: memory に加算"| add
    op_memory -->|"M: memory から減算"| sub
    op_memory -->|"P・M: cur と op を初期化"| reset["reset"]

    classDef site fill:#fff3d8,stroke:#c58a19,stroke-width:1.5px,color:#111;
    classDef helper fill:#f3efff,stroke:#7754b7,stroke-width:1.5px,color:#111;
    class one,two,three site;
    class concat,eval,op_memory,add,sub,mul,div,reset helper;
```

`=` は `res` を更新するが、そこで表示はしない。`display()` は入力文字列の終端で一度だけ呼ばれる。`P`・`M` は `op_memory()` 内で `eval()` の結果を `memory` に加減算し、`cur` と `op` を初期化する。`R`・`C` と入力分類用の `is_*` 関数は図から省略した。
