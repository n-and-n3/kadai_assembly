# Scalar x86-64 System V kernels for the one-hot packed32 experiment.
# Valid input lanes are 01=-1, 00=0, 10=+1. No function accepts 11.
.text

.globl asm_trit_neg
.type asm_trit_neg, @function
asm_trit_neg:
    movabsq $0x5555555555555555, %rdx
    movq %rdi, %rax
    andq %rdx, %rax
    shlq $1, %rax
    shrq $1, %rdi
    andq %rdx, %rdi
    orq %rdi, %rax
    ret
.size asm_trit_neg, .-asm_trit_neg

.globl asm_trit_min
.type asm_trit_min, @function
asm_trit_min:
    movabsq $0x5555555555555555, %rdx
    movq %rdi, %rax
    andq %rsi, %rdi
    xorq %rsi, %rax
    andq %rdx, %rax
    orq %rdi, %rax
    ret
.size asm_trit_min, .-asm_trit_min

.globl asm_trit_max
.type asm_trit_max, @function
asm_trit_max:
    movabsq $0xaaaaaaaaaaaaaaaa, %rdx
    movq %rdi, %rax
    andq %rsi, %rdi
    xorq %rsi, %rax
    andq %rdx, %rax
    orq %rdi, %rax
    ret
.size asm_trit_max, .-asm_trit_max

.globl asm_trit_product
.type asm_trit_product, @function
asm_trit_product:
    movq %rdi, %rdx
    andq %rsi, %rdx
    orq %rsi, %rdi
    leaq (%rdx,%rdx), %rax
    orq %rdx, %rax
    movabsq $0xaaaaaaaaaaaaaaaa, %rcx
    andq %rcx, %rax
    movq %rdi, %rdx
    shrq $1, %rdx
    andq %rdi, %rdx
    movabsq $0x5555555555555555, %rcx
    andq %rcx, %rdx
    orq %rdx, %rax
    ret
.size asm_trit_product, .-asm_trit_product

.globl asm_trit_cycle_inverse
.type asm_trit_cycle_inverse, @function
asm_trit_cycle_inverse:
    # -1 -> +1, 0 -> -1, +1 -> 0.
    movabsq $0x5555555555555555, %rdx
    movq %rdi, %rax
    shrq $1, %rax
    orq %rdi, %rax
    notq %rax
    andq %rdx, %rax
    shlq $1, %rdi
    movabsq $0xaaaaaaaaaaaaaaaa, %rdx
    andq %rdx, %rdi
    orq %rdi, %rax
    ret
.size asm_trit_cycle_inverse, .-asm_trit_cycle_inverse

.globl asm_trit_plus_count_raw
.type asm_trit_plus_count_raw, @function
asm_trit_plus_count_raw:
    movabsq $0xaaaaaaaaaaaaaaaa, %rax
    andq %rdi, %rax
    popcntq %rax, %rax
    ret
.size asm_trit_plus_count_raw, .-asm_trit_plus_count_raw

.section .note.GNU-stack,"",@progbits
