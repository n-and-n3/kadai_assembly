extern "C" void asm_solve();

asm(R"ASM(
    .section .rodata
.Linput_format:
    .asciz "%10s"
.Lodd_text:
    .asciz "odd\n"
.Leven_text:
    .asciz "even\n"

    .bss
asm_input_buf:
    .zero 11

    .text
    .globl asm_solve
asm_solve:
    pushq %rbp
    movq %rsp, %rbp

    leaq .Linput_format(%rip), %rdi
    leaq asm_input_buf(%rip), %rsi
    xorl %eax, %eax
    call scanf@PLT

    leaq asm_input_buf(%rip), %rsi
    xorl %edx, %edx
.Lloop:
    movb (%rsi), %al
    testb %al, %al
    jz .Ldone
    cmpb $'o', %al
    jne .Lnext
    xorl $1, %edx
.Lnext:
    incq %rsi
    jmp .Lloop

.Ldone:
    leaq .Leven_text(%rip), %rdi
    testl %edx, %edx
    jz .Lprint
    leaq .Lodd_text(%rip), %rdi
.Lprint:
    xorl %eax, %eax
    call printf@PLT

    popq %rbp
    ret
)ASM");

int main() {
    asm_solve();
    return 0;
}
