	.file	"calc0.c"
	.text
	.section	.rodata
.LC0:
	.string	"%c\n"
	.text
	.globl	printc
	.type	printc, @function
printc:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	movsbl	-4(%rbp), %eax
	movl	%eax, %esi
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	printc, .-printc
	.globl	prints
	.type	prints, @function
prints:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	puts@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	prints, .-prints
	.section	.rodata
.LC1:
	.string	"%d\n"
	.text
	.globl	printi
	.type	printi, @function
printi:
.LFB2:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %eax
	movl	%eax, %esi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	printi, .-printi
	.globl	hasError
	.bss
	.type	hasError, @object
	.size	hasError, 1
hasError:
	.zero	1
	.globl	last_op
	.data
	.type	last_op, @object
	.size	last_op, 1
last_op:
	.byte	43
	.globl	op
	.type	op, @object
	.size	op, 1
op:
	.byte	43
	.globl	memory
	.bss
	.align 4
	.type	memory, @object
	.size	memory, 4
memory:
	.zero	4
	.globl	res
	.align 4
	.type	res, @object
	.size	res, 4
res:
	.zero	4
	.globl	cur
	.align 4
	.type	cur, @object
	.size	cur, 4
cur:
	.zero	4
	.globl	states
	.align 4
	.type	states, @object
	.size	states, 4
states:
	.zero	4
	.text
	.globl	is_memory
	.type	is_memory, @function
is_memory:
.LFB3:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	cmpb	$67, -4(%rbp)
	je	.L5
	cmpb	$82, -4(%rbp)
	je	.L5
	cmpb	$80, -4(%rbp)
	je	.L5
	cmpb	$77, -4(%rbp)
	jne	.L6
.L5:
	movl	$1, %eax
	jmp	.L7
.L6:
	movl	$0, %eax
.L7:
	andl	$1, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	is_memory, .-is_memory
	.globl	is_number
	.type	is_number, @function
is_number:
.LFB4:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	cmpb	$83, -4(%rbp)
	je	.L10
	cmpb	$47, -4(%rbp)
	jle	.L11
	cmpb	$57, -4(%rbp)
	jg	.L11
.L10:
	movl	$1, %eax
	jmp	.L12
.L11:
	movl	$0, %eax
.L12:
	andl	$1, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE4:
	.size	is_number, .-is_number
	.globl	is_operand
	.type	is_operand, @function
is_operand:
.LFB5:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	cmpb	$43, -4(%rbp)
	je	.L15
	cmpb	$45, -4(%rbp)
	je	.L15
	cmpb	$42, -4(%rbp)
	je	.L15
	cmpb	$47, -4(%rbp)
	jne	.L16
.L15:
	movl	$1, %eax
	jmp	.L17
.L16:
	movl	$0, %eax
.L17:
	andl	$1, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE5:
	.size	is_operand, .-is_operand
	.globl	is_equal
	.type	is_equal, @function
is_equal:
.LFB6:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	cmpb	$61, -4(%rbp)
	sete	%al
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE6:
	.size	is_equal, .-is_equal
	.globl	reset
	.type	reset, @function
reset:
.LFB7:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	$0, cur(%rip)
	movb	$43, op(%rip)
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	reset, .-reset
	.globl	add
	.type	add, @function
add:
.LFB8:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$0, %ecx
	movl	-20(%rbp), %edx
	movl	-24(%rbp), %eax
	addl	%edx, %eax
	jno	.L23
	movl	$1, %ecx
.L23:
	movl	%eax, -12(%rbp)
	movl	%ecx, %eax
	movb	%al, -13(%rbp)
	andb	$1, -13(%rbp)
	cmpb	$0, -13(%rbp)
	je	.L25
	movb	$1, hasError(%rip)
	movl	$0, %eax
	jmp	.L27
.L25:
	movl	-12(%rbp), %eax
.L27:
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L28
	call	__stack_chk_fail@PLT
.L28:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	add, .-add
	.globl	sub
	.type	sub, @function
sub:
.LFB9:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$0, %edx
	movl	-20(%rbp), %eax
	subl	-24(%rbp), %eax
	jno	.L30
	movl	$1, %edx
.L30:
	movl	%eax, -12(%rbp)
	movl	%edx, %eax
	movb	%al, -13(%rbp)
	andb	$1, -13(%rbp)
	cmpb	$0, -13(%rbp)
	je	.L32
	movb	$1, hasError(%rip)
	movl	$0, %eax
	jmp	.L34
