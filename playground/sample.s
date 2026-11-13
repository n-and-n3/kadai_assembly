# 12S34+56

.data
a:
    .long 114514

.text
add5:
    movl %edi, %eax
    addl $5, %eax
    ret
col:
    movl %edi, %eax
    cmpl $1, %eax
    jz tmp1
        # 偶数の場合
        movl %edi, %eax
        sarl $1, %eax
        ret
    tmp1:
        # 奇数の場合
        movl %edi, %eax
        imull $3, %eax
        addl $1, %eax
        ret

.global main
.text
main:
    # main関数のスタックフレームの構築
    pushq   %rbp
    movq    %rsp, %rbp

    # ここから while 文
    loop:
        # ここからadd5
            movl    a(%rip),%edi
            call    col
            movl    %eax, a(%rip)

        # ここからprintf
            # 引数のセット
            leaq    .LC0(%rip), %rdi   # フォーマット文字列のアドレスを与える
            movl    %eax, %esi
            movl    $0, %eax

            # 関数呼び出し
            call    printf

    # loop を抜ける処理
    cmpl $1, a(%rip)
    jne loop

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
