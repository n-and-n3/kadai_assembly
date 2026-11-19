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
グローバル変数 `hasError` を持ち、どこかの計算で一度でもエラーが発生したら `hasError` を呼び出す

calc0.c で実装したコードは、このレポートの最後の章に埋め込んである。

## calc1.c の実装

calc0 において、入力によって吐くべきアセンブリのコードが依存するのはは main 関数の部分のみであるようにする。

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

というコードが書ける。

授業で扱った範囲では、関数はコールスタックを弄る操作が必ず行われていた。しかし、実際にgccの吐くコードを見てみると、例えば、
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
に変換される。chatGPTに聞いてみると、「スタックフレームを作ることは必須ではなく、単純な処理で少数のレジスタで実装できる場合は、スタックフレームの構築は不要である」と教わった。実際に、`%rbp` や `%rsp` が変更されないしかつスタックにも触らないのであれば、確かにこの操作は不要であると分かる。自分はこれを参考に、関数にはスタックフレームを作るコードを付けていない。

以上のことを、他のすべての関数に対しても行う。`calc1.c` の**4行目から246行目までに埋め込まれたアセンブリコードは、手書きでアセンブリを書いている**。

$$$$

今思えばgccを使えば良かったののではないかと後悔している。

詳細は calc1.c のコードを参照。



## calc2.c
加算・乗算・乗算のオーバーフローは、calc1の実装と同時に実装し終えている。除算のオーバーフローを修正する。

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
    testl %esi,%esi
    jz .L_my_div2
    cmpl $-2147483648,%edi
    jne .L_my_div1
    cmpl $-1,%esi
    jne .L_my_div1
    jmp .L_my_div2

    .L_my_div1:
    movl %edi,%eax
    cltd
    idivl %esi
    ret

    .L_my_div2:
    call put_error
    movl $0,%eax
    ret
```

if文（条件付きjump命令）を愚直に実装することで、他のオーバーフローと同様にエラーを避けている

詳細は calc2.c のコードを参照。

## calc3.c の実装

掛け算は、冪乗を高速に行うアルゴリズムである繰り返し2乗法と同じアイデアで行うことができる。要するに筆算である。

アルゴリズムを書く際には、
- if文をネストしない（アセンブリにしたときにコードが読みにくくなる）
    - 例えば符号判定のとき、「`x`,`y` のいずれか一方のみが負である」 という条件は、if文をネストすることでも書けるが、これを避けた
    - 競技プログラミングの定数倍高速化で用いられる手法として、この条件は `x^y < 0` で判定できるというのがある。よく `x*y < 0` も紹介されるがこれはオーバーフローが起きうる。また `*` よりも `^` の方が高速
- 追加レジスタをなるだけ使わない（高速化とcallする前処理の簡略化のため）
    - 関数の最後で符号を調整するとき、`x`,`y`は破壊的に変更されている。そのため値を退避する必要があるのだが、`x`,`y` の両方を退避するのではなく、`x^y` の1つのみを保持するようにしている

以下はアルゴリズムのC言語のコードである
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

```
## (%edi : int, %esi : int) -> %eax : int
my_mul:
    movl %edi,%edx
    xorl %esi,%edx

    cmp $0,%edi
    jge .L_my_mul_x_positive
        negl %edi
    .L_my_mul_x_positive:

    cmp $0,%esi
    jge .L_my_mul_y_positive
        negl %esi
    .L_my_mul_y_positive:

    movl $0,%eax

    .L_my_mul_loop: # while の開始
    testl %esi, %esi
    jz .L_my_mul_end_loop # break

        testl $1, %esi
        jz .L_my_mul_skip_add
            addl %edi, %eax
        .L_my_mul_skip_add:

        shll $1, %edi
        shrl $1, %esi

    jmp .L_my_mul_loop # ここで強制的に最初に戻る
    .L_my_mul_end_loop: # ループの終わり

    cmp $0,%edx
    jge .L_skip_neg
        negl %eax
    .L_skip_neg:

    ret
