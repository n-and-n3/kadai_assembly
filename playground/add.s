.bss
.align 4
memory:
    .zero 4
res:
    .zero 4 
cur:
    .zero 4
hasError:
    .zero 1
states:
    .zero 1

.data
op:
    .byte '+'
last_op:
    .byte '+'

.text
is_memory:
    cmpb $'C', %dil
    je .L_is_memory
    cmpb $'R', %dil
    je .L_is_memory
    cmpb $'P', %dil
    je .L_is_memory
    cmpb $'M', %dil
    je .L_is_memory

    movb $0, %al
    ret

    .L_is_memory:
    movb $1, %al
    ret

/*
is_memory:
    andb $49, %dil
    cmpb $17, %dil
    setl %al
    ret
*/

is_number:
    cmpb $'S', %dil
    je .L_is_number1
    cmpb $'0', %dil
    jl .L_is_number0
    cmpb $'9', %dil
    jg .L_is_number0
    jmp .L_is_number1

    .L_is_number0:
    movb $0, %al
    ret
    .L_is_number1:
    movb $1, %al
    ret

is_operand:
    cmpb $'+', %dil
    je .L_is_operand
    cmpb $'-', %dil
    je .L_is_operand
    cmpb $'*', %dil
    je .L_is_operand
    cmpb $'/', %dil
    je .L_is_operand
    movb $0, %al
    ret

    .L_is_operand:
    movb $1, %al
    ret

is_equal:
    cmpb $'=', %dil
    setz %al
    ret


build_in_add:
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
