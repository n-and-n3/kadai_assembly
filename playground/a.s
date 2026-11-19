.section .rodata
.LC0:
    .string "%d\n"
.section .data
x:
    .long 42
.text
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl x(%rip), %eax
    pushq %rax
    popq %rsi
    leaq .LC0(%rip), %rdi
    movl $0, %eax
    call printf
    movl $0, %eax
    popq %rbp
    ret
