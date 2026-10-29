.code16
.text
    ljmp $0x07c0, $1f
1:	
    movb $0x00, %ah
    movb $0x12, %al
    int  $0x10
#
    movw $0x03C4, %dx
    movb $0x02, %al
    outb %al, %dx
    movw $0x03C5, %dx
    movb $0x0E, %al
    outb %al, %dx
#
    movw $0xA000, %ax
    movw %ax, %gs
    movb $0xC9, %gs:0x0000
#
2:  hlt; jmp 2b
.org 510
    .word 0xaa55

