# n4tive の構文（BNF）

この文法は `25B31026/calc4.c` の現在の構文解析に対応する。`ε` は空列を表す。`<identifier>` と `<number>` の字句規則は下に示す。空白・改行はトークン間に任意に置ける。

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

字句規則は次の通り。`<positive-number>` は `<number>` と同じ字句形で、値が 1 以上であることを追加で要求する。

```text
<identifier> = [A-Za-z]+
<number>     = [0-9]+
```

比較・加減算・乗除算はそれぞれ左結合で、優先順位は `-`（単項）→ `* / %` → `+ -` → 比較。比較の結果は `0` または `1`。`for(e)` は `e` を開始時に一度評価し、正の値ならその回数だけ繰り返す。カウンタはプログラムから参照できない。`break` は `for` の内部でのみ有効。

この BNF は意図した構文を記述する。現在のパーサーは、未宣言の変数や範囲外の配列添字を十分に検査せず、裸の識別子を文として受理してしまう場合もある。これらは構文として推奨しない。コメント、`while`、`++`、ユーザー定義関数は未実装。
