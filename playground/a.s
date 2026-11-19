.global main
.text

main:
    # スタックフレームの構築
    pushq   %rbp
    movq    %rsp, %rbp

    # ほんへ
    leaq    main(%rip), %r8
    movq    %r8, 0x8(%rbp)

    # 第1引数(書式付き文字列のポインタ) を %rdi に設定
    leaq    .LC0(%rip), %rdi

    # 浮動小数点引数の数 (printfの仕様で %al / %eax に指定; 今回は0個)
    movl    $0, %eax

    # printf 関数の呼び出し
    call    printf

    # 戻り値 0 を設定
    movl    $0, %eax

    # スタックフレームの破棄
    movq    %rbp, %rsp
    popq    %rbp
    ret

.section .rodata
.LC0:
    .string "Hello, world!\n"
