# 32 trit packed ランタイムの性能探索（2026-11-17）

## 結論と適用範囲

現時点の第一候補は **`-1=01, 0=00, +1=10` を32桁詰めた64 bit値**。この符号化ではゼロ初期化済みメモリが数値0になり、`11` は無効値として検出できる。符号化24通りを小さな論理回路モデルで総当たりすると `0=11` が1点だけ有利だが、ゼロの表現、メモリ初期化、表引き、実機命令を含めた総合的な優位は証明されない。現時点では `0=00` を採用する。

- `neg`、`min/max/product/cycle`、各桁の計数、大小比較は **packed のまま**。1ワードの演算はスカラ命令で処理する。
- 加減乗除は、まず **packed → 符号付き64 bit整数 → CPU演算 → packed** を第一候補にする。変換には小表を使う。これは今回試した32桁逐次加算と4桁チャンク加算より速かった。積の範囲判定は128 bitの中間値を使う。
- AVX2は配列を**まとめて**処理する桁演算に使う。1値を返すたびにAVX2へ移す方針は取らない。
- `__trit_neg` を公開する。全桁の `+`/`-` 交換が加法逆元であり、定数 `-1` との一般乗算より明快で速い。`cycle` と逆向きの `cycle_inverse` は別の操作とする。

これは測ったCPUと入力での**暫定的な選択**であって、全CPU・全プログラムでの最適性証明ではない。特に四則演算のオーバーフロー、ゼロ除算、実際のコンパイラが変換結果を再利用する方法は言語仕様と一緒に決める。

## 正しさと式

`E=0x5555555555555555`（各tritの下位bit）、`O=~E`（上位bit）とする。`11` を含まない入力に限る。

| 操作 | ワード並列の式 | 備考 |
| --- | --- | --- |
| 符号反転 | `((x&E)<<1) \| ((x&O)>>1)` | 全桁のbitを交換 |
| `min` | `(x&y) \| ((x^y)&E)` | 各桁 `- < 0 < +` |
| `max` | `(x&y) \| ((x^y)&O)` | 同上 |
| `product` | `s=x&y; a=x\|y; ((s\|(s<<1))&O) \| ((a&(a>>1))&E)` | 同符号が `+`、異符号が `-` |
| `plus_count` | `popcnt(x&O)` | 実験関数が返す桁数はraw整数 |
| `minus_count` | `popcnt(x&E)` | 同上 |
| `zero_count` | `32-popcnt(x)` | 有効な非ゼロtritは1 bitだけ立つ |

数値比較は `x^y` の最上位の1 bitを探し、そのbitを含む2 bit桁だけを比較する。最上位の異なるtritが数値の大小を決める。`cycle` は `-→0→+→-` とし、逆向きは `-→+→0→-` とする。

公開ABIで桁数集計の返り値を packed `trint` にするなら、`popcnt` のraw整数をさらに符号化する必要がある。上のタイミングはその復路を含まない。

ユーザーが提案した `neg(~x - E)` は**逆向きcycle**として正しい。`~x` の各有効桁は `01/10/11` なので、各桁から `01` を引くとき、2 bit境界をまたぐ借りは発生しない。これと直接bit式の両方を検証した。

加算の逐次版では各桁 `s=a_i+b_i+c_i` を計算し、`c_{i+1}=(s>1)-(s<-1)`、出力桁 `s-3c_{i+1}` とする。これが正しさの基準。加算の高速版は `pext` で各bit面を8 bitずつ抜き出し、256要素の重み表から10進値へ戻す。復路は `value+(3^32-1)/2` を基数 `3^8=6561` の4桁に分解し、6561要素の16 bit表で詰め直す。BMI2のないCPU用には、4 tritを1 byteにした256要素の復号表も実験した。

`min/max/product/cycle/neg` は各桁の真理値表と20万組の乱数、加算は20万組の全幅乱数（範囲内の和）と境界値、積・除算は20万組の小整数乱数と境界値で照合した。GAS版も同じ乱数でC++参照式と照合した。無効符号 `11` に対する結果は未定義。

## 実測

環境: Intel Core i7-1355U、WSL2（Microsoft Hyper-V）、GCC 13.3.0、`-O3 -march=native`、`taskset -c 2`。1024要素を繰り返し処理し、各測定を3回行った中央値。単位は**1 packed 値あたりのns**。バッチは結果配列への書き込みを含む。数値は日ごと・周波数・コア種別で揺れるので、おおむねの桁と順位を見る。

