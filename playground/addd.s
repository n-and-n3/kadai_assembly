.bss
.balign 4
x:
    .zero 4
y:
    .zero 4

.global main
.text
main:
    # main関数のスタックフレームの構築
    pushq   %rbp
    movq    %rsp, %rbp

    # ここからadd5
    movl    a(%rip),%edi
    call    add5

    # ここからprintf
    # 引数のセット
    leaq    .LC0(%rip), %rdi   # フォーマット文字列のアドレスを与える
    movl    %eax, %esi
    movl    $0, %eax

    # 関数呼び出し
    call    printf

    # main関数の戻り値 0 を設定
    movl    $0, %eax

    # main関数のスタックフレームの破棄
    movq    %rbp, %rsp
    popq    %rbp

    # main関数の終了
    ret

.section .rodata
.LC0:
    .string "%d\n"
