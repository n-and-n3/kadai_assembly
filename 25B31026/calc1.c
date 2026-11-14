#include <stdio.h>
#include <stdbool.h>

static void emit_data(void)
{
    fputs(
        ".bss\n"
        ".align 4\n"
        "memory:\n"
        "    .zero 4\n"
        "res:\n"
        "    .zero 4 \n"
        "cur:\n"
        "    .zero 4\n"
        "hasError:\n"
        "    .zero 1\n"
        "\n"
        ".data\n"
        "op:\n"
        "    .byte '+'\n"
        "last_op:\n"
        "    .byte '+'\n"
        "\n"
        ".text\n"
        "\n",
        stdout);
}

static void emit_put_error(void)
{
    fputs(
        "put_error:\n"
        "    movb $1,hasError(%rip)\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_is_memory(void)
{
    fputs(
        "##  (%dil : char) -> %al : bool\n"
        "is_memory:\n"
        "    cmpb $'C', %dil\n"
        "    je .L_is_memory\n"
        "    cmpb $'R', %dil\n"
        "    je .L_is_memory\n"
        "    cmpb $'P', %dil\n"
        "    je .L_is_memory\n"
        "    cmpb $'M', %dil\n"
        "    je .L_is_memory\n"
        "\n"
        "    movb $0, %al\n"
        "    ret\n"
        "\n"
        "    .L_is_memory:\n"
        "    movb $1, %al\n"
        "    ret\n"
        "\n"
        "/*\n"
        "is_memory:\n"
        "    andb $49, %dil\n"
        "    cmpb $17, %dil\n"
        "    setl %al\n"
        "    ret\n"
        "*/\n"
        "\n",
        stdout);
}

static void emit_is_number(void)
{
    fputs(
        "##  (%dil : char) -> %al : bool\n"
        "is_number:\n"
        "    cmpb $'S', %dil\n"
        "    je .L_is_number1\n"
        "    cmpb $'0', %dil\n"
        "    jl .L_is_number0\n"
        "    cmpb $'9', %dil\n"
        "    jg .L_is_number0\n"
        "    jmp .L_is_number1\n"
        "\n"
        "    .L_is_number0:\n"
        "    movb $0, %al\n"
        "    ret\n"
        "    .L_is_number1:\n"
        "    movb $1, %al\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_is_operand(void)
{
    fputs(
        "##  (%dil : char) -> %al : bool\n"
        "is_operand:\n"
        "    cmpb $'+', %dil\n"
        "    je .L_is_operand\n"
        "    cmpb $'-', %dil\n"
        "    je .L_is_operand\n"
        "    cmpb $'*', %dil\n"
        "    je .L_is_operand\n"
        "    cmpb $'/', %dil\n"
        "    je .L_is_operand\n"
        "    movb $0, %al\n"
        "    ret\n"
        "\n"
        "    .L_is_operand:\n"
        "    movb $1, %al\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_is_equal(void)
{
    fputs(
        "##  (%dil : char) -> %al : bool\n"
        "is_equal:\n"
        "    cmpb $'=', %dil\n"
        "    setz %al\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_reset(void)
{
    fputs(
        "##  () -> \n"
        "reset:\n"
        "    movl $0,cur(%rip)\n"
        "    movb $'+',op(%rip)\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_my_add(void)
{
    fputs(
        "## (%edi : int, %esi : int) -> %eax\n"
        "my_add:\n"
        "    movl %edi,%eax\n"
        "    addl %esi,%eax\n"
        "    jo .L_my_add\n"
        "    ret\n"
        "    .L_my_add:\n"
        "    call put_error\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_my_sub(void)
{
    fputs(
        "## (%edi : int, %esi : int) -> %eax\n"
        "my_sub:\n"
        "    movl %edi,%eax\n"
        "    subl %esi,%eax\n"
        "    jo .L_my_sub\n"
        "    ret\n"
        "    .L_my_sub:\n"
        "    call put_error\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_my_mul(void)
{
    fputs(
        "## (%edi : int, %esi : int) -> %eax\n"
        "my_mul:\n"
        "    movl %edi,%eax\n"
        "    imul %esi,%eax\n"
        "    jo .L_my_mul\n"
        "    ret\n"
        "    .L_my_mul:\n"
        "    call put_error\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_my_div(void)
{
    fputs(
        "## (%edi : int, %esi : int) -> %eax\n"
        "my_div:\n"
        "    testl %esi,%esi\n"
        "    jz .L_my_div\n"
        "    movl %edi,%eax\n"
        "    cltd\n"
        "    idiv %esi\n"
        "    ret\n"
        "    .L_my_div:\n"
        "    call put_error\n"
        "    movl $0,%eax\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_eval(void)
{
    fputs(
        "## (%edi : int, %esi : int, %dl : char) -> %eax : int\n"
        "eval:\n"
        "    cmpb $'+',%dl\n"
        "    je .L_eval1\n"
        "    cmpb $'-',%dl\n"
        "    je .L_eval2\n"
        "    cmpb $'*',%dl\n"
        "    je .L_eval3\n"
        "    cmpb $'/',%dl\n"
        "    je .L_eval4\n"
        "\n"
        "    call put_error\n"
        "    ret\n"
        "\n"
        "    .L_eval1:\n"
        "        call my_add\n"
        "    ret\n"
        "    .L_eval2:\n"
        "        call my_sub\n"
        "    ret\n"
        "    .L_eval3:\n"
        "        call my_mul\n"
        "    ret\n"
        "    .L_eval4:\n"
        "        call my_div\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_op_memory(void)
{
    fputs(
        "## (%edi : char) -> \n"
        "op_memory:\n"
        "    cmpb $'R',%dl\n"
        "    je .L_op_memory1\n"
        "    cmpb $'C',%dl\n"
        "    je .L_op_memory2\n"
        "    cmpb $'P',%dl\n"
        "    je .L_op_memory3\n"
        "    cmpb $'M',%dl\n"
        "    je .L_op_memory4\n"
        "\n"
        "    call put_error\n"
        "    ret\n"
        "\n"
        "    .L_op_memory1:\n"
        "        movl memory(%rip), %eax\n"
        "        movl %eax, res(%rip)\n"
        "    ret\n"
        "    .L_op_memory2:\n"
        "        movl $0, memory(%rip)\n"
        "    ret\n"
        "    .L_op_memory3:\n"
        "        movl res(%rip),%edi\n"
        "        movl cur(%rip),%esi\n"
        "        movb $'+',%dl\n"
        "        call eval\n"
        "        movl memory(%rip),%edi\n"
        "        movl %eax,%esi\n"
        "        call my_add\n"
        "        movl %eax,memory(%rip)\n"
        "        call reset\n"
        "        movl $0, res(%rip)\n"
        "    ret\n"
        "    .L_op_memory4:\n"
        "        movl res(%rip),%edi\n"
        "        movl cur(%rip),%esi\n"
        "        movb $'-',%dl\n"
        "        call eval\n"
        "        movl memory(%rip),%edi\n"
        "        movl %eax,%esi\n"
        "        call my_add\n"
        "        movl %eax,memory(%rip)\n"
        "        call reset\n"
        "        movl $0, res(%rip)\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_concat(void)
{
    fputs(
        "## (%edi : int, %sil : char) -> %eax : int\n"
        "concat:\n"
        "    cmpb $'S',%sil\n"
        "    je .L_concat1\n"
        "    cmpl $0,%edi\n"
        "    jge .L_concat2\n"
        "    jmp .L_concat3\n"
        "\n"
        "    .L_concat1:\n"
        "        movl $-1,%esi\n"
        "        call my_mul\n"
        "    ret\n"
        "    .L_concat2:\n"
        "        movzbl %sil, %ebx\n"
        "        subl $'0', %ebx\n"
        "        movl $10,%esi\n"
        "        call my_mul\n"
        "        movl %eax,%edi\n"
        "        movl %ebx,%esi\n"
        "        call my_add\n"
        "    ret\n"
        "    .L_concat3:\n"
        "        movzbl %sil, %ebx\n"
        "        subl $'0', %ebx\n"
        "        movl $10,%esi\n"
        "        call my_mul\n"
        "        movl %eax,%edi\n"
        "        movl %ebx,%esi\n"
        "        call my_sub\n"
        "    ret\n"
        "\n",
        stdout);
}

static void emit_display(void)
{
    fputs(
        "## () -> \n"
        "display:\n"
        "    leaq .LC1(%rip), %rdi\n"
        "    cmpb $0, hasError(%rip)\n"
        "    je .L_display\n"
        "        leaq .LC0(%rip), %rdi\n"
        "    .L_display:\n"
        "    movl    res(%rip), %esi\n"
        "    movl    $0, %eax\n"
        "    call    printf\n"
        "    ret\n"
        "\n"
        "\n",
        stdout);
}

static void emit_rodata(void)
{
    fputs(
        ".section .rodata\n"
        ".LC0:\n"
        "    .string \"E\\n\"\n"
        ".LC1:\n"
        "    .string \"%d\\n\"\n",
        stdout);
}

static void emit_header(void)
{
    emit_data();
    emit_put_error();
    emit_is_memory();
    emit_is_number();
    emit_is_operand();
    emit_is_equal();
    emit_reset();
    emit_my_add();
    emit_my_sub();
    emit_my_mul();
    emit_my_div();
    emit_eval();
    emit_op_memory();
    emit_concat();
    emit_display();
    emit_rodata();
}

static void emit_main_begin(void)
{
    fputs(
        ".global main\n"
        ".text\n"
        "main:\n"
        "   pushq   %rbp\n"
        "   movq    %rsp, %rbp\n",
        stdout);
}

static void emit_main_end(void)
{
    fputs(
        "    movl    $0, %eax\n"
        "    movq    %rbp, %rsp\n"
        "    popq    %rbp\n"
        "    ret\n\n",
        stdout);
}

// cur = concat(cur,cmd)
static void emit_concat_call(char cmd)
{
    fputs("    movl cur(%rip),%edi\n", stdout);
    printf("    movb $'%c',%%sil\n", cmd);
    fputs(
        "    call concat\n"
        "    movl %eax,cur(%rip)\n\n",
        stdout);
}

// res = eval(last_op,res,cur);
static void emit_eval_call(void)
{
    fputs(
        "    movl res(%rip),%edi\n"
        "    movl cur(%rip),%esi\n"
        "    movb last_op(%rip),%dl\n"
        "    call eval\n"
        "    movl %eax,res(%rip)\n\n",
        stdout);
}

// last_op = op;
static void emit_copy_op_to_last_op(void)
{
    fputs(
        "    movb op(%rip),%dl\n"
        "    movb %dl,last_op(%rip)\n\n",
        stdout);
}

// reset();
static void emit_reset_call(void)
{
    fputs(
        "    call reset\n\n",
        stdout);
}

// op = cmd;
static void emit_set_op(char cmd){
    printf("    movb $'%c',op(%%rip)\n\n",cmd);
}

int states = 0; // 0:数字入力状態, 1:あた
// 関数定義
bool is_memory(char x){
    return x == 'C' || x == 'R' || x == 'P' || x == 'M';
}

bool is_number(char x){
    return x == 'S' || ('0' <= x && x <= '9');
}

bool is_operand(char x){
    return x == '+' || x == '-' || x == '*' || x == '/';
}

bool is_equal(char x){
    return x == '=';
}

static void emit_call_display(){
    printf("    call display\n");
}

// ほんへ
int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("don't match arguments.\n");
        return 1;
    }
    char *p = argv[1];

    emit_header();
    emit_main_begin();

    while (true) {
        if (*p == '\0'){break;}
        char cmd = *p++;
        printf("# %c\n",cmd);
        if (states == 0){
            if(is_operand(cmd)){
                emit_set_op(cmd);
                states = 1;
            } else if (is_number(cmd)){
                emit_concat_call(cmd);
            } else if (is_memory(cmd)){
                // op_memory(cmd);
            } else if (is_equal(cmd)){
                emit_eval_call();
                emit_copy_op_to_last_op();
                emit_reset_call();
            }
        } else {
            if(is_operand(cmd)){
                emit_set_op(cmd);
            } else if (is_number(cmd)){
                emit_eval_call();
                emit_copy_op_to_last_op();
                emit_reset_call();
                emit_concat_call(cmd);
                states = 0;
            } else if (is_memory(cmd)){
                // op_memory(cmd);
            } else if (is_equal(cmd)){
                emit_eval_call();
                emit_copy_op_to_last_op();
                emit_reset_call();
                emit_reset_call();
            }
        }
    }

    // 最後に内部状態（？、レジスタの状態？）を出力
    emit_call_display();

    emit_main_end();
}
