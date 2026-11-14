
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
    movb $'+',op(%rip)
    ret

## (%edi : int, %esi : int) -> %eax
my_add:
    movl %edi,%eax
    addl %esi,%eax
    jo .L_my_add
    ret
    .L_my_add:
    call put_error
    ret

## (%edi : int, %esi : int) -> %eax
my_sub:
    movl %edi,%eax
    subl %esi,%eax
    jo .L_my_sub
    ret
    .L_my_sub:
    call put_error
    ret

## (%edi : int, %esi : int) -> %eax
my_mul:
    movl %edi,%eax
    imul %esi,%eax
    jo .L_my_mul
    ret
    .L_my_mul:
    call put_error
    ret

## (%edi : int, %esi : int) -> %eax
my_div:
    testl %esi,%esi
    jz .L_my_div
    movl %edi,%eax
    cqto
    idiv %esi
    ret
    .L_my_div:
    call put_error
    movl $0,%eax
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

## (%edi : char) -> 
op_memory:
    cmpb $'R',%dl
    je .L_op_memory1
    cmpb $'C',%dl
    je .L_op_memory2
    cmpb $'P',%dl
    je .L_op_memory3
    cmpb $'M',%dl
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
        movb $'+',%dl
        call eval
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
        movb $'-',%dl
        call eval
        movl memory(%rip),%edi
        movl %eax,%esi
        call my_add
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
    movb $49,%sil
    call concat
    movl %eax,cur(%rip)

# 2
    movl cur(%rip),%edi
    movb $50,%sil
    call concat
    movl %eax,cur(%rip)

# S
    movl cur(%rip),%edi
    movb $83,%sil
    call concat
    movl %eax,cur(%rip)

# 3
    movl cur(%rip),%edi
    movb $51,%sil
    call concat
    movl %eax,cur(%rip)

# 4
    movl cur(%rip),%edi
    movb $52,%sil
    call concat
    movl %eax,cur(%rip)

# +
    movb $'+',op(%rip)

# 5
    movl res(%rip),%edi
    movl cur(%rip),%esi
    movb last_op(%rip),%dl
    call eval
    movl %eax,res(%rip)
    movb op(%rip),%dl
    movb %dl,last_op(%rip)
    call reset
    movl cur(%rip),%edi
    movb $53,%sil
    call concat
    movl %eax,cur(%rip)

# 6
    movl cur(%rip),%edi
    movb $54,%sil
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