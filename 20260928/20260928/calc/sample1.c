#include <stdio.h>
int main (int argc, char *argv [])
{
    char last_op, *p = argv [1];
    printf (".data\n"
            "L_fmt:\n"
            "\t.ascii \"%%d\\n\\0\"\n"
            ".text\n"
            ".globl main\n"
            "main:\n"
            "\tpushq %%rbp\n"
            "\tmovq  %%rsp, %%rbp\n");

    printf ("\tmovl  $%c, %%ecx\n", *p++);
    last_op = *p++;
    printf ("\tmovl  $%c, %%edx\n", *p++);
    switch (last_op) {
    case '+':
        printf ("\taddl  %%edx, %%ecx\n");
        break;
    case '-':
        printf ("\tsubl  %%edx, %%ecx\n");
        break;
    }
    printf ("\tleaq L_fmt(%%rip), %%rdi\n"
            "\tmovslq %%ecx, %%rsi\n"
            "\tmovb $0, %%al\n"
            "\ttestq $0xF, %%rsp; jz 1f; ud2;1:\n"
            "\tcall  printf\n"
            "\tleave\n"
            "\tret\n");
}

