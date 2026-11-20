.section .rodata
.LC0:
    .string "%ld\n"
.text
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movabsq $1, %rax
    pushq %rax
    popq %rax
    movq %rax, x(%rip)
    movq x(%rip), %rax
    pushq %rax
    movq x(%rip), %rax
    pushq %rax
    movabsq $1, %rax
    pushq %rax
    popq %rdi
    popq %rax
    addq %rdi, %rax
    pushq %rax
    popq %rdi
    popq %rax
    imulq %rdi, %rax
    pushq %rax
    movabsq $2, %rax
    pushq %rax
    popq %rdi
    popq %rax
    cqto
    idivq %rdi
    pushq %rax
    popq %rax
    movq %rax, y(%rip)
    movq y(%rip), %rax
    pushq %rax
    popq %rsi
    leaq .LC0(%rip), %rdi
    movq $0, %rax
    call printf
    movq $0, %rax
    popq %rbp
    ret

.section .bss
    .balign 8
x:
    .zero 8
    .balign 8
y:
    .zero 8