```

while文は、jmp命令で無限ループを作り、さらにbreakするif文を書くことで実装できる。




除算に関しては、以下のように筆算を実装する。
```c
int div(int x, int y){
    if (y == 0){
        hasError = true;
        return 0;
    } else {
        int s = 0;
        s ^= x;
        s ^= y;
        if (x < 0){
            x = -x;
        }
        if (y < 0){
            y = -y;
        }
        int ret = 0;
        int tmp = 1;
        while((x>>1) >= y){ // 最上位の桁まで持ってくる
            y  <<= 1;
            tmp <<= 1;
        }
        while(tmp > 0){
            if(x >= y){
                x -= y;
                ret += tmp;
            }
            y >>= 1;
            tmp >>= 1;
        }
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

アセンブリに直すと以下のようになる。
```asm
## (%edi : int, %esi : int) -> %eax : int
my_div:
    testl %esi,%esi
    jz .L_my_div2
    cmpl $-2147483648,%edi
    jne .L_my_div1
    cmpl $-1,%esi
    jne .L_my_div1

    # エラー処理
    .L_my_div2:
    call put_error
    movl $0,%eax
    ret

    # 除算処理の本編 # %edi = x, %esi = y
    .L_my_div1:
    movl %edi,%edx
    xorl %esi,%edx

    cmp $0,%edi
    jge .L_my_div_x_positive
        negl %edi
    .L_my_div_x_positive:

    cmp $0,%esi
    jge .L_my_div_y_positive
        negl %esi
    .L_my_div_y_positive:

    movl %edi,%ecx
    shrl $1, %ecx  # %ecx  = %edi / 2 = x >> 1
    movl $0,%eax
    movl $1,%r8d

    .L_my_div_loop1: # while の開始
    cmp %ecx, %esi
    ja .L_my_div_end_loop # break

        shll $1, %r8d
        shll $1, %esi

    jmp .L_my_div_loop1 # ここで強制的に最初に戻る
    .L_my_div_end_loop: # ループの終わり

    .L_my_div_loop2: # while の開始
    cmp $0,%r8d
    jz .L_my_div_end_loop2 # break
    
        cmp %esi, %edi
        jb .L_my_div_skip_sub
            subl %esi, %edi
            addl %r8d, %eax
        .L_my_div_skip_sub:

        shrl $1, %esi
        shrl $1, %r8d
    
    jmp .L_my_div_loop2 # ここで強制的に最初に戻る
    .L_my_div_end_loop2: # ループの終わり

    cmp $0,%edx
    jge .L_my_div_skip_neg
        negl %eax
    .L_my_div_skip_neg:
    ret
```

これらのコードによって、calc2.cの実装を修正することで実装できる。

詳細は calc3.c のコードを参照。

## 全体を通しての感想

この文章が採点時に読まれているということは、私は自作言語を完成させられなかった、ということだろう。

このレポートは仮提出である。
自作言語を作成するつもりであるが、課題の最終提出まで30時間を切っており、一旦仮提出をすることにした。

アセンブリを手書きすることで様々な知見を得られた。特に面白かった部分として、
- アセンブリ上で変数も関数も、同じ「ラベル」で管理されている
    - どちらもメモリ上の番地でしかなく、どうやって操作するかによって決まる
    - なので、`main(%rip)` で main の関数が置かれてある番地を取得できる。よって 「`leaq main(%rip),0x8(rbp)` をmain関数の中で実行することで、main関数がretされると再びmain関数が実行される」という *†効率的な再帰†* ができる

また、
- `addl` の `l` の部分はなくても自動で補完される
    - 型推論（のようなもの）がアセンブリにもあるのだなと驚いた
- `div` 命令は 「複数のレジスタを結合して2つに見なして計算する」
    - そうすることで商とあまりを返す幅も得られる、という運用は効率的だと思った
    - また、商とあまりを同時に取得するPythonの関数 `divmod` などは、一回の除算で両方の値を取ってきているから早いのか、と仕組みを理解できた

アセンブリやコンピュータアーキテクチャの仕様を理解でき、普段触っているコンピュータ言語との接続を強固なものにできた

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
    という方法でも良いことに気づいた。あるいは `x` に依存せずに `y` の最上位ビットが立つまで右シフトを繰り返す、という方針でも良かっただろう。
- ヘッダーの部分に、
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










