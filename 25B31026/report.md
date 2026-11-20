# アセンブリ言語 電卓コンパイラ

名前：中野 裕斗
学年：25B31026

## tl;dr 

- calc0.c (電卓インタープリタ) を事前に作成し、その設計に忠実にアセンブリのコードを書いた
- C言語で書かれたコードを合計を400行ほど自力でアセンブリに翻訳した
    - これにより、Cの一部のプリミティブな機能とアセンブリコードの対応を把握できた
    - 自作言語のコンパイラを書く上でこの苦労がとても役に立った
- 適切にインデントを用いて、アセンブリ単体で読みやすいコードを目指した
- calc3で筆算をする場合には、できるだけ仕様するレジスタを少なくするよう心がけた
    - 乗算は `%edx` の1つ
    - 除算は `%edx` `%ecx` `%r8d` の3つ
    - で実装した

## calc0.c の実装

資料の状態遷移図を参考に、インタープリタ版の電卓を実装した
実装の際は、グローバル変数と関数を利用して分離し、疎結合を意識した実装になっている。また、この実装を参考にcalc2以降が作られるため、この時点で単純なオーバーフローの実装も同時に行う。

### 状態遷移図

![](https://md.trap.jp/uploads/upload_c925b45dd83fd40044a1434caa208514.png)


### メモリーの実装・エラー処理について
グローバル変数 `hasError` を持ち、どこかの計算で一度でもエラーが発生したら `hasError` のフラグを立てる。

ここに関して、以下のようなアセンブリの関数

```asm
put_error:
    movb $1,hasError(%rip)
    ret
```

を定義し、書くコードの部分が `put_error` を適宜呼ぶ実装にしている。


なお、calc0.c で実装したコードは、このレポートの最後の章に埋め込んである。

## calc1.c の実装

### 方針

calc0 において、入力によって吐くべきアセンブリのコードが依存するのは main 関数の部分のみであるようにする。

まずは、グローバル関数をすべてアセンブリに変換する。例えば、

```c
int add(int x, int y){
    int ret;
    bool overflow = __builtin_add_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
```

これをアセンブリに変換する。
具体的には、
- 引数とレジスタの対応を考える
    - intの2引数関数でintを返すので
    `## (%edi : int, %esi : int) -> %eax : int`
    というコメントアウトを書いておく。こうすることで「第◯引数の名前なんだっけ？」といちいち調べなくてよくなりミスが減る。また後から参照したときに何の引数を用いているか直感的に分かる。
    - 少なくとも返り値は `%eax` レジスタに入れる。今回の場合は追加のレジスタは必要ないので愚直に実装する。
    - 「オーバーフローをしたときにエラーを吐く」は、`call put_error` として分離する。

ということに注意して、
```asm
## (%edi : int, %esi : int) -> %eax
my_add:
    movl %edi,%eax
    addl %esi,%eax
    jo .L_my_add
    ret
    .L_my_add:
    call put_error
    ret
```

というコードが書ける。これをすべての関数に対して行う。

### 実装上の工夫

授業で扱った方法では、関数はコールスタックを用いる方法が扱われていた。しかし、実際にgccの吐くコードを見てみると、例えば、
```c
int add5(int x){
    return x+5;
}
```
は、コンパイラの最適化オプションを `-O2` にすると
```asm
add5:
.LFB23:
	.cfi_startproc
	endbr64
	leal	5(%rdi), %eax
	ret
	.cfi_endproc
```
に変換される。

chatGPTに聞いてみると、
> スタックフレームを作ることは必須ではなく、単純な処理で少数のレジスタで実装できる場合は、スタックフレームの構築は不要である

と教わった。確かに、プログラムカウンタなどの仕様を考えれば、`%rbp` や `%rsp` が変更されないし、かつ、スタックにも触らないのであれば、確かにこの操作は不要であると分かる。

実装量を考えて、このようなコールスタックを積まない実装を書くこととした。

以上のことを、他のすべての関数に対しても行う。`calc1.c` の**4行目から246行目までに埋め込まれたアセンブリコードは、手書きでアセンブリを書いている**。


なお、今思えば関数の部分的なコンパイルはgccを使えば良かったののではないかと後悔している。

詳細は calc1.c のコードを参照。

## calc2.c

### 方針
加算・乗算・乗算のオーバーフローは、calc1の実装と同時に実装し終えている。除算のオーバーフローを修正する。

calc1.c で行ったオーバーフローの処理を説明する。


例えば、加算のオーバーフローは `jo` 命令によって以下のように実装できる
```asm
## (%edi : int, %esi : int) -> %eax
my_add:
    movl %edi,%eax
    addl %esi,%eax  # ここ部分の計算でオーバーフローするので、
    jo .L_my_add    # ここでキャッチする
    ret             # キャッチされなかったら終了
    .L_my_add:      # キャッチされたら 
    call put_error  # グローバル変数にエラーを記録
    ret             # 終了
```

減算、除算も同様である。

次に除算のオーバーフロー処理を実装する。

除算に関して、`-2147483648/-1` を計算すると `2147483648` になるが、これは 32bit 符号付き整数に収まらずオーバーフローする。よってこのときCPUの例外が発生してしまう。これを防ぐために以下のようにコードを修正する

### 修正前
```asm
## (%edi : int, %esi : int) -> %eax : int
my_div:
    testl %esi,%esi
    jz .L_my_div
    movl %edi,%eax
    cltd
    idivl %esi
    ret
    .L_my_div:
    call put_error
    movl $0,%eax
    ret
```
### 修正後
```asm
## (%edi : int, %esi : int) -> %eax : int
my_div:
    testl %esi,%esi   # 0除算の場合
    jz .L_my_div2
    cmpl $-2147483648,%edi
    jne .L_my_div1
    cmpl $-1,%esi   # 商がオーバーフローする場合
    jne .L_my_div1
    jmp .L_my_div2

    .L_my_div1:
    movl %edi,%eax
    cltd
    idivl %esi # 正常に計算できる
    ret

    .L_my_div2:
    call put_error
    movl $0,%eax # エラーを返す
    ret
```

if文（条件付きjump命令）を愚直に実装することで、他のオーバーフローと同様にエラーを避けている

詳細は calc2.c のコードを参照。

## calc3.c の実装

### 方針

乗算について

掛け算は、冪乗を高速に行うアルゴリズムである繰り返し2乗法と同じアイデアで行うことができる。要するに筆算である。

アルゴリズムを書く際には、
- if文をネストしない（アセンブリにしたときにコードが読みにくくなる）
    - 例えば符号判定のとき、「`x`,`y` のいずれか一方のみが負である」 という条件は、if文をネストすることでも書けるが、これを避けた
    - 競技プログラミングの定数倍高速化で用いられる手法として、この条件は `x^y < 0` で判定できるというのがある。よく `x*y < 0` も紹介されるがこれはオーバーフローが起きうる。また `*` よりも `^` の方が高速
- 追加レジスタをなるだけ使わない（高速化とcallする前処理の簡略化のため）
    - 関数の最後で符号を調整するとき、`x`,`y`は破壊的に変更されている。そのため値を退避する必要があるのだが、`x`,`y` の両方を退避するのではなく、`x^y` の1つのみを保持するようにしている


に留意した。
### 疑似コード
```c
int mul(int x, int y){
    int s = 0;
    // x,y のいずれか一方のみが負であるとき、sの値が負になる
    s ^= x;
    s ^= y;
    
    // 絶対値を取る
    if (x < 0){
        x = -x;
    }
    if (y < 0){
        y = -y;
    }

    // 本質部分
    int ret = 0;
    int tmp = x;
    while (y > 0){
        if (y & 1){
            ret += tmp;
        }
        tmp <<= 1;
        y >>= 1;
    }
    
    // 符号を反映する
    if (s < 0){
        ret = -ret;
    }

    return ret;
}
```
これをアセンブリで実装する。

アセンブリでの実装は「calc3.c の `static void emit_my_mul` 」を参照

while文は、jmp命令で無限ループを作り、さらにbreakするif文を書くことで実装できる。

除算に関しては、以下のように筆算を実装する。

### 疑似コード
```c
int div(int x, int y){
    if (y == 0 || (x == INT_MIN && y == -1)){
        hasError = true;
        return 0;
    } else {
        // 符号の処理
        int s = 0;
        s ^= x;
        s ^= y;
        if (x < 0){
            x = -x;
        }
        if (y < 0){
            y = -y;
        }

        // いい感じに除数を上の方にもっていく
        int ret = 0;
        int tmp = 1;
        while((x>>1) >= y){
            y  <<= 1;
            tmp <<= 1;
        }

        // 引けるなら引く、を上の桁から繰り返す
        while(tmp > 0){
            if(x >= y){
                x -= y;
                ret += tmp;
            }
            y >>= 1;
            tmp >>= 1;
        }

        // 符号処理
        if (s < 0){
            ret = -ret;
        }
        return ret;
    }
}
```

1回目の while 文 の意味は、「$y2^k \le x$ となる最大の $k$ を探す」である。すなわち十分大きいところまで $y$ を桁上げするということである。しかし、これを愚直に数式にすると $2y \le x$ の $2y$ がオーバーフローする可能性がある。ここで、
$$2y \le x \iff y \le \frac{x}{2} \iff y \le \left\lfloor\frac{x}{2}\right\rfloor$$
であることを使って、つまり `while((x>>1)>=y)` で求めている。

また、`INT_MIN/1` などを計算するときに、`INT_MIN` の絶対値が取れない問題は、-1倍した後の数値を符号なし整数だと解釈することで解決している。

アセンブリでの実装は「calc3.c の `static void emit_my_div` 」を参照

以上より、calc2.cの実装を先の方法で修正することで実装できる。

全体は calc3.c のコードを参照。

## calc4.c （拡張課題）

### はじめに

私は †[競技プログラマー](https://atcoder.jp/users/n_bitand_n_per_3)† である（唐突）

なので AtCoder の問題が解ける言語を作ろうと思った

今回の授業でアセンブリを学んだので、c-like な簡易言語のコンパイラを作るということをしてみよう

まず、以下のようなコードを書くことを目標とする。

```n4
main(){
    input N;
    input K;
    list arr[300001];
    
    for(N){
        input tmp;
        if (tmp <= 300000){
            arr[tmp] = 1;
        }
    }
    
    int x;
    for(K){
        if(arr[x] == 0){
            break;
        }
        x = x + 1;
    }
    
    print x;
}
```

基本的に必要になるものとして、
- 64bit整数の変数
- 四則演算
- for文 / if文 / break文
- 固定長配列

ここで、for は C-style for 文ではなく、ループ開始前に決まった回数しかループに用いることができない

逆に、これらの機能があれば、多くの AtCoder の問題は解くことができる（と思われる）。

#### 要件定義
以下がBNFである。
```bnf
<program>          ::= "main" "(" ")" <block>

<block>            ::= "{" <statements> "}"
<statements>       ::= ε | <statement> <statements>
<statement>        ::= <simple-statement> ";"
                     | <if-statement>
                     | <for-statement>

<simple-statement> ::= "int" <identifier>
                     | "list" <identifier> "[" <positive-number> "]"
                     | "input" <identifier>
                     | "inputc" <identifier>
                     | "print" <expression>
                     | "printc" <expression>
                     | <lvalue> "=" <expression>
                     | "break"

<lvalue>           ::= <identifier> | <identifier> "[" <expression> "]"
<if-statement>     ::= "if" "(" <expression> ")" <block> <else-part>
<else-part>        ::= ε | "else" <block>
<for-statement>    ::= "for" "(" <expression> ")" <block>

<expression>       ::= <sum> <comparison-tail>
<comparison-tail>  ::= ε | <comparison-op> <sum> <comparison-tail>
<comparison-op>    ::= "==" | "!=" | "<" | ">" | "<=" | ">="

<sum>              ::= <term> <sum-tail>
<sum-tail>         ::= ε | <add-op> <term> <sum-tail>
<add-op>           ::= "+" | "-"

<term>             ::= <factor> <term-tail>
<term-tail>        ::= ε | <mul-op> <factor> <term-tail>
<mul-op>           ::= "*" | "/" | "%"

<factor>           ::= <number>
                     | <identifier> <index-part>
                     | "(" <expression> ")"
                     | "-" <factor>
<index-part>       ::= ε | "[" <expression> "]"
<positive-number>  ::= <number>
```

こだわりPointsは以下の通り
- 固定長配列のみを許すので、`.bss` セクションを適当に使うことで実装できる
- c-likeに `;` を式の最後に置くことで
- for文はループ前にループ回数が決まっている必要がある
    - 計算機科学的に for と while は違うものだろ！という主張である。
    - もっと雑に言えば、この言語で無限ループを書くことはできない。
- `print` / `input` という（Pythonista の私にとって）直感的な名前で入出力ができる


#### 実装
再帰降下パーサーを用いて実装する。降下と同時に適切にアセンブリを吐くことで実装ができる。

また、使われる変数はコードを読む段階で記録し、アセンブリの最後にまとめて `.data` セクションに書き込む。

コンパイラの実装は calc4.c の `static void emit_my_mul` を参照

- for 文に関して
    - スタックにループ回数を積む
    - スタックのトップが0になるまで jnz 命令で文の最初に戻るだけ

- if 文に関して
    - `if()` の括弧の中が 0 なら、 jz で if 文の中身の処理を飛ばす
- break 文に関して
    - 再帰関数で、「直近のfor文は何か？」を持ちながら再帰することでできる。実装では引数を増やしたくなかったので、 `block_end_label` というグローバル変数に記録する方法にしている
- 配列に関して
    - 長さ×8 byte のbssセクションを持つ
    - `[]` アクセスなども適当に実装する。`(%rax,%rdx,8)` みたいな便利なアセンブリを知った

#### 運用

コンパイラはアセンブリを出力する。AtCoderでは AT&T 方式のアセンブリしか出力できなのだが、cまたはcppのインラインアセンブリに埋め込むことで、これを提出することができる。

このための変換に codes/gas_to_cpp.py を codex に書かせた。

実際にこの言語を使って問題を解くことができた。

A：https://atcoder.jp/contests/abc290/submissions/80946577

B：https://atcoder.jp/contests/abc290/submissions/80945774

C：https://atcoder.jp/contests/abc290/submissions/80946663

具体的なハックとして、
- 配列は制約の最大長を取ればよい
- 無限ループは for(INT_MAX) を使えばよい
- while文は 無限ループ + break でよい

などを用いることで、普通のプログラミング言語と似たようなことができる。

## 全体を通しての感想

アセンブリを手書きすることで様々な知見を得られた。特に面白かった部分として、
- アセンブリ上で変数も関数も、同じ「ラベル」で管理されている
    - どちらもメモリ上の番地でしかなく、どうやって操作するかによって決まる
    - なので、`main(%rip)` で main の関数が置かれてある番地を取得できる。よって 「`leaq main(%rip),0x8(rbp)` をmain関数の中で実行することで、main関数がretされると再びmain関数が実行される」という *†効率的な再帰†* ができる
- `addl` の `l` の部分はなくても自動で補完される
    - 型推論（のようなもの）がアセンブリにもあるのだなと驚いた
- `div` 命令は 「複数のレジスタを結合して2つに見なして計算する」
    - そうすることで商とあまりを返す幅も得られる、という運用は効率的だと思った
    - また、商とあまりを同時に取得するPythonの関数 `divmod` などは、一回の除算で両方の値を取ってきているから早いのか、と仕組みを理解できた
- 高級言語とアセンブリの接続が分かった
    - 競技プログラミングで、たまに高速化のためのアセンブリ埋め込みの話がある。今回の勉強でそれがより理解できた。
- C言語書くのつらい
    - 前期の創造演習では、c++で、形式証明のための自作言語のコンパイラを書いたのだが、AIの補助を回しながら1週間ほどかかってしまった
    - 今回もっと不慣れなC言語でコンパイラを書いたので死ぬほど頭が混乱した、String型ないとか正気かよ...
- 自作言語でAtCoderするの楽しい
    - 流行ってほしいですね

## 付録
### 改善点
- 除算の実装に関して、レポート執筆時に気づいたのだが、十分に桁上げする部分は、
    ```
    while(x > 0){
        x >>= 1;
        y <<= 1;
        tmp <<= 1;
    }
    ```
    という方法でも良いことに気づいた。あるいは `x` に依存せずに `y` の最上位ビットが立つまで右シフトを繰り返す、という方針でも良かっただろう。これらの方が使用するレジスタも少なくすんだであろう。
- ヘッダーの部分に、アセンブリとして使っていない関数がある。消せばよかった。
### 助言や参考文献
- [授業資料]()
    - 自明な参考文献
    - 表としてまとまっている部分が多く参照するときに助かった
    - 巻末に「引数一覧」「レジスタの別名一覧」「スタックフレームの構築方法」「callee-saved, caller-saved レジスタの一覧」とかの一覧表があると実際にアセンブリを書く時に役立つのにな...と思った。追加よろしくお願います。
- [定数倍高速化の技術](https://speakerdeck.com/tatyam_prime/ding-shu-bei-gao-su-hua-noji-shu)
    - コンピュータの低級な部分から競技プログラミングまでの部分が書かれている
    - 他にもCPU命令の速度など
    - コードの高速化を気にしながら書く時に参考にした
- 友人からの助言
    - `negl` という符号反転ができる命令があること
    - 除算で筆算をするときに、INT_MIN の絶対値は符号なし整数として計算することで正常に求めることができる、という実装方法
- chatGPTからの助言
    - 最適化オプションを書けたときにコンパイラが吐くコード
        - 特に、コールスタックを使わない関数(`call`命令)の使い方
    - `printf` ではなくて `fputs` を使うことで `%%` と書かなくて良くなる
    

### calc0.c のコード
ファイルとして提出していいものに含まれていなかったため、レポート末尾に添付した。

```c
#include <stdio.h>
#include <stdbool.h>


// グローバル変数
bool hasError = false;
char last_op = '+';
char op = '+';
int memory = 0; // メモリーの値
int res = 0; // レジスタの値
int cur = 0; // 今入力されている値
int states = 0; // 0:数字入力状態, 1:演算子入力状態

// 関数定義
bool is_memory(char x){
    return x == 'C' || x == 'R' || x == 'P' || x == 'M';
}

bool is_number(char x){
    return x == 'S' || ('0' <= x && x <= '9');
}

bool is_operand(char x){
    return x == '+' || x == '-' || x == '*' || x == '/';
}

bool is_equal(char x){
    return x == '=';
}

void reset(){
    cur = 0;
    op = '+';
}


int add(int x, int y){
    int ret;
    bool overflow = __builtin_add_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int sub(int x, int y){
    int ret;
    bool overflow = __builtin_sub_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int mul(int x, int y){
    int ret;
    bool overflow = __builtin_mul_overflow(x,y,&ret);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int div(int x, int y){
    int ret = (y != 0 ? x/y : 0);
    bool overflow = (y == 0);
    if (overflow){
        hasError = true;
        return 0;
    } else {
        return ret;
    }
}
int eval(char cmd, int x, int y){
    switch (cmd)
    {
    case '+':
        return add(x,y);
    case '-':
        return sub(x,y);
    case '*':
        return mul(x,y);
    case '/':
        return div(x,y);
    default:
        hasError = true;
        return 0;
    }
}

void op_memory(char cmd){
    switch (cmd)
    {
    case 'R':
        res = memory;
        break;
    case 'C':
        memory = 0;
        break;
    case 'P':
        memory = add(memory,eval(last_op,res,cur));
        last_op = op;
        reset();
        res=0;
        break;
    case 'M':
        memory = sub(memory,eval(last_op,res,cur));
        last_op = op;
        reset();
        res=0;
        break;
    }

}


int concat(int x, char c){
    if (c == 'S'){
        return mul(x,-1);
    } else {
        int d = c - '0';
        if (x >= 0){
            return add(mul(x,10),d);
        } else {
            return sub(mul(x,10),d);
        }
    }
}

void display(){
    if (hasError){
        printf("E\n");
    } else {
        printf("%d\n", res);
    }
}

// ほんへ
int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("don't match arguments.\n");
        return 1;
    }
    char *p = argv[1];


    while (true) {
        if (*p == '\0'){break;}
        char cmd = *p++;
        if (states == 0){
            if(is_operand(cmd)){
                op = cmd;
                states = 1;
            } else if (is_number(cmd)){
                cur = concat(cur,cmd);
            } else if (is_memory(cmd)){
                op_memory(cmd);
            } else if (is_equal(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
            }
        } else {
            if(is_operand(cmd)){
                op = cmd;
            } else if (is_number(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
                cur = concat(cur,cmd);
                states = 0;
            } else if (is_memory(cmd)){
                op_memory(cmd);
            } else if (is_equal(cmd)){
                res = eval(last_op,res,cur);
                last_op = op;
                reset();
            }
        }
    }

    // 最後に内部状態（？、レジスタの状態？）を出力
    display();
}

```










