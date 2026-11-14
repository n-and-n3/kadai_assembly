global main
extern scanf
extern printf

section .rodata
input_format: db '%10s', 0
odd_text:     db 'odd', 10, 0
even_text:    db 'even', 10, 0

section .bss
s: resb 11

section .text
main:
    push rbp
    mov rbp, rsp

    lea rdi, [rel input_format]
    lea rsi, [rel s]
    xor eax, eax
    call scanf wrt ..plt

    lea rsi, [rel s]
    xor edx, edx
.loop:
    mov al, [rsi]
    test al, al
    jz .done
    cmp al, 'o'
    jne .next
    xor edx, 1
.next:
    inc rsi
    jmp .loop

.done:
    lea rdi, [rel even_text]
    test edx, edx
    jz .print
    lea rdi, [rel odd_text]
.print:
    xor eax, eax
    call printf wrt ..plt

    xor eax, eax
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
