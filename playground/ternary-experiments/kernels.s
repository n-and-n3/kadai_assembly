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

.globl asm_trit_add_bct
.type asm_trit_add_bct, @function
asm_trit_add_bct:
    # Convert one-hot packed trits to the biased BCT encoding.
    movabsq $0x5555555555555555, %r10
    movq %rdi, %rax
    shrq $1, %rax
    notq %rax
    andq %r10, %rax
    xorq %rax, %rdi
    movq %rsi, %rax
    shrq $1, %rax
    notq %rax
    andq %r10, %rax
    xorq %rax, %rsi

    subq $8, %rsp
    call .L_unsigned_bct_add
    movq %rax, %rdi
    movabsq $0x5555555555555556, %rsi
    call .L_unsigned_bct_add
    addq $8, %rsp

    # The conversion is an involution.
    movq %rax, %rdx
    shrq $1, %rdx
    notq %rdx
    andq %r10, %rdx
    xorq %rdx, %rax
    ret
.size asm_trit_add_bct, .-asm_trit_add_bct

.L_unsigned_bct_add:
    # rdi=a, rsi=b, rax=result. The binary carry chain computes all trits.
    movabsq $0x5555555555555555, %rdx
    leaq (%rdi,%rdx), %rcx
    movq %rsi, %rax
    addq %rcx, %rax
    sbbq %r8, %r8
    notq %r8
    movabsq $0x4000000000000000, %r9
    andq %r9, %r8
    movq %rsi, %rdx
    xorq %rcx, %rdx
    xorq %rax, %rdx
    notq %rdx
    movabsq $0x5555555555555554, %r9
    andq %r9, %rdx
    shrq $2, %rdx
    orq %r8, %rdx
    subq %rdx, %rax
    ret

.section .note.GNU-stack,"",@progbits
