	.file	"hell.cpp"
	.text
	.globl	c
	.data
	.align 4
	.type	c, @object
	.size	c, 4
c:
	.long	10
	.section	.rodata
.LC0:
	.string	"A"
	.text
	.globl	main
	.type	main, @function
main:
.LFB3:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	%rbp, %rax
	movq	%rax, -8(%rbp)
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	c(%rip), %eax
	leal	-1(%rax), %edx
	movl	%edx, c(%rip)
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L2
	movq	-8(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rdx
	movq	-8(%rbp), %rax
	addq	$8, %rax
	subq	$2, %rdx
	movq	%rdx, (%rax)
.L2:
	leaq	main(%rip), %rax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 14.2.0-4ubuntu2~24.04.1) 14.2.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
