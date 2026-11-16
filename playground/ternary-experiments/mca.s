# llvm-mca-18 -mcpu=alderlake -iterations=100 playground/ternary-experiments/mca.s
# The 13th-gen i7-1355U is closest to this available llvm-mca model; this is
# a model estimate, not a measurement on the running WSL virtual machine.

# LLVM-MCA-BEGIN neg
movabsq $0xaaaaaaaaaaaaaaaa, %rdx
leaq (%rdi,%rdi), %rax
shrq $1, %rdi
andq %rdx, %rax
movabsq $0x5555555555555555, %rdx
andq %rdx, %rdi
orq %rdi, %rax
# LLVM-MCA-END neg

# LLVM-MCA-BEGIN min_xor
movabsq $0x5555555555555555, %rdx
movq %rdi, %rax
andq %rsi, %rdi
xorq %rsi, %rax
andq %rdx, %rax
orq %rdi, %rax
# LLVM-MCA-END min_xor

# LLVM-MCA-BEGIN min_original
movabsq $0xaaaaaaaaaaaaaaaa, %rdx
movq %rdi, %rax
orq %rsi, %rdi
andq %rsi, %rax
andq %rdx, %rax
movabsq $0x5555555555555555, %rdx
andq %rdx, %rdi
orq %rdi, %rax
# LLVM-MCA-END min_original

# LLVM-MCA-BEGIN product_swar
movabsq $0x5555555555555555, %rcx
movq %rdi, %rdx
orq %rsi, %rdi
andq %rsi, %rdx
leaq (%rdx,%rdx), %rax
orq %rdx, %rax
movabsq $0xaaaaaaaaaaaaaaaa, %rdx
andq %rdx, %rax
movq %rdi, %rdx
shrq $1, %rdx
andq %rdi, %rdx
andq %rcx, %rdx
orq %rdx, %rax
# LLVM-MCA-END product_swar

# LLVM-MCA-BEGIN cycle_inverse
movabsq $0x5555555555555555, %rdx
movq %rdi, %rax
shrq $1, %rax
orq %rdi, %rax
addq %rdi, %rdi
andnq %rdx, %rax, %rax
movabsq $0xaaaaaaaaaaaaaaaa, %rdx
andq %rdx, %rdi
orq %rdi, %rax
# LLVM-MCA-END cycle_inverse