.L32:
	movl	-12(%rbp), %eax
.L34:
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L35
	call	__stack_chk_fail@PLT
.L35:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	sub, .-sub
	.globl	mul
	.type	mul, @function
mul:
.LFB10:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$0, %edx
	movl	-20(%rbp), %eax
	imull	-24(%rbp), %eax
	jno	.L37
	movl	$1, %edx
.L37:
	movl	%eax, -12(%rbp)
	movl	%edx, %eax
	movb	%al, -13(%rbp)
	andb	$1, -13(%rbp)
	cmpb	$0, -13(%rbp)
	je	.L39
	movb	$1, hasError(%rip)
	movl	$0, %eax
	jmp	.L41
.L39:
	movl	-12(%rbp), %eax
.L41:
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L42
	call	__stack_chk_fail@PLT
.L42:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	mul, .-mul
	.globl	div
	.type	div, @function
div:
.LFB11:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	cmpl	$0, -24(%rbp)
	je	.L44
	movl	-20(%rbp), %eax
	cltd
	idivl	-24(%rbp)
	jmp	.L45
.L44:
	movl	$0, %eax
.L45:
	movl	%eax, -4(%rbp)
	cmpl	$0, -24(%rbp)
	sete	%al
	movb	%al, -5(%rbp)
	cmpb	$0, -5(%rbp)
	je	.L46
	movb	$1, hasError(%rip)
	movl	$0, %eax
	jmp	.L47
.L46:
	movl	-4(%rbp), %eax
.L47:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	div, .-div
	.globl	eval
	.type	eval, @function