| 方法 | 観測した範囲 (ns/値) | 判断 |
| --- | ---: | --- |
| 逐次32桁加算 | 207–303 | 基準用のみ |
| 4桁チャンク表加算（384 KiB） | 15.6–17.9 | 復号・CPU加算方式よりやや遅い |
| `pext`+小表復号→CPU加算→小表符号化 | 13.3–16.2 | 算術の第一候補 |
| BMI2不要の小表復号→CPU加算→小表符号化 | 17.1–21.1 | 移植用候補 |
| 同じ変換経由の積／除算 | 積15–17、除算16–22 | 小整数入力で測定 |
| packed `min/max` | 約2 | ビット演算 |
| packed `product` | 約3–4 | ビット演算 |
| 最上位異桁による `<` | 約3–4 | 復号比較は約9–10 |
| 符号反転、逆cycle、計数 | 約0.7–2 | 計数関数はインライン化され得る |
| 配列 `min` スカラ／AVX2 | 約1.1–1.8／0.35–0.45 | AVX2は約3–4倍速い |
| 配列 `product` スカラ／AVX2 | 約1.6–2.1／0.52–0.90 | AVX2は約2–3倍速い |

GAS関数を外部から呼んだ同条件の測定も行った。`min` 約4 ns、`product` 約3–4 nsなどで、C++の同一翻訳単位に置いた関数との直接比較には**呼び出し先の配置、インライン化制限、コンパイラ最適化の差**が混ざる。GAS版は主に実行可能性と同値性の確認に用いる。

静的解析は `llvm-mca-18 -mcpu=alderlake -iterations=100 mca.s`。概算の `Block RThroughput` は `neg` 1.2 cycle、`min_xor` 1.0、元の `min` 1.4、`product_swar` 1.8、逆cycle 1.3。これはCPUモデルの**ループの逆スループット**の推定で、1回の命令レイテンシの和でもWSL実機の実測でもない。実際のCPUは第13世代のP/Eコア混在で、`alderlake` は近似モデル。単に各命令の公称クロック数を足して最速を決めることはできない。

## 探索したもの・まだ探索していないもの

[`encoding_search.py`](encoding_search.py) は24種類の2 bit割当を列挙し、各出力bitのNOT/AND/OR/XOR式の最小ゲート数を小さな真理値表で探索する。`0=11,-=01,+=10` の得点11、採用案 `0=00,-=01,+=10` は12。これは出力bit間の共通部分式、レジスタ、シフト、キャッシュ、初期化、ABIを無視した**粗いスクリーニング**である。`0=11` はゼロの64 bit表現が全1となり、ゼロ初期化された配列や`.bss`を直接使えない。

桁ごとの加算は、正負の繰り上がりが相殺し得るため、単純な2本の二進carry系列では正しくない。このことは、5種類から7種類の伝播・生成状態を必要とする[Douglas W. Jonesの平衡3進加算解析](https://homepage.cs.uiowa.edu/~dwjones/ternary/arith.shtml)とも整合する。SWAR型の正しい高速加算器、SAT/SMTによる命令列の最適性証明、Leanでの形式証明は今回の成果には含まない。探索対象をCPU、命令集合、許される表サイズ、オーバーフロー仕様で固定した後に行う研究課題とする。

ユーザーの[3進数Fenwick Tree記事](https://trap.jp/post/2830/)の更新・累積和ループを写した `tit` ベンチも行った。9841要素、10万回の更新／照会を半々で実行し、同じ結果・配列内容になることを確認した。通常の `int64_t` 配列は約159–199 ns/操作、packed 配列の加減算を毎回変換する版は約261–371 ns/操作だった。したがって目標のTITでは、**数値演算を続ける値や配列は通常整数のまま保持し、桁操作が必要な箇所でpackする表現選択**が有力。言語の型は1種類の `trint` のままでも、コンパイラ内部の表現を使い分けられる。これは実装が一段複雑になるため、まずpacked版を正しさの基準にして、後から表現解析を追加する。

このTIT測定は同じ単純な操作列を2方式で実行したもので、AtCoderの入出力、実問題の入力分布、言語コンパイラのコード生成、キャッシュ容量の違いは含まない。AtCoderの実行環境で、分岐・配列アクセス込みの性能を再測定してから最終化する。

## 再現方法

```sh
g++ -std=gnu++23 -O3 -Wall -Wextra -march=native -DWITH_ASM \
  playground/ternary-experiments/packed32.cpp \
  playground/ternary-experiments/kernels.s -o /tmp/packed32-asm
taskset -c 2 /tmp/packed32-asm bench
taskset -c 2 /tmp/packed32-asm tit
python playground/ternary-experiments/encoding_search.py
llvm-mca-18 -mcpu=alderlake -iterations=100 playground/ternary-experiments/mca.s
```

`-march=native` のバイナリは別CPUでの実行を保証しない。実際の配布ランタイムではCPU機能を固定するか、BMI2/AVX2の有無で実装を選ぶ。`packed32.cpp` は性能実験用であり、`assert` を含み、本番のオーバーフロー・エラー処理や提出形式をまだ備えていない。

参考: [Intelの最適化資料](https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html)、[LLVM llvm-mcaの説明](https://github.com/llvm/llvm-project/blob/main/llvm/docs/CommandGuide/llvm-mca.md)、[Tatyamの定数倍高速化資料](https://speakerdeck.com/tatyam_prime/ding-shu-bei-gao-su-hua-noji-shu)、[ビット演算トリックのスライド](https://www.slideshare.net/slideshow/slide-www/29338849)。
