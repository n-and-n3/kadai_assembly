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

.data
op:
    .byte '+'
last_op:
    .byte '+'

.text

put_error:
    movb $1,hasError(%rip)
    ret

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
    movb $'+',op(%rip)
    ret

## (%edi : int, %esi : int) -> %eax : int
my_add:
    movl %edi,%eax
    addl %esi,%eax
    jo .L_my_add
    ret
    .L_my_add:
    call put_error
    ret

## (%edi : int, %esi : int) -> %eax : int
my_sub:
    movl %edi,%eax
    subl %esi,%eax
    jo .L_my_sub
    ret
    .L_my_sub:
    call put_error
    ret

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
    jg .L_my_div_end_loop # break

        shll $1, %r8d
        shll $1, %esi

    jmp .L_my_div_loop1 # ここで強制的に最初に戻る
    .L_my_div_end_loop: # ループの終わり

    .L_my_div_loop2: # while の開始
    cmp $0,%r8d
    jz .L_my_div_end_loop2 # break
    
        cmp %esi, %edi
        jl .L_my_div_skip_sub
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

## (%edi : int, %esi : int, %dl : char) -> %eax : int
eval:
    cmpb $'+',%dl
    je .L_eval1
    cmpb $'-',%dl
    je .L_eval2
    cmpb $'*',%dl
    je .L_eval3
    cmpb $'/',%dl
    je .L_eval4

    call put_error
    ret

    .L_eval1:
        call my_add
    ret
    .L_eval2:
        call my_sub
    ret
    .L_eval3:
        call my_mul
    ret
    .L_eval4:
        call my_div
    ret

## (%dil : char) -> 
op_memory:
    cmpb $'R',%dil
    je .L_op_memory1
    cmpb $'C',%dil
    je .L_op_memory2
    cmpb $'P',%dil
    je .L_op_memory3
    cmpb $'M',%dil
    je .L_op_memory4

    call put_error
    ret

    .L_op_memory1:
        movl memory(%rip), %eax
        movl %eax, res(%rip)
    ret
    .L_op_memory2:
        movl $0, memory(%rip)
    ret
    .L_op_memory3:
        movl res(%rip),%edi
        movl cur(%rip),%esi
        movb last_op(%rip),%dl
        call eval
        movb op(%rip),%dl
        movb %dl,last_op(%rip)
        movl memory(%rip),%edi
        movl %eax,%esi
        call my_add
        movl %eax,memory(%rip)
        call reset
        movl $0, res(%rip)
    ret
    .L_op_memory4:
        movl res(%rip),%edi
        movl cur(%rip),%esi
        movb last_op(%rip),%dl
        call eval
        movb op(%rip),%dl
        movb %dl,last_op(%rip)
        movl memory(%rip),%edi
        movl %eax,%esi
        call my_sub
        movl %eax,memory(%rip)
        call reset
        movl $0, res(%rip)
    ret

## (%edi : int, %sil : char) -> %eax : int
concat:
    cmpb $'S',%sil
    je .L_concat1
    cmpl $0,%edi
    jge .L_concat2
    jmp .L_concat3

    .L_concat1:
        movl $-1,%esi
        call my_mul
    ret
    .L_concat2:
        movzbl %sil, %ebx
        subl $'0', %ebx
        movl $10,%esi
        call my_mul
        movl %eax,%edi
        movl %ebx,%esi
        call my_add
    ret
    .L_concat3:
        movzbl %sil, %ebx
        subl $'0', %ebx
        movl $10,%esi
        call my_mul
        movl %eax,%edi
        movl %ebx,%esi
        call my_sub
    ret

## () -> 
display:
    leaq .LC1(%rip), %rdi
    cmpb $0, hasError(%rip)
    je .L_display
        leaq .LC0(%rip), %rdi
    .L_display:
    movl    res(%rip), %esi
    movl    $0, %eax
    call    printf
    ret


.section .rodata
.LC0:
    .string "E\n"
.LC1:
    .string "%d\n"

.global main
.text
main:
   pushq   %rbp
   movq    %rsp, %rbp
# 1
    movl cur(%rip),%edi
    movb $'1',%sil
    call concat
    movl %eax,cur(%rip)

# /
    movb $'/',op(%rip)

# 1
    movl res(%rip),%edi
    movl cur(%rip),%esi
    movb last_op(%rip),%dl
    call eval
    movl %eax,res(%rip)

    movb op(%rip),%dl
    movb %dl,last_op(%rip)

    call reset

    movl cur(%rip),%edi
    movb $'1',%sil
    call concat
    movl %eax,cur(%rip)

# =
    movl res(%rip),%edi
    movl cur(%rip),%esi
    movb last_op(%rip),%dl
    call eval
    movl %eax,res(%rip)

    movb op(%rip),%dl
    movb %dl,last_op(%rip)

    call reset

    call display
    movl    $0, %eax
    movq    %rbp, %rsp
    popq    %rbp
    ret