eval:
.LFB12:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, %eax
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movb	%al, -4(%rbp)
	movsbl	-4(%rbp), %eax
	cmpl	$47, %eax
	je	.L49
	cmpl	$47, %eax
	jg	.L50
	cmpl	$45, %eax
	je	.L51
	cmpl	$45, %eax
	jg	.L50
	cmpl	$42, %eax
	je	.L52
	cmpl	$43, %eax
	jne	.L50
	movl	-12(%rbp), %edx
	movl	-8(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	add
	jmp	.L53
.L51:
	movl	-12(%rbp), %edx
	movl	-8(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	sub
	jmp	.L53
.L52:
	movl	-12(%rbp), %edx
	movl	-8(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	mul
	jmp	.L53
.L49:
	movl	-12(%rbp), %edx
	movl	-8(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	div
	jmp	.L53
.L50:
	movb	$1, hasError(%rip)
	movl	$0, %eax
.L53:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE12:
	.size	eval, .-eval
	.globl	op_memory
	.type	op_memory, @function
op_memory:
.LFB13:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	movsbl	-4(%rbp), %eax
	cmpl	$82, %eax
	je	.L55
	cmpl	$82, %eax
	jg	.L60
	cmpl	$80, %eax
	je	.L57
	cmpl	$80, %eax
	jg	.L60
	cmpl	$67, %eax
	je	.L58
	cmpl	$77, %eax
	je	.L59
	jmp	.L60
.L55:
	movl	memory(%rip), %eax
	movl	%eax, res(%rip)
	jmp	.L56
.L58:
	movl	$0, memory(%rip)
	jmp	.L56
.L57:
	movl	cur(%rip), %edx
	movl	res(%rip), %ecx
	movzbl	last_op(%rip), %eax
	movsbl	%al, %eax
	movl	%ecx, %esi
	movl	%eax, %edi
	call	eval
	movl	memory(%rip), %edx
	addl	%edx, %eax
	movl	%eax, memory(%rip)
	movzbl	op(%rip), %eax
	movb	%al, last_op(%rip)
	movl	$0, %eax
	call	reset
	movl	$0, res(%rip)
	jmp	.L56
.L59:
	movl	cur(%rip), %edx
	movl	res(%rip), %ecx
	movzbl	last_op(%rip), %eax
	movsbl	%al, %eax
	movl	%ecx, %esi
	movl	%eax, %edi
	call	eval
	movl	memory(%rip), %edx
	subl	%eax, %edx
	movl	%edx, memory(%rip)
	movzbl	op(%rip), %eax
	movb	%al, last_op(%rip)
	movl	$0, %eax
	call	reset
	movl	$0, res(%rip)
	nop
.L56:
.L60:
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE13:
	.size	op_memory, .-op_memory
	.globl	concat
	.type	concat, @function
concat:
.LFB14:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	%esi, %eax
	movb	%al, -24(%rbp)
	cmpb	$83, -24(%rbp)
	jne	.L62
	movl	-20(%rbp), %eax
	movl	$-1, %esi
	movl	%eax, %edi
	call	mul
	jmp	.L63
.L62:
	movsbl	-24(%rbp), %eax
	subl	$48, %eax
	movl	%eax, -4(%rbp)
	cmpl	$0, -20(%rbp)
	js	.L64
	movl	-20(%rbp), %eax
	movl	$10, %esi
	movl	%eax, %edi
	call	mul
	movl	%eax, %edx
	movl	-4(%rbp), %eax
	movl	%eax, %esi
	movl	%edx, %edi
	call	add
	jmp	.L63
.L64:
	movl	-20(%rbp), %eax
	movl	$10, %esi
	movl	%eax, %edi
	call	mul
	movl	%eax, %edx
	movl	-4(%rbp), %eax
	movl	%eax, %esi
	movl	%edx, %edi
	call	sub
.L63:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE14:
	.size	concat, .-concat
	.section	.rodata
.LC2:
	.string	"don't match arguments."
.LC3:
	.string	"E"
	.text
	.globl	main
	.type	main, @function
main:
.LFB15:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	cmpl	$2, -20(%rbp)
	je	.L66
	leaq	.LC2(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$1, %eax
	jmp	.L67
.L66:
	movq	-32(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -8(%rbp)
.L78:
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	je	.L82
	movq	-8(%rbp), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, -8(%rbp)
	movzbl	(%rax), %eax
	movb	%al, -9(%rbp)
	movl	states(%rip), %eax
	testl	%eax, %eax
	jne	.L70
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_operand
	testb	%al, %al
	je	.L71
	movzbl	-9(%rbp), %eax
	movb	%al, op(%rip)
	movl	$1, states(%rip)
	jmp	.L78
.L71:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_number
	testb	%al, %al
	je	.L73
	movsbl	-9(%rbp), %edx
	movl	cur(%rip), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	concat
	movl	%eax, cur(%rip)
	jmp	.L78
.L73:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_memory
	testb	%al, %al
	je	.L74
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	op_memory
	jmp	.L78
.L74:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_equal
	testb	%al, %al
	je	.L78
	movl	cur(%rip), %edx
	movl	res(%rip), %ecx
	movzbl	last_op(%rip), %eax
	movsbl	%al, %eax
	movl	%ecx, %esi
	movl	%eax, %edi
	call	eval
	movl	%eax, res(%rip)
	movzbl	op(%rip), %eax
	movb	%al, last_op(%rip)
	movl	$0, %eax
	call	reset
	jmp	.L78
.L70:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_operand
	testb	%al, %al
	je	.L75
	movzbl	-9(%rbp), %eax
	movb	%al, op(%rip)
	jmp	.L78
.L75:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_number
	testb	%al, %al
	je	.L76
	movl	cur(%rip), %edx
	movl	res(%rip), %ecx
	movzbl	last_op(%rip), %eax
	movsbl	%al, %eax
	movl	%ecx, %esi
	movl	%eax, %edi
	call	eval
	movl	%eax, res(%rip)
	movzbl	op(%rip), %eax
	movb	%al, last_op(%rip)
	movl	$0, %eax
	call	reset
	movsbl	-9(%rbp), %edx
	movl	cur(%rip), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	concat
	movl	%eax, cur(%rip)
	movl	$0, states(%rip)
	jmp	.L78
.L76:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_memory
	testb	%al, %al
	je	.L77
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	op_memory
	jmp	.L78
.L77:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	is_equal
	testb	%al, %al
	je	.L78
	movl	cur(%rip), %edx
	movl	res(%rip), %ecx
	movzbl	last_op(%rip), %eax
	movsbl	%al, %eax
	movl	%ecx, %esi
	movl	%eax, %edi
	call	eval
	movl	%eax, res(%rip)
	movzbl	op(%rip), %eax
	movb	%al, last_op(%rip)
	movl	$0, %eax
	call	reset
	jmp	.L78
.L82:
	nop
	movzbl	hasError(%rip), %eax
	testb	%al, %al
	je	.L79
	leaq	.LC3(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	jmp	.L80
.L79:
	movl	res(%rip), %eax
	movl	%eax, %esi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
.L80:
	movl	$0, %eax
.L67:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
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
