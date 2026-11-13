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

##  (%dil : char) -> %al : bool
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

##  (%dil : char) -> %al : bool
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

##  (%dil : char) -> %al : bool
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

##  (%dil : char) -> %al : bool
is_equal:
    cmpb $'=', %dil
    setz %al
    ret

##  () -> 
reset:
    movl $0,cur(%rip)
    movl $'+',op(%rip)
    ret

## (%edi : int, %esi : int) -> %eax
build_in_add:
    movl %edi,%eax
    addl %esi,%eax
    jo .L_build_in_add
    ret
    .L_build_in_add:
    movb $1,hasError(%rip)
    ret

## (%edi : int, %esi : int) -> %eax
build_in_sub:
    movl %edi,%eax
    subl %esi,%eax
    jo .L_build_in_sub
    ret
    .L_build_in_sub:
    movb $1,hasError(%rip)
    ret

## (%edi : int, %esi : int) -> %eax
build_in_mul:
    movl %edi,%eax
    imul %esi,%eax
    jo .L_build_in_mul
    ret
    .L_build_in_mul:
    movb $1,hasError(%rip)
    ret

## (%edi : int, %esi : int) -> %eax
build_in_div:
    testl %esi,%esi
    jz .L_build_in_div
    movl %edi,%eax
    cqto
    idiv %esi
    ret
    .L_build_in_div:
    movb $1,hasError(%rip)
    movl $0,%eax
    ret

.global main
.text
main:
    # main関数のスタックフレームの構築
    pushq   %rbp
    movq    %rsp, %rbp

    # ここからadd
        movl    $13,%edi
        movl    $7,%esi
        call    build_in_div

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
